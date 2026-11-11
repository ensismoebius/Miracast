// SPDX-License-Identifier: GPL-2.0-or-later
#include "p2p_link.hpp"

#include <algorithm>
#include <chrono>

#include "util.hpp"

namespace mc
{

namespace
{
using Clock = std::chrono::steady_clock;

/** Value of "key=value" (value may be single-quoted) inside an event line. */
std::string field(const std::string &line, const std::string &key)
{
	const std::string needle = key + "=";
	size_t pos = line.find(needle);
	while (pos != std::string::npos && pos != 0 && line[pos - 1] != ' ')
		pos = line.find(needle, pos + 1);
	if (pos == std::string::npos)
		return {};
	pos += needle.size();
	if (pos < line.size() && line[pos] == '\'') {
		const size_t end = line.find('\'', pos + 1);
		return end == std::string::npos ? std::string()
						: line.substr(pos + 1, end - pos - 1);
	}
	const size_t end = line.find(' ', pos);
	return line.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

/** True when the deadline passed or a stop was requested. */
bool expired(Clock::time_point deadline, const std::atomic<bool> &stop)
{
	return stop.load() || Clock::now() >= deadline;
}
} // namespace

/** Connect to wpa_supplicant. */
bool P2pLink::init(const std::string &ctrlDir, const std::string &iface)
{
	return ctrl_.open(ctrlDir, iface);
}

/** True when wpa_supplicant reports P2P among the capabilities of this interface. */
bool P2pLink::supportsP2p()
{
	const auto modes = ctrl_.request("GET_CAPABILITY modes");
	return modes && modes->find("P2P") != std::string::npos;
}

/** Enable Wi-Fi Display and publish name/type/WFD subelements. */
bool P2pLink::configure(WfdDeviceType type, const std::string &name)
{
	if (!supportsP2p()) {
		logf(LogLevel::Error,
		     "this interface cannot do Wi-Fi Direct (P2P): the driver exposes no "
		     "P2P-client/P2P-GO/P2P-device mode (check 'iw phy'), or wpa_supplicant was "
		     "built "
		     "without CONFIG_P2P/CONFIG_WIFI_DISPLAY. Miracast needs a P2P-capable "
		     "adapter.");
		return false;
	}
	const bool sink = type == WfdDeviceType::PrimarySink;
	// Display/Television (7-1) for sinks, Computer/PC (1-1) for sources; WFA OUI 0050F204.
	const std::string devType = sink ? "7-0050F204-1" : "1-0050F204-1";
	const std::string wfdHex = wfdDeviceInfoSubelem(type, kDefaultRtspPort, 50);
	const bool okay = ctrl_.command("SET wifi_display 1") &&
			  ctrl_.command("WFD_SUBELEM_SET 0 " + wfdHex) &&
			  ctrl_.command("SET device_name " + name) &&
			  ctrl_.command("SET device_type " + devType) &&
			  ctrl_.command(std::string("SET p2p_go_intent ") + (sink ? "15" : "0"));
	ctrl_.command("SET config_methods virtual_push_button");
	return okay;
}

/** Update peers from a P2P-DEVICE-FOUND event. */
void P2pLink::collectPeer(const std::string &event, std::vector<Peer> &peers)
{
	if (!startsWith(event, "P2P-DEVICE-FOUND"))
		return;
	const auto parts = split(event, ' ');
	std::string addr = field(event, "p2p_dev_addr");
	if (addr.empty() && parts.size() >= 2)
		addr = parts[1];
	if (addr.empty())
		return;
	auto found = std::find_if(peers.begin(), peers.end(),
				  [&](const Peer &peer) { return peer.addr == addr; });
	if (found == peers.end()) {
		peers.emplace_back();
		found = peers.end() - 1;
		found->addr = addr;
	}
	found->name = field(event, "name");
	uint16_t port = 0;
	const auto info = parseWfdDevInfo(field(event, "wfd_dev_info"), &port);
	if (info) {
		found->hasWfd = true;
		found->wfdInfo = *info;
		found->wfdPort = port != 0 ? port : kDefaultRtspPort;
	}
}

/** Discover peers for the given number of seconds. */
std::vector<Peer> P2pLink::scan(int seconds, const std::atomic<bool> &stop)
{
	std::vector<Peer> peers;
	ctrl_.command("P2P_FLUSH");
	ctrl_.command("P2P_FIND");
	const auto deadline = Clock::now() + std::chrono::seconds(seconds);
	while (!expired(deadline, stop)) {
		const auto event = ctrl_.nextEvent(500);
		if (event)
			collectPeer(*event, peers);
	}
	ctrl_.command("P2P_STOP_FIND");
	return peers;
}

/** React to one event while waiting for a group; sets group when formed. */
bool P2pLink::handleEvent(const std::string &event, bool sinkSide, std::optional<Group> &group)
{
	const auto parts = split(event, ' ');
	if (startsWith(event, "P2P-GROUP-STARTED") && parts.size() >= 3) {
		group = Group{parts[1], parts[2] == "GO"};
		return true;
	}
	const bool request = startsWith(event, "P2P-PROV-DISC-PBC-REQ") ||
			     startsWith(event, "P2P-GO-NEG-REQUEST");
	if (sinkSide && request && parts.size() >= 2)
		ctrl_.command("P2P_CONNECT " + parts[1] + " pbc go_intent=15");
	const bool failure = startsWith(event, "P2P-GO-NEG-FAILURE") ||
			     startsWith(event, "P2P-GROUP-FORMATION-FAILURE");
	if (failure)
		logf(LogLevel::Warn, "P2P negotiation failed: %s", event.c_str());
	return failure && !sinkSide;
}

/** Wait until a group is formed, negotiation fails, or time/stop ends the wait. */
std::optional<Group> P2pLink::waitGroup(int timeoutSec, const std::atomic<bool> &stop,
					bool sinkSide)
{
	const auto deadline = Clock::now() + std::chrono::seconds(timeoutSec);
	std::optional<Group> group;
	while (!expired(deadline, stop)) {
		const auto event = ctrl_.nextEvent(500);
		if (event && handleEvent(*event, sinkSide, group))
			break;
	}
	return group;
}

/** Source role: form a group with peer. */
std::optional<Group> P2pLink::connect(const Peer &peer, int timeoutSec,
				      const std::atomic<bool> &stop)
{
	ctrl_.command("P2P_STOP_FIND");
	if (!ctrl_.command("P2P_CONNECT " + peer.addr + " pbc go_intent=0"))
		return std::nullopt;
	return waitGroup(timeoutSec, stop, false);
}

/** Sink role: listen and accept the first incoming negotiation. */
std::optional<Group> P2pLink::accept(int timeoutSec, const std::atomic<bool> &stop)
{
	ctrl_.command("P2P_LISTEN");
	return waitGroup(timeoutSec, stop, true);
}

/** Dissolve a group. */
void P2pLink::removeGroup(const Group &group)
{
	ctrl_.command("P2P_GROUP_REMOVE " + group.ifname);
}

} // namespace mc

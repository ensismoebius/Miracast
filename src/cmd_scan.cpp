// SPDX-License-Identifier: GPL-2.0-or-later
#include <sysexits.h>

#include <cstdio>

#include "commands.hpp"
#include "mdns_discovery.hpp"
#include "p2p_link.hpp"

namespace mc
{

namespace
{
constexpr int kDiscoveryMs = 3000;

/** Print sinks reachable over the network; returns how many. */
int printInfrastructure(const std::atomic<bool> &stop)
{
	const auto sinks = discoverSinks(kDiscoveryMs, stop);
	for (const SinkInfo &sink : sinks)
		std::printf("%s\tnetwork\t%s\n", sink.address.c_str(), sink.name.c_str());
	return static_cast<int>(sinks.size());
}

/** Print Wi-Fi Direct peers; returns how many (0 when P2P is unavailable). */
int printP2p(const Options &opts, const std::atomic<bool> &stop)
{
	P2pLink link;
	if (!link.init(opts.ctrlDir, opts.iface) ||
	    !link.configure(WfdDeviceType::Source, opts.name))
		return 0;
	int count = 0;
	for (const Peer &peer : link.scan(opts.scanSeconds, stop)) {
		if (!peer.hasWfd)
			continue;
		std::printf("%s\t%s\t%s\n", peer.addr.c_str(),
			    wfdIsSink(peer.wfdInfo) ? "wifi-direct-sink" : "wifi-direct-source",
			    peer.name.c_str());
		++count;
	}
	return count;
}
} // namespace

/** List sinks on the network and over Wi-Fi Direct; returns a sysexits status. */
int runScan(const Options &opts, const std::atomic<bool> &stop)
{
	int found = printInfrastructure(stop);
	if (!opts.noP2p)
		found += printP2p(opts, stop);
	return found > 0 ? EX_OK : EX_NOHOST;
}

} // namespace mc

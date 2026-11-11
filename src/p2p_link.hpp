// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Wi-Fi Direct management on top of wpa_supplicant. */
#pragma once
#include <atomic>
#include <optional>
#include <string>
#include <vector>

#include "group.hpp"
#include "peer.hpp"
#include "wfd_device_type.hpp"
#include "wpa_ctrl.hpp"

namespace mc
{

/** Drives P2P discovery and group formation and advertises the WFD information element. */
class P2pLink
{
public:
	/** Connect to wpa_supplicant. */
	bool init(const std::string &ctrlDir, const std::string &iface);
	/** Enable Wi-Fi Display and publish name/type/WFD subelements. */
	bool configure(WfdDeviceType type, const std::string &name);
	/** Discover peers for the given number of seconds. */
	std::vector<Peer> scan(int seconds, const std::atomic<bool> &stop);
	/** Source role: form a group with peer. */
	std::optional<Group> connect(const Peer &peer, int timeoutSec,
				     const std::atomic<bool> &stop);
	/** Sink role: listen and accept the first incoming negotiation. */
	std::optional<Group> accept(int timeoutSec, const std::atomic<bool> &stop);
	/** Dissolve a group. */
	void removeGroup(const Group &group);

private:
	/** True when the interface can do P2P at all. */
	bool supportsP2p();
	/** Update peers from a P2P-DEVICE-FOUND event. */
	static void collectPeer(const std::string &event, std::vector<Peer> &peers);
	/** React to one event while waiting for a group; sets group when formed. */
	bool handleEvent(const std::string &event, bool sinkSide, std::optional<Group> &group);
	/** Wait until a group is formed, negotiation fails, or time/stop ends the wait. */
	std::optional<Group> waitGroup(int timeoutSec, const std::atomic<bool> &stop,
				       bool sinkSide);

	WpaCtrl ctrl_ = {};
};

} // namespace mc

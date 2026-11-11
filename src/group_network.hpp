// SPDX-License-Identifier: GPL-2.0-or-later
/** @file IP setup of a freshly formed P2P group interface. */
#pragma once
#include <string>

#include "process.hpp"

namespace mc
{

/** Static addressing + DHCP server (owner) or DHCP client (client) for a group interface. */
class GroupNetwork
{
public:
	/** Address the group owner uses. */
	static std::string ownerAddress();
	/** Assign the owner address and start dnsmasq. */
	bool setupOwner(const std::string &ifname);
	/** Obtain a lease with whichever DHCP client is installed. */
	static bool setupClient(const std::string &ifname);

private:
	Process helper_ = {};
};

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
#include "group_network.hpp"

#include "exec.hpp"
#include "util.hpp"

namespace mc
{

namespace
{
/** argv for a DHCP client tool, or empty if unknown. */
std::vector<std::string> dhcpArgv(const std::string &tool, const std::string &ifname)
{
	if (tool == "dhcpcd")
		return {"dhcpcd", "-4", "-w", ifname};
	if (tool == "dhclient")
		return {"dhclient", "-1", ifname};
	return {"udhcpc", "-q", "-n", "-i", ifname};
}
} // namespace

/** Address the group owner uses. */
std::string GroupNetwork::ownerAddress()
{
	return "192.168.173.1";
}

/** Assign the owner address and start dnsmasq. */
bool GroupNetwork::setupOwner(const std::string &ifname)
{
	const bool addressed =
		runCommand({"ip", "addr", "replace", ownerAddress() + "/24", "dev", ifname}) == 0 &&
		runCommand({"ip", "link", "set", ifname, "up"}) == 0;
	if (!addressed) {
		logf(LogLevel::Error, "cannot configure %s (needs CAP_NET_ADMIN and iproute2)",
		     ifname.c_str());
		return false;
	}
	if (!commandExists("dnsmasq")) {
		logf(LogLevel::Error, "dnsmasq is required on the group owner to serve DHCP");
		return false;
	}
	return helper_.spawn({"dnsmasq", "--keep-in-foreground", "--bind-interfaces",
			      "--interface=" + ifname, "--except-interface=lo", "--port=0",
			      "--no-resolv", "--no-hosts",
			      "--dhcp-range=192.168.173.10,192.168.173.50,255.255.255.0,1h"});
}

/** Obtain a lease with whichever DHCP client is installed. */
bool GroupNetwork::setupClient(const std::string &ifname)
{
	for (const char *tool : {"dhcpcd", "dhclient", "udhcpc"}) {
		if (commandExists(tool))
			return runCommand(dhcpArgv(tool, ifname)) == 0;
	}
	logf(LogLevel::Error, "no DHCP client found (dhcpcd, dhclient or udhcpc)");
	return false;
}

} // namespace mc

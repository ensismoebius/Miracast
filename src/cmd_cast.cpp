// SPDX-License-Identifier: GPL-2.0-or-later
#include <sysexits.h>

#include "cast_session.hpp"
#include "commands.hpp"
#include "cast_link.hpp"
#include "group_network.hpp"
#include "net.hpp"
#include "p2p_link.hpp"
#include "sink_lookup.hpp"
#include "util.hpp"

namespace mc
{

namespace
{
/** Find the named sink in a scan. */
std::optional<Peer> pickPeer(P2pLink &link, const Options &opts, const std::atomic<bool> &stop)
{
	for (const Peer &peer : link.scan(opts.scanSeconds, stop)) {
		const bool match = peer.addr == opts.target || peer.name == opts.target;
		if (peer.hasWfd && wfdIsSink(peer.wfdInfo) && match)
			return peer;
	}
	logf(LogLevel::Error, "sink \"%s\" not found (try: miracast scan)", opts.target.c_str());
	return std::nullopt;
}

/** Form the P2P group (or use --host) and work out the sink address. */
CastLink bringUp(const Options &opts, P2pLink &link, GroupNetwork &network,
		 const std::atomic<bool> &stop)
{
	CastLink state;
	state.sinkHost = opts.host;
	if (opts.noP2p)
		return state;
	if (opts.target.empty() || !link.init(opts.ctrlDir, opts.iface) ||
	    !link.configure(WfdDeviceType::Source, opts.name)) {
		state.status = opts.target.empty() ? EX_USAGE : EX_UNAVAILABLE;
		return state;
	}
	const auto peer = pickPeer(link, opts, stop);
	if (!peer) {
		state.status = EX_NOHOST;
		return state;
	}
	state.port = peer->wfdPort;
	const auto group = link.connect(*peer, 30, stop);
	if (!group) {
		state.status = EX_TEMPFAIL;
		return state;
	}
	state.group = *group;
	state.haveGroup = true;
	const bool ready = group->owner ? network.setupOwner(group->ifname)
					: GroupNetwork::setupClient(group->ifname);
	state.status = ready ? EX_OK : EX_OSERR;
	state.sinkHost = group->owner ? std::string() : defaultGateway(group->ifname).value_or("");
	return state;
}

/** Connect to the sink's RTSP port, retrying while it comes up. */
FileDescriptor connectToSink(const CastLink &state, const std::atomic<bool> &stop)
{
	FileDescriptor conn;
	for (int attempt = 0; attempt < 20 && !conn.valid() && !stop.load(); ++attempt)
		conn = tcpConnect(state.sinkHost, state.port, 2000);
	return conn;
}

/** Cast over the network (MS-MICE) to a discovered sink. */
int castToSink(const Options &opts, const SinkInfo &sink, const std::atomic<bool> &stop)
{
	Options local = opts;
	local.host = sink.address;
	local.micePort = sink.port;
	logf(LogLevel::Info, "found \"%s\" at %s, casting over the network", sink.name.c_str(),
	     sink.address.c_str());
	return runCastMice(local, stop);
}
} // namespace

/** Source role; returns a sysexits status. */
int runCast(const Options &opts, const std::atomic<bool> &stop)
{
	if (opts.mice)
		return runCastMice(opts, stop);
	if (!opts.noP2p && !opts.p2p) {
		const auto sink = findInfrastructureSink(opts, stop);
		if (sink)
			return castToSink(opts, *sink, stop);
	}
	P2pLink link;
	GroupNetwork network;
	const CastLink state = bringUp(opts, link, network, stop);
	if (state.status != EX_OK)
		return state.status;
	if (state.sinkHost.empty()) {
		logf(LogLevel::Error, "sink address unknown (use --host with --no-p2p)");
		return EX_USAGE;
	}
	FileDescriptor conn = connectToSink(state, stop);
	if (!conn.valid()) {
		logf(LogLevel::Error, "cannot connect to %s:%u", state.sinkHost.c_str(),
		     state.port);
		return EX_UNAVAILABLE;
	}
	const int status = runSourceSession(conn, opts, state.sinkHost, stop, [] { return true; });
	if (state.haveGroup)
		link.removeGroup(state.group);
	return status;
}

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
#include <sysexits.h>

#include "commands.hpp"
#include "sink_link.hpp"
#include "gst_sink_media.hpp"
#include "group_network.hpp"
#include "net.hpp"
#include "p2p_link.hpp"
#include "session_pump.hpp"
#include "util.hpp"
#include "sink_session.hpp"

namespace mc
{

namespace
{
/** Form the P2P group (or use --listen) and decide where to bind. */
SinkLink bringUp(const Options &opts, P2pLink &link, GroupNetwork &network,
		 const std::atomic<bool> &stop)
{
	SinkLink state;
	state.bindAddr = opts.listen;
	if (opts.noP2p)
		return state;
	if (!link.init(opts.ctrlDir, opts.iface) ||
	    !link.configure(WfdDeviceType::PrimarySink, opts.name)) {
		state.status = EX_UNAVAILABLE;
		return state;
	}
	logf(LogLevel::Info, "waiting for a source to connect as \"%s\"...", opts.name.c_str());
	const auto group = link.accept(24 * 3600, stop);
	if (!group) {
		state.status = stop.load() ? EX_OK : EX_TEMPFAIL;
		return state;
	}
	state.group = *group;
	state.haveGroup = true;
	const bool ready = group->owner ? network.setupOwner(group->ifname)
					: GroupNetwork::setupClient(group->ifname);
	state.status = ready ? EX_OK : EX_OSERR;
	state.bindAddr = group->owner ? GroupNetwork::ownerAddress()
				      : ifaceAddress(group->ifname).value_or("0.0.0.0");
	return state;
}

/** Wait for the source's TCP connection. */
FileDescriptor waitForSource(const FileDescriptor &listener, const std::atomic<bool> &stop,
			     std::string &peerIp)
{
	FileDescriptor conn;
	while (!conn.valid() && !stop.load())
		conn = tcpAccept(listener.get(), 500, &peerIp);
	return conn;
}
} // namespace

/** Sink role; returns a sysexits status. */
int runSink(const Options &opts, const std::atomic<bool> &stop)
{
	P2pLink link;
	GroupNetwork network;
	const SinkLink state = bringUp(opts, link, network, stop);
	if (state.status != EX_OK || stop.load())
		return state.status;
	FileDescriptor listener = tcpListen(state.bindAddr, kDefaultRtspPort);
	if (!listener.valid())
		return EX_OSERR;
	logf(LogLevel::Info, "RTSP listening on %s:%u", state.bindAddr.c_str(), kDefaultRtspPort);

	std::string peerIp;
	FileDescriptor conn = waitForSource(listener, stop, peerIp);
	if (!conn.valid())
		return EX_OK;
	logf(LogLevel::Info, "source connected from %s", peerIp.c_str());

	GstSinkMedia media(opts.videoSink, opts.audioSink, opts.jitterMs);
	SinkConfig cfg;
	cfg.peerHost = peerIp;
	SinkSession session([&](const std::string &data) { sendAll(conn.get(), data); }, media,
			    cfg);
	pumpSession(conn, session, stop, [&] { return media.healthy(); });
	if (state.haveGroup)
		link.removeGroup(state.group);
	return session.failed() ? EX_PROTOCOL : EX_OK;
}

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
#include <sysexits.h>

#include "cast_session.hpp"
#include "commands.hpp"
#include "mice_protocol.hpp"
#include "net.hpp"
#include "util.hpp"
#include "wfd_params.hpp"

namespace mc
{

namespace
{
constexpr int kConnectTimeoutMs = 4000;
constexpr int kSinkWaitMs = 15000;

/** Accept the sink's RTSP connection, polling so Ctrl-C stays responsive. */
FileDescriptor acceptSink(const FileDescriptor &listener, const std::atomic<bool> &stop)
{
	FileDescriptor conn;
	std::string peer;
	for (int waited = 0; waited < kSinkWaitMs && !conn.valid() && !stop.load(); waited += 500)
		conn = tcpAccept(listener.get(), 500, &peer);
	return conn;
}

/** False once the sink closed the control socket or sent STOP_PROJECTION. */
bool miceAlive(const FileDescriptor &control)
{
	std::string buf;
	const int got = recvSome(control.get(), buf, 0);
	if (got < 0)
		return false;
	return !(got > 0 && miceCommandOf(buf) == MiceCommand::StopProjection);
}
} // namespace

/** Miracast over Infrastructure source (MS-MICE); returns a sysexits status. */
int runCastMice(const Options &opts, const std::atomic<bool> &stop)
{
	if (opts.host.empty()) {
		logf(LogLevel::Error, "--mice needs the sink address (--host IP)");
		return EX_USAGE;
	}
	FileDescriptor control = tcpConnect(opts.host, opts.micePort, kConnectTimeoutMs);
	if (!control.valid()) {
		logf(LogLevel::Error, "sink %s does not accept MS-MICE on TCP %u",
		     opts.host.c_str(), opts.micePort);
		return EX_UNAVAILABLE;
	}
	const FileDescriptor listener = tcpListen("0.0.0.0", kDefaultRtspPort);
	if (!listener.valid())
		return EX_OSERR;
	const std::string sourceId = miceRandomSourceId();
	logf(LogLevel::Info, "MS-MICE connected to %s, sending SOURCE_READY", opts.host.c_str());
	if (!sendAll(control.get(), miceSourceReady(opts.name, kDefaultRtspPort, sourceId)))
		return EX_IOERR;
	const FileDescriptor conn = acceptSink(listener, stop);
	if (!conn.valid()) {
		logf(LogLevel::Error, "sink did not open the RTSP connection to us on %u",
		     kDefaultRtspPort);
		return EX_TEMPFAIL;
	}
	const int status =
		runSourceSession(conn, opts, opts.host, stop, [&] { return miceAlive(control); });
	sendAll(control.get(), miceStopProjection(opts.name, sourceId));
	return status;
}

} // namespace mc

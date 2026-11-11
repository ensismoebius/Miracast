// SPDX-License-Identifier: GPL-2.0-or-later
#include "cast_session.hpp"

#include <sysexits.h>

#include "gst_source_media.hpp"
#include "gst_util.hpp"
#include "net.hpp"
#include "session_pump.hpp"
#include "source_session.hpp"

namespace mc
{

/** Stream to peerHost over conn; extra() returning false ends the session. Returns a sysexits
 * status. */
int runSourceSession(const FileDescriptor &conn, const Options &opts, const std::string &peerHost,
		     const std::atomic<bool> &stop, const std::function<bool()> &extra)
{
	CaptureSettings settings;
	settings.capture = opts.capture;
	settings.testPattern = opts.testPattern;
	settings.output = opts.output;
	settings.encoder = opts.encoder;
	settings.encoderOpts = opts.encoderOpts;
	settings.bufferMs = opts.bufferMs;
	settings.audioSource = opts.audioSource;
	gstInit();
	settings.audio = !opts.noAudio && gstFactoryExists("avenc_aac");
	GstSourceMedia media(settings);
	SourceConfig cfg;
	cfg.localHost = localAddress(conn.get());
	cfg.peerHost = peerHost;
	cfg.maxWidth = opts.maxWidth;
	cfg.maxFps = opts.maxFps;
	cfg.bitrateKbps = opts.bitrate;
	cfg.audio = settings.audio;
	SourceSession session([&](const std::string &data) { sendAll(conn.get(), data); }, media,
			      cfg);
	session.start();
	pumpSession(conn, session, stop, [&] {
		session.tick(SourceSession::Clock::now());
		return media.healthy() && extra();
	});
	return session.failed() ? EX_PROTOCOL : EX_OK;
}

} // namespace mc

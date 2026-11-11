// SPDX-License-Identifier: GPL-2.0-or-later
/** @file GStreamer capture+encode+send pipeline. */
#pragma once
#include <gst/gst.h>

#include <string>

#include "capture_settings.hpp"
#include "screen_capture.hpp"
#include "source_media.hpp"

namespace mc
{

/** Screen (or test pattern) capture, x264 encode, MPEG-TS mux and RTP send. */
class GstSourceMedia : public SourceMedia
{
public:
	/** Take the capture/encoding settings. */
	explicit GstSourceMedia(CaptureSettings settings);
	/** Stops the pipeline. */
	~GstSourceMedia() override;

	/** Start streaming to host:port, sending from localPort. */
	bool start(const StreamConfig &cfg, const std::string &host, uint16_t port,
		   uint16_t localPort) override;
	/** Stop and release the pipeline. */
	void stop() override;
	/** Force an IDR frame. */
	void requestKeyframe() override;
	/** False once the pipeline reported an error or EOS. */
	bool healthy();

private:
	/** Shutdown without virtual dispatch (safe from the destructor). */
	void release();
	/** Capture fragment to use; empty if nothing is available. */
	std::string captureFragment();

	CaptureSettings settings_ = {};
	ScreenCapture screen_ = {};
	GstElement *pipeline_ = nullptr;
};

} // namespace mc

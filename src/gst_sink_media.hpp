// SPDX-License-Identifier: GPL-2.0-or-later
/** @file GStreamer receive+render pipeline. */
#pragma once
#include <gst/gst.h>

#include <string>

#include "sink_media.hpp"

namespace mc
{

/** RTP / MPEG-TS / H.264 receive and render. */
class GstSinkMedia : public SinkMedia
{
public:
	/** Configure the render elements and jitter-buffer latency. */
	GstSinkMedia(std::string videoSink, std::string audioSink, int jitterMs);
	/** Stops the pipeline. */
	~GstSinkMedia() override;

	/** Start listening for RTP on the given UDP port. */
	bool start(uint16_t rtpPort) override;
	/** Stop and release the pipeline. */
	void stop() override;
	/** False once the pipeline reported an error or EOS. */
	bool healthy();
	/** Link a newly exposed decodebin pad to a render branch (GStreamer callback target). */
	void onPadAdded(GstPad *pad);

private:
	/** Shutdown without virtual dispatch (safe from the destructor). */
	void release();

	std::string videoSink_ = {};
	std::string audioSink_ = {};
	int jitterMs_ = {};
	GstElement *pipeline_ = nullptr;
};

} // namespace mc

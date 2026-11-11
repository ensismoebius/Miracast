// SPDX-License-Identifier: GPL-2.0-or-later
#include "gst_source_media.hpp"

#include <gst/video/video.h>

#include "encoder_chain.hpp"
#include "gst_util.hpp"
#include "util.hpp"

namespace mc
{

namespace
{
/** Muxer, RTP payloader and UDP sender; the video/audio chains link into "mux". */
std::string sendChain(const std::string &host, uint16_t port, uint16_t localPort)
{
	return "mpegtsmux name=mux alignment=7 ! rtpmp2tpay ! udpsink host=" + host +
	       " port=" + std::to_string(port) + " bind-port=" + std::to_string(localPort) +
	       " sync=false";
}
} // namespace

/** Take the capture/encoding settings. */
GstSourceMedia::GstSourceMedia(CaptureSettings settings) : settings_(std::move(settings))
{
}

/** Stops the pipeline. */
GstSourceMedia::~GstSourceMedia()
{
	release();
}

/** Shutdown without virtual dispatch (safe from the destructor). */
void GstSourceMedia::release()
{
	gstShutdown(pipeline_);
	screen_.stop();
}

/** Capture fragment to use; empty if nothing is available. */
std::string GstSourceMedia::captureFragment()
{
	if (!settings_.capture.empty())
		return settings_.capture;
	if (settings_.testPattern)
		return "videotestsrc is-live=true pattern=ball";
	if (gstFactoryExists("pipewiresrc")) {
		const auto node = screen_.start(settings_.output);
		// Compositors such as niri only offer DMA-BUF frames; vapostproc imports them.
		const char *importer = gstFactoryExists("vapostproc") ? " ! vapostproc" : "";
		if (node)
			return "pipewiresrc path=" + std::to_string(*node) + " do-timestamp=true" +
			       importer;
	}
	if (gstFactoryExists("ximagesrc"))
		return "ximagesrc use-damage=false show-pointer=true";
	return {};
}

/** Start streaming to host:port, sending from localPort. */
bool GstSourceMedia::start(const StreamConfig &cfg, const std::string &host, uint16_t port,
			   uint16_t localPort)
{
	release();
	gstInit();
	const std::string capture = captureFragment();
	if (capture.empty()) {
		logf(LogLevel::Error, "no capture source: use --capture or --test-pattern");
		return false;
	}
	std::string desc = sendChain(host, port, localPort) + " " + capture +
			   videoChain(cfg, settings_) + " ! queue ! mux.";
	if (cfg.audio)
		desc += " " + audioChain(settings_) + " ! mux.";
	logf(LogLevel::Debug, "pipeline: %s", desc.c_str());
	pipeline_ = gstLaunch(desc);
	if (pipeline_ == nullptr)
		return false;
	return gst_element_set_state(pipeline_, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;
}

/** Stop and release the pipeline. */
void GstSourceMedia::stop()
{
	release();
}

/** Force an IDR frame. */
void GstSourceMedia::requestKeyframe()
{
	if (pipeline_ == nullptr)
		return;
	GstElement *encoder = gst_bin_get_by_name(GST_BIN(pipeline_), "enc");
	if (encoder == nullptr)
		return;
	gst_element_send_event(
		encoder, gst_video_event_new_upstream_force_key_unit(GST_CLOCK_TIME_NONE, TRUE, 0));
	gst_object_unref(encoder);
}

/** False once the pipeline reported an error or EOS. */
bool GstSourceMedia::healthy()
{
	return gstBusHealthy(pipeline_);
}

} // namespace mc

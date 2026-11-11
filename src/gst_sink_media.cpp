// SPDX-License-Identifier: GPL-2.0-or-later
#include "gst_sink_media.hpp"

#include "gst_util.hpp"
#include "util.hpp"

namespace mc
{

namespace
{
/** C callback trampoline for decodebin's pad-added signal. */
void padAddedTrampoline(GstElement *, GstPad *pad, gpointer self)
{
	static_cast<GstSinkMedia *>(self)->onPadAdded(pad);
}

/** Add a bin built from desc to the pipeline and link pad to it. */
void linkBranch(GstElement *pipeline, GstPad *pad, const std::string &desc)
{
	GError *err = nullptr;
	GstElement *branch = gst_parse_bin_from_description(desc.c_str(), TRUE, &err);
	if (branch == nullptr) {
		if (err != nullptr)
			g_error_free(err);
		return;
	}
	gst_bin_add(GST_BIN(pipeline), branch);
	gst_element_sync_state_with_parent(branch);
	GstPad *sinkPad = gst_element_get_static_pad(branch, "sink");
	gst_pad_link(pad, sinkPad);
	gst_object_unref(sinkPad);
}

/** Media type ("video/x-raw", ...) of a pad. */
std::string padMediaType(GstPad *pad)
{
	GstCaps *caps = gst_pad_get_current_caps(pad);
	if (caps == nullptr)
		caps = gst_pad_query_caps(pad, nullptr);
	const std::string name = gst_structure_get_name(gst_caps_get_structure(caps, 0));
	gst_caps_unref(caps);
	return name;
}
} // namespace

/** Configure the render elements and jitter-buffer latency. */
GstSinkMedia::GstSinkMedia(std::string videoSink, std::string audioSink, int jitterMs)
    : videoSink_(std::move(videoSink)), audioSink_(std::move(audioSink)), jitterMs_(jitterMs)
{
}

/** Stops the pipeline. */
GstSinkMedia::~GstSinkMedia()
{
	release();
}

/** Shutdown without virtual dispatch (safe from the destructor). */
void GstSinkMedia::release()
{
	gstShutdown(pipeline_);
}

/** Start listening for RTP on the given UDP port. */
bool GstSinkMedia::start(uint16_t rtpPort)
{
	release();
	gstInit();
	const std::string desc =
		"udpsrc port=" + std::to_string(rtpPort) +
		" caps=\"application/x-rtp,media=(string)video,clock-rate=(int)90000,"
		"encoding-name=(string)MP2T,payload=(int)33\" ! rtpjitterbuffer latency=" +
		std::to_string(jitterMs_) + " ! rtpmp2tdepay ! decodebin name=dec";
	pipeline_ = gstLaunch(desc);
	if (pipeline_ == nullptr)
		return false;
	GstElement *decoder = gst_bin_get_by_name(GST_BIN(pipeline_), "dec");
	g_signal_connect(decoder, "pad-added", G_CALLBACK(padAddedTrampoline), this);
	gst_object_unref(decoder);
	return gst_element_set_state(pipeline_, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;
}

/** Link a newly exposed decodebin pad to a render branch (GStreamer callback target). */
void GstSinkMedia::onPadAdded(GstPad *pad)
{
	const std::string type = padMediaType(pad);
	if (startsWith(type, "video/"))
		linkBranch(pipeline_, pad, "queue ! videoconvert ! " + videoSink_ + " sync=false");
	else if (startsWith(type, "audio/"))
		linkBranch(pipeline_, pad, "queue ! audioconvert ! audioresample ! " + audioSink_);
}

/** Stop and release the pipeline. */
void GstSinkMedia::stop()
{
	release();
}

/** False once the pipeline reported an error or EOS. */
bool GstSinkMedia::healthy()
{
	return gstBusHealthy(pipeline_);
}

} // namespace mc

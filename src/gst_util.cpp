// SPDX-License-Identifier: GPL-2.0-or-later
#include "gst_util.hpp"

#include "util.hpp"

namespace mc
{

/** Initialise GStreamer once. */
void gstInit()
{
	static const bool done = [] {
		gst_init(nullptr, nullptr);
		return true;
	}();
	(void)done;
}

/** True when an element factory with this name is installed. */
bool gstFactoryExists(const char *name)
{
	GstElementFactory *factory = gst_element_factory_find(name);
	if (factory == nullptr)
		return false;
	gst_object_unref(factory);
	return true;
}

/** gst_parse_launch with error logging; nullptr on failure. */
GstElement *gstLaunch(const std::string &desc)
{
	GError *err = nullptr;
	GstElement *pipeline = gst_parse_launch(desc.c_str(), &err);
	if (err == nullptr)
		return pipeline;
	logf(LogLevel::Error, "pipeline error: %s", err->message);
	g_error_free(err);
	if (pipeline != nullptr)
		gst_object_unref(pipeline);
	return nullptr;
}

/** Drain the bus: false once an error or EOS was seen (errors are logged). */
bool gstBusHealthy(GstElement *pipeline)
{
	if (pipeline == nullptr)
		return true;
	const auto filter = static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
	GstBus *bus = gst_element_get_bus(pipeline);
	bool healthy = true;
	while (GstMessage *msg = gst_bus_pop_filtered(bus, filter)) {
		if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
			GError *err = nullptr;
			gst_message_parse_error(msg, &err, nullptr);
			logf(LogLevel::Error, "gstreamer: %s", err->message);
			g_error_free(err);
		}
		healthy = false;
		gst_message_unref(msg);
	}
	gst_object_unref(bus);
	return healthy;
}

/** Set the pipeline to NULL, release it and clear the pointer. */
void gstShutdown(GstElement *&pipeline)
{
	if (pipeline == nullptr)
		return;
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	pipeline = nullptr;
}

} // namespace mc

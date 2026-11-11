// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Small GStreamer helpers. */
#pragma once
#include <gst/gst.h>

#include <string>

namespace mc
{

/** Initialise GStreamer once. */
void gstInit();

/** True when an element factory with this name is installed. */
bool gstFactoryExists(const char *name);

/** gst_parse_launch with error logging; nullptr on failure. */
GstElement *gstLaunch(const std::string &desc);

/** Drain the bus: false once an error or EOS was seen (errors are logged). */
bool gstBusHealthy(GstElement *pipeline);

/** Set the pipeline to NULL, release it and clear the pointer. */
void gstShutdown(GstElement *&pipeline);

} // namespace mc

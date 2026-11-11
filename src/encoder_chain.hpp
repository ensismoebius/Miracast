// SPDX-License-Identifier: GPL-2.0-or-later
/** @file GStreamer pipeline fragments for encoding, muxing and audio. */
#pragma once
#include <string>

#include "capture_settings.hpp"
#include "stream_config.hpp"

namespace mc
{

/** Scale/convert, buffer, encode (selected encoder) and parse video; ends before the muxer. */
std::string videoChain(const StreamConfig &cfg, const CaptureSettings &set);

/** System-audio capture and AAC-LC (ADTS) encoding; ends before the muxer. */
std::string audioChain(const CaptureSettings &set);

} // namespace mc

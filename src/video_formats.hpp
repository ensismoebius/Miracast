// SPDX-License-Identifier: GPL-2.0-or-later
/** @file wfd_video_formats parameter. */
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "h264_codec.hpp"

namespace mc
{

/** The wfd_video_formats parameter value. */
struct VideoFormats {
	uint8_t native = 0;
	uint8_t preferredDisplay = 0;
	std::vector<H264Codec> codecs = {};

	/** Wire form, or "none" when there is no codec. */
	std::string serialize() const;
	/** Parse the wire form; nullopt when malformed. */
	static std::optional<VideoFormats> parse(const std::string &value);
};

} // namespace mc

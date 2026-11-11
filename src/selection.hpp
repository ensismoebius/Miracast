// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Choice of the video format a source will send. */
#pragma once
#include <optional>

#include "audio_codec.hpp"
#include "h264_codec.hpp"
#include "source_config.hpp"
#include "stream_config.hpp"
#include "video_formats.hpp"

namespace mc
{

/** The codec entry announced in M4 and the matching stream parameters. */
struct Selection {
	H264Codec codec = {};
	StreamConfig stream = {};
	std::optional<AudioCodec> audio = std::nullopt;
};

/** Best mode both sides support within the configured limits; nullopt if none fits. */
std::optional<Selection> selectFormat(const VideoFormats &sink, const SourceConfig &cfg);

/** AAC-LC 48 kHz stereo if the sink lists it (mode bit 0) and audio is enabled. */
std::optional<AudioCodec> selectAudio(const std::vector<AudioCodec> &sink, bool enabled);

} // namespace mc

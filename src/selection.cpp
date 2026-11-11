// SPDX-License-Identifier: GPL-2.0-or-later
#include "selection.hpp"

#include <algorithm>

#include "cea_mode.hpp"

namespace mc
{

namespace
{
/** Pixels per second, the ranking metric. */
long throughput(const CeaMode &mode)
{
	return static_cast<long>(mode.width) * mode.height * mode.fps;
}

/** Highest profile among the sink's CBP/CHP entries; nullptr if none. */
const H264Codec *bestCodec(const VideoFormats &sink)
{
	const H264Codec *best = nullptr;
	for (const H264Codec &codec : sink.codecs) {
		const bool known = codec.profile == kProfileCBP || codec.profile == kProfileCHP;
		if (known && (best == nullptr || codec.profile > best->profile))
			best = &codec;
	}
	return best;
}

/** True when the mode is usable with this codec entry and our limits. */
bool usable(const CeaMode &mode, const H264Codec &codec, const SourceConfig &cfg)
{
	if (mode.interlaced || (codec.cea & (1U << mode.bit)) == 0 || mode.level > codec.level)
		return false;
	const bool mandatory = mode.bit == 0; // 640x480p60 must always remain selectable
	return mandatory || (mode.width <= cfg.maxWidth && mode.fps <= cfg.maxFps);
}
} // namespace

/** Best mode both sides support within the configured limits; nullopt if none fits. */
std::optional<Selection> selectFormat(const VideoFormats &sink, const SourceConfig &cfg)
{
	const H264Codec *codec = bestCodec(sink);
	if (codec == nullptr)
		return std::nullopt;
	const CeaMode *best = nullptr;
	for (const CeaMode &mode : ceaModes()) {
		if (usable(mode, *codec, cfg) &&
		    (best == nullptr || throughput(mode) > throughput(*best)))
			best = &mode;
	}
	if (best == nullptr)
		return std::nullopt;
	Selection sel;
	sel.codec.profile = codec->profile;
	sel.codec.level = best->level;
	sel.codec.cea = 1U << best->bit;
	sel.stream.width = best->width;
	sel.stream.height = best->height;
	sel.stream.fps = best->fps;
	sel.stream.profile = codec->profile;
	sel.stream.level = best->level;
	sel.stream.bitrateKbps = cfg.bitrateKbps;
	return sel;
}

/** AAC-LC 48 kHz stereo if the sink lists it (mode bit 0) and audio is enabled. */
std::optional<AudioCodec> selectAudio(const std::vector<AudioCodec> &sink, bool enabled)
{
	const auto found = std::find_if(sink.begin(), sink.end(), [](const AudioCodec &codec) {
		return codec.name == "AAC" && (codec.modes & 0x1U) != 0;
	});
	if (!enabled || found == sink.end())
		return std::nullopt;
	return AudioCodec{"AAC", 0x1U, 0};
}

} // namespace mc

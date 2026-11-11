// SPDX-License-Identifier: GPL-2.0-or-later
/** @file One entry of wfd_audio_codecs. */
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace mc
{

/** Audio codec capability (LPCM, AAC, AC3) with its mode bitmap. */
struct AudioCodec {
	std::string name = {};
	uint32_t modes = 0;
	uint8_t latency = 0;

	/** Comma-separated wire form of a list, or "none". */
	static std::string serializeList(const std::vector<AudioCodec> &codecs);
	/** Parse a wfd_audio_codecs value; malformed entries are skipped. */
	static std::vector<AudioCodec> parseList(const std::string &value);
};

} // namespace mc

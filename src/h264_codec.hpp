// SPDX-License-Identifier: GPL-2.0-or-later
/** @file One H.264 entry of wfd_video_formats. */
#pragma once
#include <cstdint>
#include <optional>

namespace mc
{

constexpr uint8_t kProfileCBP = 0x01; /**< Constrained Baseline Profile bit. */
constexpr uint8_t kProfileCHP = 0x02; /**< Constrained High Profile bit. */

/** H.264 codec capability/selection tuple (profile, level and resolution masks). */
struct H264Codec {
	uint8_t profile = 0;
	uint8_t level = 0;
	uint32_t cea = 0;
	uint32_t vesa = 0;
	uint32_t hh = 0;
	uint8_t latency = 0;
	uint16_t minSlice = 0;
	uint16_t sliceEnc = 0;
	uint8_t frameRateCtrl = 0;
	std::optional<uint16_t> maxHres = {};
	std::optional<uint16_t> maxVres = {};
};

} // namespace mc

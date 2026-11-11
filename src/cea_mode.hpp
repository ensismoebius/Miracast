// SPDX-License-Identifier: GPL-2.0-or-later
/** @file CEA-861 resolution table used by wfd_video_formats. */
#pragma once
#include <cstdint>
#include <vector>

namespace mc
{

/** One CEA-861 mode; bit is its index in the wfd_video_formats CEA mask. */
struct CeaMode {
	int bit = {};
	int width = {};
	int height = {};
	int fps = {};
	bool interlaced = {};
	uint8_t level =
		{}; /**< Minimum H.264 level bit: 0x01=3.1 0x02=3.2 0x04=4 0x08=4.1 0x10=4.2 */
};

/** Entry for a mask bit, or nullptr. */
const CeaMode *ceaMode(int bit);

/** The whole table. */
const std::vector<CeaMode> &ceaModes();

/** Mask with every progressive mode of the table set. */
uint32_t progressiveCeaMask();

} // namespace mc

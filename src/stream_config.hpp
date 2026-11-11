// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Negotiated video stream parameters. */
#pragma once
#include <cstdint>

namespace mc
{

/** Resolution, frame rate and H.264 profile chosen for a session. */
struct StreamConfig {
	int width = 1280;
	int height = 720;
	int fps = 30;
	uint8_t profile = 0x01;
	uint8_t level = 0x01;
	int bitrateKbps = 6000;
	bool audio = false;
};

} // namespace mc

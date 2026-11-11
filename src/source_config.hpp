// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Source session settings. */
#pragma once
#include <cstdint>
#include <string>

namespace mc
{

/** Settings of a source session. */
struct SourceConfig {
	std::string localHost = {};
	std::string peerHost = {};
	uint16_t serverPort = 5004;
	int maxWidth = 1920;
	int maxFps = 60;
	int bitrateKbps = 8000;
	bool audio = true;
};

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Sink session settings. */
#pragma once
#include <cstdint>
#include <string>

namespace mc
{

/** Settings of a sink session. */
struct SinkConfig {
	uint16_t rtpPort = 19000;
	std::string peerHost = {};
};

} // namespace mc

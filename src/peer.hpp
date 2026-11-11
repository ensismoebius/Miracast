// SPDX-License-Identifier: GPL-2.0-or-later
/** @file A Wi-Fi Direct peer found during scanning. */
#pragma once
#include <cstdint>
#include <string>

#include "wfd_params.hpp"

namespace mc
{

/** A discovered P2P device and its WFD capabilities. */
struct Peer {
	std::string addr = {};
	std::string name = {};
	uint16_t wfdInfo = 0;
	uint16_t wfdPort = kDefaultRtspPort;
	bool hasWfd = false;
};

} // namespace mc

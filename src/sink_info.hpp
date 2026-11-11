// SPDX-License-Identifier: GPL-2.0-or-later
/** @file A sink found on the local network. */
#pragma once
#include <cstdint>
#include <string>

namespace mc
{

/** Name, address and MS-MICE port of a discovered Miracast-over-Infrastructure sink. */
struct SinkInfo {
	std::string name = {};
	std::string address = {};
	uint16_t port = 7250;
};

} // namespace mc

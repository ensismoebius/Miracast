// SPDX-License-Identifier: GPL-2.0-or-later
/** @file A formed Wi-Fi Direct group. */
#pragma once
#include <string>

namespace mc
{

/** A P2P group interface and our role in it. */
struct Group {
	std::string ifname = {};
	bool owner = false;
};

} // namespace mc

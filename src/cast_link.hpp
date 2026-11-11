// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Outcome of bringing up the link layer for the cast command. */
#pragma once
#include <sysexits.h>

#include <cstdint>
#include <string>

#include "group.hpp"
#include "wfd_params.hpp"

namespace mc
{

/** Link-layer state of a cast run. */
struct CastLink {
	int status = EX_OK;
	std::string sinkHost = {};
	uint16_t port = kDefaultRtspPort;
	bool haveGroup = false;
	Group group = {};
};

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Outcome of bringing up the link layer for the sink command. */
#pragma once
#include <sysexits.h>

#include <string>

#include "group.hpp"

namespace mc
{

/** Link-layer state of a sink run. */
struct SinkLink {
	int status = EX_OK;
	std::string bindAddr = {};
	bool haveGroup = false;
	Group group = {};
};

} // namespace mc

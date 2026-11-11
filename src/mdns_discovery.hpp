// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Discovery of Miracast-over-Infrastructure sinks through mDNS. */
#pragma once
#include <atomic>
#include <vector>

#include "sink_info.hpp"

namespace mc
{

/** Query _display._tcp for timeoutMs and return every sink that answered (deduplicated by address).
 */
std::vector<SinkInfo> discoverSinks(int timeoutMs, const std::atomic<bool> &stop);

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Resolve the sink the user asked for (name, address or the only one found). */
#pragma once
#include <atomic>
#include <optional>

#include "options.hpp"
#include "sink_info.hpp"

namespace mc
{

/** Sink matching opts.target (name or IPv4); with no target, the only sink on the network. */
std::optional<SinkInfo> findInfrastructureSink(const Options &opts, const std::atomic<bool> &stop);

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file The scan, cast and sink commands. */
#pragma once
#include <atomic>

#include "options.hpp"

namespace mc
{

/** List WFD devices; returns a sysexits status. */
int runScan(const Options &opts, const std::atomic<bool> &stop);

/** Source role; returns a sysexits status. */
int runCast(const Options &opts, const std::atomic<bool> &stop);

/** Miracast over Infrastructure (MS-MICE) source; returns a sysexits status. */
int runCastMice(const Options &opts, const std::atomic<bool> &stop);

/** Sink role; returns a sysexits status. */
int runSink(const Options &opts, const std::atomic<bool> &stop);

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Runs a source session over an established RTSP connection. */
#pragma once
#include <atomic>
#include <functional>
#include <string>

#include "file_descriptor.hpp"
#include "options.hpp"

namespace mc
{

/** Stream to peerHost over conn; extra() returning false ends the session. Returns a sysexits
 * status. */
int runSourceSession(const FileDescriptor &conn, const Options &opts, const std::string &peerHost,
		     const std::atomic<bool> &stop, const std::function<bool()> &extra);

} // namespace mc

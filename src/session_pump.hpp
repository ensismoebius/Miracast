// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Event loop that feeds a socket into an RTSP session. */
#pragma once
#include <atomic>
#include <functional>

#include "file_descriptor.hpp"
#include "rtsp_peer.hpp"

namespace mc
{

/** Pump conn into session until it finishes or stop is set; extra() returning false aborts. */
void pumpSession(const FileDescriptor &conn, RtspPeer &session, const std::atomic<bool> &stop,
		 const std::function<bool()> &extra);

} // namespace mc

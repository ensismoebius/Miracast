// SPDX-License-Identifier: GPL-2.0-or-later
/** @file MS-MICE (Miracast over Infrastructure) message encoding, per [MS-MICE] section 2.2. */
#pragma once
#include <cstdint>
#include <optional>
#include <string>

#include "mice_command.hpp"

namespace mc
{

constexpr uint16_t kMicePort = 7250; /**< TCP port the sink listens on. */

/** UTF-8 to UTF-16LE (no terminator, no BOM). */
std::string utf16le(const std::string &utf8);

/** Random 16-byte Source ID (raw bytes). */
std::string miceRandomSourceId();

/** SOURCE_READY: friendly name, RTSP port and source id TLVs. */
std::string miceSourceReady(const std::string &name, uint16_t rtspPort,
			    const std::string &sourceId);

/** STOP_PROJECTION: friendly name and source id TLVs. */
std::string miceStopProjection(const std::string &name, const std::string &sourceId);

/** Size of the first complete frame in buf, or 0 when it is still incomplete/invalid. */
size_t miceFrameSize(const std::string &buf);

/** Command of the frame at the start of buf, if it is complete and version 1. */
std::optional<MiceCommand> miceCommandOf(const std::string &buf);

} // namespace mc

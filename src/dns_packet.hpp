// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Minimal mDNS (DNS wire format) encoding and decoding for _display._tcp discovery. */
#pragma once
#include <optional>
#include <string>

#include "sink_info.hpp"

namespace mc
{

/** Multicast DNS name that Miracast sinks register (_display._tcp.local). */
extern const char *const kDisplayService;

/** PTR query for a service name, with the unicast-response (QU) bit set. */
std::string buildPtrQuery(const std::string &service);

/** Sink described by a response (instance name from PTR, port from SRV); address is left empty. */
std::optional<SinkInfo> parseDisplayResponse(const std::string &packet);

} // namespace mc

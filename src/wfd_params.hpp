// SPDX-License-Identifier: GPL-2.0-or-later
/** @file WFD text/parameters handling, WFD IE helpers and RTSP header parsing. */
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "wfd_device_type.hpp"

namespace mc
{

constexpr uint16_t kDefaultRtspPort = 7236;
extern const char *const kWfdRequire; /**< "org.wfa.wfd1.0" */

/** "key: value" pairs of a text/parameters body. */
using ParamList = std::vector<std::pair<std::string, std::string>>;

/** Parse every "key: value" line. */
ParamList parseParams(const std::string &body);

/** Value of the first parameter with the given key. */
std::optional<std::string> findParam(const ParamList &params, const std::string &key);

/** Bare names of a GET_PARAMETER body (lines without a colon). */
std::vector<std::string> parseParamNames(const std::string &body);

/** WFD device information bits: type plus session availability. */
uint16_t wfdDeviceInfoBits(WfdDeviceType type, bool sessionAvailable);

/** Hex payload for wpa_supplicant "WFD_SUBELEM_SET 0 <hex>" (length-prefixed). */
std::string wfdDeviceInfoSubelem(WfdDeviceType type, uint16_t rtspPort, uint16_t maxThroughputMbps);

/** Extract device info (and RTSP port) from a wpa_supplicant wfd_dev_info hex string. */
std::optional<uint16_t> parseWfdDevInfo(const std::string &hex, uint16_t *port);

/** True when the device-info bits describe a (primary or dual) sink. */
bool wfdIsSink(uint16_t info);

/** True when the device-info bits describe a (or dual) source. */
bool wfdIsSource(uint16_t info);

/** Session id of a Session header (drops ";timeout=..."). */
std::string sessionId(const std::string &header);

/** First client_port of a Transport header. */
std::optional<uint16_t> parseClientPort(const std::string &transport);

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
#include "wfd_params.hpp"

#include <algorithm>

#include "util.hpp"

namespace mc
{

const char *const kWfdRequire = "org.wfa.wfd1.0";

/** Parse every "key: value" line. */
ParamList parseParams(const std::string &body)
{
	ParamList out;
	for (const auto &raw : split(body, '\n')) {
		const std::string line = trim(raw);
		const size_t colon = line.find(':');
		if (colon != std::string::npos && colon != 0)
			out.emplace_back(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
	}
	return out;
}

/** Value of the first parameter with the given key. */
std::optional<std::string> findParam(const ParamList &params, const std::string &key)
{
	const auto found = std::find_if(params.begin(), params.end(),
					[&](const auto &entry) { return entry.first == key; });
	if (found == params.end())
		return std::nullopt;
	return found->second;
}

/** Bare names of a GET_PARAMETER body (lines without a colon). */
std::vector<std::string> parseParamNames(const std::string &body)
{
	std::vector<std::string> out;
	for (const auto &raw : split(body, '\n')) {
		const std::string line = trim(raw);
		if (!line.empty())
			out.push_back(line);
	}
	return out;
}

/** WFD device information bits: type plus session availability. */
uint16_t wfdDeviceInfoBits(WfdDeviceType type, bool sessionAvailable)
{
	// bits 1:0 device type, bits 5:4 session availability (01 = available)
	unsigned info = static_cast<unsigned>(type);
	if (sessionAvailable)
		info |= 1U << 4;
	return static_cast<uint16_t>(info);
}

/** Hex payload for wpa_supplicant "WFD_SUBELEM_SET 0 <hex>" (length-prefixed). */
std::string wfdDeviceInfoSubelem(WfdDeviceType type, uint16_t rtspPort, uint16_t maxThroughputMbps)
{
	return "0006" + hexString(wfdDeviceInfoBits(type, true), 4) + hexString(rtspPort, 4) +
	       hexString(maxThroughputMbps, 4);
}

/** Extract device info (and RTSP port) from a wpa_supplicant wfd_dev_info hex string. */
std::optional<uint16_t> parseWfdDevInfo(const std::string &hex, uint16_t *port)
{
	const std::string digits = startsWith(hex, "0x") ? hex.substr(2) : hex;
	if (digits.size() < 12)
		return std::nullopt; // trailing 12 digits: info(4) port(4) throughput(4)
	const std::string tail = digits.substr(digits.size() - 12);
	const auto info = parseHex(tail.substr(0, 4));
	const auto rtsp = parseHex(tail.substr(4, 4));
	if (!info || !rtsp)
		return std::nullopt;
	if (port != nullptr)
		*port = static_cast<uint16_t>(*rtsp);
	return static_cast<uint16_t>(*info);
}

/** True when the device-info bits describe a (primary or dual) sink. */
bool wfdIsSink(uint16_t info)
{
	const unsigned type = info & 0x3U;
	return type == 1 || type == 3;
}

/** True when the device-info bits describe a (or dual) source. */
bool wfdIsSource(uint16_t info)
{
	const unsigned type = info & 0x3U;
	return type == 0 || type == 3;
}

/** Session id of a Session header (drops ";timeout=..."). */
std::string sessionId(const std::string &header)
{
	return trim(split(header, ';')[0]);
}

/** First client_port of a Transport header. */
std::optional<uint16_t> parseClientPort(const std::string &transport)
{
	for (const auto &part : split(transport, ';')) {
		const std::string field = trim(part);
		if (!startsWith(field, "client_port="))
			continue;
		const auto port = parseDec(split(field.substr(12), '-')[0]);
		if (port && *port > 0 && *port < 65536)
			return static_cast<uint16_t>(*port);
	}
	return std::nullopt;
}

} // namespace mc

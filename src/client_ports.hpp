// SPDX-License-Identifier: GPL-2.0-or-later
/** @file wfd_client_rtp_ports parameter. */
#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace mc
{

/** The wfd_client_rtp_ports parameter (UDP unicast RTP/RTCP port pair). */
struct ClientPorts {
	uint16_t port0 = 0;
	uint16_t port1 = 0;

	/** Wire form, e.g. "RTP/AVP/UDP;unicast 19000 0 mode=play". */
	std::string serialize() const;
	/** Parse the wire form; nullopt when malformed. */
	static std::optional<ClientPorts> parse(const std::string &value);
};

} // namespace mc

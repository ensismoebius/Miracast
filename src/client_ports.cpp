// SPDX-License-Identifier: GPL-2.0-or-later
#include "client_ports.hpp"

#include "util.hpp"

namespace mc
{

/** Wire form, e.g. "RTP/AVP/UDP;unicast 19000 0 mode=play". */
std::string ClientPorts::serialize() const
{
	return "RTP/AVP/UDP;unicast " + std::to_string(port0) + " " + std::to_string(port1) +
	       " mode=play";
}

/** Parse the wire form; nullopt when malformed. */
std::optional<ClientPorts> ClientPorts::parse(const std::string &value)
{
	const auto tok = words(value);
	if (tok.size() < 3 || !startsWith(tok[0], "RTP/AVP"))
		return std::nullopt;
	const auto first = parseDec(tok[1]);
	const auto second = parseDec(tok[2]);
	if (!first || !second || *first > 65535 || *second > 65535)
		return std::nullopt;
	ClientPorts ports;
	ports.port0 = static_cast<uint16_t>(*first);
	ports.port1 = static_cast<uint16_t>(*second);
	return ports;
}

} // namespace mc

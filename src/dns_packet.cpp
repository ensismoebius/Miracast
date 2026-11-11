// SPDX-License-Identifier: GPL-2.0-or-later
#include "dns_packet.hpp"

#include "util.hpp"

namespace mc
{

const char *const kDisplayService = "_display._tcp.local";

namespace
{
constexpr size_t kHeaderSize = 12;
constexpr uint16_t kTypePtr = 12;
constexpr uint16_t kTypeSrv = 33;
constexpr int kMaxPointerHops = 16;

/** Big-endian 16-bit value at offset (caller checks bounds). */
uint16_t be16(const std::string &data, size_t off)
{
	return static_cast<uint16_t>((static_cast<unsigned char>(data[off]) << 8) |
				     static_cast<unsigned char>(data[off + 1]));
}

/** Read a possibly compressed name at off; next receives the offset after it. */
std::optional<std::string> readName(const std::string &pkt, size_t off, size_t &next)
{
	std::string name;
	bool jumped = false;
	for (int hops = 0; hops < kMaxPointerHops && off < pkt.size();) {
		const auto len = static_cast<unsigned char>(pkt[off]);
		if (len == 0) {
			next = jumped ? next : off + 1;
			return name;
		}
		if ((len & 0xC0) == 0xC0 && off + 1 < pkt.size()) {
			next = jumped ? next : off + 2;
			off = static_cast<size_t>(be16(pkt, off) & 0x3FFF);
			jumped = true;
			++hops;
			continue;
		}
		if (off + 1 + len > pkt.size())
			return std::nullopt;
		name += (name.empty() ? "" : ".") + pkt.substr(off + 1, len);
		off += 1 + len;
	}
	return std::nullopt;
}

/** Apply one resource record's data to info. */
void applyRecord(const std::string &pkt, uint16_t type, size_t data, size_t len, SinkInfo &info)
{
	if (type == kTypePtr) {
		size_t after = 0;
		const auto target = readName(pkt, data, after);
		if (target && target->find(kDisplayService) != std::string::npos)
			info.name = split(*target, '.')[0];
	} else if (type == kTypeSrv && len >= 6) {
		info.port = be16(pkt, data + 4);
	}
}
} // namespace

/** PTR query for a service name, with the unicast-response (QU) bit set. */
std::string buildPtrQuery(const std::string &service)
{
	std::string out(kHeaderSize, '\0');
	out[5] = 1; // QDCOUNT = 1
	for (const auto &label : split(service, '.')) {
		out.push_back(static_cast<char>(label.size()));
		out += label;
	}
	out.push_back('\0');
	out += std::string("\x00\x0c\x80\x01", 4); // PTR, IN | QU
	return out;
}

/** Sink described by a response (instance name from PTR, port from SRV); address is left empty. */
std::optional<SinkInfo> parseDisplayResponse(const std::string &packet)
{
	if (packet.size() < kHeaderSize)
		return std::nullopt;
	size_t off = kHeaderSize;
	size_t next = 0;
	for (unsigned idx = be16(packet, 4); idx > 0; --idx) {
		if (!readName(packet, off, next))
			return std::nullopt;
		off = next + 4;
	}
	SinkInfo info;
	const unsigned records = be16(packet, 6) + be16(packet, 8) + be16(packet, 10);
	for (unsigned idx = 0; idx < records && readName(packet, off, next); ++idx) {
		if (next + 10 > packet.size())
			break;
		const uint16_t type = be16(packet, next);
		const size_t len = be16(packet, next + 8);
		if (next + 10 + len > packet.size())
			break;
		applyRecord(packet, type, next + 10, len, info);
		off = next + 10 + len;
	}
	if (info.name.empty())
		return std::nullopt;
	return info;
}

} // namespace mc

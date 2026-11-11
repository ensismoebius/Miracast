// SPDX-License-Identifier: GPL-2.0-or-later
#include "mice_protocol.hpp"

#include <random>

namespace mc
{

namespace
{
constexpr uint8_t kVersion = 0x01;
constexpr uint8_t kTlvFriendlyName = 0x00;
constexpr uint8_t kTlvRtspPort = 0x02;
constexpr uint8_t kTlvSourceId = 0x03;
constexpr size_t kHeaderSize = 4;
constexpr size_t kSourceIdSize = 16;

/** Append a 16-bit big-endian value. */
void putBe16(std::string &out, uint16_t value)
{
	out.push_back(static_cast<char>(value >> 8));
	out.push_back(static_cast<char>(value & 0xFF));
}

/** Append one TLV: type(1) length(2, big-endian) value. */
void putTlv(std::string &out, uint8_t type, const std::string &value)
{
	out.push_back(static_cast<char>(type));
	putBe16(out, static_cast<uint16_t>(value.size()));
	out += value;
}

/** Wrap TLVs into a message: size(2) version(1) command(1) TLVArray. */
std::string frame(MiceCommand command, const std::string &tlvs)
{
	std::string out;
	putBe16(out, static_cast<uint16_t>(kHeaderSize + tlvs.size()));
	out.push_back(static_cast<char>(kVersion));
	out.push_back(static_cast<char>(command));
	return out + tlvs;
}

/** Append a UTF-16LE code unit. */
void putUnit(std::string &out, uint32_t unit)
{
	out.push_back(static_cast<char>(unit & 0xFF));
	out.push_back(static_cast<char>((unit >> 8) & 0xFF));
}

/** Number of continuation bytes following a UTF-8 lead byte. */
size_t continuationBytes(unsigned char lead)
{
	if (lead >= 0xF0)
		return 3;
	if (lead >= 0xE0)
		return 2;
	return lead >= 0xC0 ? 1 : 0;
}

/** Payload bits of a UTF-8 lead byte. */
uint32_t leadBits(unsigned char lead, size_t extra)
{
	static const unsigned char kMask[] = {0x7F, 0x1F, 0x0F, 0x07};
	return lead & kMask[extra];
}
} // namespace

/** UTF-8 to UTF-16LE (no terminator, no BOM). */
std::string utf16le(const std::string &utf8)
{
	std::string out;
	for (size_t pos = 0; pos < utf8.size();) {
		const auto lead = static_cast<unsigned char>(utf8[pos]);
		const size_t extra = continuationBytes(lead);
		uint32_t point = leadBits(lead, extra);
		for (size_t idx = 1; idx <= extra && pos + idx < utf8.size(); ++idx)
			point = (point << 6) | (static_cast<unsigned char>(utf8[pos + idx]) & 0x3F);
		pos += extra + 1;
		if (point >= 0x10000) {
			point -= 0x10000;
			putUnit(out, 0xD800 + (point >> 10));
			putUnit(out, 0xDC00 + (point & 0x3FF));
		} else {
			putUnit(out, point);
		}
	}
	return out;
}

/** Random 16-byte Source ID (raw bytes). */
std::string miceRandomSourceId()
{
	std::random_device device;
	std::string out;
	for (size_t idx = 0; idx < kSourceIdSize; ++idx)
		out.push_back(static_cast<char>(device() & 0xFF));
	return out;
}

/** SOURCE_READY: friendly name, RTSP port and source id TLVs. */
std::string miceSourceReady(const std::string &name, uint16_t rtspPort, const std::string &sourceId)
{
	std::string port;
	putBe16(port, rtspPort);
	std::string tlvs;
	putTlv(tlvs, kTlvFriendlyName, utf16le(name));
	putTlv(tlvs, kTlvRtspPort, port);
	putTlv(tlvs, kTlvSourceId, sourceId);
	return frame(MiceCommand::SourceReady, tlvs);
}

/** STOP_PROJECTION: friendly name and source id TLVs. */
std::string miceStopProjection(const std::string &name, const std::string &sourceId)
{
	std::string tlvs;
	putTlv(tlvs, kTlvFriendlyName, utf16le(name));
	putTlv(tlvs, kTlvSourceId, sourceId);
	return frame(MiceCommand::StopProjection, tlvs);
}

/** Size of the first complete frame in buf, or 0 when it is still incomplete/invalid. */
size_t miceFrameSize(const std::string &buf)
{
	if (buf.size() < kHeaderSize)
		return 0;
	const size_t size =
		(static_cast<unsigned char>(buf[0]) << 8) | static_cast<unsigned char>(buf[1]);
	return size >= kHeaderSize && size <= buf.size() ? size : 0;
}

/** Command of the frame at the start of buf, if it is complete and version 1. */
std::optional<MiceCommand> miceCommandOf(const std::string &buf)
{
	if (miceFrameSize(buf) == 0 || static_cast<uint8_t>(buf[2]) != kVersion)
		return std::nullopt;
	return static_cast<MiceCommand>(static_cast<uint8_t>(buf[3]));
}

} // namespace mc

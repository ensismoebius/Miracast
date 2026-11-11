// SPDX-License-Identifier: GPL-2.0-or-later
#include "video_formats.hpp"

#include "util.hpp"

namespace mc
{

namespace
{
/** Four hex digits, or "none" when absent. */
std::string hexOrNone(const std::optional<uint16_t> &value)
{
	return value ? hexString(*value, 4) : std::string("none");
}

/** Optional 16-bit field: "none" or hex. */
std::optional<uint16_t> optionalField(const std::string &token)
{
	const auto value = parseHex(token);
	if (!value)
		return std::nullopt;
	return static_cast<uint16_t>(*value);
}

/** Parse the 11 numeric tokens of one codec entry starting at index first. */
std::optional<H264Codec> parseCodec(const std::vector<std::string> &tok, size_t first)
{
	if (tok.size() < first + 11)
		return std::nullopt;
	std::optional<uint32_t> num[9];
	for (size_t idx = 0; idx < 9; ++idx) {
		num[idx] = parseHex(tok[first + idx]);
		if (!num[idx])
			return std::nullopt;
	}
	H264Codec codec;
	codec.profile = static_cast<uint8_t>(*num[0]);
	codec.level = static_cast<uint8_t>(*num[1]);
	codec.cea = *num[2];
	codec.vesa = *num[3];
	codec.hh = *num[4];
	codec.latency = static_cast<uint8_t>(*num[5]);
	codec.minSlice = static_cast<uint16_t>(*num[6]);
	codec.sliceEnc = static_cast<uint16_t>(*num[7]);
	codec.frameRateCtrl = static_cast<uint8_t>(*num[8]);
	codec.maxHres = optionalField(tok[first + 9]);
	codec.maxVres = optionalField(tok[first + 10]);
	return codec;
}

/** Wire form of a single codec entry. */
std::string codecText(const H264Codec &codec)
{
	return hexString(codec.profile, 2) + " " + hexString(codec.level, 2) + " " +
	       hexString(codec.cea, 8) + " " + hexString(codec.vesa, 8) + " " +
	       hexString(codec.hh, 8) + " " + hexString(codec.latency, 2) + " " +
	       hexString(codec.minSlice, 4) + " " + hexString(codec.sliceEnc, 4) + " " +
	       hexString(codec.frameRateCtrl, 2) + " " + hexOrNone(codec.maxHres) + " " +
	       hexOrNone(codec.maxVres);
}
} // namespace

/** Wire form, or "none" when there is no codec. */
std::string VideoFormats::serialize() const
{
	if (codecs.empty())
		return "none";
	std::string out = hexString(native, 2) + " " + hexString(preferredDisplay, 2) + " ";
	for (size_t idx = 0; idx < codecs.size(); ++idx)
		out += (idx > 0 ? ", " : "") + codecText(codecs[idx]);
	return out;
}

/** Parse the wire form; nullopt when malformed. */
std::optional<VideoFormats> VideoFormats::parse(const std::string &value)
{
	VideoFormats result;
	if (toLower(trim(value)) == "none")
		return result;
	const auto segments = split(value, ',');
	for (size_t idx = 0; idx < segments.size(); ++idx) {
		const auto tok = words(segments[idx]);
		size_t first = 0;
		if (idx == 0) {
			const auto native = tok.size() >= 2 ? parseHex(tok[0]) : std::nullopt;
			const auto pref = tok.size() >= 2 ? parseHex(tok[1]) : std::nullopt;
			if (!native || !pref)
				return std::nullopt;
			result.native = static_cast<uint8_t>(*native);
			result.preferredDisplay = static_cast<uint8_t>(*pref);
			first = 2;
		}
		const auto codec = parseCodec(tok, first);
		if (!codec)
			return std::nullopt;
		result.codecs.push_back(*codec);
	}
	return result;
}

} // namespace mc

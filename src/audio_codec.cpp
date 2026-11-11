// SPDX-License-Identifier: GPL-2.0-or-later
#include "audio_codec.hpp"

#include "util.hpp"

namespace mc
{

/** Comma-separated wire form of a list, or "none". */
std::string AudioCodec::serializeList(const std::vector<AudioCodec> &codecs)
{
	if (codecs.empty())
		return "none";
	std::string out;
	for (size_t idx = 0; idx < codecs.size(); ++idx) {
		out += (idx > 0 ? ", " : "") + codecs[idx].name + " " +
		       hexString(codecs[idx].modes, 8) + " " + hexString(codecs[idx].latency, 2);
	}
	return out;
}

/** Parse a wfd_audio_codecs value; malformed entries are skipped. */
std::vector<AudioCodec> AudioCodec::parseList(const std::string &value)
{
	std::vector<AudioCodec> out;
	if (toLower(trim(value)) == "none")
		return out;
	for (const auto &segment : split(value, ',')) {
		const auto tok = words(segment);
		const auto modes = tok.size() >= 3 ? parseHex(tok[1]) : std::nullopt;
		const auto latency = tok.size() >= 3 ? parseHex(tok[2]) : std::nullopt;
		if (modes && latency)
			out.push_back({tok[0], *modes, static_cast<uint8_t>(*latency)});
	}
	return out;
}

} // namespace mc

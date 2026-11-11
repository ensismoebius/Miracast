// SPDX-License-Identifier: GPL-2.0-or-later
#include "util.hpp"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <iterator>
#include <numeric>
#include <sstream>
#include <ctime>

namespace mc
{

namespace
{
LogLevel g_level = LogLevel::Info;

/** Three-letter tag printed in front of each record. */
const char *levelName(LogLevel level)
{
	switch (level) {
	case LogLevel::Error:
		return "ERR";
	case LogLevel::Warn:
		return "WRN";
	case LogLevel::Info:
		return "INF";
	case LogLevel::Debug:
		return "DBG";
	}
	return "???";
}

/** Value of one hex digit; the caller has validated it already. */
uint32_t hexDigit(char chr)
{
	if (chr <= '9')
		return static_cast<uint32_t>(chr - '0');
	return static_cast<uint32_t>(std::tolower(chr) - 'a' + 10);
}

/** isspace() on a plain char without sign-extension surprises. */
bool isBlank(char chr)
{
	return std::isspace(static_cast<unsigned char>(chr)) != 0;
}
} // namespace

/** Set the minimum severity that is written to stderr. */
void setLogLevel(LogLevel level)
{
	g_level = level;
}

/** printf-style logging to stderr with a timestamp. */
void logf(LogLevel level, const char *fmt, ...)
{
	if (level > g_level)
		return;
	char body[1024];
	va_list args;
	va_start(args, fmt);
	std::vsnprintf(body, sizeof body, fmt, args);
	va_end(args);
	const std::time_t now = std::time(nullptr);
	std::tm local{};
	localtime_r(&now, &local);
	char stamp[16];
	std::strftime(stamp, sizeof stamp, "%H:%M:%S", &local);
	std::fprintf(stderr, "%s [%s] %s\n", stamp, levelName(level), body);
}

/** Strip leading and trailing ASCII whitespace. */
std::string trim(const std::string &text)
{
	const auto first = std::find_if_not(text.begin(), text.end(), isBlank);
	const auto last = std::find_if_not(text.rbegin(), text.rend(), isBlank).base();
	return first < last ? std::string(first, last) : std::string();
}

/** Split on a single separator character; keeps empty fields. */
std::vector<std::string> split(const std::string &text, char sep)
{
	std::vector<std::string> out;
	size_t start = 0;
	while (true) {
		const size_t pos = text.find(sep, start);
		if (pos == std::string::npos) {
			out.push_back(text.substr(start));
			return out;
		}
		out.push_back(text.substr(start, pos - start));
		start = pos + 1;
	}
}

/** Whitespace-separated words (no empty entries). */
std::vector<std::string> words(const std::string &text)
{
	std::istringstream stream(text);
	return std::vector<std::string>(std::istream_iterator<std::string>(stream),
					std::istream_iterator<std::string>());
}

/** ASCII lower-casing. */
std::string toLower(std::string text)
{
	std::transform(text.begin(), text.end(), text.begin(),
		       [](unsigned char chr) { return static_cast<char>(std::tolower(chr)); });
	return text;
}

/** True when text begins with prefix. */
bool startsWith(const std::string &text, const std::string &prefix)
{
	return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

/** Parse up to 8 hexadecimal digits. */
std::optional<uint32_t> parseHex(const std::string &text)
{
	const auto isHex = [](char chr) {
		return std::isxdigit(static_cast<unsigned char>(chr)) != 0;
	};
	if (text.empty() || text.size() > 8 || !std::all_of(text.begin(), text.end(), isHex))
		return std::nullopt;
	return std::accumulate(text.begin(), text.end(), uint32_t{0},
			       [](uint32_t acc, char chr) { return (acc << 4) | hexDigit(chr); });
}

/** Parse a non-negative decimal number of at most 9 digits. */
std::optional<long> parseDec(const std::string &text)
{
	const auto isDec = [](char chr) {
		return std::isdigit(static_cast<unsigned char>(chr)) != 0;
	};
	if (text.empty() || text.size() > 9 || !std::all_of(text.begin(), text.end(), isDec))
		return std::nullopt;
	return std::stol(text);
}

/** Zero-padded lowercase hexadecimal rendering. */
std::string hexString(uint32_t value, int width)
{
	char buf[16];
	std::snprintf(buf, sizeof buf, "%0*x", width, value);
	return buf;
}

} // namespace mc

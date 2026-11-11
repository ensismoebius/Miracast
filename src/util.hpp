// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Logging and string helpers shared by every module. */
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mc
{

/** Severity of a log record. */
enum class LogLevel { Error = 0, Warn = 1, Info = 2, Debug = 3 };

/** Set the minimum severity that is written to stderr. */
void setLogLevel(LogLevel level);

/** printf-style logging to stderr with a timestamp. */
void logf(LogLevel level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/** Strip leading and trailing ASCII whitespace. */
std::string trim(const std::string &text);

/** Split on a single separator character; keeps empty fields. */
std::vector<std::string> split(const std::string &text, char sep);

/** Whitespace-separated words (no empty entries). */
std::vector<std::string> words(const std::string &text);

/** ASCII lower-casing. */
std::string toLower(std::string text);

/** True when text begins with prefix. */
bool startsWith(const std::string &text, const std::string &prefix);

/** Parse up to 8 hexadecimal digits. */
std::optional<uint32_t> parseHex(const std::string &text);

/** Parse a non-negative decimal number of at most 9 digits. */
std::optional<long> parseDec(const std::string &text);

/** Zero-padded lowercase hexadecimal rendering. */
std::string hexString(uint32_t value, int width);

} // namespace mc

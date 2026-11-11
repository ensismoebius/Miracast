// SPDX-License-Identifier: GPL-2.0-or-later
#include "parser.hpp"

#include "util.hpp"

namespace mc
{

namespace
{
constexpr size_t kMaxHeaderBytes = 8192;
constexpr size_t kMaxBodyBytes = 65536;

/** Fill the request or status line fields; false on a malformed line. */
bool parseStartLine(const std::string &line, Message &out)
{
	const auto parts = split(line, ' ');
	if (parts.size() < 3)
		return false;
	if (startsWith(parts[0], "RTSP/")) {
		const auto code = parseDec(parts[1]);
		if (!code)
			return false;
		out.status = static_cast<int>(*code);
		out.reason = trim(line.substr(parts[0].size() + parts[1].size() + 2));
		return true;
	}
	if (!startsWith(parts[2], "RTSP/"))
		return false;
	out.isRequest = true;
	out.method = parts[0];
	out.uri = parts[1];
	return true;
}
} // namespace

/** Append received bytes. */
void Parser::feed(const std::string &data)
{
	if (!failed_)
		buf_ += data;
}

/** Parse the start line and headers into out. */
bool Parser::parseHead(const std::string &head, Message &out)
{
	const auto lines = split(head, '\n');
	if (!parseStartLine(trim(lines[0]), out))
		return false;
	for (size_t idx = 1; idx < lines.size(); ++idx) {
		const std::string line = trim(lines[idx]);
		const size_t colon = line.find(':');
		if (line.empty())
			continue;
		if (colon == std::string::npos || colon == 0)
			return false;
		out.headers.emplace_back(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
	}
	return true;
}

/** Body length from Content-Length, or nullopt when invalid. */
std::optional<size_t> Parser::bodyLength(const Message &msg)
{
	const auto header = msg.header("Content-Length");
	if (!header)
		return 0;
	const auto length = parseDec(trim(*header));
	if (!length || static_cast<size_t>(*length) > kMaxBodyBytes)
		return std::nullopt;
	return static_cast<size_t>(*length);
}

/** Pop the next complete message, if any. */
std::optional<Message> Parser::next()
{
	if (failed_)
		return std::nullopt;
	const size_t end = buf_.find("\r\n\r\n");
	if (end == std::string::npos || end > kMaxHeaderBytes) {
		failed_ = end != std::string::npos || buf_.size() > kMaxHeaderBytes;
		return std::nullopt;
	}
	Message msg;
	const auto length = parseHead(buf_.substr(0, end), msg) ? bodyLength(msg) : std::nullopt;
	if (!length) {
		failed_ = true;
		return std::nullopt;
	}
	const size_t total = end + 4 + *length;
	if (buf_.size() < total)
		return std::nullopt;
	msg.body = buf_.substr(end + 4, *length);
	buf_.erase(0, total);
	return msg;
}

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
#include "message.hpp"

#include <algorithm>

#include "util.hpp"

namespace mc
{

/** Case-insensitive header lookup. */
std::optional<std::string> Message::header(const std::string &name) const
{
	const std::string wanted = toLower(name);
	const auto found = std::find_if(headers.begin(), headers.end(), [&](const auto &entry) {
		return toLower(entry.first) == wanted;
	});
	if (found == headers.end())
		return std::nullopt;
	return found->second;
}

/** Set or replace a header. */
void Message::set(const std::string &name, const std::string &value)
{
	const std::string wanted = toLower(name);
	const auto found = std::find_if(headers.begin(), headers.end(), [&](const auto &entry) {
		return toLower(entry.first) == wanted;
	});
	if (found != headers.end())
		found->second = value;
	else
		headers.emplace_back(name, value);
}

/** CSeq value, or -1 when absent or malformed. */
int Message::cseq() const
{
	const auto value = header("CSeq");
	if (!value)
		return -1;
	const auto number = parseDec(trim(*value));
	return number ? static_cast<int>(*number) : -1;
}

/** Wire representation (adds Content-Length for bodies). */
std::string Message::serialize() const
{
	std::string out;
	if (isRequest)
		out = method + " " + uri + " RTSP/1.0\r\n";
	else
		out = "RTSP/1.0 " + std::to_string(status) + " " + reason + "\r\n";
	for (const auto &entry : headers)
		out += entry.first + ": " + entry.second + "\r\n";
	if (!body.empty() && !header("Content-Length"))
		out += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	return out + "\r\n" + body;
}

/** Standard reason phrase for a status code. */
const char *reasonPhrase(int status)
{
	switch (status) {
	case 200:
		return "OK";
	case 400:
		return "Bad Request";
	case 404:
		return "Not Found";
	case 405:
		return "Method Not Allowed";
	case 406:
		return "Not Acceptable";
	case 454:
		return "Session Not Found";
	case 501:
		return "Not Implemented";
	default:
		return "Error";
	}
}

/** Build a request skeleton (CSeq is assigned by the sender). */
Message makeRequest(const std::string &method, const std::string &uri)
{
	Message msg;
	msg.isRequest = true;
	msg.method = method;
	msg.uri = uri;
	return msg;
}

/** Build a response skeleton carrying the given CSeq. */
Message makeResponse(int status, int cseq)
{
	Message msg;
	msg.status = status;
	msg.reason = reasonPhrase(status);
	msg.set("CSeq", std::to_string(cseq));
	return msg;
}

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file One RTSP request or response. */
#pragma once
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc
{

/** An RTSP/1.0 request or response with headers and an optional body. */
struct Message {
	bool isRequest = false;
	std::string method = {};
	std::string uri = {};
	int status = 0;
	std::string reason = {};
	std::vector<std::pair<std::string, std::string>> headers = {};
	std::string body = {};

	/** Case-insensitive header lookup. */
	std::optional<std::string> header(const std::string &name) const;
	/** Set or replace a header. */
	void set(const std::string &name, const std::string &value);
	/** CSeq value, or -1 when absent or malformed. */
	int cseq() const;
	/** Wire representation (adds Content-Length for bodies). */
	std::string serialize() const;
};

/** Build a request skeleton (CSeq is assigned by the sender). */
Message makeRequest(const std::string &method, const std::string &uri);

/** Build a response skeleton carrying the given CSeq. */
Message makeResponse(int status, int cseq);

/** Standard reason phrase for a status code. */
const char *reasonPhrase(int status);

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Incremental RTSP stream parser. */
#pragma once
#include <optional>
#include <string>

#include "message.hpp"

namespace mc
{

/** Frames RTSP messages out of a TCP byte stream, enforcing hard size limits. */
class Parser
{
public:
	/** Append received bytes. */
	void feed(const std::string &data);
	/** Pop the next complete message, if any. */
	std::optional<Message> next();
	/** True once the stream violated the grammar or the size limits. */
	bool failed() const
	{
		return failed_;
	}

private:
	/** Parse the start line and headers into out. */
	static bool parseHead(const std::string &head, Message &out);
	/** Body length from Content-Length, or nullopt when invalid. */
	static std::optional<size_t> bodyLength(const Message &msg);

	std::string buf_ = {};
	bool failed_ = false;
};

} // namespace mc

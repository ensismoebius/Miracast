// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Shared RTSP endpoint plumbing. */
#pragma once
#include <functional>
#include <map>
#include <string>

#include "message.hpp"
#include "parser.hpp"

namespace mc
{

/** Framing, CSeq bookkeeping and request/response matching shared by source and sink. */
class RtspPeer
{
public:
	using Send = std::function<void(const std::string &)>;
	using ResponseHandler = std::function<void(const Message &)>;

	/** Create a peer whose outgoing requests start at firstCseq. */
	RtspPeer(Send send, int firstCseq);
	/** Polymorphic base. */
	virtual ~RtspPeer() = default;
	/** Not copyable. */
	RtspPeer(const RtspPeer &) = delete;
	/** Not assignable. */
	RtspPeer &operator=(const RtspPeer &) = delete;

	/** Stop immediately (transport vanished). */
	virtual void abort() = 0;
	/** Feed bytes received from the transport. */
	void onData(const std::string &data);
	/** True when the session ended with an error. */
	bool failed() const
	{
		return failed_;
	}
	/** True when the session is over (success or failure). */
	bool finished() const
	{
		return finished_;
	}

protected:
	/** Send a request; the handler (optional) receives its response. */
	void sendRequest(Message req, const ResponseHandler &handler);
	/** Send a response. */
	void sendResponse(const Message &resp);
	/** Send a bodiless response with the given status. */
	void reply(const Message &req, int status);
	/** Mark the session failed. */
	void fail(const char *why);
	/** Mark the session finished. */
	void finish()
	{
		finished_ = true;
	}
	/** Handle a request from the remote side. */
	virtual void handleRequest(const Message &req) = 0;

private:
	/** Route a parsed message to a handler. */
	void dispatch(const Message &msg);

	Send send_ = {};
	int nextCseq_ = 0;
	bool failed_ = false;
	bool finished_ = false;
	std::map<int, ResponseHandler> pending_ = {};
	Parser parser_ = {};
};

} // namespace mc

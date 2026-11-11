// SPDX-License-Identifier: GPL-2.0-or-later
#include "rtsp_peer.hpp"

#include <utility>

#include "util.hpp"

namespace mc
{

/** Create a peer whose outgoing requests start at firstCseq. */
RtspPeer::RtspPeer(Send send, int firstCseq) : send_(std::move(send)), nextCseq_(firstCseq)
{
}

/** Mark the session failed. */
void RtspPeer::fail(const char *why)
{
	logf(LogLevel::Error, "RTSP session failed: %s", why);
	failed_ = true;
	finished_ = true;
}

/** Send a request; the handler (optional) receives its response. */
void RtspPeer::sendRequest(Message req, const ResponseHandler &handler)
{
	const int cseq = nextCseq_++;
	req.set("CSeq", std::to_string(cseq));
	pending_[cseq] = handler ? handler : [](const Message &) {};
	logf(LogLevel::Debug, ">> %s %s (CSeq %d)", req.method.c_str(), req.uri.c_str(), cseq);
	send_(req.serialize());
}

/** Send a response. */
void RtspPeer::sendResponse(const Message &resp)
{
	logf(LogLevel::Debug, ">> %d %s (CSeq %d)", resp.status, resp.reason.c_str(), resp.cseq());
	send_(resp.serialize());
}

/** Send a bodiless response with the given status. */
void RtspPeer::reply(const Message &req, int status)
{
	sendResponse(makeResponse(status, req.cseq()));
}

/** Feed bytes received from the transport. */
void RtspPeer::onData(const std::string &data)
{
	if (failed_)
		return;
	parser_.feed(data);
	while (!failed_) {
		const auto msg = parser_.next();
		if (!msg)
			break;
		dispatch(*msg);
	}
	if (parser_.failed())
		fail("malformed RTSP stream");
}

/** Route a parsed message to a handler. */
void RtspPeer::dispatch(const Message &msg)
{
	if (msg.isRequest) {
		logf(LogLevel::Debug, "<< %s %s (CSeq %d)", msg.method.c_str(), msg.uri.c_str(),
		     msg.cseq());
		if (msg.cseq() < 0)
			sendResponse(makeResponse(400, 0));
		else
			handleRequest(msg);
		return;
	}
	logf(LogLevel::Debug, "<< %d (CSeq %d)", msg.status, msg.cseq());
	const auto iter = pending_.find(msg.cseq());
	if (iter == pending_.end()) {
		logf(LogLevel::Warn, "response with unknown CSeq %d ignored", msg.cseq());
		return;
	}
	const ResponseHandler handler = std::move(iter->second);
	pending_.erase(iter);
	handler(msg);
}

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Miracast sink state machine. */
#pragma once
#include <string>

#include "rtsp_peer.hpp"
#include "sink_config.hpp"
#include "sink_media.hpp"
#include "wfd_params.hpp"

namespace mc
{

/** Sink side of M1..M8: answers M1/M3/M4/M5, then drives SETUP and PLAY toward the source. */
class SinkSession : public RtspPeer
{
public:
	enum class State { Init, Negotiating, Setup, Playing, Closed };

	/** Create a session using the given transport callback and media back-end. */
	SinkSession(Send send, SinkMedia &media, SinkConfig cfg);
	/** Stops the media if still running. */
	~SinkSession() override;

	/** Current protocol state. */
	State state() const
	{
		return state_;
	}
	/** Ask the source for an IDR frame. */
	void requestIdr();
	/** Tear the session down gracefully. */
	void teardown();
	/** Stop immediately (transport vanished). */
	void abort() override;

protected:
	/** Dispatch on the request method. */
	void handleRequest(const Message &req) override;

private:
	/** M1: answer and issue M2. */
	void onOptions(const Message &req);
	/** M3 and keep-alives. */
	void onGetParameter(const Message &req);
	/** M4, M5 and other parameters. */
	void onSetParameter(const Message &req);
	/** M8 from the source. */
	void onTeardown(const Message &req);
	/** Validate the source's M4 selection. */
	bool applySelection(const ParamList &params);
	/** Act on a wfd_trigger_method. */
	void onTrigger(const std::string &method);
	/** M6. */
	void sendSetup();
	/** M7. */
	void sendPlay();
	/** Stop media and mark closed. */
	void closeMedia();
	/** Value advertised for a requested capability. */
	std::string capability(const std::string &name) const;

	SinkMedia *media_ = nullptr;
	SinkConfig cfg_ = {};
	State state_ = State::Init;
	std::string presentationUrl_ = {};
	std::string session_ = {};
	bool mediaRunning_ = false;
};

} // namespace mc

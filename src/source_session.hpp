// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Miracast source state machine. */
#pragma once
#include <chrono>
#include <string>

#include "client_ports.hpp"
#include "rtsp_peer.hpp"
#include "selection.hpp"
#include "source_config.hpp"
#include "source_media.hpp"

namespace mc
{

/** Source side of M1..M8: drives negotiation, then streams once PLAY arrives. */
class SourceSession : public RtspPeer
{
public:
	enum class State { Init, Negotiating, Setup, Playing, Closed };
	using Clock = std::chrono::steady_clock;

	/** Create a session using the given transport callback and media back-end. */
	SourceSession(Send send, SourceMedia &media, SourceConfig cfg);
	/** Stops the media if still running. */
	~SourceSession() override;

	/** Send M1. */
	void start();
	/** Drive keep-alives; call regularly. */
	void tick(Clock::time_point now);
	/** Ask the sink to tear the session down (M5 TEARDOWN trigger). */
	void requestTeardown();
	/** Stop immediately (transport vanished). */
	void abort() override;
	/** Current protocol state. */
	State state() const
	{
		return state_;
	}

protected:
	/** Dispatch on the request method. */
	void handleRequest(const Message &req) override;

private:
	/** M3. */
	void sendGetCaps();
	/** Process the M3 response and choose a format. */
	void onCaps(const Message &resp);
	/** M4 followed by M5. */
	void sendSelection();
	/** M6 from the sink. */
	void onSetup(const Message &req);
	/** M7 from the sink. */
	void onPlay(const Message &req);
	/** Sink-originated SET_PARAMETER (IDR request). */
	void onSetParameter(const Message &req);
	/** Stop media and mark closed. */
	void closeMedia();

	SourceMedia *media_ = nullptr;
	SourceConfig cfg_ = {};
	State state_ = State::Init;
	Selection sel_ = {};
	ClientPorts sinkPorts_ = {};
	std::string session_ = {};
	uint16_t clientPort_ = 0;
	Clock::time_point lastKeepalive_ = {};
	bool mediaRunning_ = false;
};

} // namespace mc

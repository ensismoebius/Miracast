// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Test double for SinkMedia. */
#pragma once
#include "sink_media.hpp"

/** Records start/stop calls. */
class FakeSinkMedia : public mc::SinkMedia
{
public:
	bool started = false;
	uint16_t port = 0;

	/** Remember the port. */
	bool start(uint16_t rtpPort) override
	{
		started = true;
		port = rtpPort;
		return true;
	}
	/** Mark stopped. */
	void stop() override
	{
		started = false;
	}
};

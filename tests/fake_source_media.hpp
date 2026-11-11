// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Test double for SourceMedia. */
#pragma once
#include "source_media.hpp"

/** Records start/stop/keyframe calls. */
class FakeSourceMedia : public mc::SourceMedia
{
public:
	bool started = false;
	mc::StreamConfig config;
	uint16_t destPort = 0;
	int keyframes = 0;

	/** Remember the negotiated stream. */
	bool start(const mc::StreamConfig &cfg, const std::string &, uint16_t port,
		   uint16_t) override
	{
		started = true;
		config = cfg;
		destPort = port;
		return true;
	}
	/** Mark stopped. */
	void stop() override
	{
		started = false;
	}
	/** Count IDR requests. */
	void requestKeyframe() override
	{
		++keyframes;
	}
};

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Back-end interface used by the source state machine. */
#pragma once
#include <cstdint>
#include <string>

#include "stream_config.hpp"

namespace mc
{

/** Captures, encodes and sends the RTP stream. */
class SourceMedia
{
public:
	/** Polymorphic base. */
	virtual ~SourceMedia() = default;
	/** Start streaming to host:port, sending from localPort. */
	virtual bool start(const StreamConfig &cfg, const std::string &host, uint16_t port,
			   uint16_t localPort) = 0;
	/** Stop and release the pipeline. */
	virtual void stop() = 0;
	/** Force an IDR frame (sink asked for one). */
	virtual void requestKeyframe() = 0;
};

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Back-end interface used by the sink state machine. */
#pragma once
#include <cstdint>

namespace mc
{

/** Receives the RTP stream and renders it. */
class SinkMedia
{
public:
	/** Polymorphic base. */
	virtual ~SinkMedia() = default;
	/** Start listening for RTP on the given UDP port. */
	virtual bool start(uint16_t rtpPort) = 0;
	/** Stop and release the pipeline. */
	virtual void stop() = 0;
};

} // namespace mc

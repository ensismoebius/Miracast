// SPDX-License-Identifier: GPL-2.0-or-later
/** @file MS-MICE message command codes. */
#pragma once
#include <cstdint>

namespace mc
{

/** MS-MICE message command (section 2.2). */
enum class MiceCommand : uint8_t {
	SourceReady = 0x01,
	StopProjection = 0x02,
	SecurityHandshake = 0x03,
	SessionRequest = 0x04,
	PinChallenge = 0x05,
	PinResponse = 0x06
};

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file WFD device type bits. */
#pragma once
#include <cstdint>

namespace mc
{

/** WFD device type (bits 1:0 of the WFD device information field). */
enum class WfdDeviceType : uint8_t { Source = 0, PrimarySink = 1, SecondarySink = 2, Dual = 3 };

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
#include "cea_mode.hpp"

#include <algorithm>
#include <numeric>

namespace mc
{

namespace
{
const std::vector<CeaMode> kTable = {
	{0, 640, 480, 60, false, 0x01},	   {1, 720, 480, 60, false, 0x01},
	{2, 720, 480, 60, true, 0x01},	   {3, 720, 576, 50, false, 0x01},
	{4, 720, 576, 50, true, 0x01},	   {5, 1280, 720, 30, false, 0x01},
	{6, 1280, 720, 60, false, 0x02},   {7, 1920, 1080, 30, false, 0x08},
	{8, 1920, 1080, 60, false, 0x10},  {9, 1920, 1080, 60, true, 0x08},
	{10, 1280, 720, 25, false, 0x01},  {11, 1280, 720, 50, false, 0x02},
	{12, 1920, 1080, 25, false, 0x08}, {13, 1920, 1080, 50, false, 0x10},
	{14, 1920, 1080, 50, true, 0x08},  {15, 1280, 720, 24, false, 0x01},
	{16, 1920, 1080, 24, false, 0x08},
};
} // namespace

/** Entry for a mask bit, or nullptr. */
const CeaMode *ceaMode(int bit)
{
	const auto found = std::find_if(kTable.begin(), kTable.end(),
					[bit](const CeaMode &mode) { return mode.bit == bit; });
	return found == kTable.end() ? nullptr : &*found;
}

/** The whole table. */
const std::vector<CeaMode> &ceaModes()
{
	return kTable;
}

/** Mask with every progressive mode of the table set. */
uint32_t progressiveCeaMask()
{
	return std::accumulate(kTable.begin(), kTable.end(), uint32_t{0},
			       [](uint32_t mask, const CeaMode &mode) {
				       return mode.interlaced ? mask : (mask | (1U << mode.bit));
			       });
}

} // namespace mc

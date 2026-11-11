// SPDX-License-Identifier: GPL-2.0-or-later
/** @file User-tunable capture and encoding settings. */
#pragma once
#include <string>

namespace mc
{

/** Where frames come from and how they are encoded, as chosen on the command line. */
struct CaptureSettings {
	std::string capture = {};
	bool testPattern = false;
	std::string output = {};
	std::string encoder = "x264enc";
	std::string encoderOpts = {};
	int bufferMs = 100;
	bool audio = true;
	std::string audioSource = {};
};

} // namespace mc

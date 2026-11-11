// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Command-line options. */
#pragma once
#include <cstdint>
#include <string>

namespace mc
{

/** Parsed command line. */
struct Options {
	std::string command = {};
	std::string target = {};
	std::string iface = "wlan0";
	std::string ctrlDir = "/run/wpa_supplicant";
	std::string name = "miracast-linux";
	std::string listen = "0.0.0.0";
	std::string host = {};
	std::string capture = {};
	std::string output = {};
	std::string videoSink = "autovideosink";
	std::string audioSink = "autoaudiosink";
	std::string encoder = "x264enc";
	std::string encoderOpts = {};
	std::string audioSource = {};
	bool noP2p = false;
	bool p2p = false;
	bool testPattern = false;
	bool mice = false;
	bool noAudio = false;
	int scanSeconds = 8;
	int maxWidth = 1920;
	int maxFps = 60;
	int bitrate = 8000;
	int jitterMs = 40;
	int bufferMs = 100;
	uint16_t micePort = 7250;
};

} // namespace mc

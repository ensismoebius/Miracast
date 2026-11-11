// SPDX-License-Identifier: GPL-2.0-or-later
/** @file wpa_supplicant control-interface client. */
#pragma once
#include <deque>
#include <optional>
#include <string>

#include "file_descriptor.hpp"

namespace mc
{

/** Client for the wpa_supplicant control interface (AF_UNIX datagram protocol). */
class WpaCtrl
{
public:
	/** Not connected. */
	WpaCtrl() = default;
	/** Close the socket and remove the temporary bind path. */
	~WpaCtrl();
	/** Not copyable. */
	WpaCtrl(const WpaCtrl &) = delete;
	/** Not assignable. */
	WpaCtrl &operator=(const WpaCtrl &) = delete;

	/** Connect to ctrlDir/iface and ATTACH for unsolicited events. */
	bool open(const std::string &ctrlDir, const std::string &iface);
	/** Send a command and return its reply. */
	std::optional<std::string> request(const std::string &cmd, int timeoutMs = 5000);
	/** Send a command that must answer "OK". */
	bool command(const std::string &cmd);
	/** Next unsolicited event, without the priority prefix. */
	std::optional<std::string> nextEvent(int timeoutMs);

private:
	/** Receive one datagram. */
	bool recvDatagram(std::string &out, int timeoutMs);

	FileDescriptor sock_ = {};
	std::string localPath_ = {};
	std::string tmpDir_ = {};
	std::deque<std::string> events_ = {};
};

} // namespace mc

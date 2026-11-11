// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Thin POSIX TCP and interface helpers (no exceptions). */
#pragma once
#include <cstdint>
#include <optional>
#include <string>

#include "file_descriptor.hpp"

namespace mc
{

/** Listening IPv4 TCP socket bound to addr:port; invalid FileDescriptor on error. */
FileDescriptor tcpListen(const std::string &addr, uint16_t port);

/** Accept one connection within timeoutMs; fills peerIp when given. */
FileDescriptor tcpAccept(int listenFd, int timeoutMs, std::string *peerIp);

/** Connect to addr:port within timeoutMs; invalid FileDescriptor on error. */
FileDescriptor tcpConnect(const std::string &addr, uint16_t port, int timeoutMs);

/** Local IPv4 address of a connected socket. */
std::string localAddress(int sockFd);

/** Write the whole buffer; false on error. */
bool sendAll(int sockFd, const std::string &data);

/** Receive into out: -1 closed/error, 0 timeout, >0 bytes appended. */
int recvSome(int sockFd, std::string &out, int timeoutMs);

/** First IPv4 address of an interface. */
std::optional<std::string> ifaceAddress(const std::string &ifname);

/** Default gateway of an interface (from /proc/net/route). */
std::optional<std::string> defaultGateway(const std::string &ifname);

} // namespace mc

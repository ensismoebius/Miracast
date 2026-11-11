// SPDX-License-Identifier: GPL-2.0-or-later
#include "mdns_discovery.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>

#include <algorithm>
#include <chrono>
#include <cstring>

#include "dns_packet.hpp"
#include "file_descriptor.hpp"
#include "util.hpp"

namespace mc
{

namespace
{
constexpr uint16_t kMdnsPort = 5353;
constexpr int kQueryRepeatMs = 700;

/** UDP socket able to send to the mDNS group. */
FileDescriptor openSocket()
{
	FileDescriptor sock(::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0));
	const int ttl = 255;
	if (sock.valid())
		::setsockopt(sock.get(), IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof ttl);
	return sock;
}

/** Send the PTR query to 224.0.0.251:5353. */
void sendQuery(const FileDescriptor &sock)
{
	sockaddr_in group{};
	group.sin_family = AF_INET;
	group.sin_port = htons(kMdnsPort);
	inet_pton(AF_INET, "224.0.0.251", &group.sin_addr);
	const std::string query = buildPtrQuery(kDisplayService);
	::sendto(sock.get(), query.data(), query.size(), 0, reinterpret_cast<sockaddr *>(&group),
		 sizeof group);
}

/** Receive one datagram within timeoutMs; returns the sink with its source address. */
std::optional<SinkInfo> receiveSink(const FileDescriptor &sock, int timeoutMs)
{
	pollfd pfd{sock.get(), POLLIN, 0};
	if (::poll(&pfd, 1, timeoutMs) <= 0)
		return std::nullopt;
	char buf[2048];
	sockaddr_in from{};
	socklen_t len = sizeof from;
	const ssize_t got = ::recvfrom(sock.get(), buf, sizeof buf, 0,
				       reinterpret_cast<sockaddr *>(&from), &len);
	if (got <= 0)
		return std::nullopt;
	auto info = parseDisplayResponse(std::string(buf, static_cast<size_t>(got)));
	if (!info)
		return std::nullopt;
	char text[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &from.sin_addr, text, sizeof text);
	info->address = text;
	return info;
}
} // namespace

/** Query _display._tcp for timeoutMs and return every sink that answered (deduplicated by address).
 */
std::vector<SinkInfo> discoverSinks(int timeoutMs, const std::atomic<bool> &stop)
{
	std::vector<SinkInfo> sinks;
	const FileDescriptor sock = openSocket();
	if (!sock.valid())
		return sinks;
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
	auto nextQuery = std::chrono::steady_clock::now();
	while (!stop.load() && std::chrono::steady_clock::now() < deadline) {
		if (std::chrono::steady_clock::now() >= nextQuery) {
			sendQuery(sock);
			nextQuery += std::chrono::milliseconds(kQueryRepeatMs);
		}
		const auto info = receiveSink(sock, 100);
		const bool known =
			info && std::any_of(sinks.begin(), sinks.end(), [&](const SinkInfo &seen) {
				return seen.address == info->address;
			});
		if (info && !known)
			sinks.push_back(*info);
	}
	return sinks;
}

} // namespace mc

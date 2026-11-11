// SPDX-License-Identifier: GPL-2.0-or-later
#include "net.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>

#include "util.hpp"

namespace mc
{

namespace
{
/** Fill an IPv4 socket address; false when addr is not a dotted quad. */
bool makeAddr(const std::string &addr, uint16_t port, sockaddr_in &sock)
{
	std::memset(&sock, 0, sizeof sock);
	sock.sin_family = AF_INET;
	sock.sin_port = htons(port);
	return inet_pton(AF_INET, addr.c_str(), &sock.sin_addr) == 1;
}

/** Wait for events on a descriptor, retrying on EINTR. */
bool waitFd(int fdnum, short events, int timeoutMs)
{
	pollfd pfd{fdnum, events, 0};
	int ret;
	do {
		ret = ::poll(&pfd, 1, timeoutMs);
	} while (ret < 0 && errno == EINTR);
	return ret > 0;
}

/** Enable TCP_NODELAY. */
void setNoDelay(int sockFd)
{
	const int one = 1;
	::setsockopt(sockFd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
}

/** Dotted-quad text of an IPv4 address in network byte order. */
std::string addrText(const in_addr &addr)
{
	char buf[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addr, buf, sizeof buf);
	return buf;
}
} // namespace

/** Listening IPv4 TCP socket bound to addr:port; invalid FileDescriptor on error. */
FileDescriptor tcpListen(const std::string &addr, uint16_t port)
{
	sockaddr_in sock{};
	if (!makeAddr(addr, port, sock))
		return FileDescriptor();
	FileDescriptor lsn(::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0));
	if (!lsn.valid())
		return FileDescriptor();
	const int one = 1;
	::setsockopt(lsn.get(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
	if (::bind(lsn.get(), reinterpret_cast<sockaddr *>(&sock), sizeof sock) != 0 ||
	    ::listen(lsn.get(), 1) != 0) {
		logf(LogLevel::Error, "listen %s:%u: %s", addr.c_str(), port, std::strerror(errno));
		return FileDescriptor();
	}
	return lsn;
}

/** Accept one connection within timeoutMs; fills peerIp when given. */
FileDescriptor tcpAccept(int listenFd, int timeoutMs, std::string *peerIp)
{
	if (!waitFd(listenFd, POLLIN, timeoutMs))
		return FileDescriptor();
	sockaddr_in sock{};
	socklen_t len = sizeof sock;
	FileDescriptor conn(
		::accept4(listenFd, reinterpret_cast<sockaddr *>(&sock), &len, SOCK_CLOEXEC));
	if (!conn.valid())
		return conn;
	if (peerIp != nullptr)
		*peerIp = addrText(sock.sin_addr);
	setNoDelay(conn.get());
	return conn;
}

/** Connect to addr:port within timeoutMs; invalid FileDescriptor on error. */
FileDescriptor tcpConnect(const std::string &addr, uint16_t port, int timeoutMs)
{
	sockaddr_in sock{};
	if (!makeAddr(addr, port, sock))
		return FileDescriptor();
	FileDescriptor conn(::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0));
	if (!conn.valid())
		return FileDescriptor();
	const int ret = ::connect(conn.get(), reinterpret_cast<sockaddr *>(&sock), sizeof sock);
	if (ret != 0 && errno != EINPROGRESS)
		return FileDescriptor();
	int soError = 0;
	socklen_t errLen = sizeof soError;
	if (ret != 0 && !waitFd(conn.get(), POLLOUT, timeoutMs))
		return FileDescriptor();
	::getsockopt(conn.get(), SOL_SOCKET, SO_ERROR, &soError, &errLen);
	if (soError != 0)
		return FileDescriptor();
	::fcntl(conn.get(), F_SETFL, ::fcntl(conn.get(), F_GETFL, 0) & ~O_NONBLOCK);
	setNoDelay(conn.get());
	return conn;
}

/** Local IPv4 address of a connected socket. */
std::string localAddress(int sockFd)
{
	sockaddr_in sock{};
	socklen_t len = sizeof sock;
	if (::getsockname(sockFd, reinterpret_cast<sockaddr *>(&sock), &len) != 0)
		return {};
	return addrText(sock.sin_addr);
}

/** Write the whole buffer; false on error. */
bool sendAll(int sockFd, const std::string &data)
{
	size_t done = 0;
	while (done < data.size()) {
		const ssize_t sent =
			::send(sockFd, data.data() + done, data.size() - done, MSG_NOSIGNAL);
		if (sent < 0 && errno != EINTR)
			return false;
		if (sent > 0)
			done += static_cast<size_t>(sent);
	}
	return true;
}

/** Receive into out: -1 closed/error, 0 timeout, >0 bytes appended. */
int recvSome(int sockFd, std::string &out, int timeoutMs)
{
	if (!waitFd(sockFd, POLLIN, timeoutMs))
		return 0;
	char buf[4096];
	ssize_t got;
	do {
		got = ::recv(sockFd, buf, sizeof buf, 0);
	} while (got < 0 && errno == EINTR);
	if (got <= 0)
		return -1;
	out.append(buf, static_cast<size_t>(got));
	return static_cast<int>(got);
}

/** First IPv4 address of an interface. */
std::optional<std::string> ifaceAddress(const std::string &ifname)
{
	ifaddrs *list = nullptr;
	if (::getifaddrs(&list) != 0)
		return std::nullopt;
	std::optional<std::string> found;
	for (const ifaddrs *entry = list; entry != nullptr && !found; entry = entry->ifa_next) {
		if (entry->ifa_addr == nullptr || entry->ifa_addr->sa_family != AF_INET ||
		    ifname != entry->ifa_name)
			continue;
		const auto *sock = reinterpret_cast<const sockaddr_in *>(entry->ifa_addr);
		found = addrText(sock->sin_addr);
	}
	::freeifaddrs(list);
	return found;
}

/** Default gateway of an interface (from /proc/net/route). */
std::optional<std::string> defaultGateway(const std::string &ifname)
{
	std::ifstream table("/proc/net/route");
	std::string line;
	std::getline(table, line); // header
	while (std::getline(table, line)) {
		std::istringstream fields(line);
		std::string iface, dest, gate;
		if (!(fields >> iface >> dest >> gate) || iface != ifname)
			continue;
		const auto value = parseHex(gate);
		if (!value || *value == 0)
			continue;
		in_addr addr{};
		addr.s_addr = *value; // the kernel prints the network-order word as host hex
		return addrText(addr);
	}
	return std::nullopt;
}

} // namespace mc

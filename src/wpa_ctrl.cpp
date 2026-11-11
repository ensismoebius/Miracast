// SPDX-License-Identifier: GPL-2.0-or-later
#include "wpa_ctrl.hpp"

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "util.hpp"

namespace mc
{

namespace
{
/** Fill a UNIX socket address; false when the path is too long. */
bool fillAddr(const std::string &path, sockaddr_un &addr)
{
	if (path.size() >= sizeof addr.sun_path)
		return false;
	std::memset(&addr, 0, sizeof addr);
	addr.sun_family = AF_UNIX;
	std::memcpy(addr.sun_path, path.c_str(), path.size());
	return true;
}

/** Drop the "<n>" priority prefix of an event line. */
std::string stripPriority(const std::string &line)
{
	const size_t close = line.find('>');
	if (!line.empty() && line[0] == '<' && close != std::string::npos)
		return line.substr(close + 1);
	return line;
}
} // namespace

/** Close the socket and remove the temporary bind path. */
WpaCtrl::~WpaCtrl()
{
	sock_.reset();
	if (!localPath_.empty())
		::unlink(localPath_.c_str());
	if (!tmpDir_.empty())
		::rmdir(tmpDir_.c_str());
}

/** Connect to ctrlDir/iface and ATTACH for unsolicited events. */
bool WpaCtrl::open(const std::string &ctrlDir, const std::string &iface)
{
	const char *runtime = std::getenv("XDG_RUNTIME_DIR");
	std::string pattern =
		std::string(runtime != nullptr ? runtime : "/tmp") + "/miracast-XXXXXX";
	if (::mkdtemp(pattern.data()) == nullptr) // mode 0700, unpredictable name
		return false;
	tmpDir_ = pattern;
	localPath_ = tmpDir_ + "/ctrl";

	sock_.reset(::socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0));
	sockaddr_un local{};
	sockaddr_un remote{};
	if (!sock_.valid() || !fillAddr(localPath_, local) ||
	    !fillAddr(ctrlDir + "/" + iface, remote))
		return false;
	if (::bind(sock_.get(), reinterpret_cast<sockaddr *>(&local), sizeof local) != 0)
		return false;
	if (::connect(sock_.get(), reinterpret_cast<sockaddr *>(&remote), sizeof remote) != 0) {
		logf(LogLevel::Error, "cannot reach wpa_supplicant at %s/%s: %s", ctrlDir.c_str(),
		     iface.c_str(), std::strerror(errno));
		return false;
	}
	return command("ATTACH");
}

/** Receive one datagram. */
bool WpaCtrl::recvDatagram(std::string &out, int timeoutMs)
{
	pollfd pfd{sock_.get(), POLLIN, 0};
	int ret;
	do {
		ret = ::poll(&pfd, 1, timeoutMs);
	} while (ret < 0 && errno == EINTR);
	if (ret <= 0)
		return false;
	char buf[8192];
	const ssize_t got = ::recv(sock_.get(), buf, sizeof buf, 0);
	if (got < 0)
		return false;
	out.assign(buf, static_cast<size_t>(got));
	return true;
}

/** Send a command and return its reply. */
std::optional<std::string> WpaCtrl::request(const std::string &cmd, int timeoutMs)
{
	if (::send(sock_.get(), cmd.data(), cmd.size(), 0) < 0)
		return std::nullopt;
	std::string msg;
	while (recvDatagram(msg, timeoutMs)) {
		if (!msg.empty() && msg[0] == '<')
			events_.push_back(
				stripPriority(msg)); // keep events that race with the reply
		else
			return trim(msg);
	}
	return std::nullopt;
}

/** Send a command that must answer "OK". */
bool WpaCtrl::command(const std::string &cmd)
{
	const auto reply = request(cmd);
	if (reply && *reply == "OK")
		return true;
	logf(LogLevel::Warn, "wpa_supplicant: '%s' -> %s", cmd.c_str(),
	     reply ? reply->c_str() : "no reply");
	return false;
}

/** Next unsolicited event, without the priority prefix. */
std::optional<std::string> WpaCtrl::nextEvent(int timeoutMs)
{
	if (!events_.empty()) {
		std::string event = events_.front();
		events_.pop_front();
		return event;
	}
	std::string msg;
	if (!recvDatagram(msg, timeoutMs))
		return std::nullopt;
	return stripPriority(msg);
}

} // namespace mc

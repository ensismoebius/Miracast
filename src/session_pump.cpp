// SPDX-License-Identifier: GPL-2.0-or-later
#include "session_pump.hpp"

#include <string>

#include "net.hpp"
#include "util.hpp"

namespace mc
{

/** Pump conn into session until it finishes or stop is set; extra() returning false aborts. */
void pumpSession(const FileDescriptor &conn, RtspPeer &session, const std::atomic<bool> &stop,
		 const std::function<bool()> &extra)
{
	std::string buf;
	while (!session.finished() && !stop.load()) {
		buf.clear();
		const int got = recvSome(conn.get(), buf, 200);
		if (got < 0) {
			logf(LogLevel::Info, "peer closed the connection");
			session.abort();
			return;
		}
		if (got > 0)
			session.onData(buf);
		if (!extra()) {
			session.abort();
			return;
		}
	}
}

} // namespace mc

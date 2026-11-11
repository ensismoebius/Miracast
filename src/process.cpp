// SPDX-License-Identifier: GPL-2.0-or-later
#include "process.hpp"

#include <signal.h>
#include <sys/wait.h>

#include <cerrno>

#include "process_spawn.hpp"

namespace mc
{

/** Terminate the child. */
Process::~Process()
{
	terminate();
}

/** Start argv in the background. */
bool Process::spawn(const std::vector<std::string> &argv)
{
	pid_ = forkExec(argv);
	return pid_ > 0;
}

/** SIGTERM and reap. */
void Process::terminate()
{
	if (pid_ <= 0)
		return;
	::kill(pid_, SIGTERM);
	int status = 0;
	while (::waitpid(pid_, &status, 0) < 0 && errno == EINTR) {
	}
	pid_ = -1;
}

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
#include "exec.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>

#include "process_spawn.hpp"
#include "util.hpp"

namespace mc
{

/** True when an executable named name exists in PATH. */
bool commandExists(const std::string &name)
{
	const char *path = std::getenv("PATH");
	if (path == nullptr)
		return false;
	const auto dirs = split(path, ':');
	return std::any_of(dirs.begin(), dirs.end(), [&](const std::string &dir) {
		const std::string full = (dir.empty() ? "." : dir) + "/" + name;
		return ::access(full.c_str(), X_OK) == 0;
	});
}

/** Run argv to completion; returns the exit status or -1. */
int runCommand(const std::vector<std::string> &argv)
{
	const pid_t child = forkExec(argv);
	if (child < 0)
		return -1;
	int status = 0;
	while (::waitpid(child, &status, 0) < 0) {
		if (errno != EINTR)
			return -1;
	}
	return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

} // namespace mc

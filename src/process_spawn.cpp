// SPDX-License-Identifier: GPL-2.0-or-later
#include "process_spawn.hpp"

#include <unistd.h>

#include <algorithm>

namespace mc
{

/** fork + execvp of argv; returns the child pid or -1. */
pid_t forkExec(const std::vector<std::string> &argv)
{
	if (argv.empty())
		return -1;
	std::vector<char *> args(argv.size());
	std::transform(argv.begin(), argv.end(), args.begin(),
		       [](const std::string &arg) { return const_cast<char *>(arg.c_str()); });
	args.push_back(nullptr);
	const pid_t child = ::fork();
	if (child == 0) {
		::execvp(args[0], args.data());
		_exit(127);
	}
	return child;
}

} // namespace mc

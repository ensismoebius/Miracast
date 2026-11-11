// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Background child process. */
#pragma once
#include <sys/types.h>

#include <string>
#include <vector>

namespace mc
{

/** A child process terminated when the object goes away. */
class Process
{
public:
	/** No child yet. */
	Process() = default;
	/** Terminate the child. */
	~Process();
	/** Not copyable. */
	Process(const Process &) = delete;
	/** Not assignable. */
	Process &operator=(const Process &) = delete;

	/** Start argv in the background. */
	bool spawn(const std::vector<std::string> &argv);
	/** SIGTERM and reap. */
	void terminate();
	/** True while a child is held. */
	bool running() const
	{
		return pid_ > 0;
	}

private:
	pid_t pid_ = -1;
};

} // namespace mc

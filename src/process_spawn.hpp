// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Low-level fork+execvp used by exec.cpp and process.cpp. */
#pragma once
#include <sys/types.h>

#include <string>
#include <vector>

namespace mc
{

/** fork + execvp of argv; returns the child pid or -1. */
pid_t forkExec(const std::vector<std::string> &argv);

} // namespace mc

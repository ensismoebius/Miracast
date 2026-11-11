// SPDX-License-Identifier: GPL-2.0-or-later
/** @file fork/exec helpers; arguments are argv vectors, no shell is ever involved. */
#pragma once
#include <string>
#include <vector>

namespace mc
{

/** True when an executable named name exists in PATH. */
bool commandExists(const std::string &name);

/** Run argv to completion; returns the exit status or -1. */
int runCommand(const std::vector<std::string> &argv);

} // namespace mc

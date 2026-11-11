// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Argument parsing and help text. */
#pragma once
#include <cstdio>

#include "options.hpp"

namespace mc
{

/** Print usage to the given stream. */
void printUsage(FILE *out);

/** Parse argv into opts; returns false on a usage error. May exit for --help/--version. */
bool parseOptions(int argc, char **argv, Options &opts);

} // namespace mc

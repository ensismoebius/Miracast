// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Minimal assertion helper for the test binary. */
#pragma once
#include <atomic>
#include <cstdio>

/** Number of failed checks so far. */
inline std::atomic<int> g_failures{0};

/** Record a failed check with its location. */
inline void reportCheck(bool okay, const char *expr, int line)
{
	if (okay)
		return;
	std::fprintf(stderr, "FAIL line %d: %s\n", line, expr);
	g_failures.fetch_add(1);
}

#define CHECK(expr) reportCheck((expr), #expr, __LINE__)

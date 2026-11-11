// SPDX-License-Identifier: GPL-2.0-or-later
/** @file miracast: command-line Miracast (Wi-Fi Display) source and sink for Linux. */
#include <signal.h>
#include <sysexits.h>

#include <atomic>
#include <csignal>

#include "cli.hpp"
#include "commands.hpp"

namespace
{
std::atomic<bool> g_stop{false};

/** Signal handler: request a clean shutdown. */
void onSignal(int)
{
	g_stop.store(true);
}
} // namespace

/** Program entry point. */
int main(int argc, char **argv)
{
	mc::Options opts;
	if (!mc::parseOptions(argc, argv, opts)) {
		mc::printUsage(stderr);
		return EX_USAGE;
	}
	std::signal(SIGINT, onSignal);
	std::signal(SIGTERM, onSignal);
	std::signal(SIGPIPE, SIG_IGN);
	if (opts.command == "scan")
		return mc::runScan(opts, g_stop);
	if (opts.command == "cast")
		return mc::runCast(opts, g_stop);
	if (opts.command == "sink")
		return mc::runSink(opts, g_stop);
	mc::printUsage(stderr);
	return EX_USAGE;
}

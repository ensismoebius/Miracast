// SPDX-License-Identifier: GPL-2.0-or-later
#include "sink_lookup.hpp"

#include <arpa/inet.h>

#include <algorithm>

#include "mdns_discovery.hpp"
#include "util.hpp"

namespace mc
{

namespace
{
constexpr int kDiscoveryMs = 3000;

/** True when text is a dotted IPv4 address. */
bool isIpv4(const std::string &text)
{
	in_addr addr{};
	return inet_pton(AF_INET, text.c_str(), &addr) == 1;
}
} // namespace

/** Sink matching opts.target (name or IPv4); with no target, the only sink on the network. */
std::optional<SinkInfo> findInfrastructureSink(const Options &opts, const std::atomic<bool> &stop)
{
	if (isIpv4(opts.target)) {
		SinkInfo direct;
		direct.name = direct.address = opts.target;
		return direct;
	}
	const auto sinks = discoverSinks(kDiscoveryMs, stop);
	if (opts.target.empty())
		return sinks.size() == 1 ? std::optional<SinkInfo>(sinks[0]) : std::nullopt;
	const auto found = std::find_if(sinks.begin(), sinks.end(), [&](const SinkInfo &sink) {
		return toLower(sink.name) == toLower(opts.target);
	});
	return found == sinks.end() ? std::nullopt : std::optional<SinkInfo>(*found);
}

} // namespace mc

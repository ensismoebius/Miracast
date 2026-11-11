// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Screen capture through the org.gnome.Mutter.ScreenCast D-Bus API (niri, GNOME, ...). */
#pragma once
#include <gio/gio.h>

#include <optional>
#include <string>

namespace mc
{

/** A ScreenCast session whose PipeWire node feeds the encoder; stops the session on release. */
class ScreenCapture
{
public:
	/** Not started. */
	ScreenCapture() = default;
	/** Stops the session. */
	~ScreenCapture();
	/** Not copyable. */
	ScreenCapture(const ScreenCapture &) = delete;
	/** Not assignable. */
	ScreenCapture &operator=(const ScreenCapture &) = delete;

	/** Start recording the named output (empty: the first one); returns the PipeWire node id.
	 */
	std::optional<unsigned> start(const std::string &output);
	/** Stop the session. */
	void stop();
	/** Record the PipeWire node announced by the compositor (D-Bus callback target). */
	void onStreamAdded(unsigned node);

private:
	/** Open the stream for output and return its object path. */
	std::optional<std::string> openStream(const std::string &output);
	/** Block (iterating the main context) until the node arrives or time runs out. */
	std::optional<unsigned> waitForNode();

	GDBusConnection *bus_ = nullptr;
	std::string session_ = "";
	guint subscription_ = 0;
	std::optional<unsigned> node_ = std::nullopt;
};

} // namespace mc

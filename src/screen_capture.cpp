// SPDX-License-Identifier: GPL-2.0-or-later
#include "screen_capture.hpp"

#include <chrono>
#include <thread>

#include "util.hpp"

namespace mc
{

namespace
{
const char *const kService = "org.gnome.Mutter.ScreenCast";
const char *const kRootPath = "/org/gnome/Mutter/ScreenCast";
const char *const kSessionIface = "org.gnome.Mutter.ScreenCast.Session";
const char *const kStreamIface = "org.gnome.Mutter.ScreenCast.Stream";
constexpr int kNodeWaitMs = 5000;

/** Result of a D-Bus call as an object path, logging failures. */
std::optional<std::string> callForPath(GDBusConnection *bus, const char *path, const char *iface,
				       const char *method, GVariant *args)
{
	GError *err = nullptr;
	GVariant *reply = g_dbus_connection_call_sync(bus, kService, path, iface, method, args,
						      G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE,
						      5000, nullptr, &err);
	if (reply == nullptr) {
		logf(LogLevel::Error, "ScreenCast %s: %s", method, err->message);
		g_error_free(err);
		return std::nullopt;
	}
	const char *object = nullptr;
	g_variant_get(reply, "(&o)", &object);
	const std::string result = object;
	g_variant_unref(reply);
	return result;
}

/** Call a method that returns nothing; false (and a log line) on failure. */
bool callVoid(GDBusConnection *bus, const char *path, const char *iface, const char *method)
{
	GError *err = nullptr;
	GVariant *reply =
		g_dbus_connection_call_sync(bus, kService, path, iface, method, nullptr, nullptr,
					    G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &err);
	if (reply == nullptr) {
		logf(LogLevel::Error, "ScreenCast %s: %s", method, err->message);
		g_error_free(err);
		return false;
	}
	g_variant_unref(reply);
	return true;
}

/** GDBus signal trampoline for PipeWireStreamAdded(u). */
void streamAddedTrampoline(GDBusConnection *, const gchar *, const gchar *, const gchar *,
			   const gchar *, GVariant *params, gpointer self)
{
	guint32 node = 0;
	g_variant_get(params, "(u)", &node);
	static_cast<ScreenCapture *>(self)->onStreamAdded(node);
}
} // namespace

/** Stops the session. */
ScreenCapture::~ScreenCapture()
{
	stop();
}

/** Record the PipeWire node announced by the compositor (D-Bus callback target). */
void ScreenCapture::onStreamAdded(unsigned node)
{
	node_ = node;
}

/** Open the stream for output and return its object path. */
std::optional<std::string> ScreenCapture::openStream(const std::string &output)
{
	const auto created = callForPath(bus_, kRootPath, kService, "CreateSession",
					 g_variant_new_parsed("(@a{sv} {},)"));
	if (!created)
		return std::nullopt;
	session_ = *created;
	return callForPath(
		bus_, session_.c_str(), kSessionIface, "RecordMonitor",
		g_variant_new_parsed("(%s, {'cursor-mode': <uint32 1>})", output.c_str()));
}

/** Block (iterating the main context) until the node arrives or time runs out. */
std::optional<unsigned> ScreenCapture::waitForNode()
{
	for (int waited = 0; waited < kNodeWaitMs && !node_; waited += 20) {
		while (g_main_context_iteration(nullptr, FALSE)) {
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	return node_;
}

/** Start recording the named output (empty: the first one); returns the PipeWire node id. */
std::optional<unsigned> ScreenCapture::start(const std::string &output)
{
	stop();
	GError *err = nullptr;
	bus_ = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &err);
	if (bus_ == nullptr) {
		logf(LogLevel::Error, "no D-Bus session bus: %s", err->message);
		g_error_free(err);
		return std::nullopt;
	}
	const auto stream = openStream(output);
	if (!stream)
		return std::nullopt;
	subscription_ = g_dbus_connection_signal_subscribe(
		bus_, kService, kStreamIface, "PipeWireStreamAdded", stream->c_str(), nullptr,
		G_DBUS_SIGNAL_FLAGS_NONE, streamAddedTrampoline, this, nullptr);
	if (!callVoid(bus_, session_.c_str(), kSessionIface, "Start"))
		return std::nullopt;
	return waitForNode();
}

/** Stop the session. */
void ScreenCapture::stop()
{
	if (bus_ == nullptr)
		return;
	if (subscription_ != 0)
		g_dbus_connection_signal_unsubscribe(bus_, subscription_);
	if (!session_.empty()) {
		callVoid(bus_, session_.c_str(), kSessionIface, "Stop");
	}
	g_object_unref(bus_);
	bus_ = nullptr;
	subscription_ = 0;
	session_.clear();
	node_.reset();
}

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
#include "cli.hpp"

#include <getopt.h>
#include <sysexits.h>

#include <cstdlib>

#include "util.hpp"

namespace mc
{

namespace
{
enum LongOpt {
	kCtrlDir = 1000,
	kNoP2p,
	kP2p,
	kHost,
	kListen,
	kCapture,
	kOutput,
	kTest,
	kMice,
	kMaxW,
	kMaxFps,
	kBitrate,
	kVideoSink,
	kAudioSink,
	kEncoder,
	kEncoderOpts,
	kJitter,
	kBuffer,
	kNoAudio,
	kAudioSource
};

const option kLongOptions[] = {
	{"interface", required_argument, nullptr, 'i'},
	{"name", required_argument, nullptr, 'n'},
	{"scan-time", required_argument, nullptr, 't'},
	{"verbose", no_argument, nullptr, 'v'},
	{"help", no_argument, nullptr, 'h'},
	{"version", no_argument, nullptr, 'V'},
	{"ctrl-dir", required_argument, nullptr, kCtrlDir},
	{"no-p2p", no_argument, nullptr, kNoP2p},
	{"p2p", no_argument, nullptr, kP2p},
	{"host", required_argument, nullptr, kHost},
	{"listen", required_argument, nullptr, kListen},
	{"capture", required_argument, nullptr, kCapture},
	{"output", required_argument, nullptr, kOutput},
	{"test-pattern", no_argument, nullptr, kTest},
	{"mice", no_argument, nullptr, kMice},
	{"max-width", required_argument, nullptr, kMaxW},
	{"max-fps", required_argument, nullptr, kMaxFps},
	{"bitrate", required_argument, nullptr, kBitrate},
	{"video-sink", required_argument, nullptr, kVideoSink},
	{"audio-sink", required_argument, nullptr, kAudioSink},
	{"encoder", required_argument, nullptr, kEncoder},
	{"encoder-opts", required_argument, nullptr, kEncoderOpts},
	{"jitter-ms", required_argument, nullptr, kJitter},
	{"buffer-ms", required_argument, nullptr, kBuffer},
	{"no-audio", no_argument, nullptr, kNoAudio},
	{"audio-source", required_argument, nullptr, kAudioSource},
	{nullptr, 0, nullptr, 0},
};

/** Text-valued field selected by opt, or nullptr. */
std::string *textField(int opt, Options &opts)
{
	switch (opt) {
	case 'i':
		return &opts.iface;
	case 'n':
		return &opts.name;
	case kCtrlDir:
		return &opts.ctrlDir;
	case kHost:
		return &opts.host;
	case kListen:
		return &opts.listen;
	case kCapture:
		return &opts.capture;
	case kOutput:
		return &opts.output;
	case kVideoSink:
		return &opts.videoSink;
	case kAudioSink:
		return &opts.audioSink;
	case kEncoder:
		return &opts.encoder;
	case kEncoderOpts:
		return &opts.encoderOpts;
	case kAudioSource:
		return &opts.audioSource;
	default:
		return nullptr;
	}
}

/** Integer-valued field selected by opt, or nullptr. */
int *numberField(int opt, Options &opts)
{
	switch (opt) {
	case 't':
		return &opts.scanSeconds;
	case kMaxW:
		return &opts.maxWidth;
	case kMaxFps:
		return &opts.maxFps;
	case kBitrate:
		return &opts.bitrate;
	case kJitter:
		return &opts.jitterMs;
	case kBuffer:
		return &opts.bufferMs;
	default:
		return nullptr;
	}
}

/** Boolean flag selected by opt, or nullptr. */
bool *flagField(int opt, Options &opts)
{
	switch (opt) {
	case kNoP2p:
		return &opts.noP2p;
	case kP2p:
		return &opts.p2p;
	case kTest:
		return &opts.testPattern;
	case kMice:
		return &opts.mice;
	case kNoAudio:
		return &opts.noAudio;
	default:
		return nullptr;
	}
}

/** Store opt (with its argument) into opts; false when the value is invalid. */
bool applyOption(int opt, const char *arg, Options &opts)
{
	if (std::string *text = textField(opt, opts)) {
		*text = arg;
		return true;
	}
	if (int *number = numberField(opt, opts)) {
		const auto value = parseDec(arg);
		if (value)
			*number = static_cast<int>(*value);
		return value.has_value();
	}
	bool *flag = flagField(opt, opts);
	if (flag != nullptr)
		*flag = true;
	return flag != nullptr;
}

/** Print the command list. */
void printCommands(FILE *out)
{
	std::fputs(
		"Usage: miracast <command> [options]\n\n"
		"Commands:\n"
		"  scan                 list sinks nearby (network via mDNS, then Wi-Fi Direct)\n"
		"  cast [name|IP]       send this machine's screen (and audio) to a sink\n"
		"  sink                 receive and display a Miracast stream (sink role)\n\n",
		out);
}

/** Print general and link options. */
void printLinkOptions(FILE *out)
{
	std::fputs(
		"Link options:\n"
		"  -i, --interface IF   wireless interface / P2P device (default wlan0)\n"
		"      --ctrl-dir DIR   wpa_supplicant control dir (default /run/wpa_supplicant)\n"
		"  -n, --name NAME      advertised device name (default miracast-linux)\n"
		"  -t, --scan-time SEC  scan duration (default 8)\n"
		"      --mice           (cast) force Miracast over Infrastructure to --host IP\n"
		"      --p2p            (cast) skip network discovery, use Wi-Fi Direct\n"
		"      --no-p2p         skip Wi-Fi Direct; use an existing IP network\n"
		"      --host IP        (cast) sink address, with --mice or --no-p2p\n"
		"      --listen IP      (sink) bind address, with --no-p2p (default 0.0.0.0)\n\n",
		out);
}

/** Print media options. */
void printMediaOptions(FILE *out)
{
	std::fputs(
		"Source options:\n"
		"      --output NAME    monitor to share (e.g. eDP-1); default: compositor choice\n"
		"      --capture FRAG   GStreamer source fragment, e.g. \"pipewiresrc path=42\"\n"
		"      --test-pattern   stream a test pattern instead of the screen\n"
		"      --max-width PX   cap the resolution width (default 1920)\n"
		"      --max-fps N      cap the frame rate (default 60)\n"
		"      --bitrate KBPS   video bitrate (default 8000)\n"
		"      --encoder NAME   x264enc (default), vah264enc (GPU) or any GStreamer H.264 "
		"encoder\n"
		"      --encoder-opts S extra properties for the encoder, e.g. "
		"\"speed-preset=veryfast\"\n"
		"      --buffer-ms N    capture-to-encoder queue in ms, 0 disables (default 100)\n"
		"      --no-audio       do not send system audio (AAC-LC 48 kHz stereo)\n"
		"      --audio-source F GStreamer audio source fragment (default: system "
		"output)\n\n"
		"Sink options:\n"
		"      --jitter-ms N    receive jitter buffer in ms (default 40)\n"
		"      --video-sink E   GStreamer video sink (default autovideosink)\n"
		"      --audio-sink E   GStreamer audio sink (default autoaudiosink)\n\n"
		"  -v, --verbose        more logging (repeat for protocol trace)\n"
		"  -h, --help           show this help\n"
		"  -V, --version        show version\n",
		out);
}
} // namespace

/** Print usage to the given stream. */
void printUsage(FILE *out)
{
	printCommands(out);
	printLinkOptions(out);
	printMediaOptions(out);
}

/** Parse argv into opts; returns false on a usage error. May exit for --help/--version. */
bool parseOptions(int argc, char **argv, Options &opts)
{
	int verbosity = 0;
	int opt;
	while ((opt = getopt_long(argc, argv, "i:n:t:vhV", kLongOptions, nullptr)) != -1) {
		if (opt == 'h') {
			printUsage(stdout);
			std::exit(EX_OK);
		}
		if (opt == 'V') {
			std::puts("miracast 1.0.0");
			std::exit(EX_OK);
		}
		if (opt == 'v')
			++verbosity;
		else if (!applyOption(opt, optarg, opts))
			return false;
	}
	setLogLevel(verbosity >= 2 ? LogLevel::Debug : LogLevel::Info);
	if (optind < argc)
		opts.command = argv[optind++];
	if (optind < argc)
		opts.target = argv[optind++];
	const bool valid =
		opts.scanSeconds > 0 && opts.maxWidth > 0 && opts.maxFps > 0 && opts.bitrate > 0;
	return !opts.command.empty() && valid;
}

} // namespace mc

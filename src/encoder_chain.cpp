// SPDX-License-Identifier: GPL-2.0-or-later
#include "encoder_chain.hpp"

namespace mc
{

namespace
{
/** Encoder element with rate, GOP and zero-latency options for the known encoders. */
std::string encoderElement(const StreamConfig &cfg, const CaptureSettings &set)
{
	const std::string fps = std::to_string(cfg.fps);
	const std::string rate = std::to_string(cfg.bitrateKbps);
	std::string element;
	if (set.encoder == "x264enc") {
		element = "x264enc name=enc tune=zerolatency speed-preset=ultrafast bframes=0 "
			  "bitrate=" +
			  rate + " key-int-max=" + fps;
	} else if (set.encoder == "vah264enc") {
		element = "vah264enc name=enc bitrate=" + rate + " key-int-max=" + fps +
			  " b-frames=0 rate-control=cbr";
	} else {
		element = set.encoder + " name=enc bitrate=" + rate;
	}
	return set.encoderOpts.empty() ? element : element + " " + set.encoderOpts;
}

/** Leaky queue absorbing capture jitter; empty when disabled. */
std::string bufferQueue(int bufferMs)
{
	if (bufferMs <= 0)
		return {};
	return " ! queue max-size-buffers=0 max-size-bytes=0 max-size-time=" +
	       std::to_string(static_cast<long>(bufferMs) * 1000000L) + " leaky=downstream";
}
} // namespace

/** Scale/convert, buffer, encode (selected encoder) and parse video; ends before the muxer. */
std::string videoChain(const StreamConfig &cfg, const CaptureSettings &set)
{
	const char *profile = cfg.profile == 0x02 ? "high" : "constrained-baseline";
	return " ! videoconvert ! videoscale ! videorate ! video/x-raw,width=" +
	       std::to_string(cfg.width) + ",height=" + std::to_string(cfg.height) +
	       ",framerate=" + std::to_string(cfg.fps) + "/1" + bufferQueue(set.bufferMs) + " ! " +
	       encoderElement(cfg, set) + " ! video/x-h264,profile=" + profile +
	       ",stream-format=byte-stream,alignment=au ! h264parse config-interval=-1";
}

/** System-audio capture and AAC-LC (ADTS) encoding; ends before the muxer. */
std::string audioChain(const CaptureSettings &set)
{
	const std::string source =
		set.audioSource.empty()
			? "pipewiresrc stream-properties=\"props,stream.capture.sink=true\" "
			  "do-timestamp=true"
			: set.audioSource;
	return source + " ! audioconvert ! audioresample ! audio/x-raw,rate=48000,channels=2 ! "
			"avenc_aac bitrate=128000 ! aacparse ! "
			"audio/mpeg,mpegversion=4,stream-format=adts ! queue";
}

} // namespace mc

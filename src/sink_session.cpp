// SPDX-License-Identifier: GPL-2.0-or-later
#include "sink_session.hpp"

#include "cea_mode.hpp"
#include "client_ports.hpp"
#include "util.hpp"
#include "video_formats.hpp"

namespace mc
{

namespace
{
const char *const kControlUri = "rtsp://localhost/wfd1.0";
const char *const kParamType = "text/parameters";
constexpr int kNativeCea1080p60 = 8;
constexpr uint8_t kMaxLevel = 0x10;

/** Our wfd_video_formats capability: every progressive CEA mode, CBP and CHP. */
std::string videoCapability()
{
	VideoFormats formats;
	formats.native = static_cast<uint8_t>(kNativeCea1080p60 << 3);
	for (const uint8_t profile : {kProfileCBP, kProfileCHP}) {
		H264Codec codec;
		codec.profile = profile;
		codec.level = kMaxLevel;
		codec.cea = progressiveCeaMask();
		formats.codecs.push_back(codec);
	}
	return formats.serialize();
}

/** True when the M4 video selection is something we advertised. */
bool videoSelectionValid(const std::string &value)
{
	const auto formats = VideoFormats::parse(value);
	if (!formats || formats->codecs.size() != 1)
		return false;
	const H264Codec &codec = formats->codecs[0];
	const bool profileOk = codec.profile == kProfileCBP || codec.profile == kProfileCHP;
	const bool masksOk =
		(codec.cea & ~progressiveCeaMask()) == 0 && codec.vesa == 0 && codec.hh == 0;
	return profileOk && masksOk;
}
} // namespace

/** Create a session using the given transport callback and media back-end. */
SinkSession::SinkSession(Send send, SinkMedia &media, SinkConfig cfg)
    : RtspPeer(std::move(send), 100), media_(&media), cfg_(std::move(cfg))
{
}

/** Stops the media if still running. */
SinkSession::~SinkSession()
{
	closeMedia();
}

/** Stop media and mark closed. */
void SinkSession::closeMedia()
{
	if (mediaRunning_)
		media_->stop();
	mediaRunning_ = false;
	state_ = State::Closed;
}

/** Stop immediately (transport vanished). */
void SinkSession::abort()
{
	closeMedia();
	finish();
}

/** Dispatch on the request method. */
void SinkSession::handleRequest(const Message &req)
{
	if (req.method == "OPTIONS")
		onOptions(req);
	else if (req.method == "GET_PARAMETER")
		onGetParameter(req);
	else if (req.method == "SET_PARAMETER")
		onSetParameter(req);
	else if (req.method == "TEARDOWN")
		onTeardown(req);
	else
		reply(req, 405);
}

/** M1: answer and issue M2. */
void SinkSession::onOptions(const Message &req)
{
	Message resp = makeResponse(200, req.cseq());
	resp.set("Public", std::string(kWfdRequire) + ", GET_PARAMETER, SET_PARAMETER");
	sendResponse(resp);
	state_ = State::Negotiating;

	Message optionsReq = makeRequest("OPTIONS", "*");
	optionsReq.set("Require", kWfdRequire);
	sendRequest(optionsReq, [](const Message &answer) {
		if (answer.status != 200)
			logf(LogLevel::Warn, "M2 answered with %d", answer.status);
	});
}

/** Value advertised for a requested capability. */
std::string SinkSession::capability(const std::string &name) const
{
	if (name == "wfd_audio_codecs")
		return "LPCM 00000003 00"; // mandatory for sinks (spec 3.4.2)
	if (name == "wfd_video_formats")
		return videoCapability();
	if (name == "wfd_client_rtp_ports") {
		ClientPorts ports;
		ports.port0 = cfg_.rtpPort;
		return ports.serialize();
	}
	return "none";
}

/** M3 and keep-alives. */
void SinkSession::onGetParameter(const Message &req)
{
	Message resp = makeResponse(200, req.cseq());
	if (!req.body.empty()) {
		for (const auto &name : parseParamNames(req.body))
			resp.body += name + ": " + capability(name) + "\r\n";
		resp.set("Content-Type", kParamType);
	}
	sendResponse(resp); // an empty body is a keep-alive
}

/** Validate the source's M4 selection. */
bool SinkSession::applySelection(const ParamList &params)
{
	const auto video = findParam(params, "wfd_video_formats");
	if (video && !videoSelectionValid(*video))
		return false;
	const auto url = findParam(params, "wfd_presentation_URL");
	if (url) {
		const auto parts = split(trim(*url), ' ');
		if (startsWith(parts[0], "rtsp://"))
			presentationUrl_ = parts[0];
	}
	const auto portsValue = findParam(params, "wfd_client_rtp_ports");
	if (portsValue) {
		const auto ports = ClientPorts::parse(*portsValue);
		return ports && ports->port0 == cfg_.rtpPort;
	}
	return true;
}

/** M4, M5 and other parameters. */
void SinkSession::onSetParameter(const Message &req)
{
	const ParamList params = parseParams(req.body);
	const auto trigger = findParam(params, "wfd_trigger_method");
	if (trigger) {
		reply(req, 200);
		onTrigger(trim(*trigger));
		return;
	}
	if (!applySelection(params)) {
		logf(LogLevel::Warn, "M4 rejected: unsupported selection");
		reply(req, 406);
		return;
	}
	reply(req, 200);
}

/** Act on a wfd_trigger_method. */
void SinkSession::onTrigger(const std::string &method)
{
	if (method == "SETUP")
		sendSetup();
	else if (method == "PLAY" && state_ == State::Setup)
		sendPlay();
	else if (method == "TEARDOWN")
		teardown();
	else
		logf(LogLevel::Debug, "trigger %s ignored", method.c_str());
}

/** M6. */
void SinkSession::sendSetup()
{
	if (presentationUrl_.empty() && cfg_.peerHost.empty()) {
		fail("no presentation URL");
		return;
	}
	if (presentationUrl_.empty())
		presentationUrl_ = "rtsp://" + cfg_.peerHost + "/wfd1.0/streamid=0";
	Message setup = makeRequest("SETUP", presentationUrl_);
	setup.set("Transport", "RTP/AVP/UDP;unicast;client_port=" + std::to_string(cfg_.rtpPort));
	sendRequest(setup, [this](const Message &answer) {
		const auto sess = answer.header("Session");
		if (answer.status != 200 || !sess) {
			fail("SETUP rejected");
			return;
		}
		session_ = sessionId(*sess);
		// Listen before PLAY so the first IDR frame is not lost.
		if (!media_->start(cfg_.rtpPort)) {
			fail("cannot start media pipeline");
			return;
		}
		mediaRunning_ = true;
		state_ = State::Setup;
		sendPlay();
	});
}

/** M7. */
void SinkSession::sendPlay()
{
	Message play = makeRequest("PLAY", presentationUrl_);
	play.set("Session", session_);
	sendRequest(play, [this](const Message &answer) {
		if (answer.status != 200) {
			fail("PLAY rejected");
			return;
		}
		state_ = State::Playing;
		logf(LogLevel::Info, "streaming");
	});
}

/** M8 from the source. */
void SinkSession::onTeardown(const Message &req)
{
	reply(req, 200);
	closeMedia();
	finish();
}

/** Tear the session down gracefully. */
void SinkSession::teardown()
{
	if (session_.empty()) {
		abort();
		return;
	}
	Message request = makeRequest("TEARDOWN", presentationUrl_);
	request.set("Session", session_);
	sendRequest(request, [this](const Message &) { abort(); });
}

/** Ask the source for an IDR frame. */
void SinkSession::requestIdr()
{
	if (state_ != State::Playing)
		return;
	Message request = makeRequest("SET_PARAMETER", kControlUri);
	request.set("Content-Type", kParamType);
	request.body = "wfd_idr_request\r\n";
	sendRequest(request, nullptr);
}

} // namespace mc

// SPDX-License-Identifier: GPL-2.0-or-later
#include "source_session.hpp"

#include <random>

#include "util.hpp"
#include "video_formats.hpp"
#include "wfd_params.hpp"

namespace mc
{

namespace
{
const char *const kControlUri = "rtsp://localhost/wfd1.0";
const char *const kParamType = "text/parameters";
constexpr int kSessionTimeoutSec = 30;
constexpr int kKeepaliveSec = 15;

/** Random 32-bit session id as 8 hex digits. */
std::string randomSession()
{
	std::random_device device;
	return hexString(device(), 8);
}

/** Build a text/parameters SET_PARAMETER request. */
Message parameterRequest(const std::string &body)
{
	Message request = makeRequest("SET_PARAMETER", kControlUri);
	request.set("Content-Type", kParamType);
	request.body = body;
	return request;
}
} // namespace

/** Create a session using the given transport callback and media back-end. */
SourceSession::SourceSession(Send send, SourceMedia &media, SourceConfig cfg)
    : RtspPeer(std::move(send), 1), media_(&media), cfg_(std::move(cfg))
{
}

/** Stops the media if still running. */
SourceSession::~SourceSession()
{
	closeMedia();
}

/** Stop media and mark closed. */
void SourceSession::closeMedia()
{
	if (mediaRunning_)
		media_->stop();
	mediaRunning_ = false;
	state_ = State::Closed;
}

/** Stop immediately (transport vanished). */
void SourceSession::abort()
{
	closeMedia();
	finish();
}

/** Send M1. */
void SourceSession::start()
{
	state_ = State::Negotiating;
	Message options = makeRequest("OPTIONS", "*");
	options.set("Require", kWfdRequire);
	sendRequest(options, [this](const Message &answer) {
		const auto pub = answer.header("Public");
		if (answer.status != 200 || !pub || pub->find(kWfdRequire) == std::string::npos)
			fail("sink does not speak org.wfa.wfd1.0");
		else
			sendGetCaps();
	});
}

/** M3. */
void SourceSession::sendGetCaps()
{
	Message caps = makeRequest("GET_PARAMETER", kControlUri);
	caps.set("Content-Type", kParamType);
	caps.body = "wfd_video_formats\r\nwfd_audio_codecs\r\nwfd_client_rtp_ports\r\n";
	sendRequest(caps, [this](const Message &answer) { onCaps(answer); });
}

/** Process the M3 response and choose a format. */
void SourceSession::onCaps(const Message &resp)
{
	if (resp.status != 200) {
		fail("M3 rejected");
		return;
	}
	logf(LogLevel::Debug, "sink capabilities:\n%s", resp.body.c_str());
	const ParamList params = parseParams(resp.body);
	const auto videoText = findParam(params, "wfd_video_formats");
	const auto portsText = findParam(params, "wfd_client_rtp_ports");
	const auto formats = videoText ? VideoFormats::parse(*videoText) : std::nullopt;
	const auto ports = portsText ? ClientPorts::parse(*portsText) : std::nullopt;
	if (!formats || !ports) {
		fail("sink capabilities missing or malformed");
		return;
	}
	const auto chosen = selectFormat(*formats, cfg_);
	if (!chosen) {
		fail("no common video format");
		return;
	}
	sel_ = *chosen;
	const auto audioText = findParam(params, "wfd_audio_codecs");
	sel_.audio = selectAudio(audioText ? AudioCodec::parseList(*audioText)
					   : std::vector<AudioCodec>(),
				 cfg_.audio);
	sel_.stream.audio = sel_.audio.has_value();
	sinkPorts_ = *ports;
	logf(LogLevel::Info, "selected %dx%d@%d", sel_.stream.width, sel_.stream.height,
	     sel_.stream.fps);
	sendSelection();
}

/** M4 followed by M5. */
void SourceSession::sendSelection()
{
	VideoFormats formats;
	formats.codecs.push_back(sel_.codec);
	std::string body = "wfd_video_formats: " + formats.serialize() + "\r\n";
	if (sel_.audio)
		body += "wfd_audio_codecs: " + AudioCodec::serializeList({*sel_.audio}) + "\r\n";
	body += "wfd_presentation_URL: rtsp://" + cfg_.localHost + "/wfd1.0/streamid=0 none\r\n" +
		"wfd_client_rtp_ports: " + sinkPorts_.serialize() + "\r\n";
	sendRequest(parameterRequest(body), [this](const Message &answer) {
		if (answer.status != 200) {
			fail("sink refused the selected format (M4)");
			return;
		}
		sendRequest(parameterRequest("wfd_trigger_method: SETUP\r\n"),
			    [this](const Message &trigger) {
				    if (trigger.status != 200)
					    fail("SETUP trigger refused (M5)");
			    });
	});
}

/** Dispatch on the request method. */
void SourceSession::handleRequest(const Message &req)
{
	if (req.method == "OPTIONS") {
		Message resp = makeResponse(200, req.cseq());
		resp.set("Public",
			 std::string(kWfdRequire) +
				 ", SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER");
		sendResponse(resp);
	} else if (req.method == "SETUP") {
		onSetup(req);
	} else if (req.method == "PLAY") {
		onPlay(req);
	} else if (req.method == "SET_PARAMETER") {
		onSetParameter(req);
	} else if (req.method == "GET_PARAMETER" || req.method == "PAUSE") {
		reply(req, 200);
	} else if (req.method == "TEARDOWN") {
		reply(req, 200);
		closeMedia();
		finish();
	} else {
		reply(req, 405);
	}
}

/** M6 from the sink. */
void SourceSession::onSetup(const Message &req)
{
	const auto transport = req.header("Transport");
	const auto port = transport ? parseClientPort(*transport) : std::nullopt;
	if (!port) {
		reply(req, 400);
		return;
	}
	clientPort_ = *port;
	session_ = randomSession();
	const std::string serverPorts =
		std::to_string(cfg_.serverPort) + "-" + std::to_string(cfg_.serverPort + 1);
	Message resp = makeResponse(200, req.cseq());
	resp.set("Session", session_ + ";timeout=" + std::to_string(kSessionTimeoutSec));
	resp.set("Transport", "RTP/AVP/UDP;unicast;client_port=" + std::to_string(clientPort_) +
				      ";server_port=" + serverPorts);
	sendResponse(resp);
	state_ = State::Setup;
}

/** M7 from the sink. */
void SourceSession::onPlay(const Message &req)
{
	const auto sess = req.header("Session");
	if (state_ != State::Setup || !sess || sessionId(*sess) != session_) {
		reply(req, 454);
		return;
	}
	reply(req, 200);
	if (!media_->start(sel_.stream, cfg_.peerHost, clientPort_, cfg_.serverPort)) {
		fail("cannot start capture/encode pipeline");
		return;
	}
	mediaRunning_ = true;
	state_ = State::Playing;
	lastKeepalive_ = Clock::now();
	logf(LogLevel::Info, "streaming to %s:%u", cfg_.peerHost.c_str(), clientPort_);
}

/** Sink-originated SET_PARAMETER (IDR request). */
void SourceSession::onSetParameter(const Message &req)
{
	reply(req, 200);
	for (const auto &name : parseParamNames(req.body)) {
		if (name == "wfd_idr_request" && mediaRunning_)
			media_->requestKeyframe();
	}
}

/** Drive keep-alives; call regularly. */
void SourceSession::tick(Clock::time_point now)
{
	if (state_ != State::Playing || now - lastKeepalive_ < std::chrono::seconds(kKeepaliveSec))
		return;
	lastKeepalive_ = now;
	sendRequest(makeRequest("GET_PARAMETER", kControlUri), [this](const Message &answer) {
		if (answer.status != 200)
			fail("keep-alive rejected");
	});
}

/** Ask the sink to tear the session down (M5 TEARDOWN trigger). */
void SourceSession::requestTeardown()
{
	sendRequest(parameterRequest("wfd_trigger_method: TEARDOWN\r\n"), nullptr);
}

} // namespace mc

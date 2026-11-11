// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Unit and in-memory integration tests. */
#include <string>

#include "check.hpp"
#include "fake_sink_media.hpp"
#include "fake_source_media.hpp"
#include "dns_packet.hpp"
#include "mice_protocol.hpp"
#include "parser.hpp"
#include "selection.hpp"
#include "cea_mode.hpp"
#include "sink_session.hpp"
#include "source_session.hpp"
#include "video_formats.hpp"
#include "wfd_params.hpp"

using namespace mc;

namespace
{
/** RTSP framing, size limits and malformed input. */
void testRtspParser()
{
	Parser parser;
	parser.feed("SET_PARAMETER rtsp://localhost/wfd1.0 RTSP/1.0\r\nCSeq: 7\r\n"
		    "Content-Length: 5\r\n\r\nhel");
	CHECK(!parser.next());
	parser.feed("lo");
	const auto msg = parser.next();
	CHECK(msg && msg->isRequest && msg->method == "SET_PARAMETER" && msg->cseq() == 7);
	CHECK(msg && msg->body == "hello");
	Parser garbage;
	garbage.feed("garbage\r\n\r\n");
	CHECK(!garbage.next() && garbage.failed());
	Parser huge;
	huge.feed("OPTIONS * RTSP/1.0\r\nContent-Length: 99999999\r\n\r\n");
	CHECK(!huge.next() && huge.failed());
}

/** wfd_video_formats round trip and rejection. */
void testVideoFormats()
{
	const auto formats = VideoFormats::parse(
		"00 00 02 10 0001FFFF 1FFFFFFF 00000FFF 00 0000 0000 00 none none");
	CHECK(formats && formats->codecs.size() == 1 && formats->codecs[0].profile == 2);
	CHECK(formats && formats->codecs[0].level == 0x10 && formats->codecs[0].cea == 0x0001FFFF);
	const auto again = VideoFormats::parse(formats->serialize());
	CHECK(again && again->codecs[0].vesa == 0x1FFFFFFF);
	CHECK(!VideoFormats::parse("00 00 02"));
	const auto two = VideoFormats::parse("40 00 01 10 00000001 0 0 00 0000 0000 00 none none, "
					     "02 10 00000001 0 0 00 0000 0000 00 none none");
	CHECK(two && two->codecs.size() == 2);
}

/** WFD IE helpers. */
void testWfdIe()
{
	CHECK(wfdDeviceInfoSubelem(WfdDeviceType::PrimarySink, 7236, 50) == "000600111c440032");
	uint16_t port = 0;
	const auto info = parseWfdDevInfo("0x00000601111c440036", &port);
	CHECK(info && *info == 0x0111 && port == 7236);
	CHECK(info && wfdIsSink(*info) && !wfdIsSource(*info));
	CHECK(sessionId("abc123;timeout=30") == "abc123");
	CHECK(parseClientPort("RTP/AVP/UDP;unicast;client_port=19000-19001") == 19000);
}

/** Format selection honours capabilities and limits. */
void testSelection()
{
	SourceConfig cfg;
	VideoFormats formats;
	H264Codec codec;
	codec.profile = kProfileCHP;
	codec.level = 0x10;
	codec.cea = progressiveCeaMask();
	formats.codecs.push_back(codec);
	auto sel = selectFormat(formats, cfg);
	CHECK(sel && sel->stream.width == 1920 && sel->stream.fps == 60);
	cfg.maxWidth = 1280;
	cfg.maxFps = 30;
	sel = selectFormat(formats, cfg);
	CHECK(sel && sel->stream.width == 1280 && sel->stream.fps == 30);
	formats.codecs[0].cea = 1;
	sel = selectFormat(formats, cfg);
	CHECK(sel && sel->stream.width == 640);
	formats.codecs[0].cea = 0;
	CHECK(!selectFormat(formats, cfg));
}

/** Full M1..M7, IDR request and teardown between a source and a sink in memory. */
void testFullHandshake()
{
	FakeSinkMedia sinkMedia;
	FakeSourceMedia sourceMedia;
	SourceConfig sourceCfg;
	sourceCfg.localHost = "192.168.173.50";
	sourceCfg.peerHost = "192.168.173.1";
	std::string toSink;
	std::string toSource;
	SinkSession sink([&](const std::string &data) { toSource += data; }, sinkMedia,
			 SinkConfig());
	SourceSession source([&](const std::string &data) { toSink += data; }, sourceMedia,
			     sourceCfg);
	const auto pump = [&] {
		for (int round = 0; round < 50 && (!toSink.empty() || !toSource.empty()); ++round) {
			std::string forSink;
			forSink.swap(toSink);
			if (!forSink.empty())
				sink.onData(forSink);
			std::string forSource;
			forSource.swap(toSource);
			if (!forSource.empty())
				source.onData(forSource);
		}
	};
	source.start();
	pump();
	CHECK(!sink.failed() && !source.failed());
	CHECK(sink.state() == SinkSession::State::Playing);
	CHECK(source.state() == SourceSession::State::Playing);
	CHECK(sinkMedia.started && sinkMedia.port == 19000);
	CHECK(sourceMedia.started && sourceMedia.destPort == 19000 &&
	      sourceMedia.config.width == 1920);
	sink.requestIdr();
	pump();
	CHECK(sourceMedia.keyframes == 1);
	source.requestTeardown();
	pump();
	CHECK(sink.finished() && source.finished());
}

/** MS-MICE framing: size/version/command header and big-endian TLVs. */
void testMice()
{
	const std::string id(16, 'x');
	const std::string msg = miceSourceReady("ab", 7236, id);
	CHECK(msg.size() == 35);
	CHECK(msg.substr(0, 11) == std::string("\x00\x23\x01\x01\x00\x00\x04\x61\x00\x62\x00", 11));
	CHECK(msg.substr(11, 5) == std::string("\x02\x00\x02\x1c\x44", 5));
	CHECK(msg.substr(16, 3) == std::string("\x03\x00\x10", 3));
	CHECK(miceCommandOf(msg) == MiceCommand::SourceReady && miceFrameSize(msg) == 35);
	CHECK(!miceCommandOf(msg.substr(0, 10)));
	CHECK(miceCommandOf(miceStopProjection("ab", id)) == MiceCommand::StopProjection);
	CHECK(utf16le("\xC3\xA9") == std::string("\xE9\x00", 2));
}

/** Audio selection from a TV-style capability string. */
void testAudio()
{
	const auto tv = AudioCodec::parseList("LPCM 00000003 00, AAC 00000001 00");
	const auto chosen = selectAudio(tv, true);
	CHECK(chosen && chosen->name == "AAC" && chosen->modes == 1);
	CHECK(!selectAudio(tv, false));
	CHECK(!selectAudio(AudioCodec::parseList("LPCM 00000003 00"), true));
	CHECK(AudioCodec::serializeList({*chosen}) == "AAC 00000001 00");
}

/** mDNS: query encoding and a PTR+SRV response like the LG TV sends. */
void testMdns()
{
	const std::string query = buildPtrQuery(kDisplayService);
	CHECK(query.size() == 12 + 9 + 5 + 6 + 1 + 4 && query[5] == 1);
	std::string resp = std::string("\x00\x00\x84\x00\x00\x01\x00\x02\x00\x00\x00\x00", 12) +
			   query.substr(12);
	resp.resize(resp.size() - 4);
	resp += std::string("\x00\x0c\x80\x01", 4);
	resp += std::string(
		"\xc0\x0c\x00\x0c\x00\x01\x00\x00\x00\x0a\x00\x0e\x0bla-tevezita\xc0\x0c", 26);
	resp += std::string(
		"\xc0\x0c\x00\x21\x00\x01\x00\x00\x00\x0a\x00\x08\x00\x00\x00\x00\x1c\x52\x00\x00",
		20);
	const auto info = parseDisplayResponse(resp);
	CHECK(info && info->name == "la-tevezita" && info->port == 7250);
	CHECK(!parseDisplayResponse("short"));
}

/** M4 with an unsupported selection is refused with 406. */
void testRejectsBadSelection()
{
	FakeSinkMedia media;
	std::string out;
	SinkSession sink([&](const std::string &data) { out += data; }, media, SinkConfig());
	sink.onData("SET_PARAMETER rtsp://localhost/wfd1.0 RTSP/1.0\r\nCSeq: 3\r\n"
		    "Content-Type: text/parameters\r\nContent-Length: 82\r\n\r\n"
		    "wfd_video_formats: 00 00 01 01 00000000 00000001 00000000 00 0000 0000 00 "
		    "none none");
	Parser parser;
	parser.feed(out);
	const auto resp = parser.next();
	CHECK(resp && resp->status == 406);
}
} // namespace

/** Run all tests. */
int main()
{
	testRtspParser();
	testVideoFormats();
	testWfdIe();
	testSelection();
	testFullHandshake();
	testRejectsBadSelection();
	testMice();
	testAudio();
	testMdns();
	if (g_failures.load() == 0)
		std::puts("all tests passed");
	return g_failures.load() == 0 ? 0 : 1;
}

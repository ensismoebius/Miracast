cmake_minimum_required(VERSION 3.16)
project(miracast LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-video-1.0 gio-2.0)

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2)

add_library(mccore STATIC
  src/audio_codec.cpp
  src/cast_session.cpp
  src/cea_mode.cpp
  src/cli.cpp
  src/client_ports.cpp
  src/cmd_cast.cpp
  src/cmd_cast_mice.cpp
  src/cmd_scan.cpp
  src/cmd_sink.cpp
  src/dns_packet.cpp
  src/encoder_chain.cpp
  src/exec.cpp
  src/file_descriptor.cpp
  src/group_network.cpp
  src/gst_sink_media.cpp
  src/gst_source_media.cpp
  src/gst_util.cpp
  src/mdns_discovery.cpp
  src/message.cpp
  src/mice_protocol.cpp
  src/net.cpp
  src/p2p_link.cpp
  src/parser.cpp
  src/process.cpp
  src/process_spawn.cpp
  src/rtsp_peer.cpp
  src/screen_capture.cpp
  src/selection.cpp
  src/session_pump.cpp
  src/sink_lookup.cpp
  src/sink_session.cpp
  src/source_session.cpp
  src/util.cpp
  src/video_formats.cpp
  src/wfd_params.cpp
  src/wpa_ctrl.cpp)
target_include_directories(mccore PUBLIC src)
target_link_libraries(mccore PUBLIC PkgConfig::GST)

add_executable(miracast src/main.cpp)
target_link_libraries(miracast PRIVATE mccore)

enable_testing()
add_executable(mctests tests/test_main.cpp)
target_link_libraries(mctests PRIVATE mccore)
add_test(NAME unit COMMAND mctests)

include(GNUInstallDirs)
install(TARGETS miracast RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES docs/miracast.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES docs/miracast-sink.service
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/system)

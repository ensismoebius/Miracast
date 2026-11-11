# SPDX-License-Identifier: GPL-2.0-or-later
Name:           miracast
Version:        %{?pkg_version}%{!?pkg_version:1.0.0}
Release:        1%{?dist}
Summary:        Miracast (Wi-Fi Display) source and sink for the command line
License:        GPL-2.0-or-later
URL:            https://github.com/ensismoebius/Miracast
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake >= 3.16
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(gstreamer-video-1.0)
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  systemd-rpm-macros

Requires:       gstreamer1-plugins-base
Requires:       gstreamer1-plugins-good
Requires:       gstreamer1-plugin-libav
Recommends:     gstreamer1-plugins-bad-free
Recommends:     gstreamer1-plugins-ugly
Recommends:     pipewire-gstreamer
Recommends:     iproute
Suggests:       dnsmasq
Suggests:       wpa_supplicant

%description
Sends the screen and the system audio to a Miracast receiver such as a smart
TV, or receives and displays a Miracast stream. Works over the existing IP
network (Miracast over Infrastructure, MS-MICE) with automatic discovery by
mDNS, or over Wi-Fi Direct when the adapter supports it. Video is H.264
(CPU or GPU encoder), audio is AAC-LC, carried in MPEG-TS over RTP through
GStreamer.

The x264 encoder comes from the RPM Fusion "gstreamer1-plugins-ugly" package;
without it use --encoder vah264enc or another H.264 encoder.

%prep
%autosetup

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%check
%ctest

%install
%cmake_install

%files
%license LICENSE
%doc README.md
%{_bindir}/miracast
%{_mandir}/man1/miracast.1*
%{_prefix}/lib/systemd/system/miracast-sink.service

%changelog
* Wed Nov 11 2026 André Furlan <ensismoebius@gmail.com> - 1.0.0-1
- Initial package

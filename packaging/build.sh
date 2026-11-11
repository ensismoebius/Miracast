#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Build distribution packages of miracast from the working tree of this repository.
#
#   packaging/build.sh <arch|debian|ubuntu|fedora|all> [--native|--docker]
#
# Packages are written to dist/.  By default the build runs natively when the host is
# the target distribution (or a derivative) and inside a container otherwise.
set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
root=$(dirname "$here")
dist="$root/dist"
version=$(sed -n 's/^project(miracast.*VERSION \([0-9.]*\)).*/\1/p' "$root/CMakeLists.txt")

declare -A images=(
	[arch]="archlinux:latest"
	[debian]="debian:stable"
	[ubuntu]="ubuntu:24.04"
	[fedora]="fedora:latest"
)

usage() {
	sed -n '4,9p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
	exit "${1:-0}"
}

die() {
	echo "error: $*" >&2
	exit 1
}

# True when the running system is (or derives from) the given distribution.
host_is() {
	[[ -r /etc/os-release ]] || return 1
	local id like
	id=$(. /etc/os-release && echo "${ID:-}")
	like=$(. /etc/os-release && echo "${ID_LIKE:-}")
	[[ "$id" == "$1" || " $like " == *" $1 "* ]]
}

# Source tarball of the working tree (tracked and untracked, minus ignored files).
make_tarball() {
	local dir=$1 file
	(cd "$root" && git -c safe.directory='*' ls-files -z --cached --others --exclude-standard |
		while IFS= read -r -d '' file; do
			[[ -e $file ]] && printf '%s\0' "$file"
		done) |
		tar -C "$root" --null -T - --transform "s,^,miracast-$version/," \
			-czf "$dir/miracast-$version.tar.gz"
}

build_arch() {
	local work
	work=$(mktemp -d)
	trap 'rm -rf "$work"; trap - RETURN' RETURN
	cp "$here/arch/PKGBUILD" "$work/"
	make_tarball "$work"
	# The local tarball unpacks to miracast-<version>; the release one to Miracast-<version>.
	sed -i 's|Miracast-${pkgver}|miracast-${pkgver}|g' "$work/PKGBUILD"
	(cd "$work" && MIRACAST_SOURCE="miracast-$version.tar.gz" MIRACAST_SHA256=SKIP \
		PKGDEST="$dist" BUILDDIR="$work/build" SRCDEST="$work/src" makepkg -fs --noconfirm)
}

build_debian() {
	local target=$1 work
	work=$(mktemp -d)
	trap 'rm -rf "$work"; trap - RETURN' RETURN
	make_tarball "$work"
	tar -xzf "$work/miracast-$version.tar.gz" -C "$work"
	local src="$work/miracast-$version"
	cp -r "$here/debian" "$src/debian"
	sed -e "s/@VERSION@/$version/g" -e "s/@DATE@/$(date -R)/" \
		"$here/debian/changelog.in" >"$src/debian/changelog"
	rm -f "$src/debian/changelog.in"
	(cd "$src" && dpkg-buildpackage -us -uc -b)
	local deb base
	for deb in "$work"/*.deb; do
		base=$(basename "$deb" .deb) # name_version_arch -> name_version_<distro>_arch
		cp "$deb" "$dist/${base%_*}_${target}_${base##*_}.deb"
	done
}

build_fedora() {
	local top
	top=$(mktemp -d)
	trap 'rm -rf "$top"; trap - RETURN' RETURN
	mkdir -p "$top"/{SOURCES,SPECS}
	make_tarball "$top/SOURCES"
	cp "$here/rpm/miracast.spec" "$top/SPECS/"
	rpmbuild -ba --define "_topdir $top" --define "pkg_version $version" \
		"$top/SPECS/miracast.spec"
	cp "$top"/RPMS/*/*.rpm "$top"/SRPMS/*.rpm "$dist/"
}

# Install the build tools inside a throw-away container.
install_deps() {
	case "$1" in
	arch)
		pacman -Syu --noconfirm --needed base-devel git cmake sudo \
			gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad \
			gst-plugins-ugly gst-libav gst-plugin-va gst-plugin-pipewire pipewire glib2
		id builder >/dev/null 2>&1 || useradd -m builder
		echo 'builder ALL=(ALL) NOPASSWD: ALL' >/etc/sudoers.d/builder
		;;
	debian | ubuntu)
		export DEBIAN_FRONTEND=noninteractive
		apt-get update
		apt-get install -y --no-install-recommends build-essential debhelper cmake \
			pkg-config git ca-certificates libgstreamer1.0-dev \
			libgstreamer-plugins-base1.0-dev libglib2.0-dev fakeroot
		;;
	fedora)
		dnf install -y gcc-c++ cmake git rpm-build systemd-rpm-macros \
			'pkgconfig(gstreamer-1.0)' 'pkgconfig(gstreamer-video-1.0)' 'pkgconfig(gio-2.0)' \
			gstreamer1-plugins-base gstreamer1-plugins-good
		;;
	esac
}

# Run the native build for one target (used on the host and inside containers).
native() {
	mkdir -p "$dist"
	case "$1" in
	arch) build_arch ;;
	debian | ubuntu) build_debian "$1" ;;
	fedora) build_fedora ;;
	esac
}

in_container() {
	local target=$1
	command -v docker >/dev/null || die "docker is required to build $target on this host"
	mkdir -p "$dist"
	local inner="packaging/build.sh --inside $target"
	if [[ $target == arch ]]; then
		inner="chown -R builder /src-copy && su builder -c 'cd /src-copy && $inner'"
	fi
	docker run --rm -v "$root:/src:ro" -v "$dist:/out" "${images[$target]}" bash -ec "
		cp -r /src /src-copy && cd /src-copy
		packaging/build.sh --install-deps $target
		$inner
		cp -f /src-copy/dist/* /out/ && chown -R $(id -u):$(id -g) /out"
}

main() {
	[[ $# -ge 1 ]] || usage 1
	case "$1" in
	-h | --help) usage 0 ;;
	--install-deps) install_deps "$2"; exit 0 ;;
	--inside) native "$2"; exit 0 ;;
	esac
	local target=$1 mode=${2:---auto}
	[[ $target == all ]] && {
		for each in arch debian ubuntu fedora; do "$0" "$each" "$mode"; done
		return
	}
	[[ -n ${images[$target]:-} ]] || die "unknown target '$target'"
	case "$mode" in
	--native) native "$target" ;;
	--docker) in_container "$target" ;;
	--auto) if host_is "$target" && [[ $(id -u) -ne 0 ]]; then native "$target"; else in_container "$target"; fi ;;
	*) usage 1 ;;
	esac
	echo "packages in $dist:" && ls -1 "$dist"
}

main "$@"

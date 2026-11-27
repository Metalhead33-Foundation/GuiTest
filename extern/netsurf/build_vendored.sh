#!/bin/sh
# Builds the vendored NetSurf libraries and installs them into a private prefix.
#
# Usage: build_vendored.sh <prefix>
#
# libwapcaplet and libparserutils are built first, because libcss links against them. The
# install step publishes headers, `*.a` and `*.pc` files under <prefix>, and BUILDDIR keeps
# every object file there as well, so the checked-out submodules stay pristine.
#
# The environment variable MAKE is honoured, which is how the qmake build passes on its own
# recursive-make binary.
set -eu

prefix=${1:?usage: build_vendored.sh <prefix>}
root=$(cd "$(dirname "$0")" && pwd)
make=${MAKE:-make}
buildsystem=$root/buildsystem

# These archives end up inside libLotte.so, so they have to be position independent.
# NetSurf's buildsystem has no PIC switch of its own and its component makefiles
# append to $(CFLAGS), which a command-line CFLAGS would silently displace; $(CC) is
# passed straight to the compiler, so the flag rides along with it.
cc=${CC:-cc}
cc="$cc -fPIC"

mkdir -p "$prefix/build"

# Two subprojects may run this at the same time when the tree is built with -j.
if [ -z "${NETSURF_VENDOR_LOCKED:-}" ] && command -v flock >/dev/null 2>&1; then
    NETSURF_VENDOR_LOCKED=1 exec flock "$prefix/.build.lock" "$0" "$@"
fi

for component in libwapcaplet libparserutils; do
    "$make" -C "$root/$component" \
        NSSHARED="$buildsystem" \
        CC="$cc" \
        PREFIX="$prefix" \
        BUILDDIR="$prefix/build/$component" \
        COMPONENT_TYPE=lib-static install

done

# libcss resolves its dependencies through the prefix instead of pkg-config, so the vendored
# headers and archives win over whatever the host has installed.
"$make" -C "$root/libcss" \
    NSSHARED="$buildsystem" \
    CC="$cc" \
    PREFIX="$prefix" \
    PKGCONFIG= \
    BUILDDIR="$prefix/build/libcss" \
    COMPONENT_TYPE=lib-static install

touch "$prefix/.installed"

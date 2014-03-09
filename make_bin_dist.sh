#!/bin/sh

##
## A small helper script to build (possibly static) binary executable
##

die()
{
  BASE=$(basename "$0")
  echo "$BASE error: $@" >&2
  exit 1
}

cd $(dirname "$0") || die "failed to set directory"

CALCVER=$(./build-aux/git-version-gen .tarball-version) || die "can't get calc version"

[ -e "Makfile" ] && make distclean

KERNEL=$(uname -s)   # Linux,FreeBSD,Darwin
RELEASE=$(uname -r)  # 2.6.30, 10-RELEASE, 10.2.8
MACHINE=$(uname -m)  # x86_64, amd64, i386

# These kernels (and by proxy, operating systems) can
# generate static binaries.
STATICFLAG=
[ "$KERNEL" = "Linux" -o "$KERNEL" = "FreeBSD" ] && STATICFLAG="LDFLAGS=-static"

./configure CFLAGS="-O2" $STATICFLAG || die "./configure failed"
make || die "make failed"

SRC=./src/calc
[ -e "$SRC" ] || die "Expected program file '$SRC' not found"

DATE=$(date -u +"%F-%H%M%S")

mkdir -p bin || die "failed to create 'bin' directory"

DST=calc-${CALCVER}__${KERNEL}__${RELEASE}__${MACHINE}__${DATE}.bin
cp "$SRC" "bin/$DST" || die "failed to create destination binary ($DST)"


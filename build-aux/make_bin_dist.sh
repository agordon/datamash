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

cd $(dirname "$0")/.. || die "failed to set directory"

CALCVER=$(./build-aux/git-version-gen .tarball-version) || die "can't get calc version"

KERNEL=$(uname -s)   # Linux,FreeBSD,Darwin
RELEASE=$(uname -r)  # 2.6.30, 10-RELEASE, 10.2.8
MACHINE=$(uname -m)  # x86_64, amd64, i386

# These kernels (and by proxy, operating systems) can
# generate static binaries.
STATICFLAG=
[ "$KERNEL" = "Linux" -o "$KERNEL" = "FreeBSD" ] && STATICFLAG="LDFLAGS=-static"

# Technically incorrect, but much friendlier to users
[ "$KERNEL" = "Darwin" ] && KERNEL="MacOSX"

./configure CFLAGS="-O2" $STATICFLAG || die "./configure failed"
make clean
make || die "make failed"

SRC=./calc
[ -e "$SRC" ] || die "Expected program file '$SRC' not found"

DATE=$(date -u +"%F-%H%M%S")

NAME="calc-${CALCVER}-bin__${KERNEL}__${MACHINE}"
mkdir -p "bin/$NAME" || die "failed to create 'bin/$NAME' directory"

cp "$SRC" "bin/$NAME/calc" || die "failed to create destination binary (bin/$NAME/calc)"

cd "bin" || die
tar -czvf "$NAME.tar.gz" "$NAME" || die "failed to create TarBall for binary executable"
cd ".."

echo "Done. File ="
echo "   ./bin/$NAME.tar.gz"

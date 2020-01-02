#!/bin/sh

## Copyright (C) 2014-2020 Assaf Gordon <assafgordon@gmail.com>
##
## This file is part of GNU Datamash.
##
## GNU Datamash is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## GNU Datamash is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Datamash.  If not, see <https://www.gnu.org/licenses/>.


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

DATAMASHVER=$(./build-aux/git-version-gen .tarball-version) ||
  die "can't get datamash version"

KERNEL=$(uname -s)   # Linux,FreeBSD,Darwin
RELEASE=$(uname -r)  # 2.6.30, 10-RELEASE, 10.2.8
MACHINE=$(uname -m)  # x86_64, amd64, i386

# These kernels (and by proxy, operating systems) can
# generate static binaries.
STATICFLAG=
[ "$KERNEL" = "Linux" -o "$KERNEL" = "FreeBSD" ] &&
  STATICFLAG="LDFLAGS=-static"

# Technically incorrect, but much friendlier to users
[ "$KERNEL" = "Darwin" ] && KERNEL="MacOSX"

./configure CFLAGS="-O2" $STATICFLAG || die "./configure failed"
make clean
make || die "make failed"

SRC=./datamash
[ -e "$SRC" ] || die "Expected program file '$SRC' not found"

DATE=$(date -u +"%F-%H%M%S")

NAME="datamash-${DATAMASHVER}-bin__${KERNEL}__${MACHINE}"
mkdir -p "bin/$NAME" || die "failed to create 'bin/$NAME' directory"

cp "$SRC" "bin/$NAME/datamash" ||
  die "failed to create destination binary (bin/$NAME/datamash)"

cd "bin" || die
tar -czvf "$NAME.tar.gz" "$NAME" ||
  die "failed to create TarBall for binary executable"
cd ".."

echo "Done. File ="
echo "   ./bin/$NAME.tar.gz"
echo "Upload to AWS S3:"
echo "   ./build-aux/aws-upload.sh ./bin/$NAME.tar.gz bin"
echo

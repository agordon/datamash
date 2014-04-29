#!/bin/sh

## Copyright (C) 2014 Assaf Gordon <assafgordon@gmail.com>
##
## This file is part of Compute.
##
## Compute is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Compute is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Compute.  If not, see <http://www.gnu.org/licenses/>.


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

COMPUTEVER=$(./build-aux/git-version-gen .tarball-version) || die "can't get compute version"

KERNEL=cygwin
MACHINE=win64

SRC=./compute.exe
[ -e "$SRC" ] || die "Expected program file '$SRC' not found"

DATE=$(date -u +"%F-%H%M%S")

NAME="compute-${COMPUTEVER}-bin__${KERNEL}__${MACHINE}"
mkdir -p "bin/$NAME" || die "failed to create 'bin/$NAME' directory"

cp "$SRC" "bin/$NAME/compute.exe" || die "failed to create destination binary (bin/$NAME/compute)"
# Copy additional Cygwin DLLs
DLLS=$(ldd "$SRC" | awk '{print $1}' | grep "^cyg.*\.dll") || die "Failed to detect DLLs"
for d in $DLLS ; do
   FULLPATH=$(which "$d") || die "Failed to find full path of DLL '$d'"
   cp "$FULLPATH" "bin/$NAME" || die "failed to copy DLL '$FULLPATH' to 'bin/$NAME'"
done


cd "bin" || die
zip -r "$NAME.zip" "$NAME" || die "failed to create TarBall for binary executable"
cd ".."

echo "Done. File ="
echo "   ./bin/$NAME.zip"
echo "Upload to AWS S3:"
echo "   ./build-aux/aws-upload.sh ./bin/$NAME.zip bin"
echo

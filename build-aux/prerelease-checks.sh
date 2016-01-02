#!/bin/sh

## Copyright (C) 2014-2016 Assaf Gordon <assafgordon@gmail.com>
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
## along with GNU Datamash.  If not, see <http://www.gnu.org/licenses/>.


##
## A small helper script to easily copy, build and check
## the current repository in several different environments and configurations.
##
PACKAGE=datamash

die()
{
BASE=$(basename "$0")
echo "$BASE: error: $@" >&2
exit 1
}

##
## Clean, rebuild and test locally
##

make maintainer-clean # ignore failure
./bootstrap || die "./bootstrap failed"
./configure || die "./configure failed"
make || die "make failed"

for TYPE in check-very-expensive \
            syntax-check \
            html info pdf \
            clean deb-hard \
            clean coverage \
            clean distcheck ;
do
  make $TYPE || die "make $TYPE failed"
done

##
## Use the TARBALL (from previous 'make distcheck'),
##   and check on multiple platforms
VER=$(./build-aux/git-version-gen v) || die "failed to detect git version"
TARBALL="${PACKAGE}-${VER}.tar.gz"
[ -e "$TARBALL" ] || die "tarball $TARBALL not found"

./build-aux/check-remote-make-all.sh "$TARBALL" ||
          die "check-remote-make-all.sh $TARBALL failed"
./build-aux/check-remote-make-extra.sh "$TARBALL" ||
          die "check-remote-make-extra.sh $TARBALL failed"
./build-aux/check-remote-make-git.sh ||
          die "check-remote-make-git.sh failed"

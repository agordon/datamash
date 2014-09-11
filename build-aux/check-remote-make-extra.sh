#!/bin/sh

## Copyright (C) 2014 Assaf Gordon <assafgordon@gmail.com>
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


die()
{
BASE=$(basename "$0")
echo "$BASE: error: $@" >&2
exit 1
}

SOURCE=$1
[ -z "$SOURCE" ] &&
  die "missing SOURCE file name / URL (e.g. datamash-1.0.1.tar.gz)"

LOGDIR=$(mktemp -d -t buildlog.XXXXXX) ||
  die "Failed to create build log directory"

# Default virtual machine with these compilers
HOST=${HOST:-deb7}

##
## Test builds with extra configuration options
##
NUM=1
for config in \
  "-c --host=arm-linux-gnueabi" \
  "-c --host=mips-linux-gnu" \
  "-c --host=powerpc-linux-gnu" \
  "-c --host=x86_64-w64-mingw32" \
  "-c --host=i686-w64-mingw32" \
  "-c CC=tcc" \
; do
    LOGFILE=$LOGDIR/$NUM.log
    echo "building (config = $config) log = $LOGFILE ..."
    ./build-aux/check-remote-make.sh \
          $config $SOURCE $HOST 1>$LOGFILE 2>&1
    exitcode=$?
    if [ "$exitcode" -eq "0" ]; then
        echo "$SOURCE on $host - build OK"
    else
        echo "$SOURCE on $host - Error (log = $LOGFILE )"
    fi
    NUM=$((NUM+1))
done

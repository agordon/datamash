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

dict_set()
{
  key=$1
  value=$2
  eval "__data__$key=\"$value\""
}

dict_get()
{
  key=$1
  eval "echo \$__data__$key"
}

SOURCE=$1
[ -z "$SOURCE" ] && die "missing SOURCE file name / URL (e.g. datamash-1.0.1.tar.gz)"

LOGDIR=$(mktemp -d -t buildlog.XXXXXX) || die "Failed to create build log directory"

HOSTS=${HOSTS:="deb7 deb7clang deb732 deb732clang centos65 centos5 fbsd10
fbsd93 fbsd84 netbsd614 dilos dilos64 hurd obsd"}

##
## Start build on all hosts
##
ALLLOGFILES=""
for host in $HOSTS ;
do
    LOGFILE=$LOGDIR/$host.log
    echo "Starting remote build on $host (log = $LOGFILE ) ..."
    ./build-aux/check-remote-make.sh "$SOURCE" "$host" 1>$LOGFILE 2>&1 &
    pid=$!
    dict_set $host $pid
    ALLLOGFILES="$ALLLOGFILES $LOGFILE"
done

echo "Waiting for remote builds to complete..."
echo "To monitor build progress, run:"
echo "   tail -f $ALLLOGFILES"
echo ""


##
## For completion, report result
##
for host in $HOSTS ;
do
    LOGFILE=$LOGDIR/$host.log
    pid=$(dict_get $host)
    wait $pid
    exitcode=$?
    if [ "$exitcode" -eq "0" ]; then
        echo "$SOURCE on $host - build OK"
    else
        echo "$SOURCE on $host - Error (log = $LOGFILE )"
    fi
done

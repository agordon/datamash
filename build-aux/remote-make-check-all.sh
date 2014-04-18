#!/bin/sh

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

TARBALL=$1
[ -z "$TARBALL" ] && die "missing TARBALL file name (e.g. compute-1.0.1.tar.gz)"
[ -e "$TARBALL" ] || die "Tarball '$TARBALL' not found"

LOGDIR=$(mktemp -d -t buildlog.XXXXXX) || die "Failed to create build log directory"

HOSTS="deb7 centos fbsd obsd dilos hurd"

##
## Start build on all hosts
##
ALLLOGFILES=""
for host in $HOSTS ;
do
    LOGFILE=$LOGDIR/$host.log
    echo "Starting remote build on $host (log = $LOGFILE ) ..."
    ./build-aux/remote-make-check.sh "$TARBALL" "$host" 1>$LOGFILE 2>&1 &
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
        echo "$TARBALL on $host - build OK"
    else
        echo "$TARBALL on $host - Error (log = $LOGFILE )"
    fi
done

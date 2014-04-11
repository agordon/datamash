#!/bin/sh

##
## A small helper script to easily copy, build and check
## a tarball (after 'make dist') on a remote host.
##
## The remote host should be properly configured for password-less login,
## with all the necessary programs already installed.
##

die()
{
BASE=$(basename "$0")
echo "$BASE: error: $@" >&2
exit 1
}

TARBALL=$1
TARGET_HOST=$2

[ -z "$TARBALL" ] && die "missing TARBALL file name (e.g. compute-1.0.1.tar.gz)"
[ -e "$TARBALL" ] || die "Tarball '$TARBALL' not found"
[ -z "$TARGET_HOST" ] && die "missing HOST name (e.g. fbsd)"


BASENAME=$(basename "$TARBALL" .tar.gz) || exit 1

##
## Test connectivity
## If configured properly, this should work without requiring a password.
UNAME=$(ssh -n "$TARGET_HOST" "uname -a") || die "failed to run 'uname' on target host '$TARGET_HOST'"

##
## Copy the tarball
##
scp -q "$TARBALL" "$TARGET_HOST:" || die "Failed to scp '$TARBALL' to '$TARGET_HOST'"

##
## Extract and run on the remove machine
##
SCRIPT="hostname ; uname -a ; rm -r ./$BASENAME ; tar -xzf $TARBALL && cd $BASENAME && ./configure && make && make check && cd .. && rm -r ./$BASENAME $TARBALL"
ssh -q "$TARGET_HOST" "$SCRIPT" 2>&1 || die "Failed to build '$TARBALL' on '$TARGET_HOST'"

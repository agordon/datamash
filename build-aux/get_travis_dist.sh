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
## A small helpr script to extract the base64 file from the travis-CI logs.
## See "after_success" in '.travis.yml' for encoding
##
## The 'travis' program is required, install with 'gem install travis'.
## See http://blog.travis-ci.com/2013-01-14-new-client/
##

die()
{
  BASE=$(basename "$0")
  echo "$BASE error: $@" >&2
  exit 1
}

TRAVIS_BUILD_NUM=$1
[ -z "$TRAVIS_BUILD_NUM" ] && die "Missing Travis-CI build number (use 'travis history' to list builds)"

which travis 2>/dev/null || die "Missing 'travis' program. See http://blog.travis-ci.com/2013-01-14-new-client/"

echo "$TRAVIS_BUILD_NUM" | grep -q -E "^[[:digit:]]+(\.[[:digit:]]+)?$" || die "Invalid Travis-CI build number ($TRAVIS_BUILD_NUM). examples: '28' or '28.1'"

LOGFILE="travis-ci.$TRAVIS_BUILD_NUM.log"

travis logs "$TRAVIS_BUILD_NUM" > "$LOGFILE" || die "Failed to get travis log for build $TRAVIS_BUILD_NUM"

FILENAME=$(grep "^--BIN-DIST-START--" "$LOGFILE" | cut -f2 -d:)
[ -z "$FILENAME" ] && die "can't find bin-dist filename marker in file '$LOGFILE'"
[ -e "$FILENAME" ] && die "File from Travis-CI build already exists: $FILENAME"

# Extract filename and decode
cat "$LOGFILE" |
	sed '1,/^--BIN-DIST-START--/d' |
	sed '/^--BIN-DIST-END--/,$d' |
	base64 -d > "$FILENAME" || die "Failed to decode file '$FILENAME' from log '$LOGFILE'"

echo "done!"
echo "File = "
echo "   $FILENAME"

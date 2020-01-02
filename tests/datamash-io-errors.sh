#!/bin/sh
#   Unit Tests for GNU Datamash - I/O error simulation

#    Copyright (C) 2014-2020 Assaf Gordon <assafgordon@gmail.com>
#
#    This file is part of GNU Datamash.
#
#    GNU Datamash is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    GNU Datamash is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with GNU Datamash.  If not, see <https://www.gnu.org/licenses/>.
#
#    Written by Assaf Gordon

##
## This script tests datamash's handling of I/O errors.
## It requires special setup, and is skipped unless found.
##

. "${test_dir=.}/init.sh"; path_prepend_ ./src

expensive_

fail=0

##
## The required mounted file-systems
##
FULLFS=/tmp/fullfs/
BADFS=/tmp/badfs/

which mountpoint >/dev/null 2>&1 ||
    skip_ "requires mountpoint program"
stdbuf --version >/dev/null 2>&1 ||
    skip_ "requires GNU stdbuf program"
stat --version >/dev/null 2>&1 ||
    skip_ "requires GNU stat program"
mountpoint -q "$FULLFS" ||
    skip_ "requires special mounted file system '$FULLFS'"
mountpoint -q "$BADFS" ||
    skip_ "requires special mounted file system '$BADFS'"

##
## Clean files in the (almost) full file-system.
## This will ensure few writes are successful before getting "no space" error
## (unlike "/dev/full").
##
clean_full_fs()
{
  find "$FULLFS" -maxdepth 1 -type f -delete ||
    framework_failure_ "failed to clean full-fs"
  # Give the system time to actually delete the files
  fullfs_retries=1
  FREE=0
  while test $fullfs_retries -lt 5 && test $FREE -le 5 ; do
    sync ; sleep 1
    FREE=$(stat --file-system -c %a "$FULLFS") ||
      framework_failure_ "failed to find free space on $FULLFS"
    fullfs_retries=$((fullfs_retries+1))
  done
  # Ensure the (almost) full file system has a bit of free space...
  test "$FREE" -gt 5 ||
    framework_failure_ "almost-full-file system has no free space"
  # ... but not too much (otherwise the program will not get "no space" errors).
  test "$FREE" -lt 64 ||
    framework_failure_ "almost-full-file system has too much free spcae"
}

##
## Sanity checks:
## 1. Ensure the corrupted file system is corrupted
cat "$BADFS/numbers.txt" >/dev/null 2>&1 &&
    framework_failure_ "corrupted file system did not trigger I/O error"
## 2. Ensure the (almost) full file system gets full
clean_full_fs
seq 10000 >"$FULLFS/test.txt" 2>/dev/null &&
    framework_failure_ "almost full file system did not trigger no-space error"
clean_full_fs

## Test 1:
##  input error, reading file directly
datamash sum 1 < "$BADFS/numbers.txt" >/dev/null &&
	{ warn_ "datamash failed to detect read error" ; fail=1 ; }

## Test 2:
##  input error, using sort (and popen/pipe)
datamash -s -g 1 sum 1 < "$BADFS/numbers.txt" >/dev/null &&
	{ warn_ "datamash+sort failed to detect read error" ; fail=1 ; }

## Test 3:
##  output error, default line-buffering
seq 10000 | datamash -g 1 count 1 > "$FULLFS/test.txt" &&
	{ warn_ "datamash failed to detect no-space error" ; fail=1 ; }
clean_full_fs

## Test 4:
##  output error, with line-buffering.
##  This means few of the first "write()" calls will succeed,
##  and later ones should fail with "no space" (which is different than
##  writing to "/dev/full").
seq 10000 | stdbuf -oL datamash -g 1 count 1 > "$FULLFS/test.txt" &&
	{ warn_ "datamash failed to detect no-space error" ; fail=1 ; }
clean_full_fs

Exit $fail

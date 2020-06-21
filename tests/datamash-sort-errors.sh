#!/bin/sh
#   Unit Tests for GNU Datamash - perform simple calculation on input data

#    Copyright (C) 2014-2021 Assaf Gordon <assafgordon@gmail.com>
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
## This script tests the sort piping code for errors
##

. "${test_dir=.}/init.sh"; path_prepend_ ./src

fail=0

require_paste_

## Ensure seq is useable
openbsd_seq_replacement_
seq 10 >/dev/null 2>/dev/null ||
  skip_ "requires a working seq"

## Cross-Compiling portability hack:
##  under qemu/binfmt, argv[0] (which is used to report errors) will contain
##  the full path of the binary, if the binary is on the $PATH.
##  So we try to detect what is the actual returned value of the program
##  in case of an error.
PROG_ARGV0=$(datamash --foobar 2>&1 | head -n 1 | cut -f1 -d:)
[ -z "$PROG_ARGV0" ] && PROG_ARGV0="datamash"

##
##
## Test preparations
##
##
GROUPPARAM=$(seq 1000 2000 | paste -d "," -s -) ||
  framework_failure_ "failed to construct too-long group parameter"

## The expected error message when 'sort' is not found
printf 'sh: sort: not found\ndatamash: read error (on close)' > exp_err2 ||
  framework_failure_ "failed to create exp_err2"

##
## Create a bad 'sort' executable, to simulate failed pipe/popen
##
BADDIR1=$(mktemp -d bad_sort.XXXXXX) ||
  framework_failure_ "Failed to create temp directory for bad-sort"
printf "#!/foo/bar/bad/interpreter" > "$BADDIR1/sort" ||
  framework_failure_ "Failed to create bad-sort: $BADDIR1/sort"
chmod a+x "$BADDIR1/sort" ||
  framework_failure_ "failed to make bad-sort executable"
ORIGPATH=$PATH

## The directory where the "datamash' executable is
DATAMASHDIR=$(dirname $(which datamash))
test -z "$DATAMASHDIR" &&
  framework_failure_ "failed to find datamash's directory"

## Create a 'sort' which will crash
BADDIR=$(mktemp -d badsort.XXXXXX) ||
  framework_failure_ "failed to create bad-sort-dir"
echo '#!/bin/sh
read A
echo "$A"
read B
echo "$B"
Z=0
C=$((1/$Z))
' > "$BADDIR/sort" || framework_failure_ "failed to create $BADDIR/sort"
chmod a+x "$BADDIR/sort" ||
  framework_failure_ "failed to make $BADDIR/sort executable"


##
## Tests start here
##

##
## Test with non-existing 'sort' executable, by giving an invalid path
##
## NOTE: This run SHOULD return an error, hence the "&&" instead of "||"
##
seq 10 | datamash --sort --sort-cmd=/not/a/sort -g 1 sum 1 &&
  { warn_ "datamash --sort with non existing 'sort' did not fail " \
          "(it should have failed)" ; fail=1 ; }

##
## Test with a 'sort' that crashes
## NOTE: This run SHOULD return an error, hence the "&&" instead of "||"
##
seq 10 | datamash --sort --sort-cmd="${BADDIR}/sort" -g 1 sum 1 &&
  { warn_ "datamash --sort with crashing 'sort' did not fail " \
          "(it should have failed)" ; fail=1 ; }

Exit $fail

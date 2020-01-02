#!/bin/sh

#   Unit Tests for GNU Datamash - perform simple calculation on input data

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
#    along with GNU Datamash  If not, see <https://www.gnu.org/licenses/>.
#
#    Written by Assaf Gordon.

. "${test_dir=.}/init.sh"; path_prepend_ ./src

expensive_
require_valgrind_

## Don't use valgrind on statically-compiled binary
## (it gives some false-positives and the test fails).
if which ldd >/dev/null ; then
  ## Tricky implicit assumption:
  ## If the system has "ldd" - we can test if this is a static binary.
  ## If the system doesn't have "ldd", we can't test it, and we'll assume
  ## we can valgrind without false-positives.
  ## This is relevant for Mac OS X, where static binaries are discouraged and
  ## difficult to create
  ## (https://developer.apple.com/library/mac/qa/qa1118/_index.html)
  ldd $(which datamash) >/dev/null 2>/dev/null ||
    skip_ "skipping valgrind test for a non-dynamic-binary datamash"
fi


## Prepare file with long fields - for first/last operations.
( printf "A " ; seq 100 | paste -s -d "" ;
  printf "A " ; seq 1000 | paste -s -d "" ;
  printf "A " ; seq 2000 | paste -s -d "" ;
  printf "A " ; seq 3000 | paste -s -d "" ;
  printf "A 1\n" ;
  printf "B " ; seq 1000 | paste -s -d "" ;
  printf "B " ; seq 2000 | paste -s -d "" ;
  printf "B " ; seq 3000 | paste -s -d "" ;
  printf "B " ; seq 100 | paste -s -d "" ;
) > in_first || framework_failure_ "failed to prepare 'in_first' file"


## Prepare file with many rows, to test transpose/reverse
seq -w 16000 | paste - - - - > in_4k_rows ||
  framework_failure_ "failed to prepare 'in_4k_rows' file"
seq -w 16000 | paste - - - - - - - - > in_2k_rows ||
  framework_failure_ "failed to prepare 'in_2k_rows' file"

## Prepare file with many columns and 100 rows, to test transpose/reverse
( for i in $(seq 0 99) ; do
    seq $((i*1000)) $((i*1000+999)) | paste -s
  done
) > in_1k_cols || framework_failure_ "failed to prepare 'in_1k_cols' file"

## Prepare large file
seq -w 1000000 > 1M || framework_failure_ "failed to prepare '1M' file"
( shuf 1M ; shuf 1M ;
  shuf 1M ; shuf 1M ;
  seq -w 999000 1001000 ) > 4M ||
    framework_failure_ "failed to prepare '4M' file"
seq -w 1001000 > 1M1K || framework_failure_ "failed to prepare '1M1K' file"

## Prepare a file with very wide fields
for i in $(seq 1 193 2000) $(seq 500 -7 100);
do
  seq $i | paste -s -d "" - ;
done > wide ||
    framework_failure_ "failed to prepare 'wide' file"

fail=0

seq 10000 | valgrind --track-origins=yes  --show-reachable=yes \
                     --leak-check=full  --error-exitcode=1 \
                 datamash unique 1 > /dev/null ||
  { warn_ "unique 1 - failed" ; fail=1 ; }

seq 10000 | sed 's/^/group /' |
     valgrind --track-origins=yes  --leak-check=full \
              --show-reachable=yes  --error-exitcode=1 \
                 datamash -g 1 unique 1 > /dev/null ||
  { warn_ "-g 1 unique 1 - failed" ; fail=1 ; }

seq 10000 | valgrind --track-origins=yes  --leak-check=full \
                     --show-reachable=yes  --error-exitcode=1 \
                 datamash countunique 1 > /dev/null ||
  { warn_ "countunique 1 - failed" ; fail=1 ; }

seq 10000 | valgrind --track-origins=yes  --leak-check=full  \
                     --show-reachable=yes  --error-exitcode=1 \
                 datamash collapse 1 > /dev/null ||
  { warn_ "collapse 1 - failed" ; fail=1 ; }

(echo "values" ; seq 10000 ) |
  valgrind --track-origins=yes  --leak-check=full \
           --show-reachable=yes  --error-exitcode=1 \
            datamash -H countunique 1 > /dev/null ||
    { warn_ "-H collapse 1 - failed" ; fail=1 ; }

(echo "values" ; seq 10000 ) |
  valgrind --track-origins=yes  --leak-check=full \
           --show-reachable=yes  --error-exitcode=1 \
           datamash -g 1 -H countunique 1 > /dev/null ||
    { warn_ "-g 1 -H collapse 1 - failed" ; fail=1 ; }

cat "in_first" | valgrind --track-origins=yes  --leak-check=full \
                          --show-reachable=yes  --error-exitcode=1 \
                 datamash -W first 2 > /dev/null ||
  { warn_ "first 2 - failed" ; fail=1 ; }

cat "in_first" | valgrind --track-origins=yes  --leak-check=full \
                          --show-reachable=yes  --error-exitcode=1 \
                 datamash -W -g 1 first 2 > /dev/null ||
  { warn_ "-g 1 first 2 - failed" ; fail=1 ; }

cat "in_first" | valgrind --track-origins=yes  --leak-check=full \
                          --show-reachable=yes  --error-exitcode=1 \
                 datamash -W -g 1 last 2 > /dev/null ||
  { warn_ "-g 1 last 2 - failed" ; fail=1 ; }

## Test transpose and reverse on multiple (medium-sized) inputs
for INFILE in in_4k_rows in_2k_rows in_1k_cols ;
do
  for CMD in transpose reverse ;
  do
    cat "$INFILE" | valgrind --track-origins=yes  --leak-check=full \
                             --show-reachable=yes  --error-exitcode=1 \
                       datamash "$CMD" > /dev/null ||
  { warn_ "$CMD failed on $INFILE" ; fail=1 ; }

  done
done

## Test remove-duplicates (using gnulib's hash)
cat 4M | valgrind --track-origins=yes  --leak-check=full \
                  --show-reachable=yes  --error-exitcode=1 \
                  datamash rmdup 1 > rmdup_1M_1.t ||
  { warn_ "rmdup failed on 4M" ; fail=1 ; }
sort < rmdup_1M_1.t > rmdup_1M_1 \
    || framework_failure_ "failed to sort rmdup_1M_1.t"
cmp rmdup_1M_1 1M1K ||
  { warn_ "rmdup failed on 4M (output differences)" ;
    fail=1 ; }

## Test remove-duplicates (using gnulib's hash),
## with smaller memory buffers
cat 4M | valgrind --track-origins=yes  --leak-check=full \
                  --show-reachable=yes  --error-exitcode=1 \
                  datamash ---rmdup-test rmdup 1 > rmdup_1M_2.t ||
  { warn_ "rmdup failed on 4M (2)" ; fail=1 ; }
sort < rmdup_1M_2.t > rmdup_1M_2 \
    || framework_failure_ "failed to sort rmdup_1M_2.t"
cmp rmdup_1M_2 1M1K ||
  { warn_ "rmdup (2) failed on 4M (output differences)" ;
    fail=1 ; }

## Test Base64 encode/decode
cat wide | valgrind --track-origins=yes  --leak-check=full \
                  --show-reachable=yes  --error-exitcode=1 \
                  datamash base64 1 > wide_base64 ||
  { warn_ "base64 failed on wide" ; fail=1 ; }
cat wide_base64 | valgrind --track-origins=yes  --leak-check=full \
                           --show-reachable=yes  --error-exitcode=1 \
                       datamash debase64 1 > wide_orig ||
  { warn_ "debase64 failed on wide_base64" ; fail=1 ; }

## Test Covariance (and paired-columns)
cat in_4k_rows | valgrind --track-origins=yes  --leak-check=full \
                          --show-reachable=yes  --error-exitcode=1 \
                  datamash pcov 1:2 > /dev/null ||
  { warn_ "pcov 1:2 failed on in_4k_rows" ; fail=1 ; }

cmp wide wide_orig ||
  { warn_ "base64 decoding failed (decoded output does not match original)";
    fail=1 ; }

## Test large output formats
cat wide | valgrind --track-origins=yes  --leak-check=full \
                  --show-reachable=yes  --error-exitcode=1 \
                  datamash --format "%05000.5000f" sum 1 > /dev/null ||
  { warn_ "custom-format failed" ; fail=1 ; }

Exit $fail

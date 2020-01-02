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
#    along with GNU Datamash.  If not, see <https://www.gnu.org/licenses/>.
#
#    Written by Assaf Gordon

##
## This script tests the randomness of the 'rand' operation
##

. "${test_dir=.}/init.sh"; path_prepend_ ./src

fail=0

require_paste_

## Ensure seq is useable
## (not installed on OpenBSD by default)
seq 10 >/dev/null 2>/dev/null ||
    skip_ "requires a working seq"


##
## --- First test ---
##
##    select a random number between 0 and 9,
##    repeat selection for 1000 times.
##    Each digit should be returned at least once
##    (unless we're extremely unlucky...)

INPUT=$(seq 0 9) || framework_failure_ "generating INPUT failed"

for i in $(seq 1000) ;
do
  echo "$INPUT" | datamash rand 1
done > out_rand1 || framework_failure_ "test1 failed: datamash error"

## First Check: each number should be there once
RESULT=$(cat out_rand1 | sort -n | uniq | paste -d , -s -) ||
    framework_failure_ "test1 failed: error preparing first check"

[ "$RESULT" = "0,1,2,3,4,5,6,7,8,9" ] ||
    { warn_ "test1 failed. RESULT='$RESULT'." ; fail=1 ; }


## Second check - we expect (hope?) the distribution is uniform,
##                and each number appears more-or-less equaly.
## This is a poor-man's way of quasi-validation...
## Using 'datamash', count how many times each number appears,
## then, find the smallest count - in a uniform distribution,
## we expect each number to appear close to 100 times (1000 draws of 10 items).
##
## NOTE:
##  We use 'datamash' to validate itself... but only after assuming the
##  basic operations (sort, group, count, min) have been already tested.
RESULT=$(cat out_rand1 |
             datamash --sort --group 1 count 1 |
             datamash min 2) ||
    framework_failure_ "test1 failed: error preparing second check"

## We set the cut-off at 60 - if any number appeared less than 60 times,
## we *might* have a problem in the uniform randomness in 'datamash'.
if [ "$RESULT" -lt "60" ] ; then
  warn_ "Possible unifority problem in 'rand' operation."
  echo "--- distribution of numbers ---"
  cat out_rand1 | datamash --sort --group 1 count 1
  echo "--- end ---"
  echo "--- 1000 random draws start here---"
  cat out_rand1
  echo "---- end ----"
  fail=1
fi


Exit $fail

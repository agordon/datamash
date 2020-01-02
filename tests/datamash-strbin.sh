#!/bin/sh
#   Unit Tests for GNU Datamash - perform simple calculation on input data

#    Copyright (C) 2015-2020 Assaf Gordon <assafgordon@gmail.com>
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
## This script tests the strbin (string binning/hashing) operator
##

. "${test_dir=.}/init.sh"; path_prepend_ ./src

fail=0

## Ensure seq,awk are useable
seq 10 >/dev/null 2>/dev/null \
    || skip_ "requires a working seq"


# Generate input
seq 1000 | sed 's/^/id-/' > in \
    || framework_failure_ "generating INPUT failed"

# bin into 10 groups
datamash strbin 1 < in > out1 \
    || { warn_ "'datamash strbin 1' failed" ; fail=1 ; }

# Check output values
sort -n -u < out1 > out2 || framework_failure_ "failed to sort out1"


# Default binning to 10 bins, accept only single digits
grep -q '^[^0-9]$' < out2 \
    &&  { warn_ "'datamash strbin 1' generated invalid output (out2):" ;
         cat out2 >&2 ;
         fail=1 ; }

# Test binning into varying number of bins
for i in 5 10 100 300 ;
do
    datamash strbin:$i 1 < in > out-$i \
	|| { warn_ "'datamash strbin:$i 1' failed" ; fail=1 ; break ; }

    # Check output values
    max=$(sort -n -u -r < out-$i | head -n1)

    test -n "$max" \
	|| { warn_ "'datamash strbin:$i 1' failed - max output is empty" ;
	     fail=1 ;
	     break ; }

    test "$max" -gt 0 \
	|| { warn_ "'datamash strbin:$i 1' failed - max value too small ($max)";
	     fail=1 ;
	     break ; }

    test "$max" -lt "$i" \
	|| { warn_ "'datamash strbin:$i 1' failed - max value too large ($max)";
	     fail=1 ;
	     break ; }
done


# Same srting must result in the same bin,
# in the same run and in different runs.
# (the returned value, however, is machine-dependant)

text="hello-42-world"
for i in 5 10 100 300 ;
do
    bin1=$(printf "%s\n%s\n%s\n" "$text" "$text" "$text" \
               | datamash strbin:$i 1 | uniq)
    bin2=$(printf "%s\n" "$text" \
               | datamash strbin:$i 1 | uniq)

    test -n "$bin1" \
	|| { warn_ "'datamash strbin:$i 1' failed on text '$text' - empty";
	     fail=1 ;
	     break ; }

    test "x$bin1" = "x$bin2" \
	|| { warn_ "'datamash strbin:$i 1' failed on text '$text' - " \
                   "bin1 ($bin1) doesn't match bin2 ($bin2)" ;
	     fail=1 ;
	     break ; }
done


Exit $fail

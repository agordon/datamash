#!/bin/sh
#   Unit Tests for calc - perform simple calculation on input data
#   Copyright (C) 2014 Assaf Gordon.
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#    Written by Assaf Gordon

. "${test_dir=.}/init.sh"; path_prepend_ ./src

## Ensure this is a recent enough SED
SEDVER=$(sed --version 2>/dev/null | head -n 1 | grep GNU | cut -f4 -d" ")
[ -z "$SEDVER" ] && skip_ "requires GNU sed"

## Ensure this sed's version is 4.2.2 or higher.
## (if it's higher, it will be sorted AFTER our '4.2.2' line)
MINVER=$(printf "$SEDVER\n4\.2\.2\n" | sed 's/\./ /g' | LC_ALL=C sort -k1n,1 -k2n,2 -k3n,3 | head -n 1)
[ "$MINVER" = "4 2 2" ] || skip_ "requires GNU sed version >= 4.2.2 (found version $SEDVER)"


fail=0

# An unsorted input with a header line
INFILE="x y z
A % 1
B ( 2
A & 3
B = 4"

# The expected output with different option combinations
echo "x z
A 1
B 2
A 3
B 4" > exp_no_sort_no_header || framework_failure_ "failed to write exp_no_sort_no_header file"

echo "A 1
B 2
A 3
B 4" > exp_no_sort_in_header || framework_failure_ "failed to write exp_no_sort_in_header file"

echo "A 1,3
B 2,4" > exp_sort_in_header || framework_failure_ "failed to write exp_sort_in_header file"

echo "GroupBy(x) unique(z)
A 1
B 2
A 3
B 4" > exp_no_sort_headers || framework_failure_ "failed to write exp_no_sort_headers file"


echo "GroupBy(x) unique(z)
A 1,3
B 2,4" > exp_sort_headers || framework_failure_ "failed to write exp_sort_headers file"


echo "$INFILE" | calc -g 1 unique 3 > out1 || framework_failure_ "calc failed"
compare_ out1 exp_no_sort_no_header || { warn_ "no-sort-no-header failed" ; fail=1 ; }

echo "$INFILE" | calc -g 1 --header-in unique 3 > out2 || framework_failure_ "calc failed"
compare_ out2 exp_no_sort_in_header || { warn_ "no-sort-in-header failed" ; fail=1 ; }

echo "$INFILE" | calc -g 1 --sort --header-in unique 3 > out3 || framework_failure_ "calc failed"
compare_ out3 exp_sort_in_header || { warn_ "sort-in-header failed" ; fail=1 ; }

echo "$INFILE" | calc -g 1 --headers unique 3 > out4 || framework_failure_ "calc failed"
compare_ out4 exp_no_sort_headers || { warn_ "no-sort-headers failed" ; fail=1 ; }

echo "$INFILE" | calc -g 1 --sort --headers unique 3 > out5 || framework_failure_ "calc failed"
compare_ out5 exp_sort_headers || { warn_ "sort-headers failed" ; fail=1 ; }

echo "$INFILE" | calc -sH -g 1 unique 3 > out6 || framework_failure_ "calc failed"
compare_ out5 out6 || { warn_ "sort-headers (short options) failed" ; fail=1 ; }

Exit $fail

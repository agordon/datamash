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


echo "A % 1 1,3
B ( 2 2,4" > exp_sort_in_header_full || framework_failure_ "failed to write exp_sort_in_header_full file"

echo "GroupBy(x) unique(z)
A 1,3
B 2,4" > exp_sort_headers || framework_failure_ "failed to write exp_sort_headers file"

echo "GroupBy(field-1) unique(field-3)
A 1,3
B 2,4" > exp_sort_out_header || framework_failure_ "failed to write exp_sort_out_header"

echo "field-1 field-2 field-3 unique(field-3)
A % 1 1,3
B ( 2 2,4" > exp_sort_out_header_full || framework_failure_ "failed to write exp_sort_out_header_full"

echo "x y z unique(z)
A % 1 1,3
B ( 2 2,4" > exp_sort_headers_full || framework_failure_ "failed to write exp_sort_headers_full"


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

echo "$INFILE" | sed 1d | calc --sort --header-out -g 1 unique 3 > out7 || framework_failure_ "calc failed"
compare_ out7 exp_sort_out_header || { warn_ "sort-header-out failed" ; fail=1 ; }

echo "$INFILE" | sed 1d | calc --sort --full --header-out -g 1 unique 3 > out8 || framework_failure_ "calc failed"
compare_ out8 exp_sort_out_header_full || { warn_ "sort-header-out-full failed" ; fail=1 ; }

echo "$INFILE" | calc -g 1 --sort --full --header-in unique 3 > out9 || framework_failure_ "calc failed"
compare_ out9 exp_sort_in_header_full || { warn_ "sort-in-header-full failed" ; fail=1 ; }

echo "$INFILE" | calc -g 1 --sort --full --headers unique 3 > out10 || framework_failure_ "calc failed"
compare_ out10 exp_sort_headers_full || { warn_ "sort-headers-full failed" ; fail=1 ; }


## Check sort-piping with empty input - should always produce empty output
printf "" | calc --sort unique 3 > emp1 || framework_failure_ "calc failed"
test -s "emp1" && { warn_ "sort on empty file failed" ; fail=1; }

printf "" | calc --sort --header-in unique 3 > emp2 || framework_failure_ "calc failed"
test -s "emp2" && { warn_ "sort+header-in on empty file failed" ; fail=1; }

printf "" | calc --sort --header-out unique 3 > emp3 || framework_failure_ "calc failed"
test -s "emp3" && { warn_ "sort+header-out on empty file failed" ; fail=1; }

printf "" | calc --sort --headers unique 3 > emp4 || framework_failure_ "calc failed"
test -s "emp4" && { warn_ "sort+headers on empty file failed" ; fail=1; }

printf "" | calc --sort --full unique 3 > emp5 || framework_failure_ "calc failed"
test -s "emp5" && { warn_ "sort+full on empty file failed" ; fail=1; }

Exit $fail

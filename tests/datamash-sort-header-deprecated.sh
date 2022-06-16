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

. "${test_dir=.}/init.sh"; path_prepend_ ./src

fail=0

# An unsorted input with a header line
INFILE="x y z
A % 1
B ( 2
A & 3
B = 4"

echo "A % 1 1,3
B ( 2 2,4" > exp_sort_in_header_full ||
  framework_failure_ "failed to write exp_sort_in_header_full file"

echo "field-1 field-2 field-3 unique(field-3)
A % 1 1,3
B ( 2 2,4" > exp_sort_out_header_full ||
  framework_failure_ "failed to write exp_sort_out_header_full"

echo "x y z unique(z)
A % 1 1,3
B ( 2 2,4" > exp_sort_headers_full ||
  framework_failure_ "failed to write exp_sort_headers_full"


cat > full_deprecation.txt << EOF
datamash: Using -f/--full with non-linewise operations is deprecated and will be disabled in a future release.
EOF

echo "$INFILE" | sed 1d |
  datamash -t ' ' --sort --full --header-out -g 1 unique 3 2>out8.err > out8 ||
    framework_failure_ "datamash failed"
compare_ out8 exp_sort_out_header_full ||
  { warn_ "sort-header-out-full failed" ; fail=1 ; }
compare_ out8.err full_deprecation.txt ||
  { warn_ "sort-header-out-full failed" ; fail=1 ; }

echo "$INFILE" |
  datamash -t ' ' -g 1 --sort --full --header-in unique 3 2>out9.err > out9 ||
    framework_failure_ "datamash failed"
compare_ out9 exp_sort_in_header_full ||
  { warn_ "sort-in-header-full failed" ; fail=1 ; }
compare_ out9.err full_deprecation.txt ||
  { warn_ "sort-in-header-full failed" ; fail=1 ; }

echo "$INFILE" |
  datamash -t ' ' -g 1 --sort --full --headers unique 3 2>out10.err > out10 ||
    framework_failure_ "datamash failed"
compare_ out10 exp_sort_headers_full ||
  { warn_ "sort-headers-full failed" ; fail=1 ; }
compare_ out10.err full_deprecation.txt ||
  { warn_ "sort-headers-full failed" ; fail=1 ; }

## Check sort-piping with empty input - should always produce empty output
printf "" | datamash -t ' ' --sort --full unique 3 > emp5 ||
  framework_failure_ "datamash failed"
compare_ /dev/null "emp5" ||
  { warn_ "sort+full on empty file failed" ; fail=1; }

Exit $fail

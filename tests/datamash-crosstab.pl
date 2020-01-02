#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data
  Tests for 'transpose' and 'reverse' operation modes.


   Copyright (C) 2013-2020 Assaf Gordon <assafgordon@gmail.com>

   This file is part of GNU Datamash.

   GNU Datamash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   GNU Datamash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Datamash.  If not, see <https://www.gnu.org/licenses/>.

   Written by Assaf Gordon.
=cut
use strict;
use warnings;
use List::Util qw/max/;
use Data::Dumper;

# Until a better way comes along to auto-use Coreutils Perl modules
# as in the coreutils' autotools system.
use Coreutils;
use CuSkip;
use CuTmpdir qw(datamash);

(my $program_name = $0) =~ s|.*/||;
my $prog_bin = 'datamash';

my $prog = `$prog_bin ---print-progname`;

# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

my $in1=<<'EOF';
a	x	1
a	y	2
a	z	3
b	x	4
b	y	5
b	z	6
c	x	7
c	y	8
c	z	9
EOF

my $out1_first=<<'EOF';
	x	y	z
a	1	2	3
b	4	5	6
c	7	8	9
EOF

my $out1_count=<<'EOF';
	x	y	z
a	1	1	1
b	1	1	1
c	1	1	1
EOF

#unsorted input with duplicates
my $in2=<<'EOF';
a	x	1
a	y	2
a	x	3
EOF

# when using 'first' operation
my $out2_first=<<'EOF';
	x	y
a	1	2
EOF

# when using 'last' operation without sorting the data
# this output is considered incorrect...
# TODO: perhaps warn the user about it (like join warns of unsorted input).
my $out2_last_unsorted=<<'EOF';
	x	y
a	1	2
EOF

# when using 'last' operation with sorting the data,
# correct value is shown
my $out2_last_sorted=<<'EOF';
	x	y
a	3	2
EOF

my $out2_count_unsorted=<<'EOF';
	x	y
a	1	1
EOF

my $out2_count_sorted=<<'EOF';
	x	y
a	2	1
EOF

# using 'sum' without sorting the data.
# this output is considered incorrect...
# TODO: perhaps warn the user about it (like join warns of unsorted input).
my $out2_sum_unsorted=<<'EOF';
	x	y
a	1	2
EOF

# using 'sum' with sorting the data
my $out2_sum_sorted=<<'EOF';
	x	y
a	4	2
EOF

#input with missing values (b/y is missing)
my $in3=<<'EOF';
a	x	1
a	y	2
b	x	3
EOF

# default filler is 'N/A'
my $out3_na=<<'EOF';
	x	y
a	1	2
b	3	N/A
EOF

# custom filler 'XX'
my $out3_xx=<<'EOF';
	x	y
a	1	2
b	3	XX
EOF

my @Tests =
(
  ['c1','crosstab 1,2 first 3', {IN_PIPE=>$in1}, {OUT=>$out1_first}],
  ['c2','ct 1,2 first 3',       {IN_PIPE=>$in1}, {OUT=>$out1_first}],
  ['c3','ct 1,2 count 1',       {IN_PIPE=>$in1}, {OUT=>$out1_count}],

  # Default operation is count
  ['c4','ct 1,2',               {IN_PIPE=>$in1}, {OUT=>$out1_count}],

  # test unsorted input with duplicates
  ['c10','ct 1,2 first 3',       {IN_PIPE=>$in2}, {OUT=>$out2_first}],

  ['c11','   ct 1,2 last 3',     {IN_PIPE=>$in2}, {OUT=>$out2_last_unsorted}],
  ['c12','-s ct 1,2 last 3',     {IN_PIPE=>$in2}, {OUT=>$out2_last_sorted}],

  ['c13','   ct 1,2 sum 3',      {IN_PIPE=>$in2}, {OUT=>$out2_sum_unsorted}],
  ['c14','-s ct 1,2 sum 3',      {IN_PIPE=>$in2}, {OUT=>$out2_sum_sorted}],

  # test default operation (count) on unsorted data
  ['c15','   ct 1,2 count 3',    {IN_PIPE=>$in2}, {OUT=>$out2_count_unsorted}],
  ['c16','-s ct 1,2 count 3',    {IN_PIPE=>$in2}, {OUT=>$out2_count_sorted}],

  # Test missing values
  ['c30','ct 1,2 first 3',       {IN_PIPE=>$in3}, {OUT=>$out3_na}],
  ['c31','--filler XX ct 1,2 first 3',       {IN_PIPE=>$in3}, {OUT=>$out3_xx}],

  # Test wrong usage
  ['e1',  'ct',  {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing field for operation 'crosstab'\n"}],
  ['e2',  'ct 1',  {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: crosstab requires exactly 2 fields, found 1\n"}],
  ['e3',  'ct 1,2,3,4',  {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: crosstab requires exactly 2 fields, found 4\n"}],
  ['e4',  'ct 1,2 md5 4', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: conflicting operation found: expecting crosstab " .
            "operations, but found line operation 'md5'\n"}],
  ['e5',  'ct 1,2 sum 1,2', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: crosstab supports one operation, found 2\n"}],
  ['e6',  'ct 1,2 min 2 max 2', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: crosstab supports one operation, found 2\n"}],
  ['e7',  'ct 1:2', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field pair for operation 'crosstab'\n"}],
  ['e8',  'ct 1-2', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field range for operation 'crosstab'\n"}],
);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog_bin, \@Tests, $save_temps, $verbose);
exit $fail;

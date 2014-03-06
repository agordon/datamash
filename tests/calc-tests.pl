#!/usr/bin/perl
=pod
  Unit Tests for calc - perform simple calculation on input data
  Copyright (C) 2013 Assaf Gordon.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
   Written by Assaf Gordon
=cut
use strict;
use warnings;

# Until a better way comes along to auto-use Coreutils Perl modules
# as in the coreutils' autotools system.
use Coreutils;
use CuSkip;
use CuTmpdir qw(calc);

(my $program_name = $0) =~ s|.*/||;
my $prog = 'calc';

# TODO: add localization tests with "grouping"
# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

# note: '5' appears twice
my $in1 = join("\n", qw/1 2 3 4 5 6 7 5 8 9 10/) . "\n";

# Mix of whitespace and tabs
my $in2 = "1  2\t  3\n" .
          "4\t5 6\n";
my $in_minmax = join("\n", qw/5 90 -7e2 3 200 0.1e-3 42/) . "\n";

my $in_g1=<<'EOF';
A 100
A 10
A 50
A 35
EOF

my $in_g2=<<'EOF';
A 100
A 10
A 50
A 35
B 66
B 77
B 55
EOF

my $in_g3=<<'EOF';
A 3  W
A 5  W
A 7  W
A 11 X
A 13 X
B 17 Y
B 19 Z
C 23 Z
EOF

my $in_g4=<<'EOF';
A 5
K 6
P 2
EOF

my $in_hdr1=<<'EOF';
x y z
A 1 10
A 2 10
A 3 10
A 4 10
A 4 10
B 5 10
B 6 20
B 7 30
C 8 11
C 9 22
C 1 33
C 2 44
EOF

my $in_cnt_uniq1=<<'EOF';
x y
A 1
A 2
A 1
A 2
A 1
A 2
B 1
B 2
B 1
B 2
B 1
B 2
EOF

# When using whitespace, the second column is 1,2,3.
# When using Tab, the second column is 10,20,30.
my $in_tab1=<<"EOF";
A 1\t10
B 2\t20
C 3\t30
EOF



my @Tests =
(
  # Basic tests, single field, single group, default everything
  ['b1', 'count 1' ,    {IN_PIPE=>$in1},  {OUT => "11\n"}],
  ['b2', 'sum 1',       {IN_PIPE=>$in1},  {OUT => "60\n"}],
  ['b3', 'min 1',       {IN_PIPE=>$in1},  {OUT => "1\n"}],
  ['b4', 'max 1',       {IN_PIPE=>$in1},  {OUT => "10\n"}],
  ['b5', 'absmin 1',    {IN_PIPE=>$in1},  {OUT => "1\n"}],
  ['b6', 'absmax 1',    {IN_PIPE=>$in1},  {OUT => "10\n"}],
  ['b8', 'median 1',    {IN_PIPE=>$in1},  {OUT => "5\n"}],
  ['b9', 'mode 1',      {IN_PIPE=>$in1},  {OUT => "5\n"}],
  ['b10', 'antimode 1', {IN_PIPE=>$in1},  {OUT => "1\n"}],
  ['b11', 'unique 1',   {IN_PIPE=>$in1},  {OUT => "1,10,2,3,4,5,6,7,8,9\n"}],
  ['b12', 'uniquenc 1', {IN_PIPE=>$in1},  {OUT => "1,10,2,3,4,5,6,7,8,9\n"}],
  ['b13', 'collapse 1', {IN_PIPE=>$in1},  {OUT => "1,2,3,4,5,6,7,5,8,9,10\n"}],

  # on a different architecture, would printf(%Lg) print something else?
  # Use OUT_SUBST to trim output to 1.3 digits
  ['b14', 'mean 1',     {IN_PIPE=>$in1},  {OUT => "5.454\n"},
	  {OUT_SUBST=>'s/^(\d\.\d{3}).*/\1/'}],
  ['b15', 'pstdev 1',   {IN_PIPE=>$in1},  {OUT => "2.742\n"},
	  {OUT_SUBST=>'s/^(\d\.\d{3}).*/\1/'}],
  ['b16', 'sstdev 1',   {IN_PIPE=>$in1},  {OUT => "2.876\n"},
	  {OUT_SUBST=>'s/^(\d\.\d{3}).*/\1/'}],
  ['b17', 'pvar 1',     {IN_PIPE=>$in1},  {OUT => "7.520\n"},
	  {OUT_SUBST=>'s/^(\d\.\d{3}).*/\1/'}],
  ['b18', 'svar 1',     {IN_PIPE=>$in1},  {OUT => "8.272\n"},
	  {OUT_SUBST=>'s/^(\d\.\d{3}).*/\1/'}],
  ['b19', 'countunique 1', {IN_PIPE=>$in1}, {OUT => "10\n"}],



  ## Some error checkings
  ['e1',  'sum',  {IN_PIPE=>""}, {EXIT=>1},
	  {ERR=>"$prog: missing field number after operation 'sum'\n"}],
  ['e2',  'foobar',  {IN_PIPE=>""}, {EXIT=>1},
	  {ERR=>"$prog: invalid operation 'foobar'\n"}],
  ['e3',  '',  {IN_PIPE=>""}, {EXIT=>1},
	  {ERR=>"$prog: missing operation specifiers\n" .
		  "Try '$prog --help' for more information.\n"}],
  ['e4',  'sum 1' ,  {IN_PIPE=>"a\n"}, {EXIT=>1},
	  {ERR=>"$prog: invalid numeric input in line 1 field 1: 'a'\n"}],

  # No newline at the end of the lines
  ['nl1', 'sum 1', {IN_PIPE=>"99"}, {OUT=>"99\n"}],
  ['nl2', 'sum 1', {IN_PIPE=>"1\n99"}, {OUT=>"100\n"}],

  # empty input = empty output, regardless of options
  [ 'emp1', 'count 1', {IN_PIPE=>""}, {OUT=>""}],
  [ 'emp2', '--full count 2', {IN_PIPE=>""},{OUT=>""}],
  [ 'emp3', '--header-in count 2', {IN_PIPE=>""},{OUT=>""}],
  [ 'emp4', '--header-out count 2', {IN_PIPE=>""},{OUT=>""}],
  [ 'emp5', '--full --header-in count 2', {IN_PIPE=>""},{OUT=>""}],
  [ 'emp6', '--full --header-out count 2', {IN_PIPE=>""},{OUT=>""}],
  [ 'emp7', '--full --header-in --header-out count 2', {IN_PIPE=>""},{OUT=>""}],
  [ 'emp8', '-g3,4 --full --header-in --header-out count 2', {IN_PIPE=>""},{OUT=>""}],
  [ 'emp9', '-g3 count 2', {IN_PIPE=>""},{OUT=>""}],


  ## Field extraction
  ['f1', 'sum 1', {IN_PIPE=>$in2}, {OUT=>"5\n"}],
  ['f2', 'sum 2', {IN_PIPE=>$in2}, {OUT=>"7\n"}],
  ['f3', 'sum 3', {IN_PIPE=>$in2}, {OUT=>"9\n"}],
  ['f4', 'sum 3 sum 1', {IN_PIPE=>$in2}, {OUT=>"9 5\n"}],
  ['f5', '-t: sum 4', {IN_PIPE=>"11:12::13:14"}, {OUT=>"13\n"}],
  # collase non-last field (followed by whitespace, not new-line)
  ['f6', 'unique 1', {IN_PIPE=>$in_g2}, {OUT=>"A,B\n"}],

  # Test Absolute min/max
  ['mm1', 'min 1', {IN_PIPE=>$in_minmax}, {OUT=>"-700\n"}],
  ['mm2', 'max 1', {IN_PIPE=>$in_minmax}, {OUT=>"200\n"}],
  ['mm3', 'absmin 1', {IN_PIPE=>$in_minmax}, {OUT=>"0.0001\n"}],
  ['mm4', 'absmax 1', {IN_PIPE=>$in_minmax}, {OUT=>"-700\n"}],

  #
  # Test Grouping
  #

  # Single group (key in column 1)
  ['g1', '-k1,1 sum 2',    {IN_PIPE=>$in_g1}, {OUT=>"A 195\n"}],
  ['g2', '-k1,1 median 2', {IN_PIPE=>$in_g1}, {OUT=>"A 42.5\n"}],
  ['g3', '-k1,1 collapse 2', {IN_PIPE=>$in_g1}, {OUT=>"A 100,10,50,35\n"}],
  # Same as above, with "-g"
  ['g1.1', '-g1 sum 2',    {IN_PIPE=>$in_g1}, {OUT=>"A 195\n"}],
  ['g2.1', '-g1 median 2', {IN_PIPE=>$in_g1}, {OUT=>"A 42.5\n"}],
  ['g3.1', '-g1 collapse 2', {IN_PIPE=>$in_g1}, {OUT=>"A 100,10,50,35\n"}],

  # Two groups (key in column 1)
  ['g4', '-k1,1 min 2',    {IN_PIPE=>$in_g2}, {OUT=>"A 10\nB 55\n"}],
  ['g5', '-k1,1 median 2', {IN_PIPE=>$in_g2}, {OUT=>"A 42.5\nB 66\n"}],
  ['g6', '-k1,1 collapse 2', {IN_PIPE=>$in_g2},
     {OUT=>"A 100,10,50,35\nB 66,77,55\n"}],

  # 3 groups, single line per group, custom delimiter
  ['g7', '-k2,2 -t= mode 1', {IN_PIPE=>"1=A\n2=B\n3=C\n"},
     {OUT=>"A=1\nB=2\nC=3\n"}],
  ['g7.1', '-g2 -t= mode 1', {IN_PIPE=>"1=A\n2=B\n3=C\n"},
     {OUT=>"A=1\nB=2\nC=3\n"}],

  # Multiple keys (from different columns)
  ['g8', '-k1,1 -k3,3 sum 2', {IN_PIPE=>$in_g3},
     {OUT=>"A W 15\nA X 24\nB Y 17\nB Z 19\nC Z 23\n"}],
  ['g8.1',     '-g1,3 sum 2', {IN_PIPE=>$in_g3},
     {OUT=>"A W 15\nA X 24\nB Y 17\nB Z 19\nC Z 23\n"}],


  # --full option - without grouping, returns the first line
  ['fl1', '--full sum 2', {IN_PIPE=>$in_g3},
     {OUT=>"A 3  W 98\n"}],
  # --full with grouping - print entire line of each group
  ['fl2', '--full -g3 sum 2', {IN_PIPE=>$in_g3},
     {OUT=>"A 3  W 15\nA 11 X 24\nB 17 Y 17\nB 19 Z 42\n"}],

  # count on non-numeric fields
  ['cnt1', '-g 1 count 1', {IN_PIPE=>$in_g2},
     {OUT=>"A 4\nB 3\n"}],

  # Input Header
  ['hdr1', '-g 1 --header-in count 2',{IN_PIPE=>$in_hdr1},
     {OUT=>"A 5\nB 3\nC 4\n"}],

  # Input and output header
  ['hdr2', '-g 1 --header-in --header-out count 2',{IN_PIPE=>$in_hdr1},
     {OUT=>"GroupBy(x) count(y)\nA 5\nB 3\nC 4\n"}],

  # Input and output header, with full line
  ['hdr3', '-g 1 --full --header-in --header-out count 2',{IN_PIPE=>$in_hdr1},
     {OUT=>"x y z count(y)\nA 1 10 5\nB 5 10 3\nC 8 11 4\n"}],

  # Output Header
  ['hdr4', '-g 1 --header-out count 2', {IN_PIPE=>$in_g3},
     {OUT=>"GroupBy(field-1) count(field-2)\nA 5\nB 2\nC 1\n"}],

  # Output Header with --full
  ['hdr5', '-g 1 --full --header-out count 2', {IN_PIPE=>$in_g3},
     {OUT=>"field-1 field-2 field-3 count(field-2)\nA 3  W 5\nB 17 Y 2\nC 23 Z 1\n"}],

  # Test single line per group
  ['sl1', '-g 1 mean 2', {IN_PIPE=>$in_g4},
     {OUT=>"A 5\nK 6\nP 2\n"}],
  ['sl2', '--full -g 1 mean 2', {IN_PIPE=>$in_g4},
     {OUT=>"A 5 5\nK 6 6\nP 2 2\n"}],

  # Test countunique operation
  ['cuq1', '-g 1 countunique 3', {IN_PIPE=>$in_g3},
     {OUT=>"A 2\nB 2\nC 1\n"}],
  ['cuq2', '-g 1 countunique 2', {IN_PIPE=>$in_g4},
     {OUT=>"A 1\nK 1\nP 1\n"}],
  ['cuq3', '--header-in -g 1 countunique 2', {IN_PIPE=>$in_cnt_uniq1},
     {OUT=>"A 2\nB 2\n"}],

  # Test Tab vs White-space field separator
  ['tab1', 'sum 2',         {IN_PIPE=>$in_tab1}, {OUT=>"6\n"}],
  ['tab2', "-t '\t' sum 2", {IN_PIPE=>$in_tab1}, {OUT=>"60\n"}],
  ['tab3', '-T sum 2',      {IN_PIPE=>$in_tab1}, {OUT=>"60\n"}],

);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

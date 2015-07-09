#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2015 Assaf Gordon <assafgordon@gmail.com>

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
   along with GNU Datamash.  If not, see <http://www.gnu.org/licenses/>.

   Written by Assaf Gordon.
=cut
use strict;
use warnings;

##
## This is a continuation of 'datamash-tests.pl'
##   split into two files, as it was getting too large.
##

# Until a better way comes along to auto-use Coreutils Perl modules
# as in the coreutils' autotools system.
use Coreutils;
use CuSkip;
use CuTmpdir qw(datamash);
use MIME::Base64 ;

(my $program_name = $0) =~ s|.*/||;
my $prog_bin = 'datamash';

## Cross-Compiling portability hack:
##  under qemu/binfmt, argv[0] (which is used to report errors) will contain
##  the full path of the binary, if the binary is on the $PATH.
##  So we try to detect what is the actual returned value of the program
##  in case of an error.
my $prog = `$prog_bin ---print-progname`;
$prog = $prog_bin unless $prog;

## Portability hack:
## find the exact wording of 'nan' and inf (not-a-number).
## It's lower case in GNU/Linux,FreeBSD,OpenBSD,
## but is "NaN" on Illumos/OpenSolaris
my $nan = `$prog_bin ---print-nan`;
die "test infrastructure failed: can't determine 'nan' string" unless $nan;
my $inf = `$prog_bin ---print-inf`;
die "test infrastructure failed: can't determine 'inf' string" unless $inf;

## Portability hack
## Check if the system's sort supports stable sorting ('-s').
## If it doesn't - skip some tests
my $rc = system("sort -s < /dev/null > /dev/null 2>/dev/null");
die "testing framework failure: failed to execute sort -s"
  if ( ($rc == -1) || ($rc & 127) );
my $sort_exit_code = ($rc >> 8);
my $have_stable_sort = ($sort_exit_code==0);


# TODO: add localization tests with "grouping"
# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;


# Test the selection operation (first/last/min/max) together with "FULL":
# group by column 1 ("A" or "B"), and operate on column 2 (numeric).
# Ensure the matching "iX" is displayed, despite not being part of the
# operation (example: if 'min(2)' is the operation, then "B 8" should be
# selected, and "i7" must be displayed with "-full" (because "i7" is on the
# same line as the min(2) value zero).
my $in_full1=<<'EOF';
A 4 i1
A 3 i2
A 5 i3
B 1 i4
B 8 i5
B 0 i6
B 3 i7
EOF

# Test various combinations of N/A, NaN values
my $na1=<<'EOF';
1
2
NA
3
EOF

my $na2=<<'EOF';
1
2
nA
3
EOF

my $na3=<<'EOF';
1
2
N/A
3
EOF

my $na4=<<'EOF';
1
2
NaN
3
EOF

my $na5=<<'EOF';
1
2
nan
3
EOF

# This is not a valid input, should not be detected as "NA"
my $not_na1=<<'EOF';
1
2
NAK
3
EOF

# This is not a valid input, should not be detected as "NAN"
my $not_na2=<<'EOF';
1
2
NANA
3
EOF

my $na_first=<<'EOF';
NaN
2
5
8
EOF

my $na_last=<<'EOF';
NaN
2
5
11
EOF

my $na_mid1=<<'EOF';
1:2:3
4:NA:6
7:8:9
EOF

my $na_last1=<<'EOF';
1:2:3
4:5:6
7:8:NaN
EOF

my $na_all=<<'EOF';
NA
NA
NaN
EOF

my $bin_in1=<<'EOF';
0
3
7
9
10
11
15
EOF

# binning '$bin_in1' with default bucket-size=100
my $bin_out1_100=<<'EOF';
0
0
0
0
0
0
0
EOF

# binning '$bin_in1' with bucket-size=5
my $bin_out1_5=<<'EOF';
0
0
5
5
10
10
15
EOF

# binning '$bin_in1' with bucket-size=5.5
my $bin_out1_5_5=<<'EOF';
0
0
5.5
5.5
5.5
11
11
EOF

my $bin_in2=<<'EOF';
-3
-0.3
0.3
3
103
EOF

my $bin_out2_100=<<'EOF';
-100
-100
0
0
100
EOF

my $bin_out2_3=<<'EOF';
-6
-3
0
3
102
EOF

my @Tests =
(
  # Test 'min' + --full
  # first, verify test without "--full"
  ['slct1', '-t" " -g1 min 2', {IN_PIPE=>$in_full1}, {OUT=>"A 3\nB 0\n"}],
  # Test with "--full", "i2" and "i6" should be displayed
  ['slct2', '-t" " -f -g1 min 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 3 i2 3\nB 0 i6 0\n"}],
  # --full with --sort => should not change results
  ['slct3', '-s -t" " -f -g1 min 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 3 i2 3\nB 0 i6 0\n"}],

  # Test 'max' + --full
  # first, verify test without "--full"
  ['slct4', '-t" " -g1 max 2', {IN_PIPE=>$in_full1}, {OUT=>"A 5\nB 8\n"}],
  # Test with "--full", "i3" and "i7" should be displayed
  ['slct5', '-t" " -f -g1 max 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 5 i3 5\nB 8 i5 8\n"}],
  # --full with --sort => should not change results
  ['slct6', '-s -t" " -f -g1 max 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 5 i3 5\nB 8 i5 8\n"}],

  # Test 'first' + --full
  # first, verify test without "--full"
  ['slct7', '-t" " -g1 first 2', {IN_PIPE=>$in_full1}, {OUT=>"A 4\nB 1\n"}],
  # Test with "--full", "i1" and "i4" should be displayed
  ['slct8', '-t" " -f -g1 first 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 4 i1 4\nB 1 i4 1\n"}],
  # more --full with --sort => see test 'sortslct1' below

  # Test 'last' + --full
  # first, verify test without "--full"
  ['slct9', '-t" " -g1 last 2', {IN_PIPE=>$in_full1}, {OUT=>"A 5\nB 3\n"}],
  # Test with "--full", "i1" and "i4" should be displayed
  ['slct10', '-t" " -f -g1 last 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 5 i3 5\nB 3 i7 3\n"}],
  # more --full with --sort => see test 'sortslct2' below


  # Test --narm - ignoring NaN/NA values

  ## Test with 'NA'
  ['narm1', '--narm sum 1',  {IN_PIPE=>$na1}, {OUT=>"6\n"}],
  ['narm2', '--narm mean 1', {IN_PIPE=>$na1}, {OUT=>"2\n"}],
  # without --narm, these should fail with invalid input
  ['narm3', 'sum 1',         {IN_PIPE=>$na1}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value in line 3 field 1: 'NA'\n"}],

  ## Test with 'nA'
  ['narm4', '--narm sum 1',  {IN_PIPE=>$na2}, {OUT=>"6\n"}],
  ['narm5', '--narm mean 1', {IN_PIPE=>$na2}, {OUT=>"2\n"}],
  # without --narm, these should fail with invalid input
  ['narm6', 'sum 1',         {IN_PIPE=>$na2}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value in line 3 field 1: 'nA'\n"}],

  ## Test with 'N/A'
  ['narm7', '--narm sum 1',  {IN_PIPE=>$na3}, {OUT=>"6\n"}],
  ['narm8', '--narm mean 1', {IN_PIPE=>$na3}, {OUT=>"2\n"}],
  # without --narm, these should fail with invalid input
  ['narm9', 'sum 1',         {IN_PIPE=>$na3}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value in line 3 field 1: 'N/A'\n"}],

  ## Test with 'NaN'
  ['narm10', '--narm sum 1',  {IN_PIPE=>$na4}, {OUT=>"6\n"}],
  ['narm11', '--narm mean 1', {IN_PIPE=>$na4}, {OUT=>"2\n"}],
  # without --narm, 'nan' should be processed, not skipped
  ['narm12', 'sum 1',         {IN_PIPE=>$na4}, {OUT=>"$nan\n"}],

  ## Test with 'nan'
  ['narm13', '--narm sum 1',  {IN_PIPE=>$na5}, {OUT=>"6\n"}],
  ['narm14', '--narm mean 1', {IN_PIPE=>$na5}, {OUT=>"2\n"}],
  # without --narm, 'nan' should be processed, not skipped
  ['narm15', 'sum 1',         {IN_PIPE=>$na5}, {OUT=>"$nan\n"}],

  # These input have strings starting with 'NA' or 'NAN' but should not
  # be mistaken for them.
  ['narm16', '--narm sum 1',         {IN_PIPE=>$not_na1}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value in line 3 field 1: 'NAK'\n"}],
  ['narm17', '--narm sum 1',         {IN_PIPE=>$not_na2}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value in line 3 field 1: 'NANA'\n"}],

  ## NA value as first/last line
  ['narm18', '--narm mean 1',  {IN_PIPE=>$na_first}, {OUT=>"5\n"}],
  ['narm19', '--narm mean 1', {IN_PIPE=>$na_last}, {OUT=>"6\n"}],

  ## NA in the middle of the line
  ['narm20', '-t: --narm sum 1 sum 2 sum 3', {IN_PIPE=>$na_mid1},
    {OUT=>"12:10:18\n"}],
  ['narm21', '-t:        sum 1 sum 2 sum 3', {IN_PIPE=>$na_mid1}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value in line 2 field 2: 'NA'\n"}],

  ## NA as the last field
  ['narm22', '-t: --narm sum 1 sum 2 sum 3', {IN_PIPE=>$na_mid1},
    {OUT=>"12:10:18\n"}],
  ['narm23', '-t:        sum 1 sum 2 sum 3', {IN_PIPE=>$na_mid1}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value in line 2 field 2: 'NA'\n"}],

  # N/A affect counts
  ['narm24', '-t: count 1 count 2 count 3', {IN_PIPE=>$na_last1},
    {OUT=>"3:3:3\n"}],
  ['narm25', '-t: --narm count 1 count 2 count 3', {IN_PIPE=>$na_last1},
    {OUT=>"3:3:2\n"}],
  ['narm26', '-t: --narm sum 1 sum 2 sum 3', {IN_PIPE=>$na_last1},
    {OUT=>"12:15:9\n"}],

  # Check NA with string operations
  ['narm27', '-t: --narm unique 2', {IN_PIPE=>$na_mid1},
    {OUT=>"2,8\n"}],
  ['narm28', '-t: unique 2', {IN_PIPE=>$na_mid1},
    {OUT=>"2,8,NA\n"}],

  # Data of all NAs
  ['narm29', '--narm count 1', {IN_PIPE=>$na_all}, {OUT=>"0\n"}],
  ['narm30', '       count 1', {IN_PIPE=>$na_all}, {OUT=>"3\n"}],

  ['narm31', '--narm sum 1', {IN_PIPE=>$na_all}, {OUT=>"0\n"}],

  ['narm32', '--narm mean 1', {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],

  # Test all NA input with all operations
  # tested to comply with R code of 'a = C(NA,NA,NA)'
  # Try to be as consistent with R as possible.
  # The tested function is mainly 'field_op_summarize_empty()'
  ['narm40', '--narm count 1',   {IN_PIPE=>$na_all}, {OUT=>"0\n"}],
  ['narm41', '--narm sum 1',     {IN_PIPE=>$na_all}, {OUT=>"0\n"}],
  ['narm42', '--narm min 1',     {IN_PIPE=>$na_all}, {OUT=>"-$inf\n"}],
  ['narm43', '--narm max 1',     {IN_PIPE=>$na_all}, {OUT=>"$inf\n"}],
  ['narm44', '--narm absmin 1',  {IN_PIPE=>$na_all}, {OUT=>"-$inf\n"}],
  ['narm45', '--narm absmax 1',  {IN_PIPE=>$na_all}, {OUT=>"$inf\n"}],
  ['narm46', '--narm first 1',   {IN_PIPE=>$na_all}, {OUT=>"N/A\n"}],
  ['narm47', '--narm last 1',    {IN_PIPE=>$na_all}, {OUT=>"N/A\n"}],
  ['narm48', '--narm rand 1',    {IN_PIPE=>$na_all}, {OUT=>"N/A\n"}],
  ['narm49', '--narm mean 1',    {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm51', '--narm median 1',  {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm52', '--narm q1 1',      {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm53', '--narm q3 1',      {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm54', '--narm iqr 1',     {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm55', '--narm pstdev 1',  {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm56', '--narm sstdev 1',  {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm57', '--narm pvar 1',    {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm58', '--narm svar 1',    {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm59', '--narm mad 1',     {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm60', '--narm madraw 1',  {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm61', '--narm sskew 1',   {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm62', '--narm pskew 1',   {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm63', '--narm skurt 1',   {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm64', '--narm pkurt 1',   {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm65', '--narm jarque 1',  {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm66', '--narm dpo 1',     {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm67', '--narm mode 1',    {IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm68', '--narm antimode 1',{IN_PIPE=>$na_all}, {OUT=>"$nan\n"}],
  ['narm69', '--narm unique 1',  {IN_PIPE=>$na_all}, {OUT=>"\n"}],
  ['narm70', '--narm collapse 1',{IN_PIPE=>$na_all}, {OUT=>"\n"}],
  ['narm71', '--narm countunique 1', {IN_PIPE=>$na_all}, {OUT=>"0\n"}],

  ['narm72', '--narm base64 1',  {IN_PIPE=>$na_all}, {OUT=>"\n\n\n"}],
  ['narm73', '--narm md5 1',     {IN_PIPE=>$na_all}, {OUT=>"\n\n\n"}],
  ['narm74', '--narm sha1 1',    {IN_PIPE=>$na_all}, {OUT=>"\n\n\n"}],
  ['narm75', '--narm sha256 1',  {IN_PIPE=>$na_all}, {OUT=>"\n\n\n"}],
  ['narm76', '--narm sha512 1',  {IN_PIPE=>$na_all}, {OUT=>"\n\n\n"}],
  ['narm77', '--narm rmdup 1',   {IN_PIPE=>$na_all}, {OUT=>"NA\nNaN\n"}],
  ['narm78', '--narm reverse',   {IN_PIPE=>$na_all}, {OUT=>"NA\nNA\nNaN\n"}],
  ['narm79', '--narm transpose',   {IN_PIPE=>$na_all}, {OUT=>"NA\tNA\tNaN\n"}],


  # Test binning
  ['bin1', 'bin 1',     {IN_PIPE=>$bin_in1}, {OUT=>$bin_out1_100}],
  ['bin2', 'bin:5 1',   {IN_PIPE=>$bin_in1}, {OUT=>$bin_out1_5}],
  ['bin3', 'bin:5.5 1', {IN_PIPE=>$bin_in1}, {OUT=>$bin_out1_5_5}],
  ['bin4', 'bin 1',     {IN_PIPE=>$bin_in2}, {OUT=>$bin_out2_100}],
  ['bin5', 'bin:3 1',   {IN_PIPE=>$bin_in2}, {OUT=>$bin_out2_3}],

);

if ($have_stable_sort) {
  push @Tests, (
  # Test 'first' + --full + --sort
  # NOTE: This is subtle:
  #       Sorting should be stable: only ordering the column which is used
  #       for grouping (column 1 in this test). This means that the second
  #       column (containing numbers) should NOT affect sorting, and the order
  #       of the lines should not change. The results of this test
  #       should be the same as 'slct8'. If the system doesn't have stable
  #       'sort', then the order will change.
  ['sortslct1', '-s -t" " -f -g1 first 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 4 i1 4\nB 1 i4 1\n"}],
  # Test 'last' + --full + --sort
  # See note above regarding 'first' - applies to 'last' as well.
  ['sortslct2', '-s -t" " -f -g1 last 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 5 i3 5\nB 3 i7 3\n"}],
  )
}

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

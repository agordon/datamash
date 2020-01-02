#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data

   Copyright (C) 2018-2020 Assaf Gordon <assafgordon@gmail.com

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

##
## This script tests output format options
##


# Until a better way comes along to auto-use Coreutils Perl modules
# as in the coreutils' autotools system.
use Coreutils;
use CuSkip;
use CuTmpdir qw(datamash);

(my $program_name = $0) =~ s|.*/||;
my $prog = 'datamash';

# TODO: add localization tests with "grouping"
# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

my $in1=<<'EOF';
1.000004
0.000005
EOF

my @Tests =
(
  # Test Rouding
  ['r1', 'sum 1' ,  {IN_PIPE=>$in1},  {OUT => "1.000009\n"}],
  ['r2', '--round 1 sum 1' ,  {IN_PIPE=>$in1},  {OUT => "1.0\n"}],
  ['r3', '--round 3 sum 1' ,  {IN_PIPE=>$in1},  {OUT => "1.000\n"}],
  ['r4', '--round 5 sum 1' ,  {IN_PIPE=>$in1},  {OUT => "1.00001\n"}],
  ['r5', '--round 6 sum 1' ,  {IN_PIPE=>$in1},  {OUT => "1.000009\n"}],
  ['r6', '--round 7 sum 1' ,  {IN_PIPE=>$in1},  {OUT => "1.0000090\n"}],

  # Test short rounding option
  ['r7', '-R 7 sum 1',        {IN_PIPE=>$in1},  {OUT => "1.0000090\n"}],

  # Test multiple rounding options
  ['r8', '--round 3 -R 7 sum 1',   {IN_PIPE=>$in1},  {OUT => "1.0000090\n"}],
  ['r9', '--round 7 -R 3 sum 1',   {IN_PIPE=>$in1},  {OUT => "1.000\n"}],


  # Test Custom formats: %f
  ['f1', '--format "%07.3f" sum 1',  {IN_PIPE=>$in1},  {OUT => "001.000\n"}],
  ['f2', '--format "%.7f"   sum 1',  {IN_PIPE=>$in1},  {OUT => "1.0000090\n"}],
  ['f3', '--format "%10f"   sum 1',  {IN_PIPE=>$in1},  {OUT => "  1.000009\n"}],
  ['f4', '--format "%-10f"  sum 1',  {IN_PIPE=>$in1},  {OUT => "1.000009  \n"}],
  ['f5', '--format "%+10f"  sum 1',  {IN_PIPE=>$in1},  {OUT => " +1.000009\n"}],
  # Test %#f (alternate form: always show decimal point)
  ['f6', '--format "%.0f"   sum 1',  {IN_PIPE=>$in1},  {OUT => "1\n"}],
  ['f7', '--format "%#.0f"  sum 1',  {IN_PIPE=>$in1},  {OUT => "1.\n"}],

  # Test Custom formats: %g
  ['g1', '--format "%g"    sum 1',  {IN_PIPE=>$in1},  {OUT => "1.00001\n"}],
  ['g2', '--format "%10g"  sum 1',  {IN_PIPE=>$in1},  {OUT => "   1.00001\n"}],
  ['g3', '--format "%010g" sum 1',  {IN_PIPE=>$in1},  {OUT => "0001.00001\n"}],
  ['g4', '--format "%.10g" sum 1',  {IN_PIPE=>$in1},  {OUT => "1.000009\n"}],
  ['g5', '--format "%.3g"  sum 1',  {IN_PIPE=>$in1},  {OUT => "1\n"}],
  # Test %#g (alternate form: don't trim zero decimal digits)
  ['g6', '--format "%.4g"  sum 1',  {IN_PIPE=>$in1},  {OUT => "1\n"}],
  ['g7', '--format "%#.4g" sum 1',  {IN_PIPE=>$in1},  {OUT => "1.000\n"}],

  # Test Custom formats: %e
  ['e1', '--format "%e"    sum 1', {IN_PIPE=>$in1}, {OUT=>"1.000009e+00\n"}],
  ['e2', '--format "%.3e"  sum 1', {IN_PIPE=>$in1}, {OUT=>"1.000e+00\n"}],

  # Test Custom formats: %a
  # Disable the test for now. Valid output can differ (e.g. 0x8.000p-3 and
  # 0x1.000p0 ).
  # ['a1', '--format "%0.3a" sum 1', {IN_PIPE=>$in1}, {OUT=>"0x8.000p-3\n"}],


  # Custom formats can use lots of memory
  ['m1', '--format "%04000.0f"   sum 1',  {IN_PIPE=>$in1},
   {OUT => "0" x 3999 . "1\n"}],

  # due to binary floating representation, some decimal point digits won't be
  # zero (e.g. 1.0000090000000000000000000000000523453254320000000... or
  # 1.000008999999...).
  # The OUT_SUBST replaces exactly 3995 digits (as expected from the format)
  # with an "X".
  ['m2', '--format "%.4000f"   sum 1',  {IN_PIPE=>$in1},
   {OUT => "1.00000X\n"},
   {OUT_SUBST => 's/^(1\.00000)([0-9]{3995})$/\1X/'}],

);


my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

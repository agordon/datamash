#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data

   Copyright (C) 2018 Assaf Gordon <assafgordon@gmail.com

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
);


my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

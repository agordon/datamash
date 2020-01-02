#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data
  Tests for 'check' operation mode

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

## Cross-Compiling portability hack:
##  under qemu/binfmt, argv[0] (which is used to report errors) will contain
##  the full path of the binary, if the binary is on the $PATH.
##  So we try to detect what is the actual returned value of the program
##  in case of an error.
my $prog = `$prog_bin --foobar 2>&1 | head -n 1 | cut -f1 -d:`;
chomp $prog if $prog;
$prog = $prog_bin unless $prog;

# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;


my $in1=<<'EOF';
A	1	!
B	2	@
C	3	#
D	4	$
E	5	%
EOF

my $in2=<<'EOF';
A	1
B
C	3
EOF

my $in3=<<'EOF';
A
EOF


my @Tests =
(
 # Simple transpose and reverse
 ['c1',  'check', {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],

 # Variations on command-line parsing
 ['c2',  'check 3 field',   {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c3',  'check 3 fields',  {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c4',  'check 3 col',     {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c5',  'check 3 columns', {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c6',  'check 3 column',  {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],

 ['c7',  'check field 3',   {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c8',  'check fields 3',  {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c9',  'check col 3',     {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c10', 'check columns 3', {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c11', 'check column 3',  {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],

 ['c12', 'check 5 lines',   {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c13', 'check 5 line',    {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c14', 'check 5 rows',    {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c15', 'check 5 row',     {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],

 ['c16', 'check lines 5',   {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c17', 'check line 5',    {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c18', 'check row 5',     {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],
 ['c19', 'check rows 5',    {IN_PIPE=>$in1}, {OUT=>"5 lines, 3 fields\n"}],


 # Duplicated options
 ['e1', 'check rows 5 lines 6',  {IN_PIPE=>$in1}, {EXIT=>1},
  {ERR=>"$prog: number of lines/rows already set in operation 'check'\n"}],
 ['e2', 'check fields 6 fields 1',  {IN_PIPE=>$in1}, {EXIT=>1},
  {ERR=>"$prog: number of fields/columns already set in operation 'check'\n"}],

 # Invalid values
 ['e3', 'check 0 lines',  {IN_PIPE=>$in1}, {EXIT=>1},
  {ERR=>"$prog: invalid value zero for lines/fields in operation 'check'\n"}],
 ['e4', 'check 0 fields',  {IN_PIPE=>$in1}, {EXIT=>1},
  {ERR=>"$prog: invalid value zero for lines/fields in operation 'check'\n"}],



 # Check lines
 ['c40', 'check 4 lines',  {IN_PIPE=>$in1}, {EXIT=>1},
  {ERR=>"$prog: check failed: input had 5 lines (expecting 4)\n"}],
 ['c41', 'check 6 lines',  {IN_PIPE=>$in1}, {EXIT=>1},
  {ERR=>"$prog: check failed: input had 5 lines (expecting 6)\n"}],
 ['c42', 'check 6 lines',  {IN_PIPE=>""}, {EXIT=>1},
  {ERR=>"$prog: check failed: input had 0 lines (expecting 6)\n"}],

 # Check fields
 ['c60', 'check 2 fields',  {IN_PIPE=>$in1}, {EXIT=>1},
  {ERR=>"line 1 (3 fields):\n" .
        "  A\t1\t!\n" .
        "$prog: check failed: line 1 has 3 fields (expecting 2)\n"}],


 # Check matrix structure, no expected number of fields
 ['c61', 'check',  {IN_PIPE=>$in2}, {EXIT=>1},
  {ERR=>"line 1 (2 fields):\n" .
        "  A\t1\n" .
        "line 2 (1 fields):\n" .
        "  B\n" .
        "$prog: check failed: line 2 has 1 fields (previous line had 2)\n"}],

 # With expected number of fields
 ['c62', 'check 2 fields',  {IN_PIPE=>$in2}, {EXIT=>1},
  {ERR=>"line 2 (1 fields):\n" .
        "  B\n" .
        "$prog: check failed: line 2 has 1 fields (expecting 2)\n"}],


);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog_bin, \@Tests, $save_temps, $verbose);
exit $fail;

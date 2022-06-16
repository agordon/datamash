#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2021 Assaf Gordon <assafgordon@gmail.com>

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

my $in_g3=<<'EOF';
A 3 W
A 5 W
A 7 W
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

my $in_hdr_only=<<'EOF';
X:Y:Z
EOF

my $full_deprecation = "$prog: Using -f/--full with non-linewise operations " .
"is deprecated and will be disabled in a future release.\n";

my @Tests =
(
  # empty input = empty output, regardless of options
  ['emp2dep', '--full count 2', {IN_PIPE=>""},{OUT=>""},
    {ERR=>"$full_deprecation"}],
  ['emp5dep', '--full --header-in count 2', {IN_PIPE=>""},{OUT=>""},
    {ERR=>"$full_deprecation"}],
  ['emp6dep', '--full --header-out count 2', {IN_PIPE=>""},{OUT=>""},
    {ERR=>"$full_deprecation"}],
  ['emp7dep', '--full --header-in --header-out count 2',
    {IN_PIPE=>""},{OUT=>""},
    {ERR=>"$full_deprecation"}],
  ['emp8dep', '-g3,4 --full --header-in --header-out count 2',
    {IN_PIPE=>""},{OUT=>""},
    {ERR=>"$full_deprecation"}],

  # --full option - without grouping, returns the first line
  ['fl1dep', '-t" " --full sum 2', {IN_PIPE=>$in_g3},
    {OUT=>"A 3 W 98\n"},
    {ERR=>"$full_deprecation"}],
  # --full with grouping - print entire line of each group
  ['fl2dep', '-t" " --full -g3 sum 2', {IN_PIPE=>$in_g3},
    {OUT=>"A 3 W 15\nA 11 X 24\nB 17 Y 17\nB 19 Z 42\n"},
    {ERR=>"$full_deprecation"}],

  # Input and output header, with full line
  ['hdr3dep', '-t" " -g 1 --full --header-in --header-out count 2',
    {IN_PIPE=>$in_hdr1},
    {OUT=>"x y z count(y)\nA 1 10 5\nB 5 10 3\nC 8 11 4\n"},
    {ERR=>"$full_deprecation"}],

  # Output Header with --full
  ['hdr5dep', '-t" " -g 1 --full --header-out count 2', {IN_PIPE=>$in_g3},
    {OUT=>"field-1 field-2 field-3 count(field-2)\n" .
          "A 3 W 5\nB 17 Y 2\nC 23 Z 1\n"},
    {ERR=>"$full_deprecation"}],

  # Input has only one header line (no data lines), and the user requested
  # header-in and header-out => header line should be printed
  ['hdr15dep', '-t: --full -H sum 1', {IN_PIPE=>$in_hdr_only},
    {OUT=>"X:Y:Z:sum(X)\n"},
    {ERR=>"$full_deprecation"}],
  ['hdr17dep', '-t: --full -s -g1 -H sum 2', {IN_PIPE=>$in_hdr_only},
     {OUT=>"X:Y:Z:sum(Y)\n"},
    {ERR=>"$full_deprecation"}],

  # Test single line per group
  ['sl2dep', '-t" " --full -g 1 mean 2', {IN_PIPE=>$in_g4},
    {OUT=>"A 5 5\nK 6 6\nP 2 2\n"},
    {ERR=>"$full_deprecation"}],
);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

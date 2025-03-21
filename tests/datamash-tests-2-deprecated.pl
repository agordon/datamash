#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2021 Assaf Gordon <assafgordon@gmail.com>
   Copyright (C) 2022-2025 Timothy Rice <trice@posteo.net>

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

my $full_deprecation = "$prog: Using -f/--full with non-linewise operations " .
"is deprecated and will be disabled in a future release.\n";

my @Tests =
(
  # Test 'min' + --full
  # Test with "--full", "i2" and "i6" should be displayed
  ['slct2dep', '-t" " -f -g1 min 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 3 i2 3\nB 0 i6 0\n"},
    {ERR=>"$full_deprecation"}],
  # --full with --sort => should not change results
  ['slct3dep', '-s -t" " -f -g1 min 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 3 i2 3\nB 0 i6 0\n"},
    {ERR=>"$full_deprecation"}],

  # Test 'max' + --full
  # Test with "--full", "i3" and "i7" should be displayed
  ['slct5dep', '-t" " -f -g1 max 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 5 i3 5\nB 8 i5 8\n"},
    {ERR=>"$full_deprecation"}],
  # --full with --sort => should not change results
  ['slct6dep', '-s -t" " -f -g1 max 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 5 i3 5\nB 8 i5 8\n"},
    {ERR=>"$full_deprecation"}],

  # Test 'first' + --full
  # Test with "--full", "i1" and "i4" should be displayed
  ['slct8dep', '-t" " -f -g1 first 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 4 i1 4\nB 1 i4 1\n"},
    {ERR=>"$full_deprecation"}],
  # more --full with --sort => see test 'sortslct1' below

  # Test 'last' + --full
  # Test with "--full", "i1" and "i4" should be displayed
  ['slct10dep', '-t" " -f -g1 last 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 5 i3 5\nB 3 i7 3\n"},
    {ERR=>"$full_deprecation"}],
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
  ['sortslct1dep', '-s -t" " -f -g1 first 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 4 i1 4\nB 1 i4 1\n"},
    {ERR=>"$full_deprecation"}],
  # Test 'last' + --full + --sort
  # See note above regarding 'first' - applies to 'last' as well.
  ['sortslct2dep', '-s -t" " -f -g1 last 2', {IN_PIPE=>$in_full1},
    {OUT=>"A 5 i3 5\nB 3 i7 3\n"},
    {ERR=>"$full_deprecation"}],
  )
}

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

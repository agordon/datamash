#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data

   Copyright (C) 2022      Timothy Rice <trice@posteo.net>
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

   Written by Assaf Gordon and Tim Rice.
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

# An unsorted input with a header line
my $INFILE=<<'EOF';
x y z
A % 1
B ( 2
A & 3
B = 4
EOF

my @INFILE_lines = split /\n/, $INFILE, -1;
my $INFILE_NO_HEADER = join("\n", @INFILE_lines[1..$#INFILE_lines]);

my $exp_sort_in_header_full=<<'EOF';
A % 1 1,3
B ( 2 2,4
EOF

my $exp_sort_out_header_full=<<'EOF';
field-1 field-2 field-3 unique(field-3)
A % 1 1,3
B ( 2 2,4
EOF

my $exp_sort_headers_full=<<'EOF';
x y z unique(z)
A % 1 1,3
B ( 2 2,4
EOF

my $deprecation_notice="$prog: Using -f/--full with non-linewise " .
                       "operations is deprecated and will be disabled " .
                       "in a future release.\n";

my @Tests =
(
  ['shdep01',  '-t " " --sort --full --header-out -g 1 unique 3',
    {IN_PIPE=>$INFILE_NO_HEADER}, {OUT=>$exp_sort_out_header_full},
    {ERR=>$deprecation_notice}],
  ['shdep02',  '-t " " -g 1 --sort --full --header-in unique 3',
    {IN_PIPE=>$INFILE}, {OUT=>$exp_sort_in_header_full},
    {ERR=>$deprecation_notice}],
  ['shdep03',  '-t " " -g 1 --sort --full --headers unique 3',
    {IN_PIPE=>$INFILE}, {OUT=>$exp_sort_headers_full},
    {ERR=>$deprecation_notice}],

  # Check sort-piping with empty input - should always produce empty output
  ['shdep04',  '-t " " --sort --full unique 3',
    {IN_PIPE=>""}, {OUT=>""},
    {ERR=>$deprecation_notice}],
);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog_bin, \@Tests, $save_temps, $verbose);
exit $fail;

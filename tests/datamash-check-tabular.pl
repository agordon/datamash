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
b	z	6
c	x	7
EOF

# missing field on second line
my $in2=<<'EOF';
a	x	1
b	z
c	x	7
EOF

# Same as in2, with whitespace delimiters
my $in2_ws=<<"EOF";
a    x  \t  1
  b   \t  z
c x 7
EOF

# second line has 2 tab characters, thus 3 fields
# (the last field is empty).
# version 1.1.0 and before rejected such input.
my $in3=<<"EOF";
a	x	1
b	z\t
c	x	7
EOF

# Same as in3, with whitespace delimiters
my $in3_ws=<<"EOF";
a     x  \t  1
b \t  z  \t
   c\t\t\tx     \t  7
EOF


# one line
my $in4=<<'EOF';
a	x	1
EOF

# one field
my $in5=<<'EOF';
a
b
c
d
EOF

# one field, with bad input (fourth line has 0 fields)
my $in6=<<'EOF';
a
b
c

e
EOF

my @Tests =
(
  ['c1', 'check', {IN_PIPE=>$in1}, {OUT=>"3 lines, 3 fields\n"}],

  ['c2', 'check', {IN_PIPE=>$in4}, {OUT=>"1 line, 3 fields\n"}],
  ['c3', 'check', {IN_PIPE=>$in5}, {OUT=>"4 lines, 1 field\n"}],
  ['c4', 'check', {IN_PIPE=>$in3}, {OUT=>"3 lines, 3 fields\n"}],
  ['c5', '-W check', {IN_PIPE=>$in3_ws}, {OUT=>"3 lines, 3 fields\n"}],

  # Check bad input:
  # The first four lines will be something like:
  #   'line X has N fields:'
  #   '  [content of line X]'
  #   'line Y has M fields:'
  #   '  [content of line Y]'
  # The ERR_SUBSTR will remove these messages, as they are highly variable
  # and dependant on the input. Then only the last line of error message
  # is checked.
  ['e1', 'check', {IN_PIPE=>$in2}, {EXIT=>1},
      {ERR_SUBST => 's/^(li|  ).*$//'},
      {ERR => "\n\n\n\n$prog: check failed: line 2 has 2 fields " .
                             "(previous line had 3)\n"}],
  ['e1ws', '-W check', {IN_PIPE=>$in2_ws}, {EXIT=>1},
      {ERR_SUBST => 's/^(li|  ).*$//'},
      {ERR => "\n\n\n\n$prog: check failed: line 2 has 2 fields " .
                             "(previous line had 3)\n"}],

  ['e2', 'check', {IN_PIPE=>$in6}, {EXIT=>1},
    {ERR_SUBST => 's/^(li|  ).*$//'},
    {ERR => "\n\n\n\n$prog: check failed: line 4 has 0 fields " .
                           "(previous line had 1)\n"}],
  ['e2ws', '-W check', {IN_PIPE=>$in6}, {EXIT=>1},
    {ERR_SUBST => 's/^(li|  ).*$//'},
    {ERR => "\n\n\n\n$prog: check failed: line 4 has 0 fields " .
                           "(previous line had 1)\n"}],

);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog_bin, \@Tests, $save_temps, $verbose);
exit $fail;

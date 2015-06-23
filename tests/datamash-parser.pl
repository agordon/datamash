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

# TODO: add localization tests with "grouping"
# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

my $in1=<<"EOF";
A\t100\tx
A\t10\tx
B\t10\tx
B\t35\ty
EOF

my $in2=<<"EOF";
1	2	3	4	5
6	7	8	9	10
EOF

my $out2=<<'EOF';
7	9	11	13	15
EOF

my @Tests =
(
  # no explicit mode - 'sum' implies 'group by' without any columns -
  # operate on entire file.
  ['p1', 'sum 2',               {IN_PIPE=>$in1}, {OUT=>"155\n"}],

  # 'old' syntax - with '-g'
  ['p2', '-g 1 sum 2',          {IN_PIPE=>$in1}, {OUT=>"A\t110\nB\t45\n"}],

  # 'new' syntax - without '-g'
  ['p3', 'groupby 1 sum 2',     {IN_PIPE=>$in1}, {OUT=>"A\t110\nB\t45\n"}],
  ['p4', 'gb 1 sum 2',          {IN_PIPE=>$in1}, {OUT=>"A\t110\nB\t45\n"}],

  # group by multiple columns
  ['p5', 'gb 1,3 sum 2',        {IN_PIPE=>$in1},
                         {OUT=>"A\tx\t110\nB\tx\t10\nB\ty\t35\n"}],
  # operate by multiple columns with comma
  ['p6', 'gb 1 last 2,3',        {IN_PIPE=>$in1},
                         {OUT=>"A\t10\tx\nB\t35\ty\n"}],

  # many groupby columns, force the parser to allocate more array items
  ['p7', 'gb 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23' .
         ',24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,44,44' .
         ',45,46,47,48,49,50 sum 2',          {IN_PIPE=>""}, {OUT=>""}],
  # many operator columns, force the parser to allocate more array items
  ['p8', 'gb 1 sum 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22' .
         ',23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,44,44' .
         ',45,46,47,48,49,50',          {IN_PIPE=>""}, {OUT=>""}],

  # Invalid numeric value for column prasing should be treated as named column
  ['p9', 'sum 1x', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: -H or --header-in must be used with named columns\n"}],

  # Processing mode without operation
  ['p10','groupby 1', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing operation\n"}],

  # invalid operation after valid mode
  ['p11','groupby 1 foobar 2', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid operation 'foobar'\n"}],

  # missing field number after processing mode
  ['p12','groupby', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing field number\n"}],

  # field range syntax
  ['p20','sum 1-44', {IN_PIPE=>""}, {OUT=>""}],

  # compare results of equivalent syntaxes
  ['p21','sum 1,2,3,4,5', {IN_PIPE=>$in2}, {OUT=>$out2}],
  ['p22','sum 1-2,3-4,5', {IN_PIPE=>$in2}, {OUT=>$out2}],
  ['p23','sum 1-2,3-5',   {IN_PIPE=>$in2}, {OUT=>$out2}],
  ['p24','sum 1-4,5',     {IN_PIPE=>$in2}, {OUT=>$out2}],
  ['p25','sum 1-5',       {IN_PIPE=>$in2}, {OUT=>$out2}],
  ['p26','sum 1 sum 2 sum 3 sum 4 sum 5',
                          {IN_PIPE=>$in2}, {OUT=>$out2}],
  ['p27','sum 1,2 sum 3-5',
                          {IN_PIPE=>$in2}, {OUT=>$out2}],

  # Field range with invalid syntax
  ['e20','sum 1-',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field range '1-'\n"}],
  ['e21','sum 1-x',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field range '1-x'\n"}],
  ['e22','sum 4-2',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field range '4-2'\n"}],
  # zero in range
  ['e23','sum 0-2',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field range '0-2'\n"}],
  ['e24','sum 1-0',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field range '1-0'\n"}],
  #Negative in range
  ['e25','sum 0--5',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field range '0--5'\n"}],
);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

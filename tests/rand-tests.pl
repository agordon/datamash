#!/usr/bin/env perl
=pod
  Unit Tests for rand

   Copyright (C) 2022- Timothy Rice <trice@posteo.net>

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

   Written by Timothy Rice.
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
my $prog = 'rand';

my $out_unif_rep=<<'EOF';
0.840188
0.394383
0.783099
0.798440
EOF

my $out_unif_zero="" .
"0.840188\x00"      .
"0.394383\x00"      .
"0.783099\x00"      .
"0.798440\x00";

my $out_exp_rep=<<'EOF';
0.174130
0.930433
0.244496
0.225095
EOF

my $out_norm_rep_even=<<'EOF';
-0.464893
0.363503
0.209560
-0.667140
EOF

my $out_norm_rep_odd=<<'EOF';
-0.464893
0.363503
0.209560
-0.667140
0.139192
EOF

# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

my @Tests =
(
  # Basic tests, default everything except the seed.
  # (Of course setting the seed is necessary to make tests reproducible.)
  ['unif-default', '-S0 unif',    {IN_PIPE=>""}, {OUT=>"0.840188\n"}],
  ['exp-default',  '-S0 exp',     {IN_PIPE=>""}, {OUT=>"0.174130\n"}],
  ['norm-default', '-S0 norm',    {IN_PIPE=>""}, {OUT=>"-0.464893\n"}],

  # Increase the number reps
  ['unif-rep',     '-S0 unif 4',  {IN_PIPE=>""}, {OUT=>$out_unif_rep}],
  ['exp-rep',      '-S0 exp 4',   {IN_PIPE=>""}, {OUT=>$out_exp_rep}],
  ['norm-rep-even','-S0 norm 4',  {IN_PIPE=>""}, {OUT=>$out_norm_rep_even}],
  ['norm-rep-odd', '-S0 norm 5',  {IN_PIPE=>""}, {OUT=>$out_norm_rep_odd}],

  # Check zero termination
  ['unif-zero',    '-z -S0 unif 4',  {IN_PIPE=>""}, {OUT=>$out_unif_zero}],

  # Check different parameters work
  ['unif-min',        '-S0 -a -1 unif',
    {IN_PIPE=>""}, {OUT=>"0.680375\n"}],
  ['unif-max',        '-S0 -b2 unif',
    {IN_PIPE=>""}, {OUT=>"1.680375\n"}],
  ['unif-min-max',    '-S0 -a1 -b2 unif',
    {IN_PIPE=>""}, {OUT=>"1.840188\n"}],
  ['exp-rate',        '-S0 -r 5 exp',
    {IN_PIPE=>""}, {OUT=>"0.034826\n"}],
  ['exp-mean',        '-S0 -m 5 exp',
    {IN_PIPE=>""}, {OUT=>"0.870650\n"}],
  ['norm-mean-short', '-S0 -m 5 norm',
    {IN_PIPE=>""}, {OUT=>"4.535107\n"}],
  ['norm-mean-long',  '-S0 --mean 5 norm',
    {IN_PIPE=>""}, {OUT=>"4.535107\n"}],
  ['norm-sd-short',   '-S0 -s 10 norm',
    {IN_PIPE=>""}, {OUT=>"-4.648926\n"}],
  ['norm-sd-long',    '-S0 --stdev 10 norm',
    {IN_PIPE=>""}, {OUT=>"-4.648926\n"}],
  ['norm-mean-sd',    '-S0 -m 5 -s 10 norm',
    {IN_PIPE=>""}, {OUT=>"0.351074\n"}],

  # Check conflicting parameters
  ['unif-big-min',    '-S0 -a 2 unif',
    {IN_PIPE=>""}, {EXIT=>1},
    {ERR=>"$prog: min and max contradict: 2.000000 > 1.000000\n"}],
  ['unif-small-max',  '-S0 -b -1 unif',
    {IN_PIPE=>""}, {EXIT=>1},
    {ERR=>"$prog: min and max contradict: 0.000000 > -1.000000\n"}],
  ['unif-max-min',    '-S0 -a 2 -b -1 unif',
    {IN_PIPE=>""}, {EXIT=>1},
    {ERR=>"$prog: min and max contradict: 2.000000 > -1.000000\n"}],
  ['exp-rate-mean',   '-S0 -r 5 -m 5 exp',
    {IN_PIPE=>""}, {EXIT=>1},
    {ERR=>"$prog: only one of rate and mean may parametrize " .
          "the exponential distribution\n"}],
);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

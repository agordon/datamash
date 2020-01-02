#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data

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

##
## Portability hack:
## find the exact wording of 'nan' and inf (not-a-number).
## It's lower case in GNU/Linux,FreeBSD,OpenBSD,
## but is "NaN" on Illumos/OpenSolaris
my $nan = `$prog ---print-nan`;
die "test infrastructure failed: can't determine 'nan' string" unless $nan;
my $inf = `$prog ---print-inf`;
die "test infrastructure failed: can't determine 'inf' string" unless $inf;

=pod
Equivalent R code

    pop.sd=function(x)(sqrt(var(x)*(length(x)-1)/length(x)))
    smp.sd=sd

    # alternatively, use the built-in covariance function:
    # smp.cov=cov
    smp.cov <- function(x,y) {
      stopifnot(identical(length(x), length(y)))
      sum((x - mean(x)) * (y - mean(y))) / (length(x) - 1)
    }
    pop.cov <- function(x,y) {
      stopifnot(identical(length(x), length(y)))
      sum((x - mean(x)) * (y - mean(y))) / (length(x) )
   }

   # alternative, use the built-in covariance fuction:
   #  smp.pearsoncor=cor
   smp.pearsoncor=function(x,y) { smp.cov(x,y)/ ( smp.sd(x)*smp.sd(y) ) }
   pop.pearsoncor=function(x,y) { pop.cov(x,y)/ ( pop.sd(x)*pop.sd(y) ) }

   in1.x=c(-0.49,0.14,1.62,2.76,-0.46,3.28,-0.01,2.90,2.46,1.52)
   in1.y=c(-0.21,-0.16,1.86,1.81,0.39,4.17,0.38,1.90,2.69,0.78)

   in2.x = c(1.599,-1.011,-1.687,5.070,6.944,7.934,2.134,5.150,
             10.197,11.427,10.379,14.867,11.399,13.479,18.328,16.573,
             17.804,18.694,16.690,21.805)
   in2.y = seq(20)

=cut

my $in1=<<"EOF";
-0.49	-0.21
0.14	-0.16
1.62	1.86
2.76	1.81
-0.46	0.39
3.28	4.17
-0.01	0.38
2.90	1.90
2.46	2.69
1.52	0.78
EOF


my $out1_scov=<<'EOF';
1.802
EOF

my $out1_pcov=<<'EOF';
1.622
EOF

my $out1_pcov_hdr=<<'EOF';
pcov(field-2)
1.622
EOF

my $in2=<<'EOF';
1.599	1
-1.011	2
-1.687	3
5.070	4
6.944	5
7.934	6
2.134	7
5.150	8
10.197	9
11.427	10
10.379	11
14.867	12
11.399	13
13.479	14
18.328	15
16.573	16
17.804	17
18.694	18
16.690	19
21.805	20
EOF

my $out2_p=<<'EOF';
0.944
EOF

my $out2_s=<<'EOF';
0.944
EOF

my $in3=<<'EOF';
1	2
EOF

my $in4=<<'EOF';
NA	NA
EOF

my $in5=<<'EOF';
1	2
2	NA
3	6
EOF

my @Tests =
(
  ['c1', 'scov 1:2', {IN_PIPE=>$in1}, {OUT=>$out1_scov}],
  ['c2', 'pcov 1:2', {IN_PIPE=>$in1}, {OUT=>$out1_pcov}],

  # Pair with output headers - only one field and header should be printed
  ['c3', '--header-out pcov 1:2', {IN_PIPE=>$in1}, {OUT=>$out1_pcov_hdr}],

  ['p1', 'ppearson 1:2', {IN_PIPE=>$in2}, {OUT=>$out2_p}],
  ['p2', 'spearson 1:2', {IN_PIPE=>$in2}, {OUT=>$out2_s}],

  # Test operations on edge-cases of input (one items, no items,
  # different number of items)
  ['c4', 'scov 1:2',     {IN_PIPE=>$in3}, {OUT=>"$nan\n"}],
  ['p4', 'spearson 1:2', {IN_PIPE=>$in3}, {OUT=>"$nan\n"}],

  ['c5', '--narm scov 1:2',     {IN_PIPE=>$in4}, {OUT=>"$nan\n"}],
  ['p5', '--narm spearson 1:2', {IN_PIPE=>$in4}, {OUT=>"$nan\n"}],

  ['c6', '--narm scov 1:2',     {IN_PIPE=>$in5}, {EXIT=>1},
    {ERR=>"$prog: input error for operation 'scov': " .
          "fields 1,2 have different number of items\n"}],
  ['p6', '--narm spearson 1:2', {IN_PIPE=>$in5}, {EXIT=>1},
    {ERR=>"$prog: input error for operation 'spearson': " .
          "fields 1,2 have different number of items\n"}],
);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

##
## For each test, trim the resulting value to maximum three digits
## after the decimal point.
##
for my $t (@Tests) {
 push @{$t}, {OUT_SUBST=>'s/^(-?\d+\.\d{1,3})\d*/\1/'};
}


my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

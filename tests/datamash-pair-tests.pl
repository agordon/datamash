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

=pod
Equivalent R code

   x=c(-0.49,0.14,1.62,2.76,-0.46,3.28,-0.01,2.90,2.46,1.52)
   y=c(-0.21,-0.16,1.86,1.81,0.39,4.17,0.38,1.90,2.69,0.78)
   scov <- function(x,y) {
      stopifnot(identical(length(x), length(y)))
      sum((x - mean(x)) * (y - mean(y))) / (length(x) - 1)
   }
   pcov <- function(x,y) {
      stopifnot(identical(length(x), length(y)))
      sum((x - mean(x)) * (y - mean(y))) / (length(x) )
   }

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


my @Tests =
(
  ['c1', 'scov 1:2', {IN_PIPE=>$in1}, {OUT=>$out1_scov}],
  ['c2', 'pcov 1:2', {IN_PIPE=>$in1}, {OUT=>$out1_pcov}],
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

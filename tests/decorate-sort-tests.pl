#!/usr/bin/env perl
=pod
  Unit Tests for decorate

   Copyright (C) 2020 Assaf Gordon <assafgordon@gmail.com>

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
use CuTmpdir qw(decorate);
use MIME::Base64 ;

(my $program_name = $0) =~ s|.*/||;
my $prog = 'decorate';


# TODO: add localization tests with "grouping"
# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;


my $in1=<<'EOF';
I   1.20.30.41
II  1.20.30.1
IV  1.10.30.14
M   1.2.10.3
M   192.168.43.1
II  1.20.30.41
D   192.168.17.8
C   1.2.10.3
L   192.168.17.10
EOF


my $out1_dec_roman=<<'EOF';
I   1.20.30.41
II  1.20.30.1
II  1.20.30.41
IV  1.10.30.14
L   192.168.17.10
C   1.2.10.3
D   192.168.17.8
M   1.2.10.3
M   192.168.43.1
EOF

my $out1_dec_ipv4=<<'EOF';
C   1.2.10.3
M   1.2.10.3
IV  1.10.30.14
II  1.20.30.1
I   1.20.30.41
II  1.20.30.41
D   192.168.17.8
L   192.168.17.10
M   192.168.43.1
EOF

my $out1_dec_ipv4_stable=<<'EOF';
M   1.2.10.3
C   1.2.10.3
IV  1.10.30.14
II  1.20.30.1
I   1.20.30.41
II  1.20.30.41
D   192.168.17.8
L   192.168.17.10
M   192.168.43.1
EOF


my $out1_dec_ipv4_rev=<<'EOF';
M   192.168.43.1
L   192.168.17.10
D   192.168.17.8
I   1.20.30.41
II  1.20.30.41
II  1.20.30.1
IV  1.10.30.14
C   1.2.10.3
M   1.2.10.3
EOF

my $out1_dec_ipv4_rev_header1=<<'EOF';
I   1.20.30.41
M   192.168.43.1
L   192.168.17.10
D   192.168.17.8
II  1.20.30.41
II  1.20.30.1
IV  1.10.30.14
C   1.2.10.3
M   1.2.10.3
EOF

my $out1_dec_ipv4_rev_header2=<<'EOF';
I   1.20.30.41
II  1.20.30.1
M   192.168.43.1
L   192.168.17.10
D   192.168.17.8
II  1.20.30.41
IV  1.10.30.14
C   1.2.10.3
M   1.2.10.3
EOF


my $out1_dec_roman_ipv4rev=<<'EOF';
I   1.20.30.41
II  1.20.30.41
II  1.20.30.1
IV  1.10.30.14
L   192.168.17.10
C   1.2.10.3
D   192.168.17.8
M   192.168.43.1
M   1.2.10.3
EOF

my $out1_dec_ipv4_romanrev=<<'EOF';
M   1.2.10.3
C   1.2.10.3
IV  1.10.30.14
II  1.20.30.1
II  1.20.30.41
I   1.20.30.41
D   192.168.17.8
L   192.168.17.10
M   192.168.43.1
EOF



my $out1_dec_roman_k2=<<'EOF';
I   1.20.30.41
II  1.20.30.1
II  1.20.30.41
IV  1.10.30.14
L   192.168.17.10
C   1.2.10.3
D   192.168.17.8
M   1.2.10.3
M   192.168.43.1
EOF

my $out1_dec_k2n_roman=<<'EOF';
IV  1.10.30.14
I   1.20.30.41
II  1.20.30.1
II  1.20.30.41
C   1.2.10.3
M   1.2.10.3
L   192.168.17.10
D   192.168.17.8
M   192.168.43.1
EOF


my @Tests =
(
 ['s1', '-k1,1:roman', {IN_PIPE=>$in1}, {OUT => $out1_dec_roman}],
 ['s2', '-k2,2:ipv4',  {IN_PIPE=>$in1}, {OUT => $out1_dec_ipv4}],
 ['s3', '-k2,2:ipv4 --stable', {IN_PIPE=>$in1}, {OUT => $out1_dec_ipv4_stable}],
 ['s4', '-k2,2r:ipv4',  {IN_PIPE=>$in1}, {OUT => $out1_dec_ipv4_rev}],

 ['s5', '-k1,1:roman -k2,2r:ipv4' , {IN_PIPE=>$in1},
  {OUT => $out1_dec_roman_ipv4rev}],
 ['s6', '-k2,2:ipv4 -k1r,1:roman' , {IN_PIPE=>$in1},
  {OUT => $out1_dec_ipv4_romanrev}],

 ['s10', '-k1,1:roman -k2,2' , {IN_PIPE=>$in1},
  {OUT => $out1_dec_roman_k2}],
 ['s11', '-k2n,2 -k1,1:roman' , {IN_PIPE=>$in1},
  {OUT => $out1_dec_k2n_roman}],


 # Sort with header lines
 ['sh1', '-H -k2,2r:ipv4', {IN_PIPE=>$in1}, {OUT=>$out1_dec_ipv4_rev_header1}],
 ['sh2', '--header=2 -k2,2r:ipv4',  {IN_PIPE=>$in1},
  {OUT => $out1_dec_ipv4_rev_header2}],
 # More header lines than in the input
 ['sh3', '--header=9 -k2,2r:ipv4',  {IN_PIPE=>$in1}, {OUT => $in1}],
 ['sh4', '--header=10 -k2,2r:ipv4',  {IN_PIPE=>$in1}, {OUT => $in1}],

);

# Repeat all tests with --debug option, ensure it does not cause any regression
my @debug_tests;
foreach my $t (@Tests)
  {
    # Skip tests with EXIT!=0 or ERR_SUBST part
    # (as '--debug' requires its own ERR_SUBST).
    my $exit_val;
    my $have_err_subst;
    foreach my $e (@$t)
      {
        next unless ref $e && ref $e eq 'HASH';
        $exit_val = $e->{EXIT} if defined $e->{EXIT};
        $have_err_subst = 1 if defined $e->{ERR_SUBST};
      }
    next if $exit_val || $have_err_subst;

    # Duplicate the test, add '--debug' argument
    my @newt = @$t;
    $newt[0] = 'dbg_' . $newt[0];
    $newt[1] = '---debug ' . $newt[1];

    # Discard all debug printouts before comparing output
    push @newt, {ERR_SUBST => q!s/.*\n//m!};

    push @debug_tests, \@newt;
  }
push @Tests, @debug_tests;


my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

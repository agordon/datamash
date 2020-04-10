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
XI  1.2.10.3
M   192.168.43.1
D   192.168.17.8
C   192.168.17.100
L   192.168.17.10
EOF

my $out1_asis=<<'EOF';
I I   1.20.30.41
II II  1.20.30.1
IV IV  1.10.30.14
XI XI  1.2.10.3
M M   192.168.43.1
D D   192.168.17.8
C C   192.168.17.100
L L   192.168.17.10
EOF

my $out1_strlen1=<<'EOF';
000000000000000000001 000000000000000000010 I   1.20.30.41
000000000000000000002 000000000000000000009 II  1.20.30.1
000000000000000000002 000000000000000000010 IV  1.10.30.14
000000000000000000002 000000000000000000008 XI  1.2.10.3
000000000000000000001 000000000000000000012 M   192.168.43.1
000000000000000000001 000000000000000000012 D   192.168.17.8
000000000000000000001 000000000000000000014 C   192.168.17.100
000000000000000000001 000000000000000000013 L   192.168.17.10
EOF

## strlen on the entire line
my $out1_strlen2=<<'EOF';
000000000000000000014 I   1.20.30.41
000000000000000000013 II  1.20.30.1
000000000000000000014 IV  1.10.30.14
000000000000000000012 XI  1.2.10.3
000000000000000000016 M   192.168.43.1
000000000000000000016 D   192.168.17.8
000000000000000000018 C   192.168.17.100
000000000000000000017 L   192.168.17.10
EOF

## two strlens: first on a single field, second on the entire line
my $out1_strlen3=<<'EOF';
000000000000000000001 000000000000000000014 I   1.20.30.41
000000000000000000002 000000000000000000013 II  1.20.30.1
000000000000000000002 000000000000000000014 IV  1.10.30.14
000000000000000000002 000000000000000000012 XI  1.2.10.3
000000000000000000001 000000000000000000016 M   192.168.43.1
000000000000000000001 000000000000000000016 D   192.168.17.8
000000000000000000001 000000000000000000018 C   192.168.17.100
000000000000000000001 000000000000000000017 L   192.168.17.10
EOF

## two strlens: first on a single field, second on the second field
## but skipping the first 2 characters
my $out1_strlen4=<<'EOF';
000000000000000000001 000000000000000000009 I   1.20.30.41
000000000000000000002 000000000000000000008 II  1.20.30.1
000000000000000000002 000000000000000000009 IV  1.10.30.14
000000000000000000002 000000000000000000007 XI  1.2.10.3
000000000000000000001 000000000000000000011 M   192.168.43.1
000000000000000000001 000000000000000000011 D   192.168.17.8
000000000000000000001 000000000000000000013 C   192.168.17.100
000000000000000000001 000000000000000000012 L   192.168.17.10
EOF


my $out1_dec_roman=<<'EOF';
000000000000000000001 I   1.20.30.41
000000000000000000002 II  1.20.30.1
000000000000000000004 IV  1.10.30.14
000000000000000000011 XI  1.2.10.3
000000000000000001000 M   192.168.43.1
000000000000000000500 D   192.168.17.8
000000000000000000100 C   192.168.17.100
000000000000000000050 L   192.168.17.10
EOF

my $out1_dec_ipv4=<<'EOF';
01141E29 I   1.20.30.41
01141E01 II  1.20.30.1
010A1E0E IV  1.10.30.14
01020A03 XI  1.2.10.3
C0A82B01 M   192.168.43.1
C0A81108 D   192.168.17.8
C0A81164 C   192.168.17.100
C0A8110A L   192.168.17.10
EOF

my $out1_dec_roman_ipv4=<<'EOF';
000000000000000000001 01141E29 I   1.20.30.41
000000000000000000002 01141E01 II  1.20.30.1
000000000000000000004 010A1E0E IV  1.10.30.14
000000000000000000011 01020A03 XI  1.2.10.3
000000000000000001000 C0A82B01 M   192.168.43.1
000000000000000000500 C0A81108 D   192.168.17.8
000000000000000000100 C0A81164 C   192.168.17.100
000000000000000000050 C0A8110A L   192.168.17.10
EOF

my $out1_dec_ipv4_roman=<<'EOF';
01141E29 000000000000000000001 I   1.20.30.41
01141E01 000000000000000000002 II  1.20.30.1
010A1E0E 000000000000000000004 IV  1.10.30.14
01020A03 000000000000000000011 XI  1.2.10.3
C0A82B01 000000000000000001000 M   192.168.43.1
C0A81108 000000000000000000500 D   192.168.17.8
C0A81164 000000000000000000100 C   192.168.17.100
C0A8110A 000000000000000000050 L   192.168.17.10
EOF


my $in2=<<'EOF';
10.010.0x10.0xA
EOF

my $out2_ipv4inet=<<'EOF';
0A08100A 10.010.0x10.0xA
EOF

my $in3=<<'EOF';
2001:0db8:85a3:0000:0000:8a2e:0370:7334
2001:db8:85a3:0:0:8a2e:370:7334
2001:db8:85a3::8a2e:370:7334
0:0:0:0:0:0:0:1
::1
0:0:0:0:0:0:0:0
::
::ffff:192.0.2.128
::ffff:c000:0280
EOF

my $out3_ipv6=<<'EOF';
2001:0DB8:85A3:0000:0000:8A2E:0370:7334 2001:0db8:85a3:0000:0000:8a2e:0370:7334
2001:0DB8:85A3:0000:0000:8A2E:0370:7334 2001:db8:85a3:0:0:8a2e:370:7334
2001:0DB8:85A3:0000:0000:8A2E:0370:7334 2001:db8:85a3::8a2e:370:7334
0000:0000:0000:0000:0000:0000:0000:0001 0:0:0:0:0:0:0:1
0000:0000:0000:0000:0000:0000:0000:0001 ::1
0000:0000:0000:0000:0000:0000:0000:0000 0:0:0:0:0:0:0:0
0000:0000:0000:0000:0000:0000:0000:0000 ::
0000:0000:0000:0000:0000:FFFF:C000:0280 ::ffff:192.0.2.128
0000:0000:0000:0000:0000:FFFF:C000:0280 ::ffff:c000:0280
EOF


my $out1_dec_ipv4_rev_header1=<<'EOF';
I   1.20.30.41
C0A8110A L   192.168.17.10
C0A81164 C   192.168.17.100
C0A81108 D   192.168.17.8
C0A82B01 M   192.168.43.1
01020A03 XI  1.2.10.3
010A1E0E IV  1.10.30.14
01141E01 II  1.20.30.1
EOF

my $out1_dec_ipv4_header1=<<'EOF';
I   1.20.30.41
01141E01 II  1.20.30.1
010A1E0E IV  1.10.30.14
01020A03 XI  1.2.10.3
C0A82B01 M   192.168.43.1
C0A81108 D   192.168.17.8
C0A81164 C   192.168.17.100
C0A8110A L   192.168.17.10
EOF

my $out1_dec_ipv4_header2=<<'EOF';
I   1.20.30.41
II  1.20.30.1
010A1E0E IV  1.10.30.14
01020A03 XI  1.2.10.3
C0A82B01 M   192.168.43.1
C0A81108 D   192.168.17.8
C0A81164 C   192.168.17.100
C0A8110A L   192.168.17.10
EOF


my @Tests =
(
 ## basic decoration functions
 ['d1', '--decorate -k1,1:roman' , {IN_PIPE=>$in1}, {OUT => $out1_dec_roman}],
 ['d2', '--decorate -k2,2:ipv4' , {IN_PIPE=>$in1}, {OUT => $out1_dec_ipv4}],
 ['d3', '--decorate -k1,1:roman -k2,2:ipv4' , {IN_PIPE=>$in1},
  {OUT => $out1_dec_roman_ipv4}],
 ['d4', '--decorate -k2,2:ipv4 -k1,1:roman' , {IN_PIPE=>$in1},
  {OUT => $out1_dec_ipv4_roman}],
 ['d5', '--decorate -k1,1:as-is' , {IN_PIPE=>$in1}, {OUT => $out1_asis}],
 ['d6', '--decorate -k1,1:strlen -k2,2:strlen' , {IN_PIPE=>$in1},
  {OUT => $out1_strlen1}],
 ['d7', '--decorate -k1,1:ipv4inet', {IN_PIPE=>$in2}, {OUT => $out2_ipv4inet}],
 ['d8', '--decorate -k1,1:ipv6',     {IN_PIPE=>$in3}, {OUT => $out3_ipv6}],
 ['d9', '--decorate -k1:strlen' ,    {IN_PIPE=>$in1}, {OUT=>$out1_strlen2}],
 ['d10', '--decorate -k1,1:strlen -k1:strlen' ,  {IN_PIPE=>$in1},
  {OUT=>$out1_strlen3}],
 ['d11', '--decorate -k1,1:strlen -k2.2,2:strlen' ,  {IN_PIPE=>$in1},
  {OUT=>$out1_strlen4}],

 ## basic undecoration
 ['u1', '--undecorate 1' , {IN_PIPE=>$out1_dec_roman}, {OUT => $in1}],
 ['u2', '--undecorate 1' , {IN_PIPE=>$out1_dec_ipv4}, {OUT => $in1}],
 ['u3', '--undecorate 2' , {IN_PIPE=>$out1_dec_roman_ipv4}, {OUT => $in1}],
 ['u4', '--undecorate 2' , {IN_PIPE=>$out1_dec_ipv4_roman}, {OUT => $in1}],

 ## sort arguments adjustment
 ['sa1', '--print-sort-args -k1,1:roman', {OUT=>"sort -k1,1\n"}],
 ['sa2', '--print-sort-args -k2,2:roman', {OUT=>"sort -k1,1\n"}],
 ['sa3', '--print-sort-args -k2,2:roman -k4,4n -k1,1:strlen',
  {OUT=>"sort -k1,1 -k6,6n -k2,2\n"}],
 ['sa4', '--print-sort-args -k2,2:ipv4 -k4,4Vr',{OUT=>"sort -k1,1 -k5,5rV\n"}],
 ['sa5', '--print-sort-args -k2,2:ipv4 -k4r,4V',{OUT=>"sort -k1,1 -k5,5rV\n"}],
 ['sa6', '--print-sort-args -k2,2:ipv4 -k4V,4r',{OUT=>"sort -k1,1 -k5,5rV\n"}],
 ['sa7', '--print-sort-args -k2,2r:ipv4 ',{OUT=>"sort -k1,1r\n"}],
 ['sa8', '--print-sort-args -k2,2r:ipv4 ',{OUT=>"sort -k1,1r\n"}],
 ['sa9', '--print-sort-args -k2,2r:ipv4 -s ',{OUT=>"sort -k1,1r -s\n"}],
 ['sa10', '--print-sort-args -k2,2r:ipv4 -szu ',
  {OUT=>"sort -k1,1r -s -z -u\n"}],
 ['sa11', '--print-sort-args -k2,2r:ipv4 -S 2G -szu ',
  {OUT=>"sort -k1,1r -S 2G -s -z -u\n"}],
 ['sa12', '--print-sort-args -k2,2r:ipv4 -S 2G -T /foo/bar -szu ',
  {OUT=>"sort -k1,1r -S 2G -T /foo/bar -s -z -u\n"}],
 ['sa13', '--print-sort-args -k2,2r:ipv4 -t: -S 2G -T /foo/bar -szu ',
  {OUT=>"sort -k1,1r -t : -S 2G -T /foo/bar -s -z -u\n"}],
 ['sa14', '--print-sort-args -k2,2r:ipv4 -t: -t:',
  {OUT=>"sort -k1,1r -t : -t :\n"}],
 ['sa15', '--print-sort-args -k2,2r:ipv4 -t "\\\\0"',
  {OUT=>"sort -k1,1r -t \\0\n"}],
 ['sa16', '--print-sort-args -k2,2r:ipv4 --compress-program=ZOOP',
  {OUT=>"sort -k1,1r --compress-program ZOOP\n"}],
 ['sa17', '--print-sort-args -k2,2r:ipv4 --compress-program ZOOP',
  {OUT=>"sort -k1,1r --compress-program ZOOP\n"}],
 ['sa18', '--print-sort-args -k2,2r:ipv4 --random-source /foo/bar',
  {OUT=>"sort -k1,1r --random-source /foo/bar\n"}],
 ['sa19', '--print-sort-args -k2,2r:ipv4 --batch-size 9 --parallel=42',
  {OUT=>"sort -k1,1r --batch-size 9 --parallel 42\n"}],
 ['sa21', '--print-sort-args -k2,2r:ipv4 -c',
  {OUT=>"sort -k1,1r -c\n"}],
 ['sa22', '--print-sort-args -k2,2r:ipv4 --check',
  {OUT=>"sort -k1,1r --check\n"}],
 ['sa23', '--print-sort-args -k2,2r:ipv4 -C',
  {OUT=>"sort -k1,1r -C\n"}],
 ['sa24', '--print-sort-args -k2,2r:ipv4 --check=diagnose-first',
  {OUT=>"sort -k1,1r --check=diagnose-first\n"}],
 ['sa25', '--print-sort-args -k2,2r:ipv4 --check=foobar12343124321123421432145',
  {OUT=>"sort -k1,1r --check=foobar12343124321123421432145\n"}],
 ['sa26', '--print-sort-args -k2,2r:ipv4 --check=quiet',
  {OUT=>"sort -k1,1r --check=quiet\n"}],
 ['sa27', '--print-sort-args -k7 -k2,2r:ipv4 -k5.3,6.2',
  {OUT=>"sort -k8 -k1,1r -k6.3,7.2\n"}],


 # Edge Cases
 ['g1', '--decorate -k1,1:roman' , {IN_PIPE=>""}, {OUT => ""}],

 # Header Lines
 ['h1', '-H --decorate -k2,2:ipv4', {IN_PIPE=>$in1},
  {OUT=>$out1_dec_ipv4_header1}],
 ['h2', '--header 1 --decorate -k2,2:ipv4', {IN_PIPE=>$in1},
  {OUT=>$out1_dec_ipv4_header1}],
 ['h3', '--header=1 --decorate -k2,2:ipv4', {IN_PIPE=>$in1},
  {OUT=>$out1_dec_ipv4_header1}],
 ['h4', '--header 2 --decorate -k2,2:ipv4', {IN_PIPE=>$in1},
  {OUT=>$out1_dec_ipv4_header2}],
 # More header lines than in the input
 ['h5', '--header 8 --decorate -k2,2:ipv4' , {IN_PIPE=>$in1}, {OUT => $in1}],
 ['h6', '--header 9 --decorate -k2,2:ipv4' , {IN_PIPE=>$in1}, {OUT => $in1}],

  # on a different architecture, would printf(%Lg) print something else?
  # Use OUT_SUBST to trim output to 1.3 digits
  #['b14', 'mean 1',     {IN_PIPE=>$in1},  {OUT => "5.454\n"},
  #    {OUT_SUBST=>'s/^(\d\.\d{3}).*/\1/'}],

  ## Some error checkings
  #['e1',  'sum',  {IN_PIPE=>""}, {EXIT=>1},
  #    {ERR=>"$prog: missing field for operation 'sum'\n"}],
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

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


my $ordering_flags_error_prefix="ordering flags (b/d/i/h/n/g/M/R/V) " .
    "cannot be combined with a conversion function: " .
    "invalid field specification";

my @Tests =
(
 ['e1','--foo-bar',{ERR=>"$prog: unrecognized option '--foo-bar'\n" .
                        "Try '$prog --help' for more information.\n"},
  {EXIT=>2}],
 ['e2','-k1,1:strlen no-such-file.txt', {EXIT=>2},
  {ERR=>"$prog: no-such-file.txt: No such file or directory\n"}],
 ['e3','--decorate -k1,1:strlen no-such-file.txt', {EXIT=>2},
  {ERR=>"$prog: no-such-file.txt: No such file or directory\n"}],
 ['e4','--undecorate 2 no-such-file.txt', {EXIT=>2},
  {ERR=>"$prog: no-such-file.txt: No such file or directory\n"}],

 ['e5','-k1,1%', {EXIT=>2},
  {ERR=>"$prog: invalid key specification: " .
        "invalid field specification '1,1%'\n"}],
 ['e6','-k1,1:', {EXIT=>2},
  {ERR=>"$prog: missing internal conversion function: " .
        "invalid field specification '1,1:'\n"}],
 ['e7','-k1,1@', {EXIT=>2},
  {ERR=>"$prog: missing external conversion command: " .
        "invalid field specification '1,1\@'\n"}],
 ['e8','-k1,1@foobar', {EXIT=>2},
  {ERR=>"$prog: external commands are not implemented (yet)\n"}],
 ['e9','-k1,1:foobar', {EXIT=>2},
  {ERR=>"$prog: invalid built-in conversion option: " .
        "invalid field specification '1,1:foobar'\n"}],

 ## Bad --key=KEYDEF syntax (should be identical to sort's behavior)
 ['e10','-k0', {EXIT=>2},
  {ERR=>"$prog: field number is zero: invalid field specification '0'\n"}],
 ['e11','-k1.0', {EXIT=>2},
  {ERR=>"$prog: character offset is zero: " .
        "invalid field specification '1.0'\n"}],
 ['e12','-k1.A', {EXIT=>2},
  {ERR=>"$prog: invalid number after '.': invalid count at start of 'A'\n"}],
 ['e13','-k1,0', {EXIT=>2},
  {ERR=>"$prog: field number is zero: invalid field specification '1,0'\n"}],
 ['e14','-k1,2.B', {EXIT=>2},
  {ERR=>"$prog: invalid number after '.': invalid count at start of 'B'\n"}],


 ## sort key ordering aren't allowed with conversion functions
 ## (except 'r' for reverse order)
 ['e20','-k1b,1:foobar', {EXIT=>2},
  {ERR=>"$prog: $ordering_flags_error_prefix '1b,1:foobar'\n"}],
 ['e21','-k1,1b:foobar', {EXIT=>2},
  {ERR=>"$prog: $ordering_flags_error_prefix '1,1b:foobar'\n"}],
 ['e22','-k1f,1:foobar', {EXIT=>2},
  {ERR=>"$prog: $ordering_flags_error_prefix '1f,1:foobar'\n"}],
 ['e23','-k1d,1:foobar', {EXIT=>2},
  {ERR=>"$prog: $ordering_flags_error_prefix '1d,1:foobar'\n"}],
 ['e24','-k1n,1:foobar', {EXIT=>2},
  {ERR=>"$prog: $ordering_flags_error_prefix '1n,1:foobar'\n"}],
 ['e25','-k1g,1:foobar', {EXIT=>2},
  {ERR=>"$prog: $ordering_flags_error_prefix '1g,1:foobar'\n"}],
 ['e26','-k1M,1:foobar', {EXIT=>2},
  {ERR=>"$prog: $ordering_flags_error_prefix '1M,1:foobar'\n"}],
 ['e27','-k1h,1:foobar', {EXIT=>2},
  {ERR=>"$prog: $ordering_flags_error_prefix '1h,1:foobar'\n"}],
 ['e28','-k1V,1:foobar', {EXIT=>2},
  {ERR=>"$prog: $ordering_flags_error_prefix '1V,1:foobar'\n"}],
 ['e29','-k1,1R:foobar', {EXIT=>2},
  {ERR=>"$prog: $ordering_flags_error_prefix '1,1R:foobar'\n"}],


 ['e40','-t "" -k1,1:foobar', {EXIT=>2},
  {ERR=>"$prog: empty tab\n"}],
 ['e41','-tab -k1,1:foobar', {EXIT=>2},
  {ERR=>"$prog: multi-character tab 'ab'\n"}],
 ['e42','-t: -t, -k1,1:foobar', {EXIT=>2},
  {ERR=>"$prog: incompatible tabs\n"}],


 ['e50','--undecorate 0', {EXIT=>2},
  {ERR=>"$prog: invalid number of fields to undecorate '0'\n"}],
 ['e51','--undecorate A', {EXIT=>2},
  {ERR=>"$prog: invalid number of fields to undecorate 'A'\n"}],
 ['e52','--undecorate -4', {EXIT=>2},
  {ERR=>"$prog: invalid number of fields to undecorate '-4'\n"}],

 ['e60','--decorate -k1,1:roman --undecorate 4', {EXIT=>2},
  {ERR=>"$prog: --decorate and --undecorate options are mutually exclusive\n"}],
 ['e61','-k1,1:roman --undecorate 4', {EXIT=>2},
  {ERR=>"$prog: --undecorate cannot be used with --keys or --decorate\n"}],
 ['e62','', {EXIT=>2},
  {ERR=>"$prog: missing -k/--key decoration or --undecorate options\n"}],

 ['e70', '--header X --decorate -k2,2:ipv4' , {EXIT=>2},
  {ERR=>"$prog: invalid number of header lines 'X'\n"}],


 # Conversion Errors
 ['c1', '--decorate -k1,1:roman' , {IN_PIPE=>"\n"}, {OUT => " "}, {EXIT=>2},
  {ERR=>"$prog: invalid empty roman numeral\n" .
        "$prog: conversion failed in line 1\n" }],
 ['c2', '--decorate -k2,2:roman' , {IN_PIPE=>"M"}, {OUT => " "}, {EXIT=>2},
  {ERR=>"$prog: invalid empty roman numeral\n" .
        "$prog: conversion failed in line 1\n" }],
 ['c3', '--decorate -k1,1:ipv4' , {IN_PIPE=>"FOO\n"}, {OUT => " "}, {EXIT=>2},
  {ERR=>"$prog: invalid dot-decimal IPv4 address 'FOO'\n" .
        "$prog: conversion failed in line 1\n" }],
 ['c4', '--decorate -k1,1:ipv4inet' , {IN_PIPE=>"FOO\n"}, {OUT => " "},
  {EXIT=>2},
  {ERR=>"$prog: invalid IPv4 address 'FOO'\n" .
        "$prog: conversion failed in line 1\n" }],
 ['c5', '--decorate -k1,1:ipv6' , {IN_PIPE=>"FOO\n"}, {OUT => " "}, {EXIT=>2},
  {ERR=>"$prog: invalid IPv6 address 'FOO'\n" .
        "$prog: conversion failed in line 1\n" }],


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

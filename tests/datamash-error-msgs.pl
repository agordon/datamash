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


# MAX_IDENTIFIER_LENGTH is defined in op-scanner.h
my $ident_too_long = "x" x 512 ;

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

my @Tests =
(
  # Invalid numeric value for column prasing should be treated as named column
  ['e1', 'sum 1x', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value '1x'\n"}],

  # Processing mode without operation
  ['e2','groupby 1', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing operation\n"}],

  # invalid operation after valid mode
  ['e3','groupby 1 foobar 2', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid operation 'foobar'\n"}],

  # missing field number after processing mode
  ['e4','groupby', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing field for operation 'groupby'\n"}],

  # Field range with invalid syntax
  ['e20','sum 1-',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field range for operation 'sum'\n"}],
  ['e21','sum 1-x',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: field range for 'sum' must be numeric\n"}],
  ['e22','sum 4-2',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field range for operation 'sum'\n"}],
  # zero in range
  ['e23','sum 0-2',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field '0' for operation 'sum'\n"}],
  ['e24','sum 1-0',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field '0' for operation 'sum'\n"}],
  #Negative in range
  ['e25','sum 1--5',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field range for operation 'sum'\n"}],

  # Test field pair syntaax
  ['e41','pcov 1', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: operation 'pcov' requires field pairs\n"}],
  ['e42','pcov 1:', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field pair for operation 'pcov'\n"}],
  ['e43','pcov :', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field pair for operation 'pcov'\n"}],
  ['e44','pcov :1', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field pair for operation 'pcov'\n"}],
  ['e46','pcov hello:world', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: -H or --header-in must be used with named columns\n"}],
  ['e47','sum 1:3', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: operation 'sum' cannot use pair of fields\n"}],

  # Test scanner edge-cases
  # Floating point value
  ['e60','sum 4.5',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field '4.5' for operation 'sum'\n"}],
  ['e61','sum 4.',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field '4.' for operation 'sum'\n"}],

  # invalid numbers
  ['e62','sum 4a',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value '4a'\n"}],
  ['e63','sum 4_',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value '4_'\n"}],
  # Overflow strtol
  ['e64','sum 1234567890123456789012345678901234567', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value " .
            "'1234567890123456789012345678901234567'\n"}],
  # Invalid charcters
  ['e65','sum "foo^bar"', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid operand '^bar'\n"}],

  # Empty columns
  ['e66','sum 1,,', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing field for operation 'sum'\n"}],

  # Range with names instead of numbers
  ['e67','sum foo-bar', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: field range for 'sum' must be numeric\n"}],

  # Invalid numeric value for column prasing should be treated as named column
  ['e70', 'sum 1x', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid numeric value '1x'\n"}],

  # Processing mode without operation
  ['e71','groupby 1', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing operation\n"}],

  # invalid operation after valid mode
  ['e72','groupby 1 foobar 2', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid operation 'foobar'\n"}],

  # missing field number after processing mode
  ['e73','groupby', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing field for operation 'groupby'\n"}],

  # Bin and optional parameters
  ['e80','bin:10:30 1', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: too many parameters for operation 'bin'\n"}],
  ['e81','bin: 1',      {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing parameter for operation 'bin'\n"}],

  # NOTE about the message: because the parser first parses parameters,
  #      then checks if the operation actually needs parameters, the
  #      error first complains about 'missing' because there are colons
  #      but no numeric values.
  ['e82','sum: 1',      {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing parameter for operation 'sum'\n"}],

  ['e83','bin:10: 1',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing parameter for operation 'bin'\n"}],

  # These ensures the '1' is not accidentally parsed as the field number
  ['e84','bin:10:1',    {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing field for operation 'bin'\n"}],
  ['e85','bin:10, 1',   {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing field for operation 'bin'\n"}],
  ['e86','bin:, 1',     {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid parameter , for operation 'bin'\n"}],
  ['e87','bin,  1',     {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: missing field for operation 'bin'\n"}],
  ['e88','bin:-  1',    {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid parameter - for operation 'bin'\n"}],
  ['e89','sum:10 1',    {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: too many parameters for operation 'sum'\n"}],

  # Invalid field specifications for primary operations
  ['e90',  'groupby 1:2', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field pair for operation 'groupby'\n"}],
  ['e91',  'groupby 1-2', {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid field range for operation 'groupby'\n"}],

  # values for strbin operation
  ['e92','strbin:-  1',    {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid parameter - for operation 'strbin'\n"}],
  ['e93','strbin:0  1',    {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: strbin bucket size must not be zero\n"}],

   # values for percentile operation
  ['e94','perc:0  1',    {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid percentile value 0\n"}],
  ['e95','perc:101  1',    {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid percentile value 101\n"}],
  ['e96','perc:foo  1',    {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid parameter foo for operation 'perc'\n"}],
  ['e97','perc:-32  1',    {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: invalid parameter - for operation 'perc'\n"}],
  ['e98','perc:1:2  1',    {IN_PIPE=>""}, {EXIT=>1},
      {ERR=>"$prog: too many parameters for operation 'perc'\n"}],

  # Invalid output delimiters
  ['e100', '--output-delimiter', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: option '--output-delimiter' requires an argument\n".
         "Try '$prog --help' for more information.\n"}],
  ['e101', '--output-delimiter ""', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: the delimiter must be a single character\n"}],
  ['e102', '--output-delimiter "XX"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: the delimiter must be a single character\n"}],

   # values for trimmean operation
  ['e103','trimmean:12  1',    {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: invalid trim mean value 12 (expected 0 <= X <= 0.5)\n"}],
  ['e104','trimmean:0.51  1',    {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: invalid trim mean value 0.51 (expected 0 <= X <= 0.5)\n"}],
  ['e105','trimmean:-32  1',    {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: invalid parameter - for operation 'trimmean'\n"}],
  ['e106','trimmean:1:2  1',    {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: too many parameters for operation 'trimmean'\n"}],

  # Rounding
  ['e110','--round ""', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: missing rounding digits value\n"}],
  ['e111','--round "3a"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: invalid rounding digits value '3a'\n"}],
  # Rounding Currently hard-coded to 1 to 50 decimal-point digits.
  ['e112','--round "0"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: invalid rounding digits value '0'\n"}],
  ['e113','--round "51"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: invalid rounding digits value '51'\n"}],

  # Custom Output Formats
  ['e120','--format ""', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: format '' has no % directive\n"}],
  ['e121','--format "foobar"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: format 'foobar' has no % directive\n"}],
  ['e122','--format "aa%%ff"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: format 'aa%%ff' has no % directive\n"}],
  ['e123','--format "%Lg"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: format '%Lg' has unknown/invalid type %L directive\n"}],
  ['e124','--format "%*g"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: format '%*g' has unknown/invalid type %* directive\n"}],
  ['e125','--format "%g %f"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: format '%g %f' has too many % directives\n"}],
  ['e126','--format "%"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: format '%' missing valid type after '%'\n"}],
  ['e127','--format "%3"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: format '%3' missing valid type after '%'\n"}],
  ['e128','--format "%#.4"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: format '%#.4' missing valid type after '%'\n"}],
  ['e129','--format "%f%"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: format '%f%' has too many % directives\n"}],
  ['e130','--format "%f%3"', {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: format '%f%3' has too many % directives\n"}],


  # identifier scanning errors
  ['e140',"-H sum 'foo\\'", {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: backslash at end of identifier\n"}],
  ['e141',"-H sum $ident_too_long", {IN_PIPE=>""}, {EXIT=>1},
   {ERR=>"$prog: identifier name too long\n"}],
);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

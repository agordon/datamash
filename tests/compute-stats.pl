#!/usr/bin/perl
=pod
  Unit Tests for compute - perform simple calculation on input data
  Copyright (C) 2013-2014 Assaf Gordon.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
   Written by Assaf Gordon
=cut
use strict;
use warnings;
#
##
## This script tests the statistics-related operations.
##


# Until a better way comes along to auto-use Coreutils Perl modules
# as in the coreutils' autotools system.
use Coreutils;
use CuSkip;
use CuTmpdir qw(compute);

(my $program_name = $0) =~ s|.*/||;
my $prog = 'compute';

# TODO: add localization tests with "grouping"
# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

=pod
Helper function, given a list of items,
returns a string scalar of the items with newlines.

Example:
   my $a = c(1,2,3,4);
   $a == "1\n2\n3\n4\n"
=cut
sub c
{
  return join "", map { "$_\n" } @_ ;
}

my $seq1 = c( 1,2,3,4 );
my $seq2 = c( 1,2,3 );
my $seq3 = c( 2 );

# These sequences are taken from the nice illustration
# of R's 'summary()' function at:
# http://tolstoy.newcastle.edu.au/R/e17/help/att-1067/Quartiles_in_R.pdf
my @data12_unsorted = qw/23 11 19 37 13 7 2 3 29 31 5 17/;
my @data12 = sort { $a <=> $b } @data12_unsorted;
my @data11 = qw/2 3 5 7 11 13 17 19 23 29 31/;
my @data10 = qw/2 3 5 7 11 13 17 19 23 29/;
my @data9  = qw/2 3 5 7 11 13 17 19 23/;

my $seq12_unsorted = c(@data12_unsorted);
my $seq12   = c(@data12);
my $seq11   = c(@data11);
my $seq10   = c(@data10);
my $seq9    = c(@data9);

##
## NOTE:
##   When comparing results, only the first three digits after decimal point
##   should be checked. See below for the trimming OUT_SUBST code.
##
my @Tests =
(
  # Test mean
  ['mean1', 'mean 1' ,  {IN_PIPE=>$seq1},  {OUT => "2.5\n"}],
  ['mean2', 'mean 1' ,  {IN_PIPE=>$seq2},  {OUT => "2\n"}],
  ['mean3', 'mean 1' ,  {IN_PIPE=>$seq3},  {OUT => "2\n"}],
  # sorted/unsorted should not change the result.
  ['mean4', 'mean 1' ,  {IN_PIPE=>$seq12}, {OUT => "16.416\n"},],
  ['mean5', 'mean 1' ,  {IN_PIPE=>$seq12_unsorted}, {OUT => "16.416\n"},],
  ['mean6', '--sort mean 1' ,  {IN_PIPE=>$seq12_unsorted}, {OUT => "16.416\n"},],
  ['mean7', 'mean 1' ,  {IN_PIPE=>$seq11}, {OUT => "14.545\n"},],
  ['mean8', 'mean 1' ,  {IN_PIPE=>$seq10}, {OUT => "12.9\n"}],
  ['mean9', 'mean 1' ,  {IN_PIPE=>$seq9},  {OUT => "11.111\n"},],

  # Test median
  ['med1', 'median 1' ,  {IN_PIPE=>$seq1},  {OUT => "2.5\n"}],
  ['med2', 'median 1' ,  {IN_PIPE=>$seq2},  {OUT => "2\n"}],
  ['med3', 'median 1' ,  {IN_PIPE=>$seq3},  {OUT => "2\n"}],
  # sorted/unsorted should not change the result.
  ['med4', 'median 1' ,  {IN_PIPE=>$seq12},  {OUT => "15\n"}],
  ['med5', 'median 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "15\n"}],
  ['med6', '--sort median 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "15\n"}],
  ['med7', 'median 1' ,  {IN_PIPE=>$seq11},  {OUT => "13\n"}],
  ['med8', 'median 1' ,  {IN_PIPE=>$seq10},  {OUT => "12\n"}],
  ['med9', 'median 1' ,  {IN_PIPE=>$seq9},   {OUT => "11\n"}],

  # Test Q1
  ['q1_1', 'q1 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.75\n"}],
  ['q1_2', 'q1 1' ,  {IN_PIPE=>$seq2},   {OUT => "1.5\n"}],
  ['q1_3', 'q1 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  # sorted/unsorted should not change the result.
  ['q1_4', 'q1 1' ,  {IN_PIPE=>$seq12},  {OUT => "6.5\n"}],
  ['q1_5', 'q1 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "6.5\n"}],
  ['q1_6', '--sort q1 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "6.5\n"}],
  ['q1_7', 'q1 1' ,  {IN_PIPE=>$seq11},  {OUT => "6\n"}],
  ['q1_8', 'q1 1' ,  {IN_PIPE=>$seq10},  {OUT => "5.5\n"}],
  ['q1_9', 'q1 1' ,  {IN_PIPE=>$seq9},   {OUT => "5\n"}],

  # Test Q3
  ['q3_1', 'q3 1' ,  {IN_PIPE=>$seq1},   {OUT => "3.25\n"}],
  ['q3_2', 'q3 1' ,  {IN_PIPE=>$seq2},   {OUT => "2.5\n"}],
  ['q3_3', 'q3 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  # sorted/unsorted should not change the result.
  ['q3_4', 'q3 1' ,  {IN_PIPE=>$seq12},  {OUT => "24.5\n"}],
  ['q3_5', 'q3 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "24.5\n"}],
  ['q3_6', '--sort q3 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "24.5\n"}],
  ['q3_7', 'q3 1' ,  {IN_PIPE=>$seq11},  {OUT => "21\n"}],
  ['q3_8', 'q3 1' ,  {IN_PIPE=>$seq10},  {OUT => "18.5\n"}],
  ['q3_9', 'q3 1' ,  {IN_PIPE=>$seq9},   {OUT => "17\n"}],

  # Test IQR
  ['iqr_1', 'iqr 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.5\n"}],
  ['iqr_2', 'iqr 1' ,  {IN_PIPE=>$seq2},   {OUT => "1\n"}],
  ['iqr_3', 'iqr 1' ,  {IN_PIPE=>$seq3},   {OUT => "0\n"}],
  ['iqr_4', 'iqr 1' ,  {IN_PIPE=>$seq12},  {OUT => "18\n"}],
  ['iqr_5', 'iqr 1' ,  {IN_PIPE=>$seq11},  {OUT => "15\n"}],
  ['iqr_6', 'iqr 1' ,  {IN_PIPE=>$seq10},  {OUT => "13\n"}],
  ['iqr_7', 'iqr 1' ,  {IN_PIPE=>$seq9},   {OUT => "12\n"}],

  # Test sample standard deviation
  ['sstdev_1', 'sstdev 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.290\n"}],
  ['sstdev_2', 'sstdev 1' ,  {IN_PIPE=>$seq2},   {OUT => "1\n"}],
  ['sstdev_3', 'sstdev 1' ,  {IN_PIPE=>$seq3},   {OUT => "-nan\n"}],
  ['sstdev_4', 'sstdev 1' ,  {IN_PIPE=>$seq12},  {OUT => "11.649\n"}],
  ['sstdev_5', 'sstdev 1' ,  {IN_PIPE=>$seq11},  {OUT => "10.152\n"}],
  ['sstdev_6', 'sstdev 1' ,  {IN_PIPE=>$seq10},  {OUT => "9.024\n"}],
  ['sstdev_7', 'sstdev 1' ,  {IN_PIPE=>$seq9},   {OUT => "7.457\n"}],

  # Test population standard deviation
  ['pstdev_1', 'pstdev 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.118\n"}],
  ['pstdev_2', 'pstdev 1' ,  {IN_PIPE=>$seq2},   {OUT => "0.816\n"}],
  ['pstdev_3', 'pstdev 1' ,  {IN_PIPE=>$seq3},   {OUT => "0\n"}],
  ['pstdev_4', 'pstdev 1' ,  {IN_PIPE=>$seq12},  {OUT => "11.153\n"}],
  ['pstdev_5', 'pstdev 1' ,  {IN_PIPE=>$seq11},  {OUT => "9.680\n"}],
  ['pstdev_6', 'pstdev 1' ,  {IN_PIPE=>$seq10},  {OUT => "8.560\n"}],
  ['pstdev_7', 'pstdev 1' ,  {IN_PIPE=>$seq9},   {OUT => "7.030\n"}],

  # Test sample variance
  ['svar_1', 'svar 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.666\n"}],
  ['svar_2', 'svar 1' ,  {IN_PIPE=>$seq2},   {OUT => "1\n"}],
  ['svar_3', 'svar 1' ,  {IN_PIPE=>$seq3},   {OUT => "-nan\n"}],
  ['svar_4', 'svar 1' ,  {IN_PIPE=>$seq12},  {OUT => "135.719\n"}],
  ['svar_5', 'svar 1' ,  {IN_PIPE=>$seq11},  {OUT => "103.072\n"}],
  ['svar_6', 'svar 1' ,  {IN_PIPE=>$seq10},  {OUT => "81.433\n"}],
  ['svar_7', 'svar 1' ,  {IN_PIPE=>$seq9},   {OUT => "55.611\n"}],

  # Test population variance
  ['pvar_1', 'pvar 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.25\n"}],
  ['pvar_2', 'pvar 1' ,  {IN_PIPE=>$seq2},   {OUT => "0.666\n"}],
  ['pvar_3', 'pvar 1' ,  {IN_PIPE=>$seq3},   {OUT => "0\n"}],
  ['pvar_4', 'pvar 1' ,  {IN_PIPE=>$seq12},  {OUT => "124.409\n"}],
  ['pvar_5', 'pvar 1' ,  {IN_PIPE=>$seq11},  {OUT => "93.702\n"}],
  ['pvar_6', 'pvar 1' ,  {IN_PIPE=>$seq10},  {OUT => "73.29\n"}],
  ['pvar_7', 'pvar 1' ,  {IN_PIPE=>$seq9},   {OUT => "49.432\n"}],

  # Test MAD (Median Absolute Deviation), with default
  # scaling factor of 1.486 for normal distributions
  ['mad_1', 'mad 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.482\n"}],
  ['mad_2', 'mad 1' ,  {IN_PIPE=>$seq2},   {OUT => "1.482\n"}],
  ['mad_3', 'mad 1' ,  {IN_PIPE=>$seq3},   {OUT => "0\n"}],
  ['mad_4', 'mad 1' ,  {IN_PIPE=>$seq12},  {OUT => "13.343\n"}],
  ['mad_5', 'mad 1' ,  {IN_PIPE=>$seq11},  {OUT => "11.860\n"}],
  ['mad_6', 'mad 1' ,  {IN_PIPE=>$seq10},  {OUT => "10.378\n"}],
  ['mad_7', 'mad 1' ,  {IN_PIPE=>$seq9},   {OUT => "8.895\n"}],

  # Test MAD-Raw (Median Absolute Deviation), with scaling factor of 1
  ['madraw_1', 'madraw 1' ,  {IN_PIPE=>$seq1},   {OUT => "1\n"}],
  ['madraw_2', 'madraw 1' ,  {IN_PIPE=>$seq2},   {OUT => "1\n"}],
  ['madraw_3', 'madraw 1' ,  {IN_PIPE=>$seq3},   {OUT => "0\n"}],
  ['madraw_4', 'madraw 1' ,  {IN_PIPE=>$seq12},  {OUT => "9\n"}],
  ['madraw_5', 'madraw 1' ,  {IN_PIPE=>$seq11},  {OUT => "8\n"}],
  ['madraw_6', 'madraw 1' ,  {IN_PIPE=>$seq10},  {OUT => "7\n"}],
  ['madraw_7', 'madraw 1' ,  {IN_PIPE=>$seq9},   {OUT => "6\n"}],

);

##
## For each test, trim the resulting value to maximum three digits
## after the decimal point.
##
for my $t (@Tests) {
 push @{$t}, {OUT_SUBST=>'s/^(\d+\.\d{1,3}).*/\1/'};
}

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

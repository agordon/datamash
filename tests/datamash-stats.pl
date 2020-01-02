#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2020 Assaf Gordon <assafgordon@gmail.com

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
#
##
## This script tests the statistics-related operations.
##


# Until a better way comes along to auto-use Coreutils Perl modules
# as in the coreutils' autotools system.
use Coreutils;
use CuSkip;
use CuTmpdir qw(datamash);

(my $program_name = $0) =~ s|.*/||;
my $prog = 'datamash';

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

=pod
 rep(value,count) = returns a list of 'values' repeated 'count' times.
 Akin to R's "rep()" function.

 my @a = rep(65,4)
 @a == ( 65, 65, 65, 65 )
=cut
sub rep
{
  my ($value,$count) = @_;
  my @result = map { $value } ( 1 .. $count ) ;
  return @result ;
}

my $seq1 = c( 1,2,3,4 );
my $seq2 = c( 1,2,3 );
my $seq3 = c( 2 );
my $seq4 = c( 1,2 );

# These sequences are taken from the nice illustration
# of R's 'summary()' function at:
# http://tolstoy.newcastle.edu.au/R/e17/help/att-1067/Quartiles_in_R.pdf
my @data9  = qw/2 3 5 7 11 13 17 19 23/;
my @data10 = qw/2 3 5 7 11 13 17 19 23 29/;
my @data11 = qw/2 3 5 7 11 13 17 19 23 29 31/;
my @data12_unsorted = qw/23 11 19 37 13 7 2 3 29 31 5 17/;
my @data12 = sort { $a <=> $b } @data12_unsorted;

my $seq9    = c(@data9);
my $seq10   = c(@data10);
my $seq11   = c(@data11);
my $seq12_unsorted = c(@data12_unsorted);
my $seq12   = c(@data12);

# The following random sequence was generated using R:
#   floor(rnorm(100,sd=10,mean=100))
# It is drawn from a normal distribution, and should have near-zero skewness.
my $seq20 = c(117,89,86,101,90,110,97,91,106,99,118,110,82,91,105,90,108,96,
              92,103,107,87,100,101,101,86,97,94,97,87,114,104,97,107,94,117,
              100,111,111,113,93,113,107,108,95,117,106,105,105,105,105,99,
              91,103,84,99,99,99,108,94,103,96,93,90,107,111,103,98,103,96,
              113,111,84,109,96,110,90,78,111,85,102,91,107,99,120,109,107,
              92,106,86,108,107,104,78,100,97,99,86,98,82);
# A non-symetric values, skewness should be large
my $seq21 = c(63,13,64,23,86,61,76,28,84,27,38,40,15,29,120,56,59,33,73,103,
              15,22,36,45,40,35,3,114,66,55,16,17,29,30,42,32,34,110,16,33,
              57,35,48,78,35,84,20,83,78,49,26,29,50,41,23,21,24,79,92,41,
              77,64,12,31,6,32,22,8,19,27,14,12,64,51,29,33,24,58,1,56,47,
              98,44,33,18,38,5,33,17,21,116,169,57,40,2,59,88,42,68,23);
# Sequence from worked example at:
#   http://www.tc3.edu/instruct/sbrown/stat/shape.htm
my $seq22 = c(61,61,61,61,61,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
              64,64,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,
              67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,
              67,67,70,70,70,70,70,70,70,70,70,70,70,70,70,70,70,70,70,70,70,
              70,70,70,70,70,70,70,70,73,73,73,73,73,73,73,73);
# Sequence from worked example:
#   http://www.tc3.edu/instruct/sbrown/stat/shape.htm
# "Example 2: Size of Rat Litters"
my $seq23 =  c(rep(1,7),rep(2,33),rep(3,58),rep(4,116),rep(5,125),rep(6,126),
               rep(7,121),rep(8,107),rep(9,56),rep(10,37),rep(11,25),rep(12,4));

=pod
The datamash tests below should return the same results are thes R commands:

    seq1  = c(1,2,3,4)
    seq2  = c(1,2,3)
    seq3  = c(2)
    seq9  = c(2,3,5,7,11,13,17,19,23)
    seq10 = c(2,3,5,7,11,13,17,19,23,29)
    seq11 = c(2,3,5,7,11,13,17,19,23,29,31)
    seq12 = c(2,3,5,7,11,13,17,19,23,29,31,37)
    seq20 = c(117,89,86,101,90,110,97,91,106,99,118,110,82,91,105,90,108,96,
              92,103,107,87,100,101,101,86,97,94,97,87,114,104,97,107,94,117,
              100,111,111,113,93,113,107,108,95,117,106,105,105,105,105,99,
              91,103,84,99,99,99,108,94,103,96,93,90,107,111,103,98,103,96,
              113,111,84,109,96,110,90,78,111,85,102,91,107,99,120,109,107,
              92,106,86,108,107,104,78,100,97,99,86,98,82)
    seq21 = c(63,13,64,23,86,61,76,28,84,27,38,40,15,29,120,56,59,33,73,103,
              15,22,36,45,40,35,3,114,66,55,16,17,29,30,42,32,34,110,16,33,
              57,35,48,78,35,84,20,83,78,49,26,29,50,41,23,21,24,79,92,41,
              77,64,12,31,6,32,22,8,19,27,14,12,64,51,29,33,24,58,1,56,47,
              98,44,33,18,38,5,33,17,21,116,169,57,40,2,59,88,42,68,23)
    seq22 = c(61,61,61,61,61,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
              64,64,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,
              67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,
              67,67,70,70,70,70,70,70,70,70,70,70,70,70,70,70,70,70,70,70,70,
              70,70,70,70,70,70,70,70,73,73,73,73,73,73,73,73)
    seq23 =  c(rep(1,7),rep(2,33),rep(3,58),rep(4,116),rep(5,125),rep(6,126),
               rep(7,121),rep(8,107),rep(9,56),rep(10,37),rep(11,25),rep(12,4))

    # define a population-flavor variance and sd functions
    # (R's default are sample-variance and sample-sd)
    pop.var=function(x)(var(x)*(length(x)-1)/length(x))
    pop.sd=function(x)(sqrt(var(x)*(length(x)-1)/length(x)))
    smp.var=var
    smp.sd=sd

    # Helper functions for quartiles
    q1=function(x) { quantile(x, prob=0.25) }
    q3=function(x) { quantile(x, prob=0.75) }

    # Helper function for 'range'
    # R's built-in range() function returns a vector of min/max.
    # datamash's range returns the actual range
    dm_range=function(x) { t = range(x) ; return(t[2]-t[1]) }

    # Helper functions for quartiles
    perc1=function(x) { quantile(x, prob=0.01) }
    perc90=function(x) { quantile(x, prob=0.90) }
    perc95=function(x) { quantile(x, prob=0.95) }
    perc99=function(x) { quantile(x, prob=0.99) }
    perc100=function(x) { quantile(x, prob=1) }

    trimmean0=function(x){mean(x,trim=0)}
    trimmean10=function(x){mean(x,trim=0.1)}
    trimmean20=function(x){mean(x,trim=0.2)}
    trimmean30=function(x){mean(x,trim=0.3)}
    trimmean40=function(x){mean(x,trim=0.4)}
    trimmean50=function(x){mean(x,trim=0.5)}


    # Helper function for madraw
    madraw=function(x) { mad(x,constant=1.0) }

    # 'moments' library contains skewness and kurtosis
    library(moments)

    # helper functions for skewness
    pop.skewness=skewness
    smp.skewness=function(x) {
            skewness(x) * sqrt( length(x)*(length(x)-1) ) / (length(x)-2) }

    # helper functions for excess kurtosis
    pop.excess_kurtosis=function(x) { kurtosis(x)-3 }
    smp.excess_kurtosis=function(x) {
          n=length(x)
          ( (n-1) / ( (n-2)*(n-3) ) ) *
             ( (n+1) * pop.excess_kurtosis(x) + 6 )
     }

    # Helper function for SES - Standard Error of Skewness
    SES = function(n) { sqrt( (6*n*(n-1)) / ( (n-2)*(n+1)*(n+3) ) ) }
    # Helper function for SEK - Standard ERror of Kurtosis
    SEK = function(n) { 2*SES(n) * sqrt ( (n*n-1) / ( (n-3)*(n+5)  ) ) }

    # Helper function for Skewness Test Statistics
    skewnessZ = function(x) { smp.skewness(x)/SES(length(x)) }
    # Helper function for Kurtosis  Test Statistics
    kurtosisZ = function(x) { smp.excess_kurtosis(x)/SEK(length(x)) }

    # Helper function for Jarque-Bera pValue
    jarque.bera.pvalue=function(x) { t=jarque.test(x) ; t$p.value }

    # Helper function for D'Agostino-Pearson Omnibus Test for normality
    dpo=function(x) {
         DP = skewnessZ(x)^2 + kurtosisZ(x)^2
         pval = 1.0 - pchisq(DP,df=2)
       }

    # Helper function to execute function 'f' on all input sequences
    test=function(f) {
         f_name =  deparse(substitute(f))
         cat(f_name,"(seq1)=",  f(seq1), "\n")
         cat(f_name,"(seq2)=",  f(seq2), "\n")
         cat(f_name,"(seq3)=",  f(seq3), "\n")
         cat(f_name,"(seq9)=",  f(seq9), "\n")
         cat(f_name,"(seq10)=", f(seq10),"\n")
         cat(f_name,"(seq11)=", f(seq11),"\n")
         cat(f_name,"(seq12)=", f(seq12),"\n")
         cat(f_name,"(seq20)=", f(seq20),"\n")
         cat(f_name,"(seq21)=", f(seq21),"\n")
         cat(f_name,"(seq22)=", f(seq22),"\n")
         cat(f_name,"(seq23)=", f(seq23),"\n")
    }

    # Run tests
    test(range)
    test(mean)
    test(median)
    test(q1)
    test(q3)
    test(perc90)
    test(perc95)
    test(perc99)
    test(trimmean0)
    test(trimmean10)
    test(trimmean20)
    test(trimmean30)
    test(trimmean40)
    test(trimmean50)
    test(iqr)
    test(smp.sd)
    test(pop.sd)
    test(smp.var)
    test(pop.var)
    test(mad)
    test(madraw)
    test(skewness)
    test(smp.skewness)
    test(pop.excess_kurtosis)
    test(smp.excess_kurtosis)
    test(jarque.bera.pvalue)

=cut


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
  ['mean4', 'mean 1' ,  {IN_PIPE=>$seq9},  {OUT => "11.111\n"},],
  ['mean5', 'mean 1' ,  {IN_PIPE=>$seq10}, {OUT => "12.9\n"}],
  ['mean6', 'mean 1' ,  {IN_PIPE=>$seq12}, {OUT => "16.416\n"},],
  ['mean7', 'mean 1' ,  {IN_PIPE=>$seq11}, {OUT => "14.545\n"},],
  ['mean8', 'mean 1' ,  {IN_PIPE=>$seq12_unsorted}, {OUT => "16.416\n"},],
  ['mean9', '--sort mean 1' ,
     {IN_PIPE=>$seq12_unsorted}, {OUT => "16.416\n"},],
  ['mean10','mean 1' ,  {IN_PIPE=>$seq20}, {OUT => "100.06\n"},],
  ['mean11','mean 1' ,  {IN_PIPE=>$seq21}, {OUT => "45.32\n"},],
  ['mean12','mean 1' ,  {IN_PIPE=>$seq22}, {OUT => "67.45\n"},],
  ['mean13','mean 1' ,  {IN_PIPE=>$seq23}, {OUT => "6.125\n"},],

  # Test median
  ['med1', 'median 1' ,  {IN_PIPE=>$seq1},  {OUT => "2.5\n"}],
  ['med2', 'median 1' ,  {IN_PIPE=>$seq2},  {OUT => "2\n"}],
  ['med3', 'median 1' ,  {IN_PIPE=>$seq3},  {OUT => "2\n"}],
  # sorted/unsorted should not change the result.
  ['med4', 'median 1' ,  {IN_PIPE=>$seq9},   {OUT => "11\n"}],
  ['med5', 'median 1' ,  {IN_PIPE=>$seq10},  {OUT => "12\n"}],
  ['med6', 'median 1' ,  {IN_PIPE=>$seq11},  {OUT => "13\n"}],
  ['med7', 'median 1' ,  {IN_PIPE=>$seq12},  {OUT => "15\n"}],
  ['med8', 'median 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "15\n"}],
  ['med9', '--sort median 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "15\n"}],
  ['med10','median 1' ,  {IN_PIPE=>$seq20}, {OUT => "100\n"},],
  ['med11','median 1' ,  {IN_PIPE=>$seq21}, {OUT => "37\n"},],
  ['med12','median 1' ,  {IN_PIPE=>$seq22}, {OUT => "67\n"},],
  ['med13','median 1' ,  {IN_PIPE=>$seq23}, {OUT => "6\n"},],

  # Test Q1
  ['q1_1', 'q1 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.75\n"}],
  ['q1_2', 'q1 1' ,  {IN_PIPE=>$seq2},   {OUT => "1.5\n"}],
  ['q1_3', 'q1 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  # sorted/unsorted should not change the result.
  ['q1_4', 'q1 1' ,  {IN_PIPE=>$seq9},   {OUT => "5\n"}],
  ['q1_5', 'q1 1' ,  {IN_PIPE=>$seq10},  {OUT => "5.5\n"}],
  ['q1_6', 'q1 1' ,  {IN_PIPE=>$seq11},  {OUT => "6\n"}],
  ['q1_7', 'q1 1' ,  {IN_PIPE=>$seq12},  {OUT => "6.5\n"}],
  ['q1_8', 'q1 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "6.5\n"}],
  ['q1_9', '--sort q1 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "6.5\n"}],
  ['q1_10','q1 1' ,  {IN_PIPE=>$seq20},  {OUT => "93\n"},],
  ['q1_11','q1 1' ,  {IN_PIPE=>$seq21},  {OUT => "23\n"},],
  ['q1_12','q1 1' ,  {IN_PIPE=>$seq22},  {OUT => "67\n"},],
  ['q1_13','q1 1' ,  {IN_PIPE=>$seq23},  {OUT => "4\n"},],

  # Test Q3
  ['q3_1', 'q3 1' ,  {IN_PIPE=>$seq1},   {OUT => "3.25\n"}],
  ['q3_2', 'q3 1' ,  {IN_PIPE=>$seq2},   {OUT => "2.5\n"}],
  ['q3_3', 'q3 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  # sorted/unsorted should not change the result.
  ['q3_4', 'q3 1' ,  {IN_PIPE=>$seq9},   {OUT => "17\n"}],
  ['q3_5', 'q3 1' ,  {IN_PIPE=>$seq10},  {OUT => "18.5\n"}],
  ['q3_6', 'q3 1' ,  {IN_PIPE=>$seq11},  {OUT => "21\n"}],
  ['q3_7', 'q3 1' ,  {IN_PIPE=>$seq12},  {OUT => "24.5\n"}],
  ['q3_8', 'q3 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "24.5\n"}],
  ['q3_9', '--sort q3 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "24.5\n"}],
  ['q3_10','q3 1' ,  {IN_PIPE=>$seq20},  {OUT => "107\n"},],
  ['q3_11','q3 1' ,  {IN_PIPE=>$seq21},  {OUT => "61.5\n"},],
  ['q3_12','q3 1' ,  {IN_PIPE=>$seq22},  {OUT => "70\n"},],
  ['q3_13','q3 1' ,  {IN_PIPE=>$seq23},  {OUT => "8\n"},],

  # Test range
  ['range_1', 'range 1' ,  {IN_PIPE=>$seq1},   {OUT => "3\n"}],
  ['range_2', 'range 1' ,  {IN_PIPE=>$seq2},   {OUT => "2\n"}],
  ['range_3', 'range 1' ,  {IN_PIPE=>$seq3},   {OUT => "0\n"}],
  ['range_4', 'range 1' ,  {IN_PIPE=>$seq9},   {OUT => "21\n"}],
  ['range_5', 'range 1' ,  {IN_PIPE=>$seq10},  {OUT => "27\n"}],
  ['range_6', 'range 1' ,  {IN_PIPE=>$seq11},  {OUT => "29\n"}],
  ['range_7', 'range 1' ,  {IN_PIPE=>$seq12},  {OUT => "35\n"}],
  ['range_8', 'range 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "35\n"}],
  ['range_9', '--sort range 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "35\n"}],
  ['range_10','range 1' ,  {IN_PIPE=>$seq20},  {OUT => "42\n"},],
  ['range_11','range 1' ,  {IN_PIPE=>$seq21},  {OUT => "168\n"},],
  ['range_12','range 1' ,  {IN_PIPE=>$seq22},  {OUT => "12\n"},],
  ['range_13','range 1' ,  {IN_PIPE=>$seq23},  {OUT => "11\n"},],


  # Test perc:90
  ['perc90_1', 'perc:90 1' ,  {IN_PIPE=>$seq1},   {OUT => "3.7\n"}],
  ['perc90_2', 'perc:90 1' ,  {IN_PIPE=>$seq2},   {OUT => "2.8\n"}],
  ['perc90_3', 'perc:90 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  # sorted/unsorted should not change the result.
  ['perc90_4', 'perc:90 1' ,  {IN_PIPE=>$seq9},   {OUT => "19.8\n"}],
  ['perc90_5', 'perc:90 1' ,  {IN_PIPE=>$seq10},  {OUT => "23.6\n"}],
  ['perc90_6', 'perc:90 1' ,  {IN_PIPE=>$seq11},  {OUT => "29\n"}],
  ['perc90_7', 'perc:90 1' ,  {IN_PIPE=>$seq12},  {OUT => "30.8\n"}],
  ['perc90_8', 'perc:90 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "30.8\n"}],
  ['perc90_9', '--sort perc:90 1',
    {IN_PIPE=>$seq12_unsorted},  {OUT => "30.8\n"}],
  ['perc90_10','perc:90 1' ,  {IN_PIPE=>$seq20},  {OUT => "111\n"},],
  ['perc90_11','perc:90 1' ,  {IN_PIPE=>$seq21},  {OUT => "84.2\n"},],
  ['perc90_12','perc:90 1' ,  {IN_PIPE=>$seq22},  {OUT => "70\n"},],
  ['perc90_13','perc:90 1' ,  {IN_PIPE=>$seq23},  {OUT => "9\n"},],

  # Test perc:95
  ['perc95_1', 'perc:95 1' ,  {IN_PIPE=>$seq1},   {OUT => "3.85\n"}],
  ['perc95_2', 'perc:95 1' ,  {IN_PIPE=>$seq2},   {OUT => "2.9\n"}],
  ['perc95_3', 'perc:95 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  # sorted/unsorted should not change the result.
  ['perc95_4', 'perc:95 1' ,  {IN_PIPE=>$seq9},   {OUT => "21.4\n"}],
  ['perc95_5', 'perc:95 1' ,  {IN_PIPE=>$seq10},  {OUT => "26.3\n"}],
  ['perc95_6', 'perc:95 1' ,  {IN_PIPE=>$seq11},  {OUT => "30\n"}],
  ['perc95_7', 'perc:95 1' ,  {IN_PIPE=>$seq12},  {OUT => "33.7\n"}],
  ['perc95_8', 'perc:95 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "33.7\n"}],
  ['perc95_9', '--sort perc:95 1',
    {IN_PIPE=>$seq12_unsorted},  {OUT => "33.7\n"}],
  ['perc95_10','perc:95 1' ,  {IN_PIPE=>$seq20},  {OUT => "114.15\n"},],
  ['perc95_11','perc:95 1' ,  {IN_PIPE=>$seq21},  {OUT => "103.35\n"},],
  ['perc95_12','perc:95 1' ,  {IN_PIPE=>$seq22},  {OUT => "73\n"},],
  ['perc95_13','perc:95 1' ,  {IN_PIPE=>$seq23},  {OUT => "10\n"},],

  # Test perc:99
  ['perc99_1', 'perc:99 1' ,  {IN_PIPE=>$seq1},   {OUT => "3.97\n"}],
  ['perc99_2', 'perc:99 1' ,  {IN_PIPE=>$seq2},   {OUT => "2.98\n"}],
  ['perc99_3', 'perc:99 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  # sorted/unsorted should not change the result.
  ['perc99_4', 'perc:99 1' ,  {IN_PIPE=>$seq9},   {OUT => "22.68\n"}],
  ['perc99_5', 'perc:99 1' ,  {IN_PIPE=>$seq10},  {OUT => "28.46\n"}],
  ['perc99_6', 'perc:99 1' ,  {IN_PIPE=>$seq11},  {OUT => "30.8\n"}],
  ['perc99_7', 'perc:99 1' ,  {IN_PIPE=>$seq12},  {OUT => "36.34\n"}],
  ['perc99_8', 'perc:99 1' ,  {IN_PIPE=>$seq12_unsorted},  {OUT => "36.34\n"}],
  ['perc99_9', '--sort perc:99 1',
    {IN_PIPE=>$seq12_unsorted},  {OUT => "36.34\n"}],
  ['perc99_10','perc:99 1' ,  {IN_PIPE=>$seq20},  {OUT => "118.02\n"},],
  ['perc99_11','perc:99 1' ,  {IN_PIPE=>$seq21},  {OUT => "120.49\n"},],
  ['perc99_12','perc:99 1' ,  {IN_PIPE=>$seq22},  {OUT => "73\n"},],
  ['perc99_13','perc:99 1' ,  {IN_PIPE=>$seq23},  {OUT => "11\n"},],

  # Test edge cases: perc:1 and perc:100
  ['perc1_1',  'perc:1 1',    {IN_PIPE=>$seq20},  {OUT => "78\n"},],
  ['perc100_1','perc:100 1',  {IN_PIPE=>$seq20},  {OUT => "120\n"},],

  # Sanity check: percentile:50 should be equal to 'median' 'op'
  ['perc50_1','perc:50 1' ,  {IN_PIPE=>$seq20}, {OUT => "100\n"},],
  ['perc50_2','perc:50 1' ,  {IN_PIPE=>$seq21}, {OUT => "37\n"},],
  ['perc50_3','perc:50 1' ,  {IN_PIPE=>$seq22}, {OUT => "67\n"},],
  ['perc50_4','perc:50 1' ,  {IN_PIPE=>$seq23}, {OUT => "6\n"},],

  # Sanity check: percentile:25 should be equal to 'q1' op
  ['perc25_1','perc:25 1' ,  {IN_PIPE=>$seq20},  {OUT => "93\n"},],
  ['perc25_2','perc:25 1' ,  {IN_PIPE=>$seq21},  {OUT => "23\n"},],
  ['perc25_3','perc:25 1' ,  {IN_PIPE=>$seq22},  {OUT => "67\n"},],
  ['perc25_4','perc:25 1' ,  {IN_PIPE=>$seq23},  {OUT => "4\n"},],

  # Sanity check: percentile:75 should be equal to 'q3' op
  ['perc75_10','perc:75 1' ,  {IN_PIPE=>$seq20},  {OUT => "107\n"},],
  ['perc75_11','perc:75 1' ,  {IN_PIPE=>$seq21},  {OUT => "61.5\n"},],
  ['perc75_12','perc:75 1' ,  {IN_PIPE=>$seq22},  {OUT => "70\n"},],
  ['perc75_13','perc:75 1' ,  {IN_PIPE=>$seq23},  {OUT => "8\n"},],


  # Trimmed Mean:0
  ['tmean0_1', 'trimmean:0 1' ,  {IN_PIPE=>$seq1},   {OUT => "2.5\n"}],
  ['tmean0_2', 'trimmean:0 1' ,  {IN_PIPE=>$seq2},   {OUT => "2\n"}],
  ['tmean0_3', 'trimmean:0 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  ['tmean0_4', 'trimmean:0 1' ,  {IN_PIPE=>$seq9},   {OUT => "11.111\n"}],
  ['tmean0_5', 'trimmean:0 1' ,  {IN_PIPE=>$seq10},  {OUT => "12.9\n"}],
  ['tmean0_6', 'trimmean:0 1' ,  {IN_PIPE=>$seq11},  {OUT => "14.545\n"}],
  ['tmean0_7', 'trimmean:0 1' ,  {IN_PIPE=>$seq12},  {OUT => "16.416\n"}],
  ['tmean0_8', 'trimmean:0 1' ,  {IN_PIPE=>$seq20},  {OUT => "100.06\n"},],
  ['tmean0_9', 'trimmean:0 1' ,  {IN_PIPE=>$seq21},  {OUT => "45.32\n"},],
  ['tmean0_10','trimmean:0 1' ,  {IN_PIPE=>$seq22},  {OUT => "67.45\n"},],
  ['tmean0_11','trimmean:0 1' ,  {IN_PIPE=>$seq23},  {OUT => "6.125\n"},],

  # Trimmed Mean:0.1
  ['tmean1_1', 'trimmean:0.1 1' ,  {IN_PIPE=>$seq1},   {OUT => "2.5\n"}],
  ['tmean1_2', 'trimmean:0.1 1' ,  {IN_PIPE=>$seq2},   {OUT => "2\n"}],
  ['tmean1_3', 'trimmean:0.1 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  ['tmean1_4', 'trimmean:0.1 1' ,  {IN_PIPE=>$seq9},   {OUT => "11.111\n"}],
  ['tmean1_5', 'trimmean:0.1 1' ,  {IN_PIPE=>$seq10},  {OUT => "12.25\n"}],
  ['tmean1_6', 'trimmean:0.1 1' ,  {IN_PIPE=>$seq11},  {OUT => "14.111\n"}],
  ['tmean1_7', 'trimmean:0.1 1' ,  {IN_PIPE=>$seq12},  {OUT => "15.8\n"}],
  ['tmean1_8', 'trimmean:0.1 1' ,  {IN_PIPE=>$seq20},  {OUT => "100.275\n"},],
  ['tmean1_9', 'trimmean:0.1 1' ,  {IN_PIPE=>$seq21},  {OUT => "42\n"},],
  ['tmean1_10','trimmean:0.1 1' ,  {IN_PIPE=>$seq22},  {OUT => "67.45\n"},],
  ['tmean1_11','trimmean:0.1 1' ,  {IN_PIPE=>$seq23},  {OUT => "6.076\n"},],

  # Trimmed Mean:0.2
  ['tmean2_1', 'trimmean:0.2 1' ,  {IN_PIPE=>$seq1},   {OUT => "2.5\n"}],
  ['tmean2_2', 'trimmean:0.2 1' ,  {IN_PIPE=>$seq2},   {OUT => "2\n"}],
  ['tmean2_3', 'trimmean:0.2 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  ['tmean2_4', 'trimmean:0.2 1' ,  {IN_PIPE=>$seq9},   {OUT => "10.714\n"}],
  ['tmean2_5', 'trimmean:0.2 1' ,  {IN_PIPE=>$seq10},  {OUT => "12\n"}],
  ['tmean2_6', 'trimmean:0.2 1' ,  {IN_PIPE=>$seq11},  {OUT => "13.571\n"}],
  ['tmean2_7', 'trimmean:0.2 1' ,  {IN_PIPE=>$seq12},  {OUT => "15.5\n"}],
  ['tmean2_8', 'trimmean:0.2 1' ,  {IN_PIPE=>$seq20},  {OUT => "100.516\n"},],
  ['tmean2_9', 'trimmean:0.2 1' ,  {IN_PIPE=>$seq21},  {OUT => "40.1\n"},],
  ['tmean2_10','trimmean:0.2 1' ,  {IN_PIPE=>$seq22},  {OUT => "67.6\n"},],
  ['tmean2_11','trimmean:0.2 1' ,  {IN_PIPE=>$seq23},  {OUT => "6.053\n"},],

  # Trimmed Mean:0.4
  ['tmean4_1', 'trimmean:0.4 1' ,  {IN_PIPE=>$seq1},   {OUT => "2.5\n"}],
  ['tmean4_2', 'trimmean:0.4 1' ,  {IN_PIPE=>$seq2},   {OUT => "2\n"}],
  ['tmean4_3', 'trimmean:0.4 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  ['tmean4_4', 'trimmean:0.4 1' ,  {IN_PIPE=>$seq9},   {OUT => "10.333\n"}],
  ['tmean4_5', 'trimmean:0.4 1' ,  {IN_PIPE=>$seq10},  {OUT => "12\n"}],
  ['tmean4_6', 'trimmean:0.4 1' ,  {IN_PIPE=>$seq11},  {OUT => "13.666\n"}],
  ['tmean4_7', 'trimmean:0.4 1' ,  {IN_PIPE=>$seq12},  {OUT => "15\n"}],
  ['tmean4_8', 'trimmean:0.4 1' ,  {IN_PIPE=>$seq20},  {OUT => "100.55\n"},],
  ['tmean4_9', 'trimmean:0.4 1' ,  {IN_PIPE=>$seq21},  {OUT => "37.3\n"},],
  ['tmean4_10','trimmean:0.4 1' ,  {IN_PIPE=>$seq22},  {OUT => "67\n"},],
  ['tmean4_11','trimmean:0.4 1' ,  {IN_PIPE=>$seq23},  {OUT => "6.067\n"},],

  # Trimmed Mean:0.5
  ['tmean5_1', 'trimmean:0.5 1' ,  {IN_PIPE=>$seq1},   {OUT => "2.5\n"}],
  ['tmean5_2', 'trimmean:0.5 1' ,  {IN_PIPE=>$seq2},   {OUT => "2\n"}],
  ['tmean5_3', 'trimmean:0.5 1' ,  {IN_PIPE=>$seq3},   {OUT => "2\n"}],
  ['tmean5_4', 'trimmean:0.5 1' ,  {IN_PIPE=>$seq9},   {OUT => "11\n"}],
  ['tmean5_5', 'trimmean:0.5 1' ,  {IN_PIPE=>$seq10},  {OUT => "12\n"}],
  ['tmean5_6', 'trimmean:0.5 1' ,  {IN_PIPE=>$seq11},  {OUT => "13\n"}],
  ['tmean5_7', 'trimmean:0.5 1' ,  {IN_PIPE=>$seq12},  {OUT => "15\n"}],
  ['tmean5_8', 'trimmean:0.5 1' ,  {IN_PIPE=>$seq20},  {OUT => "100\n"},],
  ['tmean5_9', 'trimmean:0.5 1' ,  {IN_PIPE=>$seq21},  {OUT => "37\n"},],
  ['tmean5_10','trimmean:0.5 1' ,  {IN_PIPE=>$seq22},  {OUT => "67\n"},],
  ['tmean5_11','trimmean:0.5 1' ,  {IN_PIPE=>$seq23},  {OUT => "6\n"},],


  # Test IQR
  ['iqr_1', 'iqr 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.5\n"}],
  ['iqr_2', 'iqr 1' ,  {IN_PIPE=>$seq2},   {OUT => "1\n"}],
  ['iqr_3', 'iqr 1' ,  {IN_PIPE=>$seq3},   {OUT => "0\n"}],
  ['iqr_4', 'iqr 1' ,  {IN_PIPE=>$seq9},   {OUT => "12\n"}],
  ['iqr_5', 'iqr 1' ,  {IN_PIPE=>$seq10},  {OUT => "13\n"}],
  ['iqr_6', 'iqr 1' ,  {IN_PIPE=>$seq11},  {OUT => "15\n"}],
  ['iqr_7', 'iqr 1' ,  {IN_PIPE=>$seq12},  {OUT => "18\n"}],
  ['iqr_8', 'iqr 1' ,  {IN_PIPE=>$seq20},  {OUT => "14\n"},],
  ['iqr_9', 'iqr 1' ,  {IN_PIPE=>$seq21},  {OUT => "38.5\n"},],
  ['iqr_10','iqr 1' ,  {IN_PIPE=>$seq22},  {OUT => "3\n"},],
  ['iqr_11','iqr 1' ,  {IN_PIPE=>$seq23},  {OUT => "4\n"},],

  # Test sample standard deviation
  ['sstdev_1', 'sstdev 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.290\n"}],
  ['sstdev_2', 'sstdev 1' ,  {IN_PIPE=>$seq2},   {OUT => "1\n"}],
  ['sstdev_3', 'sstdev 1' ,  {IN_PIPE=>$seq3},   {OUT => "$nan\n"}],
  ['sstdev_4', 'sstdev 1' ,  {IN_PIPE=>$seq9},   {OUT => "7.457\n"}],
  ['sstdev_5', 'sstdev 1' ,  {IN_PIPE=>$seq10},  {OUT => "9.024\n"}],
  ['sstdev_6', 'sstdev 1' ,  {IN_PIPE=>$seq11},  {OUT => "10.152\n"}],
  ['sstdev_7', 'sstdev 1' ,  {IN_PIPE=>$seq12},  {OUT => "11.649\n"}],
  ['sstdev_8', 'sstdev 1' ,  {IN_PIPE=>$seq20},  {OUT => "9.576\n"},],
  ['sstdev_9', 'sstdev 1' ,  {IN_PIPE=>$seq21},  {OUT => "30.448\n"},],
  ['sstdev_10','sstdev 1' ,  {IN_PIPE=>$seq22},  {OUT => "2.934\n"},],
  ['sstdev_11','sstdev 1' ,  {IN_PIPE=>$seq23},  {OUT => "2.275\n"},],

  # Test population standard deviation
  ['pstdev_1', 'pstdev 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.118\n"}],
  ['pstdev_2', 'pstdev 1' ,  {IN_PIPE=>$seq2},   {OUT => "0.816\n"}],
  ['pstdev_3', 'pstdev 1' ,  {IN_PIPE=>$seq3},   {OUT => "0\n"}],
  ['pstdev_4', 'pstdev 1' ,  {IN_PIPE=>$seq9},   {OUT => "7.030\n"}],
  ['pstdev_5', 'pstdev 1' ,  {IN_PIPE=>$seq10},  {OUT => "8.560\n"}],
  ['pstdev_6', 'pstdev 1' ,  {IN_PIPE=>$seq11},  {OUT => "9.680\n"}],
  ['pstdev_7', 'pstdev 1' ,  {IN_PIPE=>$seq12},  {OUT => "11.153\n"}],
  ['pstdev_8', 'pstdev 1' ,  {IN_PIPE=>$seq20},  {OUT => "9.528\n"},],
  ['pstdev_9', 'pstdev 1' ,  {IN_PIPE=>$seq21},  {OUT => "30.296\n"},],
  ['pstdev_10','pstdev 1' ,  {IN_PIPE=>$seq22},  {OUT => "2.920\n"},],
  ['pstdev_11','pstdev 1' ,  {IN_PIPE=>$seq23},  {OUT => "2.274\n"},],

  # Test sample variance
  ['svar_1', 'svar 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.666\n"}],
  ['svar_2', 'svar 1' ,  {IN_PIPE=>$seq2},   {OUT => "1\n"}],
  ['svar_3', 'svar 1' ,  {IN_PIPE=>$seq3},   {OUT => "$nan\n"}],
  ['svar_4', 'svar 1' ,  {IN_PIPE=>$seq9},   {OUT => "55.611\n"}],
  ['svar_5', 'svar 1' ,  {IN_PIPE=>$seq10},  {OUT => "81.433\n"}],
  ['svar_6', 'svar 1' ,  {IN_PIPE=>$seq11},  {OUT => "103.072\n"}],
  ['svar_7', 'svar 1' ,  {IN_PIPE=>$seq12},  {OUT => "135.719\n"}],
  ['svar_8', 'svar 1' ,  {IN_PIPE=>$seq20},  {OUT => "91.713\n"},],
  ['svar_9', 'svar 1' ,  {IN_PIPE=>$seq21},  {OUT => "927.128\n"},],
  ['svar_10','svar 1' ,  {IN_PIPE=>$seq22},  {OUT => "8.613\n"},],
  ['svar_11','svar 1' ,  {IN_PIPE=>$seq23},  {OUT => "5.178\n"},],

  # Test population variance
  ['pvar_1', 'pvar 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.25\n"}],
  ['pvar_2', 'pvar 1' ,  {IN_PIPE=>$seq2},   {OUT => "0.666\n"}],
  ['pvar_3', 'pvar 1' ,  {IN_PIPE=>$seq3},   {OUT => "0\n"}],
  ['pvar_4', 'pvar 1' ,  {IN_PIPE=>$seq9},   {OUT => "49.432\n"}],
  ['pvar_5', 'pvar 1' ,  {IN_PIPE=>$seq10},  {OUT => "73.29\n"}],
  ['pvar_6', 'pvar 1' ,  {IN_PIPE=>$seq11},  {OUT => "93.702\n"}],
  ['pvar_7', 'pvar 1' ,  {IN_PIPE=>$seq12},  {OUT => "124.409\n"}],
  ['pvar_8', 'pvar 1' ,  {IN_PIPE=>$seq20},  {OUT => "90.796\n"},],
  ['pvar_9', 'pvar 1' ,  {IN_PIPE=>$seq21},  {OUT => "917.857\n"},],
  ['pvar_10','pvar 1' ,  {IN_PIPE=>$seq22},  {OUT => "8.527\n"},],
  ['pvar_11','pvar 1' ,  {IN_PIPE=>$seq23},  {OUT => "5.172\n"},],

  # Test MAD (Median Absolute Deviation), with default
  # scaling factor of 1.486 for normal distributions
  ['mad_1', 'mad 1' ,  {IN_PIPE=>$seq1},   {OUT => "1.482\n"}],
  ['mad_2', 'mad 1' ,  {IN_PIPE=>$seq2},   {OUT => "1.482\n"}],
  ['mad_3', 'mad 1' ,  {IN_PIPE=>$seq3},   {OUT => "0\n"}],
  ['mad_4', 'mad 1' ,  {IN_PIPE=>$seq9},   {OUT => "8.895\n"}],
  ['mad_5', 'mad 1' ,  {IN_PIPE=>$seq10},  {OUT => "10.378\n"}],
  ['mad_6', 'mad 1' ,  {IN_PIPE=>$seq11},  {OUT => "11.860\n"}],
  ['mad_7', 'mad 1' ,  {IN_PIPE=>$seq12},  {OUT => "13.343\n"}],
  ['mad_7.1', 'mad 1' ,  {IN_PIPE=>$seq12_unsorted},
      {OUT => "13.343\n"}],
  ['mad_7.2', '--sort mad 1' ,  {IN_PIPE=>$seq12_unsorted},
     {OUT => "13.343\n"}],
  ['mad_8', 'mad 1' ,  {IN_PIPE=>$seq20},  {OUT => "10.378\n"},],
  ['mad_9', 'mad 1' ,  {IN_PIPE=>$seq21},  {OUT => "27.428\n"},],
  ['mad_10','mad 1' ,  {IN_PIPE=>$seq22},  {OUT => "4.447\n"},],
  ['mad_11','mad 1' ,  {IN_PIPE=>$seq23},  {OUT => "2.965\n"},],

  # Test MAD-Raw (Median Absolute Deviation), with scaling factor of 1
  ['madraw_1', 'madraw 1' ,  {IN_PIPE=>$seq1},   {OUT => "1\n"}],
  ['madraw_2', 'madraw 1' ,  {IN_PIPE=>$seq2},   {OUT => "1\n"}],
  ['madraw_3', 'madraw 1' ,  {IN_PIPE=>$seq3},   {OUT => "0\n"}],
  ['madraw_4', 'madraw 1' ,  {IN_PIPE=>$seq9},   {OUT => "6\n"}],
  ['madraw_5', 'madraw 1' ,  {IN_PIPE=>$seq10},  {OUT => "7\n"}],
  ['madraw_6', 'madraw 1' ,  {IN_PIPE=>$seq11},  {OUT => "8\n"}],
  ['madraw_7', 'madraw 1' ,  {IN_PIPE=>$seq12},  {OUT => "9\n"}],
  ['madraw_7.1', 'madraw 1' ,  {IN_PIPE=>$seq12_unsorted},
      {OUT => "9\n"}],
  ['madraw_7.2', '--sort madraw 1' ,  {IN_PIPE=>$seq12_unsorted},
      {OUT => "9\n"}],
  ['madraw_8', 'madraw 1' ,  {IN_PIPE=>$seq20},  {OUT => "7\n"},],
  ['madraw_9', 'madraw 1' ,  {IN_PIPE=>$seq21},  {OUT => "18.5\n"},],
  ['madraw_10','madraw 1' ,  {IN_PIPE=>$seq22},  {OUT => "3\n"},],
  ['madraw_11','madraw 1' ,  {IN_PIPE=>$seq23},  {OUT => "2\n"},],

  # Test Skewness for a population
  ['pskew_1', 'pskew 1' ,  {IN_PIPE=>$seq1},   {OUT => "0\n"}],
  ['pskew_2', 'pskew 1' ,  {IN_PIPE=>$seq2},   {OUT => "0\n"}],
  ['pskew_3', 'pskew 1' ,  {IN_PIPE=>$seq3},   {OUT => "$nan\n"}],
  ['pskew_4', 'pskew 1' ,  {IN_PIPE=>$seq9},   {OUT => "0.254\n"}],
  ['pskew_5', 'pskew 1' ,  {IN_PIPE=>$seq10},  {OUT => "0.403\n"}],
  ['pskew_6', 'pskew 1' ,  {IN_PIPE=>$seq11},  {OUT => "0.332\n"}],
  ['pskew_7', 'pskew 1' ,  {IN_PIPE=>$seq12},  {OUT => "0.371\n"}],
  ['pskew_8', 'pskew 1' ,  {IN_PIPE=>$seq20},  {OUT => "-0.204\n"},],
  ['pskew_9', 'pskew 1' ,  {IN_PIPE=>$seq21},  {OUT => "1.193\n"},],
  ['pskew_10','pskew 1' ,  {IN_PIPE=>$seq22},  {OUT => "-0.108\n"},],
  ['pskew_11','pskew 1' ,  {IN_PIPE=>$seq23},  {OUT => "0.172\n"},],

  # Test Skewness for a sample
  ['sskew_1', 'sskew 1' ,  {IN_PIPE=>$seq1},   {OUT => "0\n"}],
  ['sskew_2', 'sskew 1' ,  {IN_PIPE=>$seq2},   {OUT => "0\n"}],
  ['sskew_3', 'sskew 1' ,  {IN_PIPE=>$seq3},   {OUT => "$nan\n"}],
  ['sskew_4', 'sskew 1' ,  {IN_PIPE=>$seq9},   {OUT => "0.307\n"}],
  ['sskew_5', 'sskew 1' ,  {IN_PIPE=>$seq10},  {OUT => "0.477\n"}],
  ['sskew_6', 'sskew 1' ,  {IN_PIPE=>$seq11},  {OUT => "0.387\n"}],
  ['sskew_7', 'sskew 1' ,  {IN_PIPE=>$seq12},  {OUT => "0.426\n"}],
  ['sskew_8', 'sskew 1' ,  {IN_PIPE=>$seq20},  {OUT => "-0.207\n"},],
  ['sskew_9', 'sskew 1' ,  {IN_PIPE=>$seq21},  {OUT => "1.212\n"},],
  ['sskew_10','sskew 1' ,  {IN_PIPE=>$seq22},  {OUT => "-0.109\n"},],
  ['sskew_11','sskew 1' ,  {IN_PIPE=>$seq23},  {OUT => "0.173\n"},],
  ['sskew_12','sskew 1' ,  {IN_PIPE=>$seq4},   {OUT => "$nan\n"}],

  # Test Popluation Excess Kurtosis
  ['pkurt_1', 'pkurt 1' ,  {IN_PIPE=>$seq1},   {OUT => "-1.36\n"}],
  ['pkurt_2', 'pkurt 1' ,  {IN_PIPE=>$seq2},   {OUT => "-1.5\n"}],
  ['pkurt_3', 'pkurt 1' ,  {IN_PIPE=>$seq3},   {OUT => "$nan\n"}],
  ['pkurt_4', 'pkurt 1' ,  {IN_PIPE=>$seq9},   {OUT => "-1.273\n"}],
  ['pkurt_5', 'pkurt 1' ,  {IN_PIPE=>$seq10},  {OUT => "-0.987\n"}],
  ['pkurt_6', 'pkurt 1' ,  {IN_PIPE=>$seq11},  {OUT => "-1.169\n"}],
  ['pkurt_7', 'pkurt 1' ,  {IN_PIPE=>$seq12},  {OUT => "-1.098\n"}],
  ['pkurt_8', 'pkurt 1' ,  {IN_PIPE=>$seq20},  {OUT => "-0.608\n"},],
  ['pkurt_9', 'pkurt 1' ,  {IN_PIPE=>$seq21},  {OUT => "1.802\n"},],
  ['pkurt_10','pkurt 1' ,  {IN_PIPE=>$seq22},  {OUT => "-0.258\n"},],
  ['pkurt_11','pkurt 1' ,  {IN_PIPE=>$seq23},  {OUT => "-0.480\n"},],

  # Test Sample Excess Kurtosis
  ['skurt_1', 'skurt 1' ,  {IN_PIPE=>$seq1},   {OUT => "-1.2\n"}],
  ['skurt_2', 'skurt 1' ,  {IN_PIPE=>$seq2},   {OUT => "$nan\n"}],
  ['skurt_3', 'skurt 1' ,  {IN_PIPE=>$seq3},   {OUT => "$nan\n"}],
  ['skurt_4', 'skurt 1' ,  {IN_PIPE=>$seq9},   {OUT => "-1.283\n"}],
  ['skurt_5', 'skurt 1' ,  {IN_PIPE=>$seq10},  {OUT => "-0.781\n"}],
  ['skurt_6', 'skurt 1' ,  {IN_PIPE=>$seq11},  {OUT => "-1.116\n"}],
  ['skurt_7', 'skurt 1' ,  {IN_PIPE=>$seq12},  {OUT => "-1.012\n"}],
  ['skurt_8', 'skurt 1' ,  {IN_PIPE=>$seq20},  {OUT => "-0.577\n"},],
  ['skurt_9', 'skurt 1' ,  {IN_PIPE=>$seq21},  {OUT => "1.958\n"},],
  ['skurt_10','skurt 1' ,  {IN_PIPE=>$seq22},  {OUT => "-0.209\n"},],
  ['skurt_11','skurt 1' ,  {IN_PIPE=>$seq23},  {OUT => "-0.476\n"},],

  # Test Jarque-Bera normality pVale
  ['jarque_1', 'jarque 1' ,  {IN_PIPE=>$seq1},   {OUT => "0.857\n"}],
  ['jarque_2', 'jarque 1' ,  {IN_PIPE=>$seq2},   {OUT => "0.868\n"}],
  ['jarque_3', 'jarque 1' ,  {IN_PIPE=>$seq3},   {OUT => "$nan\n"}],
  ['jarque_4', 'jarque 1' ,  {IN_PIPE=>$seq9},   {OUT => "0.702\n"}],
  ['jarque_5', 'jarque 1' ,  {IN_PIPE=>$seq10},  {OUT => "0.712\n"}],
  ['jarque_6', 'jarque 1' ,  {IN_PIPE=>$seq11},  {OUT => "0.660\n"}],
  ['jarque_7', 'jarque 1' ,  {IN_PIPE=>$seq12},  {OUT => "0.644\n"}],
  ['jarque_8', 'jarque 1' ,  {IN_PIPE=>$seq20},  {OUT => "0.327\n"},],
  ['jarque_9', 'jarque 1' ,  {IN_PIPE=>$seq21},  {OUT => "8.011e-09\n"},],
  ['jarque_10','jarque 1' ,  {IN_PIPE=>$seq22},  {OUT => "0.789\n"},],
  ['jarque_11','jarque 1' ,  {IN_PIPE=>$seq23},  {OUT => "0.002\n"},],

  # Test D'Agostino-Pearson omnibus test for normality
  ['dpo_1', 'dpo 1' ,  {IN_PIPE=>$seq1},   {OUT => "0.900\n"}],
  ['dpo_2', 'dpo 1' ,  {IN_PIPE=>$seq2},   {OUT => "$nan\n"}],
  ['dpo_3', 'dpo 1' ,  {IN_PIPE=>$seq3},   {OUT => "$nan\n"}],
  ['dpo_4', 'dpo 1' ,  {IN_PIPE=>$seq9},   {OUT => "0.599\n"}],
  ['dpo_5', 'dpo 1' ,  {IN_PIPE=>$seq10},  {OUT => "0.661\n"}],
  ['dpo_6', 'dpo 1' ,  {IN_PIPE=>$seq11},  {OUT => "0.575\n"}],
  ['dpo_7', 'dpo 1' ,  {IN_PIPE=>$seq12},  {OUT => "0.570\n"}],
  ['dpo_8', 'dpo 1' ,  {IN_PIPE=>$seq20},  {OUT => "0.334\n"},],
  ['dpo_9', 'dpo 1' ,  {IN_PIPE=>$seq21},  {OUT => "7.689e-10\n"},],
  ['dpo_10','dpo 1' ,  {IN_PIPE=>$seq22},  {OUT => "0.819\n"},],
  ['dpo_11','dpo 1' ,  {IN_PIPE=>$seq23},  {OUT => "0.002\n"},],

);

##
## For each test, trim the resulting value to maximum three digits
## after the decimal point.
##
for my $t (@Tests) {
 push @{$t}, {OUT_SUBST=>'s/^(-?\d+\.\d{1,3})\d*/\1/'};
}

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

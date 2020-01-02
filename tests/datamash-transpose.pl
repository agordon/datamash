#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data
  Tests for 'transpose' and 'reverse' operation modes.


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
use List::Util qw/max/;
use Data::Dumper;

# Until a better way comes along to auto-use Coreutils Perl modules
# as in the coreutils' autotools system.
use Coreutils;
use CuSkip;
use CuTmpdir qw(datamash);

(my $program_name = $0) =~ s|.*/||;
my $prog_bin = 'datamash';

## Cross-Compiling portability hack:
##  under qemu/binfmt, argv[0] (which is used to report errors) will contain
##  the full path of the binary, if the binary is on the $PATH.
##  So we try to detect what is the actual returned value of the program
##  in case of an error.
my $prog = `$prog_bin --foobar 2>&1 | head -n 1 | cut -f1 -d:`;
chomp $prog if $prog;
$prog = $prog_bin unless $prog;

# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

sub perl_reverse_fields
{
	my $field_sep = shift;
	my $input = shift;
	return join ("\n",
		map {
		   join($field_sep,
		          reverse(
			           split /$field_sep/, $_
			         )
		       )
		}
		split /\n/, $input) . "\n";
}

sub perl_transpose
{
	my $field_sep = shift;
	my $filler = shift ;
	my $input = shift;
	my @lines = map { [ split /$field_sep/, $_ ] } split /\n/, $input;
	my $max_field = max ( map{ scalar(@$_) } @lines );

	my @output;
	foreach my $i ( 0 .. ( $max_field - 1) ) {
		my @new_line ;
		foreach my $l (@lines) {
			push @new_line,
			     (scalar(@$l) > $i) ? $l->[$i] : $filler;
		}

		push @output, join($field_sep, @new_line);
	}

	return join("\n", @output) . "\n";
}

my $in1=<<'EOF';
A	1	!
B	2	@
C	3	#
D	4	$
E	5	%
EOF

my $in2 = $in1;
$in2 =~ s/\t/:/gms;

my $in3=<<'EOF';
A	1
B
C	3
EOF

my $in4=<<'EOF';
A
B
C
EOF

my $in5="A\tB\tC\tD\n";

my $in6="A\n";

my $in7="";

my $out1_rev = perl_reverse_fields ( "\t", $in1 );
my $out2_rev = perl_reverse_fields ( ":",  $in2 );
my $out3_rev = perl_reverse_fields ( "\t", $in3 );
my $out4_rev = perl_reverse_fields ( "\t", $in4 );
my $out5_rev = perl_reverse_fields ( "\t", $in5 );

my $out1_tr = perl_transpose ( "\t", "N/A", $in1 );
my $out2_tr = perl_transpose ( ":", "N/A", $in2 ) ;
my $out3_tr = perl_transpose ("\t", "N/A", $in3 ) ;
my $out3_filler_tr = perl_transpose ("\t", "xxx", $in3 ) ;
my $out4_tr = perl_transpose ("\t", "N/A", $in4 ) ;
my $out5_tr = perl_transpose ("\t", "N/A", $in5 ) ;

my $in_hdr1=<<'EOF';
X:Y
1:a
2:b
EOF


# Transposing with missing value in the last line
# (bug in 1.1.0 would result in 'c' being silently dropped).
my $in_missing1=<<'EOF';
a	b	c
1	2
EOF
my $out_missing1=<<'EOF';
a	1
b	2
c	N/A
EOF

my @Tests =
(
    # Simple transpose and reverse
    ['tr1',  'transpose', {IN_PIPE=>$in1}, {OUT=>$out1_tr}],
    ['rev1', 'reverse',   {IN_PIPE=>$in1}, {OUT=>$out1_rev}],

    # non-tab delimiter
    ['tr2',  '-t: transpose', {IN_PIPE=>$in2}, {OUT=>$out2_tr}],
    ['rev2', '-t: reverse',   {IN_PIPE=>$in2}, {OUT=>$out2_rev}],

    # missing fields, strict mode
    ['tr3',  'transpose', {IN_PIPE=>$in3}, {EXIT=>1},
      {OUT_SUBST=>'s/.*//'},
      {ERR=>"$prog: transpose input error: line 2 has 1 fields ".
	    "(previous lines had 2);\nsee --help to disable strict mode\n"}],
    ['rev3', 'reverse',   {IN_PIPE=>$in3}, {EXIT=>1},
      {OUT_SUBST=>'s/.*//s'},
      {ERR=>"$prog: reverse-field input error: line 2 has 1 fields ".
	    "(previous lines had 2);\nsee --help to disable strict mode\n"}],

    # missing fields, non-strict mode
    ['tr4',  '--no-strict transpose', {IN_PIPE=>$in3}, {OUT=>$out3_tr}],
    ['rev4', '--no-strict reverse',   {IN_PIPE=>$in3}, {OUT=>$out3_rev}],
    ['tr4.1', '--no-strict --filler xxx transpose',
        {IN_PIPE=>$in3}, {OUT=>$out3_filler_tr}],


    # Single column
    ['tr5',  'transpose', {IN_PIPE=>$in4}, {OUT=>$out4_tr}],
    ['rev5', 'reverse',   {IN_PIPE=>$in4}, {OUT=>$out4_rev}],

    # Single row
    ['tr6',  'transpose', {IN_PIPE=>$in5}, {OUT=>$out5_tr}],
    ['rev6', 'reverse',   {IN_PIPE=>$in5}, {OUT=>$out5_rev}],

    # Single field
    ['tr7',  'transpose', {IN_PIPE=>$in6}, {OUT=>$in6}],
    ['rev7', 'reverse',   {IN_PIPE=>$in6}, {OUT=>$in6}],

    # Empty input
    ['tr8',  'transpose', {IN_PIPE=>$in7}, {OUT=>""}],
    ['rev8', 'reverse',   {IN_PIPE=>$in7}, {OUT=>""}],

    # Extra operands
    ['tr9',  'transpose aaa', {IN_PIPE=>''}, {EXIT=>1},
      {ERR=>"$prog: extra operand 'aaa'\n"}],
    ['rev9', 'reverse aaa', {IN_PIPE=>''}, {EXIT=>1},
      {ERR=>"$prog: extra operand 'aaa'\n"}],

    # empty input
    ['tr10',  'transpose', {IN_PIPE=>""}, {OUT=>""}],
    ['rev10', 'reverse',   {IN_PIPE=>""}, {OUT=>""}],

    # Reverse with header combinations
    ['rev-hdr1','-H reverse', {IN_PIPE=>""}, {OUT=>""}],
    ['rev-hdr2','--header-in reverse', {IN_PIPE=>""}, {OUT=>""}],
    ['rev-hdr3','-t: reverse', {IN_PIPE=>$in_hdr1},
       {OUT=>"Y:X\na:1\nb:2\n"}],
    ['rev-hdr4','-t: -H reverse', {IN_PIPE=>$in_hdr1},
       {OUT=>"Y:X\na:1\nb:2\n"}],
    # first line is header line, discard it (there's no --header-out).
    ['rev-hdr5','-t: --header-in reverse', {IN_PIPE=>$in_hdr1},
       {OUT=>"a:1\nb:2\n"}],
    # Generate a new header, assuming the first line is a NOT header line.
    ['rev-hdr6','-t: --header-out reverse', {IN_PIPE=>$in_hdr1},
       {OUT=>"field-2:field-1\nY:X\na:1\nb:2\n"}],

    # bug uncovered by report in:
    # http://lists.gnu.org/archive/html/bug-datamash/2016-09/msg00000.html
    ['msg1', '--no-strict transpose', {IN_PIPE=>$in_missing1},
       {OUT=>$out_missing1}],
);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog_bin, \@Tests, $save_temps, $verbose);
exit $fail;

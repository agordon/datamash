#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2021 Assaf Gordon <assafgordon@gmail.com>
   Copyright (C) 2022 Dima Kogan <dima@secretsauce.net>

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

use lib '.';
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


my $in_basic=<<'EOF';
#! comment
##comment
## comment
 ## comment

   
#x y z
   

4 2 3 # comment
4 - 6# comment
## comment
- 8 9
EOF

my $in_fake_comment=<<'EOF';
# w x y z
1 2 3 4
; not a comment
EOF

my $in_fake_comment2=<<'EOF';
# w x y z
1 2 3 4;no_comment
5 6 7 ;no_comment
8 9 ; no_comment
EOF

my $in_numeric_columns=<<'EOF';
# 0 1 2
1 2 3
4 - 6
- 8 9
EOF

# trailing whitespace
my $in_trailing_whitespace=<<'EOF';
# x y
bar	5
bbb	4   
EOF

my $in_xy=<<'EOF';
# x y
1 0.5
2 1
3 1.5
4 2
EOF

my $in_header_comment=<<'EOF';
# x y # this is not a comment
a 1
b 2
EOF

my $in_header_comment2=<<'EOF';

# x y # z
bar     5 1 2
bbb     4 7 8


EOF

my $in_header_comment3=<<'EOF';

# ##
3
15
EOF

my $in_header_comment4=<<'EOF';
# # a
1 2
EOF

my $in_legend1=<<'EOF';
#!/my/interpreter
##regular vnlog pre-legend comment
#!unusual vnlog pre-legend comment
##
## lines matching regex [ \t]*#[ \t]* are also ignored
#
#x
42
EOF

my $in_legend2=<<'EOF';
#!/my/interpreter
##regular vnlog pre-legend comment
#!unusual vnlog pre-legend comment
##
## lines matching regex [ \t]*#[ \t]* are also ignored
 #
#x
42
EOF

my $in_legend3=<<'EOF';
#!/my/interpreter
##regular vnlog pre-legend comment
#!unusual vnlog pre-legend comment
##
## lines matching regex [ \t]*#[ \t]* are also ignored
 # 
#x
42
EOF

my $in_legend4=<<'EOF';
#!/my/interpreter
##regular vnlog pre-legend comment
#!unusual vnlog pre-legend comment
##
## lines matching regex [ \t]*#[ \t]* are also ignored
#
      #  
	# 
 #			 
		#		
			#
#			
 # 
     #         x
42
EOF

# unusual field names are allowed in vnlog legends
my $in_legend5=<<'EOF';
#  #  7 1+  -  ,  :  ;  .. %CPU TIME+
   1  2  3  4  5  6  7   8    9    10
  10 20 30 40 50 60 70  80   90   100
EOF

# duplicate field names are not prohibited in vnlog legends, but how
# they are processed is not specified -- datamash uses the first field
# with the specified name
my $in_legend6=<<'EOF';
#  a  a
   1  2
  10 20
EOF

my $out_groupby=<<'EOF';
# GroupBy(x) sum(z)
4 9
- 9
EOF

my $out_rmdup_x=<<'EOF';
# x y z
4 2 3
- 8 9
EOF

my $out_rmdup_y=<<'EOF';
# x y z
4 2 3
4 - 6
- 8 9
EOF

my $out_reverse=<<'EOF';
# z y x
3 2 4
6 - 4
9 8 -
EOF

my $out_ct1=<<'EOF';
# GroupBy(x) GroupBy(y) count(x)
 0.5 1 1.5 2
1 1 - - -
2 - 1 - -
3 - - 1 -
4 - - - 1
EOF

my $out_ct2=<<'EOF';
# GroupBy(x) GroupBy(y) sum(y)
 0.5 1 1.5 2
1 0.5 - - -
2 - 1 - -
3 - - 1.5 -
4 - - - 2
EOF

my $out_pcov=<<'EOF';
# pcov(x,x) pcov(x,y) pcov(y,y)
1.25 0.625 0.3125
EOF

my $out_header_comment2_sum=<<'EOF';
# sum(#) sum(z)
8 10
EOF

my $out_header_comment3_sum=<<'EOF';
# sum(##)
18
EOF

my $out_header_comment4_sum=<<'EOF';
# sum(#)
1
EOF

my $out_legend_sum=<<'EOF';
# sum(x)
42
EOF

my $out_legend_sum2=<<'EOF';
# sum(#) sum(7) sum(1+) sum(-) sum(,) sum(:) sum(;) sum(..) sum(%CPU) sum(TIME+)
11 22 33 44 55 66 77 88 99 110
EOF

my $out_legend_sum3=<<'EOF';
# sum(a)
11
EOF

my @Tests =
(
  ['vnl-sum',   '--vnlog sum z',
    {IN_PIPE=>$in_basic}, {OUT=>"# sum(z)\n18\n"}],

  ['vnl-check', '--vnlog check',
    {IN_PIPE=>$in_basic}, {OUT=>"3 lines, 3 fields\n"}],

  # See also nc[5-6] in datamash-tests.pl
  ['vnl-nc5', '--vnlog sum zzz',
    {IN_PIPE=>$in_basic},
    {EXIT=>1}, {ERR=>"$prog: column name 'zzz' not found in input file\n"}],

  # Make sure trailing whitespace is ignored properly
  ['vnl-eol-space', '--vnlog check',
    {IN_PIPE=>$in_trailing_whitespace}, {OUT=>"2 lines, 2 fields\n"}],

  # See also e4 in datamash-tests.pl
  ['vnl-e4', '--vnlog sum x',
    {IN_PIPE=>$in_basic}, {OUT=>"# sum(x)\n"},
    {EXIT=>1}, {ERR=>"$prog: invalid numeric value in line 4 field 1: '-'\n"}],

  ['vnl-unique', '--vnlog unique x',
    {IN_PIPE=>$in_basic}, {OUT=>"# unique(x)\n-,4\n"}],

  ['vnl-collapse', '--vnlog collapse x',
    {IN_PIPE=>$in_basic}, {OUT=>"# collapse(x)\n4,4,-\n"}],

  ['vnl-data-before-legend', '--vnlog sum z',
    {IN_PIPE=>"5\n" . $in_basic},
    {EXIT=>1}, {ERR=>"$prog: invalid vnlog data: " .
                     "received record before header: '5'\n"}],

  ['vnl-unnamed-field', '--vnlog sum 1',
    {IN_PIPE=>$in_basic},
    {EXIT=>1}, {ERR=>"$prog: column name '1' not found in input file\n"}],

  ['vnl-numeric-field', '--vnlog sum 2',
    {IN_PIPE=>$in_numeric_columns}, {OUT=>"# sum(2)\n18\n"}],

  ['vnl-groupby', '--vnlog -g x sum z',
    {IN_PIPE=>$in_basic}, {OUT=>$out_groupby}],
  ['vnl-rmdup-x', '--vnlog rmdup x',
    {IN_PIPE=>$in_basic}, {OUT=>$out_rmdup_x}],
  ['vnl-rmdup-y', '--vnlog rmdup y',
    {IN_PIPE=>$in_basic}, {OUT=>$out_rmdup_y}],
  ['vnl-reverse', '--vnlog reverse',
    {IN_PIPE=>$in_basic}, {OUT=>$out_reverse}],
  ['vnl-getnum1', '--vnlog getnum A',
    {IN_PIPE=>"# A B\nx 1\ny 2\n"}, {OUT=>"# getnum(A)\n0\n0\n"}],
  ['vnl-getnum2', '--vnlog getnum B',
    {IN_PIPE=>"# A B\nx 1\ny 2\n"}, {OUT=>"# getnum(B)\n1\n2\n"}],

  # empty input = empty output
  ['vnl-empty1', '--vnlog count x',
    {IN_PIPE=>""},    {ERR=>""}],
  ['vnl-empty2', '--vnlog count x',
    {IN_PIPE=>"# x"}, {OUT=>"# count(x)\n"}],

  # various errors
  ['vnl-groupby-unmatched-field', '--vnlog -g zzz sum z',
    {IN_PIPE=>$in_basic},
    {EXIT=>1}, {ERR=>"$prog: column name 'zzz' not found in input file\n"} ],

  ['vnl-error-sum-empty-field', '--vnlog sum ""',
    {IN_PIPE=>$in_basic},
    {EXIT=>1}, {ERR=>"$prog: missing field for operation 'sum'\n"}],

  ['vnl-groupby-missing-field', '--vnlog -g x,,y sum z',
    {IN_PIPE=>$in_basic},
    {EXIT=>1}, {ERR=>"$prog: missing field for operation 'groupby'\n"}],

  # Check that semicolon is *not* a comment.
  ['vnl-semicolon', '--vnlog unique w',
    {IN_PIPE=>$in_fake_comment}, {EXIT=>0}, {OUT=>"# unique(w)\n1,;\n"}],
  ['vnl-semicolon2', '--vnlog check',
    {IN_PIPE=>$in_fake_comment2},{EXIT=>0}, {OUT=>"3 lines, 4 fields\n"}],
  ['vnl-semicolon3', '--vnlog unique y',
    {IN_PIPE=>$in_fake_comment2},{EXIT=>0},
    {OUT=>"# unique(y)\n3,7,;\n"}],
  ['vnl-semicolon4', '--vnlog unique z',
    {IN_PIPE=>$in_fake_comment2},{EXIT=>0},
    {OUT=>"# unique(z)\n4;no_comment,;no_comment,no_comment\n"}],

  # Commandline errors
  ['vnl-option-parsing-error1', '--vnlog -t: sum x',
    {IN_PIPE=>$in_basic},
    {EXIT=>1}, {ERR=>"$prog: vnlog processing is whitespace-delimited\n"}],

  ['vnl-basic-check-no-op-options', '--vnlog -C --header-in --header-out check',
    {IN_PIPE=>$in_basic}, {OUT=>"3 lines, 3 fields\n"}],

  ['vnl-pcov', '--vnlog pcov x:x pcov x:y pcov y:y',
    {IN_PIPE=>$in_xy}, {OUT=>$out_pcov}],

  # transpose isn't handled in any special way. The result is not a vnl
  ['vnl-transpose', '--vnlog transpose',
    {IN_PIPE=>$in_xy}, {OUT=>"1 2 3 4\n0.5 1 1.5 2\n"}],

  # crosstab isn't handled in any special way. The result is not a vnl
  ['vnl-ct1', '--vnlog crosstab x,y',
    {IN_PIPE=>$in_xy}, {OUT=>$out_ct1}],
  ['vnl-ct2', '--vnlog crosstab x,y sum y',
    {IN_PIPE=>$in_xy}, {OUT=>$out_ct2}],

  # the header line does not recognize trailing comments
  ['vnl-no-header-comment1', '--vnlog check',
    {IN_PIPE=>$in_header_comment}, {OUT=>"2 lines, 2 fields\n"}],
  ['vnl-no-header-comment2', '--vnlog sum y',
    {IN_PIPE=>$in_header_comment}, {OUT=>"# sum(y)\n3\n"}],
  ['vnl-no-header-comment3', '--vnlog sum comment',
    {IN_PIPE=>$in_header_comment},
    {EXIT=>1}, {OUT=>"# sum(comment)\n"},
    {ERR=>"$prog: invalid input:" .
          " field 8 requested, line 2 has only 2 fields\n"}],
  ['vnl-no-header-comment4', '--vnlog sum "\\#",z',
    {IN_PIPE=>$in_header_comment2}, {OUT=>$out_header_comment2_sum}],
  ['vnl-no-header-comment5', '--vnlog sum "\\#\\#"',
    {IN_PIPE=>$in_header_comment3}, {OUT=>$out_header_comment3_sum}],
  ['vnl-no-header-comment6', '--vnlog sum "\\#"',
    {IN_PIPE=>$in_header_comment4}, {OUT=>$out_header_comment4_sum}],

  # ignore specific lines before the legend
  ['vnl-legend1', '--vnlog sum x',
    {IN_PIPE=>$in_legend1}, {OUT=>$out_legend_sum}],
  ['vnl-legend2', '--vnlog sum x',
    {IN_PIPE=>$in_legend2}, {OUT=>$out_legend_sum}],
  ['vnl-legend3', '--vnlog sum x',
    {IN_PIPE=>$in_legend3}, {OUT=>$out_legend_sum}],
  ['vnl-legend4', '--vnlog sum x',
    {IN_PIPE=>$in_legend4}, {OUT=>$out_legend_sum}],

  # vnlog allows unusual field names
  ['vnl-legend5',
    "--vnlog sum '\\#,7,\\1\\+,\\-,\\,,\\:,\\;,\\.\\.,\\%CPU,TIME\\+'",
    {IN_PIPE=>$in_legend5}, {OUT=>$out_legend_sum2}],
  # duplicate field names are not an error in vnlog
  ['vnl-legend6', '--vnlog sum a',
    {IN_PIPE=>$in_legend6}, {OUT=>$out_legend_sum3}],

);


my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

#!/usr/bin/env perl
=pod
   Unit Tests for GNU Datamash - tests sha1/256/5125 operations

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
## NOTE: Perl<5.10  don't have Digest::SHA core module -
##       So skip only those tests if needed.
##       Other line operations (e.g. md5/base64) are tested in the main
##       unit test module 'datamash-tests.pl'.
use strict;
use warnings;

# Until a better way comes along to auto-use Coreutils Perl modules
# as in the coreutils' autotools system.
use Coreutils;
use CuSkip;
use CuTmpdir qw(datamash);

## Perl 5.8 and earlier do not have Digest::SHA as core module.
## Skip the test if it is not found.
my $have_sha =
   eval qq{use Digest::SHA qw(sha1_hex sha256_hex  sha512_hex);1;};

CuSkip::skip "requires Perl>5.8 with Digest::SHA module\nload error:\n$@"
   unless $have_sha;

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

my $in_g1=<<'EOF';
A 100
A 10
A 50
A 35
EOF

# Header line, with custom field separator
my $in_hdr2=<<'EOF';
x:y:z
A:3:W
A:5:W
A:7:W
A:11:X
A:13:X
B:17:Y
B:19:Z
C:23:Z
EOF

=pod
  Example:
  my $data = "a 1\nb 2\n";
  my $out = transform_column($data, 2, \&md5_hex);
  # out => md5_hex("1") . "\n" . md5_hex("2") . "\n" ;
=cut
sub transform_column($$$)
{
  my $input_text = shift;
  my $input_column = shift;
  my $function = shift;

  return join "",
		map { "$_\n" }
		map { &$function($_->[ $input_column - 1 ]) }
		map { [ split / / ] }
		split("\n", $input_text);
}

my $out_g1_sha1 = transform_column ($in_g1, 2, \&sha1_hex);
my $out_g1_sha256 = transform_column ($in_g1, 2, \&sha256_hex);
my $out_g1_sha512 = transform_column ($in_g1, 2, \&sha512_hex);

my @Tests =
(
   ['sha1-1',  '-W sha1 2',   {IN_PIPE=>$in_g1}, {OUT=>$out_g1_sha1}],
   ['sha256-1','-W sha256 2', {IN_PIPE=>$in_g1}, {OUT=>$out_g1_sha256}],
   ['sha512-1','-W sha512 2', {IN_PIPE=>$in_g1}, {OUT=>$out_g1_sha512}],
);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;

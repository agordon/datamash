#!/usr/bin/env perl
=pod
  Unit Tests for GNU Datamash - check German locale (de_DE.UTF-8).

   Copyright (C) 2013-2021 Assaf Gordon <assafgordon@gmail.com>
   Copyright (C) 2022-     Timothy Rice <trice@posteo.net>

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

   Written by Assaf Gordon and Timothy Rice.
=cut
use strict;
use warnings;

# Until a better way comes along to auto-use Coreutils Perl modules
# as in the coreutils' autotools system.
use Coreutils;
use CuSkip;
use CuTmpdir qw(datamash);
use MIME::Base64 ;

## Skip this test if Deutsche (German) locale not found.
use POSIX qw(locale_h);
use locale;
my $lc_de = setlocale(LC_ALL, "de_DE.utf8");
CuSkip::skip "requires de_DE.utf8 locale\n"
   if $lc_de eq undef;

(my $program_name = $0) =~ s|.*/||;
my $prog_bin = 'datamash';

## Cross-Compiling portability hack:
##  under qemu/binfmt, argv[0] (which is used to report errors) will contain
##  the full path of the binary, if the binary is on the $PATH.
##  So we try to detect what is the actual returned value of the program
##  in case of an error.
my $prog = `$prog_bin ---print-progname`;
$prog = $prog_bin unless $prog;

## Portability hack
## Check if the system's sort supports stable sorting ('-s').
## If it doesn't - skip some tests
my $rc = system("sort -s < /dev/null > /dev/null 2>/dev/null");
die "testing framework failure: failed to execute sort -s"
  if ( ($rc == -1) || ($rc & 127) );
my $sort_exit_code = ($rc >> 8);
my $have_stable_sort = ($sort_exit_code==0);


# Deutsche Prüfungen

# TODO: mix-and-match different locales,
# eg LC_ALL=C combined with LC_NUMERIC=de_DE.utf8.
# LC_NUMERIC is currently ignored; this is a bug.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('de_DE.utf8') x 3;

my @Prufungen =
(
  # Prüfen Sie, ob das Komma als Dezimaltrennzeichen funktioniert
  ['de1', 'sum 1',     {IN_PIPE=>"1,1\n"},  {OUT=>"1,1\n"}],

  # There is a bug where the bin operation does not respect
  # the locale's choice of decimal separator.
  # TODO: Be able to uncomment the following line.
  #['de2', 'bin:0,1 1'  {IN_PIPE=>"1,15\n"},  {OUT=>"1,1\n"}]
);

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Prufungen, $save_temps, $verbose);

exit $fail;

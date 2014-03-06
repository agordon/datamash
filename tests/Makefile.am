# Makefile for calc/tests.
#
# Copyright 2013 Assaf Gordon.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

TESTS = calc-tests.pl \
        calc-valgrind.sh

EXTRA_DIST = Coreutils.pm CuSkip.pm CuTmpdir.pm init.sh \
             $(TESTS)

# (Copied from coreutils' tests/local.mk)
# Use automake's TESTS_ENVIRONMENT to add ./src/ to the search path.
TESTS_ENVIRONMENT =				\
  export \
  PERL5LIB='$(top_srcdir)/tests$(PATH_SEPARATOR)'"$$PERL5LIB" \
  PATH='$(abs_top_builddir)/src$(PATH_SEPARATOR)'"$$PATH" \
  ;


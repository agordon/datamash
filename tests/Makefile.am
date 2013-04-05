# Copyright (C) 2013 Assaf Gordon <assafgordon@gmail.com>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#

PERL_TESTS = calc-tests.pl

EXTRA_DIST = Coreutils.pm CuSkip.pm CuTmpdir.pm \
             $(PERL_TESTS)

noinst_DATA = $(EXTRA_DIST)

check_PROGRAMS =

# (Copied from coreutils' tests/local.mk)
# Use automake's TESTS_ENVIRONMENT to add ./src/ to the search path.
TESTS_ENVIRONMENT =				\
  export \
  PERL5LIB='$(top_srcdir)/tests$(PATH_SEPARATOR)'"$$PERL5LIB" \
  PATH='$(abs_top_builddir)/src$(PATH_SEPARATOR)'"$$PATH" \
  ;

TESTS = $(check_PROGRAMS) \
	$(PERL_TESTS)

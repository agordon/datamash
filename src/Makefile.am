# Copyright (C) 2013 Assaf Gordon <assafgordon@gmail.com>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = calc

EXTRA_DIST = system.h \
	     strnumcmp.h strnumcmp-in.h \
	     version.h

calc_SOURCES = calc.c \
	       key-compare.h key-compare.c \
	       strnumcmp.c \
	       version.c

# gnulib header files - is this the right way to find them?
#
# TODO: set -Werror only for GCC
EXTRA_CFLAGS=-Wall -Wextra -Wformat-security -Wswitch-enum
calc_o_CFLAGS=-Wswitch-default -Wformat-nonliteral -Wunused-parameter -Werror


calc_CFLAGS = -I$(top_srcdir)/lib $(EXTRA_CFLAGS)

# gnulib static library
calc_LDFLAGS = -L$(top_builddir)/lib
LIBS = -lgnu \
       $(FABSL_LIBM) \
       $(FABS_LIBM) \
       $(FREXPL_LIBM) \
       $(FREXP_LIBM) \
       $(ISNANL_LIBM) \
       $(LDEXPL_LIBM) \
       $(LDEXP_LIBM) \
       $(SQRTL_LIBM) \
       $(SQRT_LIBM)

version.c: Makefile
	rm -f $@
	printf '#include <config.h>\n' > $@t
	printf 'char const *Version = "$(PACKAGE_VERSION)";\n' >> $@t
	chmod a-w $@t
	mv $@t $@

DISTCLEANFILES = version.c

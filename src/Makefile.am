# Makefile.am for calc/src.
#
# Copyright (C) 2013 Assaf Gordon.
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

bin_PROGRAMS = calc

calc_SOURCES = calc.c \
	       key-compare.h key-compare.c \
           system.h \
	       strnumcmp.c strnumcmp.h strnumcmp-in.h \
	       version.c version.h

# TODO: set -Werror only if using GCC
calc_o_CFLAGS=-Wall -Wextra -Wformat-security -Wswitch-enum \
              -Wswitch-default -Wformat-nonliteral -Wunused-parameter -Werror


# gnulib header files - is this the right way to find them?
calc_CFLAGS = -I$(top_srcdir)/lib

# Add LOCALDIR for gettext
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

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

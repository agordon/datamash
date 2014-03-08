# Makefile.am for calc/src.
#
# Copyright (C) 2013-2014 Assaf Gordon.
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

calc_SOURCES = system.h \
	       version.c version.h \
	       strnumcmp.c strnumcmp.h strnumcmp-in.h \
	       text-options.c text-options.h \
	       utils.c utils.h \
	       text-lines.c text-lines.h \
	       column-headers.c column-headers.h \
	       field-ops.c field-ops.h \
	       calc.c

# Extra compile flags for these source files.
# Can't use them when compiling gnulib, so not adding them globally.
AM_CFLAGS = $(WARN_CFLAGS)

# gnulib header files - is this the right way to find them?
AM_CPPFLAGS = -I$(top_srcdir)/lib

# Add LOCALDIR for gettext
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

# gnulib static library
AM_LDFLAGS = -L$(top_builddir)/lib
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

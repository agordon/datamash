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
	     version.h

calc_SOURCES = calc.c \
	       version.c

# gnulib header files - is this the right way to find them?
calc_CFLAGS = -I$(top_srcdir)/lib

# gnulib static library
calc_LDFLAGS = -L$(top_builddir)/lib
LIBS = -lgnu

version.c: Makefile
	rm -f $@
	printf '#include <config.h>\n' > $@t
	printf 'char const *Version = "$(PACKAGE_VERSION)";\n' >> $@t
	chmod a-w $@t
	mv $@t $@

DISTCLEANFILES = version.c

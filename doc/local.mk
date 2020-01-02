# Make GNU Datamash documentation.				-*-Makefile-*-
# This is included by the top-level Makefile.am.

# Based on GNU Hello:
#   Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
#   2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014 Free
#   Software Foundation, Inc.

# Modifications for GNU Datamash are
# Copyright (C) 2014-2020 Assaf Gordon <assafgordon@gmail.com>

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

info_TEXINFOS = doc/datamash.texi
EXTRA_DIST += doc/datamash-texinfo.css

# For the 'make html' target - generate a single HTML file
# and embed the CSS statements in it.
AM_MAKEINFOHTMLFLAGS = --no-split \
	--css-include=$(top_srcdir)/doc/datamash-texinfo.css

# Changes to the CSS should trigger a new HTML regeneration
$(top_builddir)/doc/datamash.html: $(top_srcdir)/doc/datamash-texinfo.css

doc_datamash_TEXINFOS = \
  doc/fdl.texi

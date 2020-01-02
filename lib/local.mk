# Copyright (C) 2014-2020 Assaf Gordon <assafgordon@gmail.com>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

include lib/gnulib.mk

# Allow "make distdir" to succeed before "make all" has run.
dist-hook: $(noinst_LIBRARIES)
.PHONY: dist-hook

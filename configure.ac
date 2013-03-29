# Copyright (C) 2013 Assaf Gordon <assafgordon@gmail.com>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT([calc],
        m4_esyscmd([config/git-version-gen .tarball-version]),
        [A. Gordon assafgordon@gmail.com])

AC_CONFIG_SRCDIR([src/calc.c])

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([])

AC_CONFIG_MACRO_DIR([m4])

dnl === Enable  this if you're going to use libtool
dnl LT_INIT

AC_PROG_CC_STDC
gl_EARLY
AM_PROG_CC_C_O
AC_PROG_CPP

AX_C_LONG_LONG

gl_INIT

AC_CONFIG_FILES([
   Makefile
   m4/Makefile
   lib/Makefile
   src/Makefile
   scripts/Makefile
   tests/Makefile
])

AC_OUTPUT

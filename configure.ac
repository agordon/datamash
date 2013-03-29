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


dnl --enable-wall
EXTRA_CHECKS="-Wall -Wextra -Wformat-nonliteral -Wformat-security -Wswitch-default -Wswitch-enum -Wunused-parameter -Wfloat-equal -Werror"
AC_ARG_ENABLE(wall,
 [  --enable-wall          Enable many common GCC warnings (-Wall,-Wextra, -Werror etc., default enabled)],
 [case "${enableval}" in
   yes) wall=true ;;
   no)  wall=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-wall) ;;
  esac],
  [wall=true])
if test "$wall" = "true"
then
  CFLAGS="${CFLAGS} ${EXTRA_CHECKS}"
fi


AC_CONFIG_FILES([
   Makefile
   m4/Makefile
   lib/Makefile
   src/Makefile
   scripts/Makefile
   tests/Makefile
])

AC_OUTPUT

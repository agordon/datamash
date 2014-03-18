dnl Copyright (C) 2013 Assaf Gordon <assafgordon@gmail.com>
dnl
dnl This file is free software; as a special exception the author gives
dnl unlimited permission to copy and/or distribute it, with or without
dnl modifications, as long as this notice is preserved.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
dnl implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT([compute],
        [m4_esyscmd([build-aux/git-version-gen .tarball-version])],
	[assafgordon@gmail.com], [],
	[http://agordon.github.io/compute/])

dnl Must come before AM_INIT_AUTOMAKE.
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([
	1.11.1
	parallel-tests
	subdir-objects
])

# Minimum Autoconf version required.
AC_PREREQ(2.62)

# Where to generate output; srcdir location.
AC_CONFIG_HEADERS([config.h:config.in])dnl Keep filename to 8.3 for MS-DOS.
AC_CONFIG_SRCDIR([src/compute.c])

dnl Checks for programs.
# We need a C compiler.
AC_PROG_CC
AC_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_CPP

# Since we use gnulib: gl_EARLY must be called as soon as possible after
# the C compiler is checked.  The others could be later, but we just
# keep everything together.
gl_EARLY
gl_INIT

# GNU help2man creates man pages from --help output; in many cases, this
# is sufficient, and obviates the need to maintain man pages separately.
# However, this means invoking executables, which we generally cannot do
# when cross-compiling, so we test to avoid that (the variable
# "cross_compiling" is set by AC_PROG_CC).
if test $cross_compiling = no; then
  AM_MISSING_PROG(HELP2MAN, help2man)
else
  HELP2MAN=:
fi

# Add extra compilation warnings. These will only apply to compute.c not to any
# gnulib modules). See $(WARN_CFLAGS) in src/Makefile.am .
gl_WARN_ADD([-Wall])
gl_WARN_ADD([-Wextra])
gl_WARN_ADD([-Wformat-security])
gl_WARN_ADD([-Wswitch-enum])
gl_WARN_ADD([-Wswitch-default])
gl_WARN_ADD([-Wformat-nonliteral])
gl_WARN_ADD([-Wunused-parameter])
gl_WARN_ADD([-Werror])

# Add Debian-specific Hardening flags
# See:  https://wiki.debian.org/HardeningWalkthrough
# Requires 'dpkg-buildflags' from the 'dpkg-dev' package
#
# To use:
#    ./configure --enable-debian-hardening
#
AC_ARG_ENABLE(debian-hardening,
	      AS_HELP_STRING([--enable-debian-hardening],
			     [Build with Debian-specific hardening flags]),
			     [
			      if test "$enableval" = "yes" ; then
				      :
			      elif test "$enableval" != "no" ; then
				      AC_MSG_ERROR([--enable-debian-hardening does not accept an argument])
			      fi
			      ])

AC_CHECK_PROG([DPKG_BUILDFLAGS],[dpkg-buildflags],[yes],[no],[])

if test "x$enable_debian_hardening" = "xyes" ; then
	if test "x$DPKG_BUILDFLAGS" != "xyes" ; then
		AC_MSG_ERROR([debian-hardening requires the program dpkg-buildflag, which was not found])
	fi
	DEBHARD_CFLAGS=`dpkg-buildflags --get CFLAGS`
	DEBHARD_CPPFLAGS=`dpkg-buildflags --get CPPFLAGS`
	DEBHARD_LDFLAGS=`dpkg-buildflags --get LDFLAGS`

	CFLAGS="$CFLAGS $DEBHARD_CFLAGS"
	CPPFLAGS="$CPPFLAGS $DEBHARD_CPPFLAGS"
	LDFLAGS="$LDFLAGS $DEBHARD_LDFLAGS"
fi


# i18n support from GNU gettext.
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

AC_CONFIG_FILES([Makefile
	po/Makefile.in
])

AC_OUTPUT

dnl
dnl Report Configuration
dnl
AC_MSG_RESULT([])
AC_MSG_RESULT([ Configuration summary for $PACKAGE])
AC_MSG_RESULT([    version:  $VERSION])
AC_MSG_RESULT([    CFLAGS:   $CFLAGS])
AC_MSG_RESULT([    CPPFLAGS: $CPPFLAGS])
AC_MSG_RESULT([    LDFLAGS:  $LDFLAGS])
AC_MSG_RESULT([])
AC_MSG_RESULT([ Default installation directories:])
AC_MSG_RESULT([    program:  ${prefix}/bin/ ])
dnl The default 'datarootdir' is unexpanded, still contains
dnl '${prefix}/share' (not the single quotes)
if test "x$datarootdir" = 'x${prefix}/share' ; then
AC_MSG_RESULT([    examples: ${prefix}/share/${PACKAGE_NAME}/examples])
else
AC_MSG_RESULT([    examples: ${datarootdir}/${PACKAGE_NAME}/examples])
fi
AC_MSG_RESULT([])
AC_MSG_RESULT([ To change installation path, re-run:])
AC_MSG_RESULT([    ./configure --prefix NEW-PATH])
AC_MSG_RESULT([])

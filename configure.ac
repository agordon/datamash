dnl Copyright (C) 2013-2020 Assaf Gordon <assafgordon@gmail.com>
dnl
dnl This file is free software; as a special exception the author gives
dnl unlimited permission to copy and/or distribute it, with or without
dnl modifications, as long as this notice is preserved.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
dnl implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT([GNU datamash],
	[m4_esyscmd([build-aux/git-version-gen .tarball-version])],
	[assafgordon@gmail.com], [],
	[https://www.gnu.org/software/datamash])

dnl Must come before AM_INIT_AUTOMAKE.
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([
	1.11.1
	parallel-tests
	subdir-objects
])

# Minimum Autoconf version required.
AC_PREREQ([2.69])

# Where to generate output; srcdir location.
AC_CONFIG_HEADERS([config.h:config.in])dnl Keep filename to 8.3 for MS-DOS.
AC_CONFIG_SRCDIR([src/datamash.c])

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

# Add extra compilation warnings. These will only apply to datamash.c not to
# any gnulib modules). See $(WARN_CFLAGS) in Makefile.am
gl_WARN_ADD([-Wall])
gl_WARN_ADD([-Wextra])
gl_WARN_ADD([-Wformat-security])
gl_WARN_ADD([-Wswitch-enum])
gl_WARN_ADD([-Wswitch-default])
gl_WARN_ADD([-Wunused-parameter])
gl_WARN_ADD([-Wfloat-equal])
gl_WARN_ADD([-fdiagnostics-show-option])
gl_WARN_ADD([-funit-at-a-time])
gl_WARN_ADD([-Wmissing-format-attribute])
gl_WARN_ADD([-Wstrict-overflow])
gl_WARN_ADD([-Wsuggest-attribute=const])
gl_WARN_ADD([-Wsuggest-attribute=pure])
AC_SUBST([WARN_CFLAGS])


# Enable lint checks, for coverage/static-analyzers.
AC_ARG_ENABLE([lint],
              [AS_HELP_STRING([--enable-lint],[enable lint])],
              [case $enableval in
                 yes|no) ;;
                 *)   AC_MSG_ERROR([bad value $enableval for lint option]) ;;
               esac
               use_lint=$enableval],
              [use_lint=no]
              )
if test "$use_lint" = yes ; then
  AC_DEFINE([lint], [1], [Define to 1 if the compiler is checking for lint.])
fi



## Add --enable-werror option to ./configure,
## To enable/disable treating compiler warnings as errors.
## If not specified AND we're compiling from .git repository,
## enable this automatically.
AC_ARG_ENABLE([werror],
  [AS_HELP_STRING([--enable-werror],
     [treat compiler warnings as errors (for developers)])],
  [case $enableval in
     yes|no) ;;
     *)      AC_MSG_ERROR([bad value $enableval for werror option]) ;;
   esac
   werror=$enableval],
   [test -d "$srcdir"/.git \
     && ! test -f "$srcdir"/.tarball-version \
     && werror=yes]
)
if test "$werror" = yes; then
  gl_WARN_ADD([-Werror], [WERROR_CFLAGS])
  AC_SUBST([WERROR_CFLAGS])
fi

## Test for CygWin hosts - these have a broken 'strtold()' implementation:
##  calling strtold("4\t5",NULL) returns "e+9999",
## whereas every other (sane) system returns "4" and stops at the whitespace.
case "$host" in
  *-cygwin) have_broken_strtold=yes ;;
esac
if test "x$have_broken_strtold" = "xyes" ; then
AC_DEFINE([HAVE_BROKEN_STRTOLD],[1],
          [Define to 1 if strtold does not work properly (e.g. in cygwin)])
fi


## Check for bash-completion using pkg-config
##   ./configure --with-bash-completion-dir=[no|local|global|PATH] .
## See README for details.
AC_ARG_WITH([bash-completion-dir],
    AS_HELP_STRING([--with-bash-completion-dir[=PATH]],
        [Where to install the bash auto-completion script: no|local|global|PATH.
         @<:@default=local@:>@]),
    [],
    [with_bash_completion_dir=local])

if test "x$with_bash_completion_dir" = "xlocal" ; then
  bashcompdir="$datarootdir/${PACKAGE}/bash-completion.d"
elif test "x$with_bash_completion_dir" = "xglobal"; then
  PKG_CHECK_VAR(bashcompdir, [bash-completion], [completionsdir], ,
                bashcompdir="$datarootdir/${PACKAGE}/bash-completion.d")
else
  # either 'no', or a user-specified custom DIR - use it.
  bashcompdir="$with_bash_completion_dir"
fi
AC_SUBST(bashcompdir)
AM_CONDITIONAL([ENABLE_BASH_COMPLETION],
               [test "x$with_bash_completion_dir" != "xno"])


##
## Check if the system's sort support "-s" (stable) sort.
##  'sort' from GNU Coreutils, FreeBSD, OpenBSD, Busybox support it.
##  DilOS (illumos/opensolaris) does not.
##
if sort -s < /dev/null > /dev/null 2>/dev/null ; then
  AC_DEFINE([HAVE_STABLE_SORT],[1],
            [Define to 1 if sort on this system supports -s for stable sorting])
fi

## Are we building from git checked-out sources?
## (if not, assume it's from a tarball)
## This is used in "Makefile.am" to avoid re-generating the manpage
## when building from tarballs.
AM_CONDITIONAL([BUILD_FROM_GIT], [test -d "$srcdir/.git"])


## Perl is needed for some tests (but not required for building)
## 'PERL_FOUND' is used in Makefile.am to skip some tests.
AC_PATH_PROG([PERL], [perl])
AC_SUBST([PERL])
AM_CONDITIONAL([PERL_FOUND],[test -n "$PERL"])

# i18n support from GNU gettext.
AM_GNU_GETTEXT_VERSION([0.19.4])
AM_GNU_GETTEXT([external])

AC_CONFIG_FILES([Makefile
  po/Makefile.in
])


# This is needed when building outside the source dir
# with --disable-dependency-tracking.
# Inspired by sed's https://bugs.gnu.org/25371
AS_MKDIR_P([lib])
AS_MKDIR_P([src])
AS_MKDIR_P([doc])
AS_MKDIR_P([tests])

AC_OUTPUT

dnl
dnl Report Configuration
dnl
AC_MSG_RESULT([])
AC_MSG_RESULT([ Configuration summary for $PACKAGE])
AC_MSG_RESULT([    version:  $VERSION])
AC_MSG_RESULT([    COMPILER: $CC])
AC_MSG_RESULT([    CFLAGS:   $CFLAGS $WARN_CFLAGS $MINGW_CFLAGS])
AC_MSG_RESULT([    CPPFLAGS: $CPPFLAGS])
AC_MSG_RESULT([    LDFLAGS:  $LDFLAGS])

# Show which sha* implementation is used
# for hash functions (md5/sha*)
if test "x$LIB_CRYPTO" = x ; then
  lib_crypto_desc="internal (gnulib)"
else
  lib_crypto_desc="external ($LIB_CRYPTO)"
fi
AC_MSG_RESULT([    md5/sha*: $lib_crypto_desc])

AC_MSG_RESULT([])
AC_MSG_RESULT([ Default installation directories:])
AC_MSG_RESULT([    program:   ${prefix}/bin/ ])
eval example_dir=${datarootdir}/${PACKAGE}/examples
AC_MSG_RESULT([    examples:  ${example_dir}])

if test "x$with_bash_completion_dir" != "xno" ; then
   eval bash_comp_dir=$bashcompdir
else
   bash_comp_dir="Not installed"
fi
AC_MSG_RESULT([    bash-comp: ${bash_comp_dir}])
AC_MSG_RESULT([])
AC_MSG_RESULT([ To change installation path, re-run:])
AC_MSG_RESULT([    ./configure --prefix NEW-PATH])
AC_MSG_RESULT([])

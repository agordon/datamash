# Copyright (C) 2013-2021 Assaf Gordon <assafgordon@gmail.com>
# Copyright (C) 2022-2025 Timothy Rice <trice@posteo.net>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS=subdir-objects

AM_CPPFLAGS = -Ilib -I$(top_srcdir)/lib -Isrc -I$(top_srcdir)/src

EXTRA_DIST = \
	build-aux/make_bin_dist.sh \
	build-aux/prerelease-checks.sh \
	build-aux/git-log-fix \
	build-aux/create_small_file_system.sh \
	build-aux/create_corrupted_file_system.sh \
	contrib/bash-completion/datamash \
	m4/gnulib-cache.m4

SUBDIRS = po

##
## Main Program
##
bin_PROGRAMS = datamash decorate

datamash_SOURCES = src/system.h \
	       src/die.h \
	       src/text-options.c src/text-options.h \
	       src/utils.c src/utils.h \
	       src/randutils.c src/randutils.h \
	       src/text-lines.c src/text-lines.h \
	       src/column-headers.c src/column-headers.h \
	       src/op-defs.c src/op-defs.h \
	       src/op-scanner.c src/op-scanner.h \
	       src/op-parser.c src/op-parser.h \
	       src/field-ops.c src/field-ops.h \
	       src/crosstab.c src/crosstab.h \
	       src/double-format.c src/double-format.h \
	       src/datamash.c

datamash_CFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS) $(MINGW_CFLAGS)
datamash_LDADD = \
       lib/lib$(PACKAGE).a \
       $(CEIL_LIBM) \
       $(CEILL_LIBM) \
       $(EXP_LIBM) \
       $(EXPL_LIBM) \
       $(FABS_LIBM) \
       $(FABSL_LIBM) \
       $(FLOOR_LIBM) \
       $(FLOORL_LIBM) \
       $(FREXP_LIBM) \
       $(FREXPL_LIBM) \
       $(ISNANL_LIBM) \
       $(LDEXP_LIBM) \
       $(LDEXPL_LIBM) \
       $(LIB_CRYPTO) \
       $(LIB_HARD_LOCALE) \
       $(LIB_MBRTOWC) \
       $(LIB_SETLOCALE) \
       $(LIB_SETLOCALE_NULL) \
       $(LIBICONV) \
       $(LIBINTL) \
       $(LIBTHREAD) \
       $(LOG_LIBM) \
       $(LOGL_LIBM) \
       $(MODF_LIBM) \
       $(MODFL_LIBM) \
       $(ROUND_LIBM) \
       $(ROUNDL_LIBM) \
       $(SQRT_LIBM) \
       $(SQRTL_LIBM) \
       $(TRUNC_LIBM) \
       $(TRUNCL_LIBM) \
       $(INTL_MACOSX_LIBS)

decorate_SOURCES = src/decorate.c \
	src/key-compare.c src/key-compare.h \
	src/decorate-functions.c src/decorate-functions.h
decorate_CFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS) $(MINGW_CFLAGS)
decorate_CPPFLAGS = $(AM_CPPFLAGS) \
	-DKEY_COMPARE_DECORATION \
	-DKEY_COMPARE_HUMAN_NUMERIC \
	-DKEY_COMPARE_NONPRINTING \
	-DKEY_COMPARE_MONTH \
	-DKEY_COMPARE_RANDOM \
	-DKEY_COMPARE_REVERSE \
	-DKEY_COMPARE_VERSION

decorate_LDADD = lib/lib$(PACKAGE).a \
       $(INET_PTON_LIB) \
       $(LIBICONV) \
       $(LIBINTL) \
       $(LIB_MBRTOWC)

# Add LOCALDIR for gettext
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@


##
## Man Page
##

man_MANS = datamash.1 decorate.1
EXTRA_DIST += $(man_MANS) \
		man/datamash.x \
		man/decorate.x \
		man/help2man

CLEANFILES =

## Re-generate the man-page from the --help screen only if building from
##   a GIT repository (implying developers).
## When building from tar-ball, the manpage should already exist.
## (This conditional is defined in 'configure.ac')
## NOTE:
##  If user is building from GIT *and* cross-compiling,
##  the manpage would be required, but can't be generated.
##  'make' will fail with an error.
##  If this is ever a real-world issue, deal with it...
if BUILD_FROM_GIT
datamash.1: datamash $(top_srcdir)/man/datamash.x
	PATH='$(abs_top_builddir)$(PATH_SEPARATOR)'$$PATH \
	   perl $(abs_top_srcdir)/man/help2man \
	       --include=$(top_srcdir)/man/datamash.x \
	       datamash -o $@-t
	chmod u=rw,go=r $@-t
	mv -f $@-t $@

decorate.1: decorate $(top_srcdir)/man/decorate.x
	PATH='$(abs_top_builddir)$(PATH_SEPARATOR)'$$PATH \
	   perl $(abs_top_srcdir)/man/help2man \
	       --include=$(top_srcdir)/man/decorate.x \
	       decorate -o $@-t
	chmod u=rw,go=r $@-t
	mv -f $@-t $@

CLEANFILES += $(man_MANS)
endif

##
## Tests
##
TEST_EXTENSIONS = .pl .sh
if PERL_FOUND
PL_LOG_COMPILER=$(PERL)
else
PL_LOG_COMPILER=$(SHELL) $(top_srcdir)/tests/no-perl
endif


TESTS = \
	tests/datamash-show-env.sh \
	tests/datamash-tests.pl \
	tests/datamash-tests-deprecated.pl \
	tests/datamash-tests-2.pl \
	tests/datamash-tests-2-deprecated.pl \
	tests/datamash-parser.pl \
	tests/datamash-error-msgs.pl \
	tests/datamash-md5.pl \
	tests/datamash-sha.pl \
	tests/datamash-rand.sh \
	tests/datamash-rand.pl \
	tests/datamash-stats.pl \
	tests/datamash-transpose.pl \
	tests/datamash-crosstab.pl \
	tests/datamash-check.pl \
	tests/datamash-pair-tests.pl \
	tests/datamash-check-tabular.pl \
	tests/datamash-output-format.pl \
	tests/datamash-i18n-de.pl \
	tests/datamash-sort-header.pl \
	tests/datamash-sort-header-deprecated.pl \
	tests/datamash-sort-errors.sh \
	tests/datamash-io-errors.sh \
	tests/datamash-io-errors-cheap.sh \
	tests/datamash-strbin.sh \
	tests/datamash-valgrind.sh \
	tests/datamash-vnlog.pl \
	tests/decorate-tests.pl \
	tests/decorate-errors.pl \
	tests/decorate-sort-tests.pl

EXTRA_DIST += \
	tests/no-perl \
	tests/Coreutils.pm \
	tests/CuSkip.pm \
	tests/CuTmpdir.pm \
	tests/init.sh \
	init.cfg \
	$(TESTS)

# Note: the "9>&2" redirection is part of Automake's parallel-tests.
#       see also stderr_fileno in 'init.cfg'
TESTS_ENVIRONMENT = \
  top_srcdir=$(abs_top_srcdir) \
  PERL5LIB='$(abs_top_srcdir)/tests$(PATH_SEPARATOR)'"$$PERL5LIB" \
  PATH='$(abs_top_builddir)$(PATH_SEPARATOR)'$$PATH \
  DATAMASH=`echo datamash | sed '$(transform)'` \
  DECORATE=`echo decorate | sed '$(transform)'` \
  test_dir='$(abs_top_srcdir)/tests' ; \
  export top_srcdir PERL5LIB PATH DATAMASH DECORATE test_dir ; \
  9>&2

##
## For 'make syntax-check'
##
EXTRA_DIST += \
	cfg.mk



##
## Examples
##

## These example files will be installed in the
## "{prefix}/share/APPNAME/examples" directory
## (default "/usr/local/share/datamash/examples")
examplesdir = $(pkgdatadir)/examples

dist_examples_DATA = examples/readme.md \
		     examples/scores.txt examples/scores_h.txt \
		     examples/genes.txt examples/genes_h.txt

EXTRA_DIST += $(dist_examples_DATA)

if ENABLE_BASH_COMPLETION
dist_bashcomp_DATA = contrib/bash-completion/datamash
endif




##
## PO/gettext check
##

# Verify that all source files using _() are listed in po/POTFILES.in.
# The idea is to run this before making pretests, as well as official
# releases, so that translators will be sure to have all the messages.
# (From coreutils.)
po-check:
	if test -f po/POTFILES.in; then					\
	  grep -E -v '^(#|$$)' po/POTFILES.in				\
	    | grep -v '^src/false\.c$$' | sort > $@-1;			\
	  files=;							\
	  for file in $$($(CVS_LIST_EXCEPT)) `find * -name '*.[ch]'`; do \
	    case $$file in						\
	    gnulib/* | man/*) continue;;				\
	    esac;							\
	    case $$file in						\
	    *.[ch])							\
	      base=`expr " $$file" : ' \(.*\)\..'`;			\
	      { test -f $$base.l || test -f $$base.y; } && continue;;	\
	    esac;							\
	    files="$$files $$file";					\
	  done;								\
	  grep -E -l '\b(N?_|gettext *)\([^)"]*("|$$)' $$files		\
	    | sort -u > $@-2;						\
	  diff -u $@-1 $@-2 || exit 1;					\
	  rm -f $@-1 $@-2;						\
	fi

##
## GnuLib
##

LOG_COMPILER =
MAINTAINERCLEANFILES=
noinst_LIBRARIES =
MOSTLYCLEANFILES =
MOSTLYCLEANDIRS =
BUILT_SOURCES =

##
## Documentation, using Gnulib's 'gendocs' module
##
gendocs = build-aux/gendocs.sh
gendocs_envvars = GENDOCS_TEMPLATE_DIR=doc
#
manual = datamash
manual_title=GNU datamash
email = $(PACKAGE_BUGREPORT)
gendocs_args = --email $(email) $(manual) $(manual_title)

# This will affect the result of 'make web-manual'
# all generated HTML files will contain the embedded CSS statements.
# 'gendocs_options_' is used in the 'web-manual' target in 'maint.mk'.
gendocs_options_=\
    --html "--css-include=$(abs_top_srcdir)/doc/datamash-texinfo.css"


include $(top_srcdir)/lib/local.mk
include $(top_srcdir)/doc/local.mk

##
## Compute program's code complexity
## using gnulib's pmccabe2html module

cyclo_sources = ${top_srcdir}/src/*.[ch]
svweburl="https://git.savannah.gnu.org/cgit/"

cyclo-$(PACKAGE).html: $(cyclo_sources)
	$(PMCCABE) $(cyclo_sources) \
		| sort -nr \
		| $(AWK) -f ${top_srcdir}/build-aux/pmccabe2html \
			-v lang=html -v name="$(PACKAGE_NAME)" \
			-v vcurl="$(svweburl)/$(PACKAGE).git/tree/%FILENAME%" \
			-v url="https://www.gnu.org/software/$(PACKAGE)/" \
			-v css=${top_srcdir}/build-aux/pmccabe.css \
			-v cut_dir=${top_srcdir}/ \
			> $@-tmp
	mv $@-tmp $@

##
##
##

dist-hook: gen-ChangeLog
	$(AM_V_GEN)echo $(VERSION) > $(distdir)/.tarball-version

gen_start_date = 2011-12-17
.PHONY: gen-ChangeLog
gen-ChangeLog:
	$(AM_V_GEN)if test -d .git; then				\
	  $(top_srcdir)/build-aux/gitlog-to-changelog			\
	    --amend=$(srcdir)/build-aux/git-log-fix			\
	    --since=$(gen_start_date) > $(distdir)/cl-t;		\
	  rm -f $(distdir)/ChangeLog;					\
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;			\
	fi

# Shortcut targets to make it easier to run (very) expensive tests.
check-expensive:
	$(MAKE) check RUN_EXPENSIVE_TESTS=yes
check-very-expensive:
	$(MAKE) check-expensive RUN_VERY_EXPENSIVE_TESTS=yes

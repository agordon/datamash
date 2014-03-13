# Copyright (C) 2013-2014 Assaf Gordon <assafgordon@gmail.com>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -Ilib -I$(top_srcdir)/lib -Isrc -I$(top_srcdir)/src

EXTRA_DIST = \
        build-aux/make_bin_dist.sh \
        build-aux/get_travis_dist.sh \
        build-aux/rebuild-coverage.sh \
        build-aux/gen-coverage-report.sh \
	build-aux/git-log-fix \
	m4/gnulib-cache.m4 \
	man/calc.x

SUBDIRS = po

##
## Main Program
##
bin_PROGRAMS = calc

calc_SOURCES = src/system.h \
	       src/text-options.c src/text-options.h \
	       src/utils.c src/utils.h \
	       src/text-lines.c src/text-lines.h \
	       src/column-headers.c src/column-headers.h \
	       src/field-ops.c src/field-ops.h \
	       src/calc.c

calc_LDADD = \
       $(top_builddir)/lib/lib$(PACKAGE).a \
       $(FABSL_LIBM) \
       $(FABS_LIBM) \
       $(FREXPL_LIBM) \
       $(FREXP_LIBM) \
       $(ISNANL_LIBM) \
       $(LDEXPL_LIBM) \
       $(LDEXP_LIBM) \
       $(LIBICONV) \
       $(LIBINTL) \
       $(SQRTL_LIBM) \
       $(SQRT_LIBM)

# Add LOCALDIR for gettext
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@


##
## Man Page
##

man_MANS = calc.1
EXTRA_DIST += $(man_MANS)
CLEANFILES = $(man_MANS)

calc.1: calc
	$(HELP2MAN) --include=$(top_srcdir)/man/calc.x $(top_builddir)/calc -o $@-t
	chmod a=r $@-t
	mv -f $@-t $@

##
## Tests
##
TESTS = \
	tests/calc-show-env.sh \
	tests/calc-tests.pl \
	tests/calc-sort-header.sh \
	tests/calc-sort-errors.sh \
	tests/calc-valgrind.sh

EXTRA_DIST += \
	tests/Coreutils.pm \
	tests/CuSkip.pm \
	tests/CuTmpdir.pm \
	tests/init.sh \
	$(TESTS)

TESTS_ENVIRONMENT = \
  top_srcdir=$(abs_top_srcdir) \
  PERL5LIB='$(abs_top_srcdir)/tests$(PATH_SEPARATOR)'"$$PERL5LIB" \
  PATH='$(abs_top_builddir)$(PATH_SEPARATOR)'$$PATH \
  CALC=`echo calc | sed '$(transform)'` \
  test_dir='$(abs_top_srcdir)/tests' ; \
  export top_srcdir PERL5LIB PATH CALC test_dir ;

##
## For 'make syntax-check'
##
EXTRA_DIST += \
	cfg.mk



##
## Examples
##

## These example files will be installed in the "{prefix}/share/APPNAME/examples"
## directory (default "/usr/local/share/calc/examples")
examplesdir = $(pkgdatadir)/examples

dist_examples_DATA = examples/readme.md \
		     examples/scores.txt examples/scores_h.txt \
		     examples/genes.txt examples/genes_h.txt

EXTRA_DIST += $(dist_examples_DATA)




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


include $(top_srcdir)/lib/local.mk
#include $(top_srcdir)/doc/local.mk

##
##
##

dist-hook: gen-ChangeLog

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

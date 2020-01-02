# Copyright (C) 2014-2020 Assaf Gordon <assafgordon@gmail.com>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

#
# Syntax-Check rules (sc_XXX) copied from GNU Coreutils' cfg.mk file:
# Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011,
# 2012, 2013, 2014, 2017 Free Software Foundation, Inc.
#

# Syntax-checks to skip:
#  sc_copyright_check: requires the word 'Free' in texi after year.
#  sc_prohibit_strings_without_use: using memset/memmove requirs <string.h>
local-checks-to-skip = \
  sc_copyright_check \
  sc_prohibit_strings_without_use

# Hash of the current/old NEWS file.
# checked by 'sc_immutable_NEWS' rule.
# updated by 'update-NEWS-hash' rule.
old_NEWS_hash = d41d8cd98f00b204e9800998ecf8427e

update-copyright-env = \
  env \
  UPDATE_COPYRIGHT_HOLDER='Assaf Gordon' \
  UPDATE_COPYRIGHT_USE_INTERVALS=2



# Helper rules to compile with Debian-Hardening flags
# See https://wiki.debian.org/Hardening
init-deb-hard:
	$(MAKE) $(AM_MAKEFLAGS) clean

build-deb-hard:
	$(MAKE) $(AM_MAKEFLAGS) \
           CFLAGS="$$(dpkg-buildflags --get CFLAGS)" \
           CPPFLAGS="$$(dpkg-buildflags --get CPPFLAGS)" \
           LDFLAGS="$$(dpkg-buildflags --get LDFLAGS)"

deb-hard: init-deb-hard build-deb-hard check

# Coverage - clean 'gcda/gcno' files when initializing coverage.
# (NOTE: 'init-coverage' is defined in 'maint.mk'.
#        Here another clean step is added to this target)
init-coverage: clean-coverage-files
clean-coverage-files:
	 find $$PWD -name '*.gcno' -o -name '*.gcda' | xargs rm -f --
# Generate coverage information, including expensive tests for better coverage
coverage-expensive:
	$(MAKE) RUN_EXPENSIVE_TESTS=yes coverage

# Exclude markdown (*.md) files from no-trailing-blanks rule.
# Exclude the second unit test file - some of the expected tests
# have trailing spaces.
# The exclusion list is initially defined in ./gnulib/cfg.mk ,
#   and is overridden here.
# The 'sc_trailing_blank' rule is defined in ./maint.mk
#    (which is auto-generated from gnulib).
exclude_file_name_regexp--sc_trailing_blank = \
  ^(.*\.md|.*datamash-tests-2\.pl)$$


# Scan-Build: use clang's static analysis tool
static-analysis-have-prog:
	which scan-build 1>/dev/null 2>&1 || \
	    { echo "scan-build program not found" >&2 ; exit 1 ;}

static-analysis-configure: static-analysis-have-prog
	test -x ./configure || \
	    { echo "./configure script not found" >&2 ; exit 1 ;}
	scan-build ./configure

static-analysis-make: static-analysis-have-prog
	$(MAKE) clean
	scan-build make CFLAGS="-g -O0 -std=c99"

static-analysis: static-analysis-configure static-analysis-make


# Look for lines longer than 80 characters, except omit:
# - the help2man script copied from upstream,
# - example files
# - Markdown files
LINE_LEN_MAX = 80
FILTER_LONG_LINES =						\
  \|^[^:]*man/help2man:| d;			\
  \|^[^:]*examples/.*| d;			\
  \|^HACKING\.md| d;
sc_long_lines:
	@files=$$($(VC_LIST_EXCEPT))					\
	halt='line(s) with more than $(LINE_LEN_MAX) characters; reindent'; \
	for file in $$files; do						\
	  expand $$file | grep -nE '^.{$(LINE_LEN_MAX)}.' |		\
	  sed -e "s|^|$$file:|" -e '$(FILTER_LONG_LINES)';		\
	done | grep . && { msg="$$halt" $(_sc_say_and_exit) } || :

# Ensure that the end of each release's section is marked by two empty lines.
sc_NEWS_two_empty_lines:
	@sed -n 4,/Noteworthy/p $(srcdir)/NEWS				\
	    | perl -n0e '/(^|\n)\n\n\* Noteworthy/ or exit 1'		\
	  || { echo '$(ME): use two empty lines to separate NEWS sections' \
		 1>&2; exit 1; } || :

# With split lines, don't leave an operator at end of line.
# Instead, put it on the following line, where it is more apparent.
# Don't bother checking for "*" at end of line, since it provokes
# far too many false positives, matching constructs like "TYPE *".
# Similarly, omit "=" (initializers).
binop_re_ ?= [-/+^!<>]|[-/+*^!<>=]=|&&?|\|\|?|<<=?|>>=?
sc_prohibit_operator_at_end_of_line:
	@prohibit='. ($(binop_re_))$$'					\
	in_vc_files='\.[chly]$$'					\
	halt='found operator at end of line'				\
	  $(_sc_search_regexp)

# Indent only with spaces.
tbi_1 = ^tests/pr/|(^gl/lib/reg.*\.c\.diff|\.mk|^man/help2man)$$
tbi_2 = ^scripts/git-hooks/(pre-commit|pre-applypatch|applypatch-msg)$$
tbi_3 = (GNU)?[Mm]akefile(\.am)?$$|$(_ll)
exclude_file_name_regexp--sc_prohibit_tab_based_indentation = \
  $(tbi_1)|$(tbi_2)|$(tbi_3)
sc_prohibit_tab_based_indentation:
	@prohibit='^ *	'						\
	halt='TAB in indentation; use only spaces'			\
	  $(_sc_search_regexp)

# Use print_ver_ (from init.cfg), not open-coded $VERBOSE check.
sc_prohibit_verbose_version:
	@prohibit='test "\$$VERBOSE" = yes && .* --version'		\
	halt='use the print_ver_ function instead...'			\
	  $(_sc_search_regexp)

# Use framework_failure_, not the old name without the trailing underscore.
sc_prohibit_framework_failure:
	@prohibit='\<framework_''failure\>'				\
	halt='use framework_failure_ instead'				\
	  $(_sc_search_regexp)

# Prohibit the use of `...` in tests/.  Use $(...) instead.
exclude_file_name_regexp--sc_prohibit_test_backticks = \
  ^tests/(local\.mk|init\.sh|[a-z0-9\-]*\.pl)$$
sc_prohibit_test_backticks:
	@prohibit='`' in_vc_files='^tests/'				\
	halt='use $$(...), not `...` in tests/'				\
	  $(_sc_search_regexp)

# Ensure that compare is used to check empty files
# so that the unexpected contents are displayed
sc_prohibit_test_empty:
	@prohibit='test -s.*&&' in_vc_files='^tests/'			\
	halt='use `compare /dev/null ...`, not `test -s ...` in tests/'	\
	  $(_sc_search_regexp)

_space_before_paren_exempt =? \\n\\$$
_space_before_paren_exempt = \
  (^ *\#|\\n\\$$|%s\(to %s|(date|group|character)\(s\)|STREQ_LEN)
# Ensure that there is a space before each open parenthesis in C code.
sc_space_before_open_paren:
	@if $(VC_LIST_EXCEPT) | grep -l '\.[ch]$$' > /dev/null; then	\
	  fail=0;							\
	  for c in $$($(VC_LIST_EXCEPT) | grep '\.[ch]$$'); do		\
	    sed '$(_sed_rm_comments_q)' $$c 2>/dev/null			\
	      | grep -i '[[:alnum:]]('					\
	      | grep -vE '$(_space_before_paren_exempt)'		\
	      | grep . && { fail=1; echo "*** $$c"; };			\
	  done;								\
	  test $$fail = 1 &&						\
	    { echo '$(ME): the above files lack a space-before-open-paren' \
		1>&2; exit 1; } || :;					\
	else :;								\
	fi

# Similar to the gnulib maint.mk rule for sc_prohibit_strcmp
# Use STREQ_LEN or STRPREFIX rather than comparing strncmp == 0, or != 0.
sc_prohibit_strncmp:
	@grep -nE '! *str''ncmp *\(|\<str''ncmp *\(.+\) *[!=]='		\
	    $$($(VC_LIST_EXCEPT))					\
	  | grep -vE ':# *define STR(N?EQ_LEN|PREFIX)\(' &&		\
	  { echo '$(ME): use STREQ_LEN or STRPREFIX instead of str''ncmp' \
		1>&2; exit 1; } || :

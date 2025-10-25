#!/bin/sh

#   Unit Tests for GNU Decorate - auxiliary program for sort preprocessing

#    Copyright (C) 2014-2021 Assaf Gordon <assafgordon@gmail.com>
#    Copyright (C) 2025 Erik Auerswald <auerswal@unix-ag.uni-kl.de>
#
#    This file is part of GNU Datamash.
#
#    GNU Datamash is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    GNU Datamash is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with GNU Datamash  If not, see <https://www.gnu.org/licenses/>.
#
#    Written by Assaf Gordon.

. "${test_dir=.}/init.sh"; path_prepend_ ./src

require_valgrind_

## Don't use valgrind on statically-compiled binary
## (it gives some false-positives and the test fails).
if which ldd >/dev/null ; then
  ## Tricky implicit assumption:
  ## If the system has "ldd" - we can test if this is a static binary.
  ## If the system doesn't have "ldd", we can't test it, and we'll assume
  ## we can valgrind without false-positives.
  ## This is relevant for Mac OS X, where static binaries are discouraged and
  ## difficult to create
  ## (https://developer.apple.com/library/mac/qa/qa1118/_index.html)
  ldd $(which decorate) >/dev/null 2>/dev/null ||
    skip_ "skipping valgrind test for a non-dynamic-binary decorate"
fi


fail=0

# check fix for buffer under-read (CWE-127) reported by Frank Busse in
# <https://lists.gnu.org/archive/html/bug-datamash/2025-10/msg00000.html>
echo | valgrind --error-exitcode=1 decorate --undecorate 6 > /dev/null ||
  { warn_ "--undecorate 6 buffer under-read - failed" ; fail=1 ; }

Exit $fail

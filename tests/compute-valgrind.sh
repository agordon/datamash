#!/bin/sh

#   Unit Tests for compute - perform simple calculation on input data
#   Copyright (C) 2014 Assaf Gordon.
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#    Written by Assaf Gordon

. "${test_dir=.}/init.sh"; path_prepend_ ./src

## Ensure valgrind is useable
## (copied from coreutils' init.cfg)
valgrind --error-exitcode=1 true 2>/dev/null ||
    skip_ "requires a working valgrind"

## Don't use valgrind on statically-compiled binary
## (it gives some false-positives and the test fails).
if which ldd >/dev/null ; then
  ## Tricky implicit assumption:
  ## If the system has "ldd" - we can test if this is a static binary.
  ## If the system doesn't have "ldd", we can't test it, and we'll assume
  ## we can valgrind without false-positives.
  ## This is relevant for Mac OS X, where static binaries are discouraged and
  ## difficult to create (https://developer.apple.com/library/mac/qa/qa1118/_index.html)
  ldd $(which compute) >/dev/null 2>/dev/null ||
    skip_ "skipping valgrind test for a non-dynamic-binary compute"
fi

fail=0

seq 10000 | valgrind --track-origins=yes  --show-reachable=yes \
                     --leak-check=full  --error-exitcode=1 \
                 compute unique 1 > /dev/null || { warn_ "unique 1 - failed" ; fail=1 ; }

seq 10000 | sed 's/^/group /' |
     valgrind --track-origins=yes  --leak-check=full \
              --show-reachable=yes  --error-exitcode=1 \
                 compute -g 1 unique 1 > /dev/null || { warn_ "-g 1 unique 1 - failed" ; fail=1 ; }

seq 10000 | valgrind --track-origins=yes  --leak-check=full \
                     --show-reachable=yes  --error-exitcode=1 \
                 compute countunique 1 > /dev/null || { warn_ "countunique 1 - failed" ; fail=1 ; }

seq 10000 | valgrind --track-origins=yes  --leak-check=full  \
                     --show-reachable=yes  --error-exitcode=1 \
                 compute collapse 1 > /dev/null || { warn_ "collapse 1 - failed" ; fail=1 ; }

(echo "values" ; seq 10000 ) | valgrind --track-origins=yes  --leak-check=full \
                                        --show-reachable=yes  --error-exitcode=1 \
                 compute -H countunique 1 > /dev/null || { warn_ "-H collapse 1 - failed" ; fail=1 ; }

(echo "values" ; seq 10000 ) | valgrind --track-origins=yes  --leak-check=full \
                                        --show-reachable=yes  --error-exitcode=1 \
                 compute -g 1 -H countunique 1 > /dev/null || { warn_ "-g 1 -H collapse 1 - failed" ; fail=1 ; }

Exit $fail

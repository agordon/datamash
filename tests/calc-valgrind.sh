#!/bin/sh

. "${test_dir=.}/init.sh"; path_prepend_ ./src

## Ensure valgrind is useable
## (copied from coreutils' init.cfg)
valgrind --error-exitcode=1 true 2>/dev/null ||
    skip_ "requires a working valgrind"

fail=0

seq 10000 | valgrind --leak-check=full --error-exitcode=1 \
                 calc unique 1 > /dev/null || { warn_ "unique 1 - failed" ; fail=1 ; }

seq 10000 | sed 's/^/group /' |
     valgrind --leak-check=full --error-exitcode=1 \
                 calc -g 1 unique 1 > /dev/null || { warn_ "-g 1 unique 1 - failed" ; fail=1 ; }

seq 10000 | valgrind --leak-check=full --error-exitcode=1 \
                 calc countunique 1 > /dev/null || { warn_ "countunique 1 - failed" ; fail=1 ; }

seq 10000 | valgrind --leak-check=full --error-exitcode=1 \
                 calc collapse 1 > /dev/null || { warn_ "collapse 1 - failed" ; fail=1 ; }

Exit $fail


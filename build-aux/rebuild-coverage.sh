#!/bin/sh

##
## A small helper script to rebuild the project with coverage instrumentation.
##

die()
{
  BASE=$(basename "$0")
  echo "$BASE error: $@" >&2
  exit 1
}

cd $(dirname "$0")/.. || die "failed to set directory"

find "$PWD" \( -name "*.gcda" -o -name "*.gcno" \) -delete || die "deleting existing gcda/gcno files failed"

./configure CFLAGS="-g -fprofile-arcs -ftest-coverage" || die "./configure failed"
make clean || die "make clean failed"
make || die "make failed"

## Automatically run the basic checks, at least once.
make check || die "make check failed"

## Generate the initial coverage report
DIR=$(dirname "$0") || die "failed to detect script's directory"
GEN="$DIR/gen-coverage-report.sh"
[ -e "$GEN" ] || die "Required script '$GEN' not found"

"$GEN" || die "failed to generate coverage report"

#!/bin/sh

## Copyright (C) 2014 Assaf Gordon <assafgordon@gmail.com>
##
## This file is part of Compute.
##
## Compute is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Compute is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Compute.  If not, see <http://www.gnu.org/licenses/>.


##
## A small helper script to re-generate coverage report.
##

die()
{
  BASE=$(basename "$0")
  echo "$BASE error: $@" >&2
  exit 1
}

cd $(dirname "$0")/.. || die "failed to set directory"

GCDAFILES=$(find -name "*.gcda") || die "failed to search for *.gcda files"
GCNOFILES=$(find -name "*.gcno") || die "failed to search for *.gcno files"
[ -z "$GCDAFILES" -o -z "$GCNOFILES" ] && die "No coverage files found (*.gcda/*.gcno) - did you rebuild with coverage instrumentation? try ./build-aux/rebuild-coverage.sh"

PROJECT=compute
LCOVFILE="$PROJECT.lcov"
REPORTDIR="$PROJECT-cov"

rm -f  "./$LCOVFILE"
rm -rf "./$REPORTDIR"

## Then generate the coverage report
lcov -t "$PROJECT" -q -d src -c -o "$LCOVFILE" || die "lcov failed"
genhtml -t "$PROJECT" --output-directory "$REPORTDIR" "$LCOVFILE" || die "genhtml failed"

echo
echo
echo "Initial coverage report: ./$REPORTDIR/index.html"
echo ""
echo "To accumulate more coverage information, run '$PROJECT' again,"
echo "then, re-generate the coverage report by re-running $0"
echo ""

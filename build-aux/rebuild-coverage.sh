#!/bin/sh

## Copyright (C) 2014-2015 Assaf Gordon <assafgordon@gmail.com>
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
## A small helper script to rebuild the project with coverage instrumentation.
##

die()
{
  BASE=$(basename "$0")
  echo "$BASE error: $@" >&2
  exit 1
}

cd $(dirname "$0")/.. || die "failed to set directory"

find "$PWD" \( -name "*.gcda" -o -name "*.gcno" \) -delete ||
  die "deleting existing gcda/gcno files failed"

./configure CFLAGS="-g -fprofile-arcs -ftest-coverage" ||
  die "./configure failed"
make clean || die "make clean failed"
make || die "make failed"

## Automatically run the basic checks, at least once.
make check || die "make check failed"

## Generate the initial coverage report
DIR=$(dirname "$0") || die "failed to detect script's directory"
GEN="$DIR/gen-coverage-report.sh"
[ -e "$GEN" ] || die "Required script '$GEN' not found"

"$GEN" || die "failed to generate coverage report"

#!/bin/sh

## Copyright (C) 2014 Assaf Gordon <assafgordon@gmail.com>
##
## This file is part of GNU Datamash.
##
## GNU Datamash is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## GNU Datamash is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Datamash.  If not, see <http://www.gnu.org/licenses/>.

## Push the current repository to a temporary branch,
## and check building from clean git repository on two systems.

PACKAGE=datamash

die()
{
BASE=$(basename "$0")
echo "$BASE: error: $@" >&2
exit 1
}

HOSTS="deb7 fbsd10"


git push gnu :prerelease_test  # ignore failure
git push gnu HEAD:prerelease_test || die "failed to git-push :prerelease_test"

./build-aux/check-remote-make-all.sh -b prerelease_test \
          git://git.savannah.gnu.org/$PACKAGE.git \
          $HOSTS


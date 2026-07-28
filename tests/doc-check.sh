#!/bin/sh

#   Check documentation

#    Copyright (C) 2026 Timothy Rice <trice@posteo.net> and Assaf Gordon
#    <assafgordon@gmail.com>
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
#    Written by Timothy Rice based on tests/datamash-valgrind.sh by Assaf
#    Gordon.

. "${test_dir=.}/init.sh"

fail=0

for man in datamash decorate; do
  [[ -z "$(nroff -man -ww -z ../${man}.1 2>&1)" ]] \
    || { warn_ "nroff detected issues in ${man}.1" ; fail=1 ; }
done

Exit $fail

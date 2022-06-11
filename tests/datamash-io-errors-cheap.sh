#!/bin/sh

# Unit Tests for GNU Datamash - simple I/O error simulation

# Copyright (C) 2022 Erik Auerswald <auerswal@unix-ag.uni-kl.de>
#
# This file is part of GNU Datamash.
#
# GNU Datamash is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GNU Datamash is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Datamash.  If not, see <https://www.gnu.org/licenses/>.
#
# Written by Erik Auerswald,
# based on datamash-io-errors.sh written by Assaf Gordon

##
## This script tests GNU Datamash's handling of basic I/O errors.
##

. "${test_dir=.}/init.sh"; path_prepend_ ./src

fail=0

##
## This test requires the special file /dev/full
##
test -w /dev/full || skip_ 'requires writable /dev/full'

## Test 1: output error
echo 0 | datamash -g 1 count 1 > /dev/full &&
	{ warn_ "datamash failed to detect no-space error" ; fail=1 ; }

Exit $fail

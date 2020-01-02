#!/bin/sh

#    Copyright (C) 2014-2020 Assaf Gordon <assafgordon@gmail.com>
#
#    datamash I/O error testing module
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
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
#    Written by Assaf Gordon.

BLOCK_SIZE=1024
IMGFILE=tiny_disk.img
LOG=log.txt

die()
{
	BASE=$(basename "$0")
	## Print the STDOUT and STDERR we have so far
	cat $LOG >&2
	echo "$BASE: error: $@" >&2
	exit 1
}

# Reset log
echo > $LOG

## Ugly hack: Add common places were required programs might be.
## This script is run as a non-root user, so these directories
##  might not be on the $PATH
PATH=$PATH:/sbin:/usr/sbin:/usr/local/sbin

for PROG in mkfs.ext3 ;
do
	which $PROG >/dev/null 2>&1 ||
		die "required program '$PROG' not found in \$PATH."
done

## Create a tiny ext3 file system image
dd if=/dev/zero of=$IMGFILE bs=1k count=64 1>$LOG 2>&1 || die "dd failed"
yes | mkfs.ext3 -b ${BLOCK_SIZE} $IMGFILE 1>$LOG 2>&1 ||
	die "mkfs.ext3 failed"

echo "
Done!

To use this file-systems, run:
  mkdir /tmp/fullfs/
  sudo mount -o sync $IMGFILE /tmp/fullfs/
  sudo chown \$USER /tmp/fullfs/

Clean the file system (before any testing):
  find /tmp/fullfs/ -maxdepth 1 -type f -delete

Writing to this file system should fail with 'no space'
(But unlike /dev/full, only after ~30KB are succesfully written):
  seq 100000 >/tmp/fullfs/test.txt

"

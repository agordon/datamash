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
IMGFILE=bad_disk.img
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

for PROG in mkfs.ext3 debugfs ;
do
	which $PROG >/dev/null 2>&1 ||
		die "required program '$PROG' not found in \$PATH."
done

## Create data files, each 30000 bytes
seq 10000 | head -c 30000 > numbers.txt ||
        die "failed to create numbers.txt"
printf "%d bottles of beer on the wall\n" $(seq 2000 -1 2) |
	head -c 30000 > bottles.txt ||
        die "failed to create bottles.txt"

## Create ext3 file system image
dd if=/dev/zero of=$IMGFILE bs=1k count=128 1>$LOG 2>&1 || die "dd failed"
yes | mkfs.ext3 -b ${BLOCK_SIZE} $IMGFILE 1>$LOG 2>&1 ||
	die "mkfs.ext3 failed"

## Add two files and corrupt them
for FILE in numbers.txt bottles.txt ;
do
	## Add data file to file system
	echo "write ./$FILE $FILE"| debugfs -w $IMGFILE >$LOG 2>&1 ||
		die "failed to add $FILE to $IMGFILE"

	## Corrupt the file-system

	## Find the IND (indirect) block, which should contain pointers to
	## valid other blocks occupied by this file.
	INDBLOCK=$(echo "stat $FILE" |
			debugfs $IMGFILE 2>/dev/null |
			grep IND | perl -ne '/\(IND\):(\d+)/ && print $1')
	[ -z "$INDBLOCK" ] &&
		die "failed to find IND block for $FILE in $IMGFILE"

	## Mess-up the IND block, by overriting it with random data
	dd if=/dev/urandom bs=${BLOCK_SIZE} count=1 \
		of=$IMGFILE conv=notrunc seek=${INDBLOCK} >$LOG 2>&1 ||
			die "failed to override IND block $INDBLOCK"
done


## Verify that the file system image has errors in it
## (otherwise we failed to simuate errors)
fsck.ext3 -nf $IMGFILE >$LOG 2>&1 &&
	die "Failed to simulate bad file system (mkfs did not report errors)"

echo "
Done!

To use this file-systems, run:
  mkdir /tmp/badfs/
  sudo mount -o sync $IMGFILE /tmp/badfs/

To verify I/O errors, run:
  cat /tmp/badfs/numbers.txt > /dev/null
  cat /tmp/badfs/bottles.txt > /dev/null

"

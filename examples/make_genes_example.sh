#!/bin/sh

## Copyright (C) 2014-2021 Assaf Gordon <assafgordon@gmail.com>
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
## along with GNU Datamash.  If not, see <https://www.gnu.org/licenses/>.


##
## A short script to general a sample of genes based on HG19/RefSeq file.
##

if [ ! -e "refGene.txt" ] ; then
	wget http://hgdownload.soe.ucsc.edu/goldenPath/hg19/database/refGene.txt.gz || exit 1
	gunzip refGene.txt.gz || exit 1
fi

(cat refGene.txt |
		sort -k13,13 |
		../datamash -g 13 countunique 3 countunique 4 |
		awk '$2>1 || $3>1' | sort -R | head -n 100 | cut -f1 -d " " ;
	   cut -f13 refGene.txt | sort -R -u | head -n 1000 ) |
		 sort -u > genelist.txt

grep -F -f genelist.txt refGene.txt | grep -E -v "chrUn|hap" > genes.txt

( echo "bin
name
chrom
strand
txStart
txEnd
cdsStart
cdsEnd
exonCount
exonStarts
exonEnds
score
name2
cdsStartStat
cdsEndStat
exonFrames" | paste -s -d '	' ; cat genes.txt ) > genes_h.txt

rm -f genelist.txt

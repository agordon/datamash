#!/bin/sh

##
## A short script to general a sample of genes based on HG19/RefSeq file.
##

if [ ! -e "refGene.txt" ] ; then
	wget http://hgdownload.soe.ucsc.edu/goldenPath/hg19/database/refGene.txt.gz || exit 1
	gunzip refGene.txt.gz || exit 1
fi

(cat refGene.txt |
		sort -k13,13 |
		../compute -g 13 countunique 3 countunique 4 |
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

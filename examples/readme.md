# GNU Datamash - Usage Examples

This directory contains sample files demonstrating typical usage of the `datamash`
program.

`datamash` is a command-line calculator of basic operations on columnar input files.

## Synopsis

`datamash` reads input from STDIN, and performs operations (e.g. sum, mean, count) on
specified columns:

    datamash [OPTIONS] op1 column1 [op2 column2]...

**op1** is the operation to perform, one of: count,sum,min,max,absmin,
absmax,mean,median,mode,antimode,pstdev,sstdev,pvar,svar.

**column1** is the column (in the input file) to use for **op1**.

**OPTIONS** are possible command-line options which affect the behaviour of `datamash`.


Example: sum and count the number of even values between 0 and 100:

    $ seq 0 2 100 | datamash sum 1
    2550
    $ seq 0 2 100 | datamash count 1
    51


## Example: Test Scores

The file `scores.txt` contains tests scores of college students of different majors
(Arts, Business, Health and Medicine, Life Sciences, Engineering, Social Sciences).

The files has three columns: Name, Major, Score:

    $ cat scores.txt
    Shawn     Arts  65
    Marques   Arts  58
    Fernando  Arts  78
    Paul      Arts  63
    Walter    Arts  75
    ...

Using `datamash`, find the lowest (min) and highest (max) score for each College Major:
(Major is in column 2, the score values are in column 3):

    $ datamash -g 2 min 3 max 3 < scores.txt
    Arts            46  88
    Business        79  94
    Health-Medicine 72  100
    Social-Sciences 27  90
    Life-Sciences   14  91
    Engineering     39  99

Similarly, find the number of students, mean score and sample-standard-deviation for each College major:

    $ datamash -g 2 count 3 mean 3 sstdev 3 < scores.txt
    Arts             68.9474  10.4215
    Business         87.3636  5.18214
    Health-Medicine  90.6154  9.22441
    Social-Sciences  60.2667  17.2273
    Life-Sciences    55.3333  20.606
    Engineering      66.5385  19.8814


## Example: Header Lines

A *header line* is an optional first line in the input or output files, which labels each column.
`datamash` can generate header line in the output file, even if the input file doesn't have a header line (`scores.txt` does not have a header line, the first line in the file contains data).

Use '--header-out' to add a header line to the output (when the input does not contain a header line):

    $ datamash --header-out -g 2 count 3 mean 3 pstdev 3 < scores.txt
    GroupBy(field-2)    mean(field-3)  sstdev(field-3)
    Arts                68.9474        10.4215
    Business            87.3636        5.18214
    Health-Medicine     90.6154        9.22441
    Social-Sciences     60.2667        17.2273
    Life-Sciences       55.3333        20.606
    Engineering         66.5385        19.8814


When the input file has a header line, `datamash` can will use the labels from each column in the output header line. `scores_h.txt` contains the same information as `scores.txt`, with an additional header line:

    $ cat scores_h.txt
    Name        Major   Score
    Shawn       Arts    65
    Marques     Arts    58
    Fernando    Arts    78
    Paul        Arts    63
    Walter      Arts    75
    ...


Use `-H` (equivalent to `--header-in --header-out`) to use input headers and print output headers:

    $ datamash -H -g 2 count 3 mean 3 pstdev 3 < scores_h.txt
    GroupBy(Major)      mean(Score)    sstdev(Score)
    Arts                68.9474        10.4215
    Business            87.3636        5.18214
    Health-Medicine     90.6154        9.22441
    Social-Sciences     60.2667        17.2273
    Life-Sciences       55.3333        20.606
    Engineering         66.5385        19.8814


## Example: Human Genes

**NOTE:** The follow examples assume some biology background knowledge.

The `genes.txt` file contains a small subset of the Human Genome genes.
The full dataset is available at the [UCSC Genome Browser](http://hgdownload.cse.ucsc.edu/downloads.html)'s
[Human Genome Database](http://hgdownload.soe.ucsc.edu/goldenPath/hg19/database/) as
[refGene.txt.gz](http://hgdownload.soe.ucsc.edu/goldenPath/hg19/database/refGene.txt.gz).

The columns of `genes.txt` are:

1. bin
2. name - isoform/transcript identifier
3. chromosome
4. strand
5. txStart - transcription start site
6. txEnda - transcription end site
7. cdsStart - coding start site
8. cdsEnd - coding end site
9. exonCount - number of exons
10. exonStarts
11. exonEnds
12. score
13. GeneName - gene identifier
14. cdsStartStat
15. cdsEndStat
16. exonFrames

### Number of isoforms per gene

The gene identifiers are in column 13, the transcript identifiers are in column 2.
To count how many isoforms each gene has, use `datamash` to group by column 13, and for each group, count the values in column 2 (use `-s` to automatically sort the input file):

    $ datamash -s -g 13 count 2 < genes.txt
    ABCC1   1
    ABCC10  2
    ABCC11  3
    ABCC12  1
    ABCC13  2
    ...

Using the `collapse` operation, we can print all the isoforms for each gene:

    $ datamash -s -g 13 count 2 collapse 2 < genes.txt
    ABCC1   1  NM_004996
    ABCC10  2  NM_001198934,NM_033450
    ABCC11  3  NM_032583,NM_033151,NM_145186
    ABCC12  1  NM_033226
    ABCC13  2  NR_003087,NR_003088
    ...


### Combining datamash with other programs

Combining `datamash` with additional filtering programs (such as `awk`), we can find relevant information, such as:

Which genes have more than 5 isoforms?

    $ cat genes.txt | datamash -s -g 13 count 2 collapse 2 | awk '$2>5'
    AC159540.1  6  NR_040097,NR_103732,NR_103733,NR_040097,NR_103732,NR_103733
    ACSF3       6  NM_001127214,NM_001243279,NM_001284316,NM_174917,NR_045667,NR_104293
    ADAM29      7  NM_001130703,NM_001130704,NM_001130705,NM_001278125,NM_001278126,NM_001278127,NM_014269
    AIPL1       8  NM_001033054,NM_001033055,NM_001285399,NM_001285400,NM_001285401,NM_001285402,NM_001285403,NM_014336
    ANXA8       6  NM_001040084,NM_001271702,NM_001271703,NM_001040084,NM_001271702,NM_001271703
    ...


Using `datamash` we can quickly explore the dataset and answer simple question, such as:

How many genes are transcribes from both strands (that is, they have isoforms with both positive and negative strands.
strand column is number 4):

    $ cat genes.txt | datamash -s -g 13 countunique 4 | awk '$2>1'
    AC159540.1   2
    AMY1C        2
    ANXA8        2
    BMS1P17      2
    BMS1P18      2
    ...

Which genes are transcribes from multiple chromosomes (that is, they have isoforms from multiple chromosomes.
Chromosome column is number 3):

    $ cat genes.txt | datamash -s -g 13 countunique 2 unique 2 | awk '$2>1'
    AKAP17A      2   chrX,chrY
    ASMT         2   chrX,chrY
    ASMTL        2   chrX,chrY
    ASMTL-AS1    2   chrX,chrY
    BMS1P17      2   chr14,chr22
    ...


Explore Exon-count variability (for each gene, list the minimum, maximum, mean and stddev of the
exon-count of its isoforms. Exon-Count column is number 9):

    $ cat genes.txt | datamash -s -g 13 count 9 min 9 max 9 mean 9 pstdev 9 | awk '$2>1'
    ABCC10     2   20   22     21   1
    ABCC11     3   29   30   29.3   0.471405
    ABCC13     2    5    6    5.5   0.5
    ABCC3      2   12   31   21.5   9.5
    AC159540.1 6    4    5    4.1   0.372678
    ...

### Grouping multiple fields

Chromosome name is in column 3. How many transcripts are in each chromosome?

    $ datamash -s -g 3 count 2 < genes.txt
    chr1  365
    chr10 164
    chr11 189
    chr12 187
    chr13 66
    ...

Strand information is in column 4. How many transcripts are in each chromsomse AND strand?

    $ datamash -s -g 3,4 count 2 < genes.txt
    chr1  - 183
    chr1  + 182
    chr10 -  52
    chr10 + 112
    chr11 - 105
    chr11 +  84
    chr12 - 117
    chr12 +  70
    ...


## More Information

For more information about `datamash` usage, run `datamash --help`, and the [datamash Website](http://www.gnu.org/software/datamash).

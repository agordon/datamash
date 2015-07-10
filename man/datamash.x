[NAME]
datamash - command-line calculations

[>OPTIONS]
.PP
.SH AVAILABLE OPERATIONS
.PP

.SS "Primary Operations"
Primary operations affect the way the file is processed. If used, the
primary operation must be listed first. Some operations require field
numbers (groupby, crosstab) while others do not (reverse,check,transpose).
If primary operation is not listed the entire file is processed -
either line-by-line (for 'per-line' operations) or all lines as one group
(for grouping operations). See Examples section below.
.PP

.TP "\w'\fBcountunique\fR'u+1n"
.B groupby X,Y,... op fld ...
group the file by given fields. Equivalent to option '\-g'.
For each group perform operation \fBop\fR on field \fBfld\fR.

.TP
.B crosstab X,Y [op fld ...]
cross-tabulate a file by two fields (cross-tabulation is also known
as pivot tables). If no operation is specified, counts how many incidents
exist of X,Y.

.TP
.B transpose
transpose rows, columns of the input file

.TP
.B reverse
reverse field order in each line

.TP
.B check
verify the input file has same number of fields in all lines.
number of lines and fields are printed to STDOUT. Exits with non-zero code
and prints the offending line if there's a mismatch in the number of fields.
.PP


.SS "Line-Filtering operations"

.TP "\w'\fBcountunique\fR'u+1n"
.B rmdup
remove lines with duplicated key value
.PP

.SS "Per-Line operations"

.TP "\w'\fBcountunique\fR'u+1n"
.B base64
Encode the field as base64

.TP
.B debase64
Decode the field as base64, exit with error if invalid base64 string

.TP
.B md5/sha1/sha256/sha512
Calculate md5/sha1/sha256/sha512 hash of the field value

.TP
.B bin[:BUCKET-SIZE]
bin numeric values into buckets of size \fBBUCKET-SIZE\fR (defaults to 100).

.TP
.B round/floor/ceil/trunc/frac
numeric rounding operations. round (round half away from zero),
floor (round up), ceil (ceiling, round down), trunc (truncate, round towards
zero), frac (fraction, return fraction part of a decimal-point value).
.PP


.SS "Numeric Grouping operations"

.TP "\w'\fBcountunique\fR'u+1n"
.B sum
sum the of values

.TP
.B min
minimum value

.TP
.B max
maximum value

.TP
.B absmin
minimum of the absolute values

.TP
.B absmax
maximum of the absolute values
.PP

.SS "Textual/Numeric Grouping operations"

.TP "\w'\fBcountunique\fR'u+1n"
.B count
count number of elements in the group

.TP
.B first
the first value of the group

.TP
.B last
the last value of the group

.TP
.B rand
one random value from the group

.TP
.B unique
comma-separated sorted list of unique values

.TP
.B collapse
comma-separated list of all input values

.TP
.B countunique
number of unique/distinct values
.PP


.SS "Statistical Grouping operations"
A \fBp/s\fR prefix indicates the varient: \fBp\fRopulation or \fBs\fRample.
Typically, the \fBs\fRample variant is equivalent with \fBGNU R\fR's
internal functions (e.g datamash's \fBsstdev\fR operation is equivalent
to R's \fBsd()\fR function).
.PP

.TP "\w'\fBcountunique\fR'u+1n"
.B mean
mean of the values

.TP
.B median
median value

.TP
.B q1
1st quartile value

.TP
.B q3
3rd quartile value

.TP
.B iqr
inter-quartile range

.TP
.B mode
mode value (most common value)

.TP
.B antimode
anti-mode value (least common value)

.TP
.B pstdev/sstdev
population/sample standard deviation

.TP
.B pvar/svar
population/sample variance

.TP
.B mad
median absolute deviation, scaled by constant 1.4826 for normal distributions

.TP
.B madraw
median absolute deviation, unscaled

.TP
.B pskew/sskew
skewness of the group
  values x reported by 'sskew' and 'pskew' operations:
.nf
          x > 0       -  positively skewed / skewed right
      0 > x           -  negatively skewed / skewed left
          x > 1       -  highly skewed right
      1 > x >  0.5    -  moderately skewed right
    0.5 > x > \-0.5    -  approximately symmetric
   \-0.5 > x > \-1      -  moderately skewed left
     \-1 > x           -  highly skewed left
.fi

.TP
.B   pkurt/skurt
excess Kurtosis of the group

.TP
.B   jarque/dpo
p-value of the Jarque-Beta (\fBjarque\fR) and D'Agostino-Pearson Omnibus
(\fBdpo\fR) tests for normality:
   null hypothesis is normality;
   low p-Values indicate non-normal data;
   high p-Values indicate null-hypothesis cannot be rejected.

.TP
.B  pcov/scov [X:Y]
covariance of fields X and Y

.TP
.B  ppearson/spearson [X:Y]
Pearson product-moment correlation coefficient [Pearson's R]
of fields X and Y



[=EXAMPLES]

.SS "Basic usage"

Print the sum and the mean of values from field 1:
.PP
.nf
.RS
$ seq 10 | \fBdatamash\fR sum 1 mean 1
55  5.5
.RE
.fi
.PP
Group input based on field 1, and sum values (per group) on field 2:
.PP
.nf
.RS
$ cat example.txt
A  10
A  5
B  9
B  11

$ \fBdatamash\fR \-g 1 sum 2 < example.txt
A  15
B  20

$ \fBdatamash\fR groupby 1 sum 2 < example.txt
A  15
B  20
.RE
.fi
.PP

Unsorted input must be sorted (with '\-s'):
.PP
.nf
.RS
$ cat example.txt
A  10
C  4
B  9
C  1
A  5
B  11

$ \fBdatamash\fR \-s \-g1 sum 2 < example.txt
A  15
B  20
C  5
.RE
.fi
.PP

Which is equivalent to:
.PP
.nf
.RS
$ cat example.txt | sort \-k1,1 | \fBdatamash\fR \-g 1 sum 2
.RE
.fi



.SS "Header lines"
.PP
Use \fB\-h\fR \fB(\-\-headers)\fR if the input file has a header line:
.PP
.nf
.RS
# Given a file with student name, field, test score...
$ head \-n5 scores_h.txt
Name           Major            Score
Shawn          Engineering      47
Caleb          Business         87
Christian      Business         88
Derek          Arts             60

# Calculate the mean and standard devian for each major
$ \fBdatamash\fR \-\-sort \-\-headers \-\-group 2 mean 3 pstdev 3 < scores_h.txt

 (or use short form)

$ \fBdatamash\fR \-sH \-g2 mean 3 pstdev 3 < scores_h.txt

 (or use named fields)

$ \fBdatamash\fR \-sH \-g Major mean Score pstdev Score < scores_h.txt
GroupBy(Major)    mean(Score)   pstdev(Score)
Arts              68.9          10.1
Business          87.3           4.9
Engineering       66.5          19.1
Health-Medicine   90.6           8.8
Life-Sciences     55.3          19.7
Social-Sciences   60.2          16.6
.RE
.fi
.PP


.SS "Multiple fields"

Use comma or dash to specify multiple fields. The following are equivalent:
.nf
.RS
$ seq 9 | paste \- \- \-
1   2   3
4   5   6
7   8   9

$ seq 9 | paste \- \- \- | datamash sum 1 sum 2 sum 3
12  15  18

$ seq 9 | paste \- \- \- | datamash sum 1,2,3
12  15  18

$ seq 9 | paste \- \- \- | datamash sum 1-3
12  15  18
.RE
.fi
.PP


.SS "Rounding"
The following demonstrate the different rounding operations:
.nf
.RS
.RE
." NOTE: The weird spacing/alignment is due to extract backslash
."       characters. Modify with caution.
$ ( echo X ; seq \-1.25 0.25 1.25 ) \\
      | datamash \-\-full \-H round 1 ceil 1 floor 1 trunc 1 frac 1

  X     round(X)  ceil(X)  floor(X)  trunc(X)   frac(X)
\-1.25   \-1        \-1       \-2        \-1         \-0.25
\-1.00   \-1        \-1       \-1        \-1          0
\-0.75   \-1         0       \-1         0         \-0.75
\-0.50   \-1         0       \-1         0         \-0.5
\-0.25    0         0       \-1         0         \-0.25
 0.00    0         0        0         0          0
 0.25    0         1        0         0          0.25
 0.50    1         1        0         0          0.5
 0.75    1         1        0         0          0.75
 1.00    1         1        1         1          0
 1.25    1         2        1         1          0.25
.fi
.PP



.SS "Reversing fields"
.PP
.nf
.RS
$ seq 6 | paste \- \- | \fBdatamash\fR reverse
2    1
4    3
6    5
.RE
.fi
.PP



.SS "Transposing a file"
.PP
.nf
.RS
$ seq 6 | paste \- \- | \fBdatamash\fR transpose
1    3    5
2    4    6
.RE
.fi
.PP



.SS "Removing Duplicated lines"
Remove lines with duplicate key value from field 1
(Unlike \fBfirst\fR,\fBlast\fR operations, \fBrmdup\fR is much faster and
does not require sorting the file with \-s):
.PP
.nf
.RS
# Given a list of files and sample IDs:
$ cat INPUT
SampleID  File
2         cc.txt
3         dd.txt
1         ab.txt
2         ee.txt
3         ff.txt

# Remove lines with duplicated Sample-ID (field 1):
$ \fBdatamash\fR rmdup 1 < INPUT

# or use named field:
$ \fBdatamash\fR \-H rmdup SampleID < INPUT
SampleID  File
2         cc.txt
3         dd.txt
1         ab.txt
.RE
.fi
.PP


.SS "Checksums"
Calculate the sha1 hash value of each TXT file,
after calculating the sha1 value of each file's content:
.PP
.nf
.RS
$ sha1sum *.txt | datamash -Wf sha1 2
.RE
.fi
.PP


.SS "Check file structure"
Check the structure of the input file (ensure all lines
have the same number of fields):
.PP
.nf
.RS
$ seq 10 | paste \- \- | datamash check && echo ok || echo fail
5 lines, 2 fields
ok

$ seq 13 | paste \- \- \- | datamash check && echo ok || echo fail
line 4 (3 fields):
  10  11  12
line 5 (2 fields):
  13
datamash: check failed: line 5 has 2 fields (previous line had 3)
fail
.RE
.fi
.PP



.SS "Cross-Tabulation"
Cross-tabulation compares the relationship between two fields.
Given the following input file:
.nf
.RS
$ cat input.txt
a    x    3
a    y    7
b    x    21
a    x    40
.RE
.fi
.PP
Show cross-tabulation between the first field (a/b) and the second field
(x/y) - counting how many times each pair appears (note: sorting is required):
.PP
.nf
.RS
$ \fBdatamash\fR \-s crosstab 1,2 < input.txt
     x    y
a    2    1
b    1    N/A
.RE
.fi
.PP
An optional grouping operation can be used instead of counting:
.PP
.nf
.RS
.PP
$ \fBdatamash\fR \-s crosstab 1,2 sum 3 < input.txt
     x    y
a    43   7
b    21   N/A

$ \fBdatamash\fR \-s crosstab 1,2 unique 3 < input.txt
     x    y
a    3,40 7
b    21   N/A
.RE
.fi
.PP


.SS "Binning numeric values"
Bin input values into buckets of size 5:
.PP
.nf
.RS
$  ( echo X ; seq \-10 2.5 10 ) \\
      | \fBdatamash\fR \-H \-\-full bin:5 1
    X  bin(X)
\-10.0    \-15
 \-7.5    \-10
 \-5.0    \-10
 \-2.5     \-5
  0.0      0
  2.5      0
  5.0      5
  7.5      5
 10.0     10
.RE
.fi
.PP

[ADDITIONAL INFORMATION]
See
.UR http://www.gnu.org/software/datamash
GNU Datamash Website (http://www.gnu.org/software/datamash)

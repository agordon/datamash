." GNU decorate - manual page
." Copyright (C) 2014-2020 Assaf Gordon <assafgordon@gmail.com>
[NAME]
decorate - command-line calculations


[>OVERVIEW]
The \fBdecorate\fR program allows sorting input according to various
ordering, e.g. IP addresses, roman numerals, etc.
It works in tandem with sort(1) to perform the actual sorting.

The idea was suggested by
.UR https://lists.gnu.org/r/bug-coreutils/2015-06/msg00076.html
Pádraig Brady in https://lists.gnu.org/r/bug-coreutils/2015-06/msg00076.html:

1. Decorate: convert the input to a sortable-format as additional fields
.br
2. Sort according to the inserted fields
.br
3. Undecorate: remove the inserted fields

Example of preparing to sort by roman numerals:
.PP
.nf
.RS
$ printf "%s\\n" C V III IX XI | \fBdecorate\fR \-k1,1:roman \-\-decorate
0000100 C
0000005 V
0000003 III
0000009 IX
0000011 XI
.RE
.fi
.PP

The output can now be sent to sort(1), followed by removing (=undecorate)
the first field.

.PP
.nf
.RS
$ printf "%s\\n" C V III IX XI \\
       | \fBdecorate\fR \-k1,1:roman \-\-decorate \\
       | sort \-k1,1 \\
       | \fBdecorate\fR \-\-undecorate 1
III
V
IX
XI
C
.RE
.fi
.PP

\fBdecorate(1)\fR can automatically combine the decorate-sort-undecorate steps
(when run without \-\-decorate or \-\-undecorate):

.PP
.nf
.RS
$ printf "%s\\n" C V III IX XI | \fBdecorate\fR \-k1,1:roman
III
V
IX
XI
C
.RE
.fi
.PP




[=EXAMPLES]

[ADDITIONAL INFORMATION]
See
.UR https://www.gnu.org/software/datamash
GNU Datamash Website (https://www.gnu.org/software/datamash)

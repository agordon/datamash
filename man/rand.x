." GNU rand - manual page
." Copyright (C) 2022 Timothy Rice <trice@posteo.net>
[NAME]
rand - generate pseudo-random numbers from assorted distributions

The \fBrand\fR program is inspired by the various functions in the R
statistical programming language which all prefix the distribution name with
the letter "r". For example, "runif", "rnorm" and "rexp".

[>OPTIONS]

[=EXAMPLES]

.SS "Basic usage"

Simulate the sample mean and standard deviation of ten standard normal
independent and identically-distributed random variables (i.e. N(0,1) iidrvs):
.PP
.nf
.RS
$ \fBrand\fR norm 10 | \fBdatamash\fR mean 1 sstdev 1
-0.2336997      0.99112189348592
.RE
.fi
.PP

[ADDITIONAL INFORMATION]
See
.UR https://www.gnu.org/software/datamash
GNU Datamash Website (https://www.gnu.org/software/datamash)

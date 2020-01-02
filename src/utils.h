/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2020 Assaf Gordon <assafgordon@gmail.com>

   This file is part of GNU Datamash.

   GNU Datamash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   GNU Datamash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Datamash.  If not, see <https://www.gnu.org/licenses/>.
*/

/* Written by Assaf Gordon */
#ifndef __UTILS_H__
#define __UTILS_H__

/*
 Generate Utility Functions module.
 */

/* Returns true if 'value' can be considered a valid N/A.
    'NA', 'N/A', 'NaN' and empty cells are valid N/As. */
bool
is_na (const char* value, const size_t len);


/* Given an array of doubles, return the arithmetic mean value */
long double
arithmetic_mean_value (const long double * const values, const size_t n);

/*
 Given a sorted array of doubles, return the value of 'percentile'.
 Example of valid 'percentile':
    0.10 = First decile
    0.25 = First quartile
    0.50 = median
    0.75 = third quartile
    0.99 = 99nt percentile
*/
long double
percentile_value (const long double * const values,
                  const size_t n, const double percentile);

/* Given a sorted array of doubles, return the value of the median */
long double
median_value (const long double * const values, size_t n);

/* Given a sorted array of doubles, return the value of 1st quartile */
static inline long double
quartile1_value (const long double * const values, size_t n)
{
  return percentile_value (values, n, 1.0/4.0);
}

/* Given a sorted array of doubles, return the value of 3rd quartile */
static inline long double
quartile3_value (const long double * const values, size_t n)
{
  return percentile_value (values, n, 3.0/4.0);
}

/* Given a sorted array of doubles, return the MAD value
   (median absolute deviation), with scale constant 'scale' */
long double
mad_value (const long double * const values, size_t n, double scale) ;


/* Sorts (in-place) an array of long-doubles */
void
qsortfl (long double *values, size_t n);


enum degrees_of_freedom
{
  DF_POPULATION = 0,
  DF_SAMPLE = 1
};

/*
 Given an array of doubles, return the variance value.
 'df' is degrees-of-freedom. Use DF_POPULATION or DF_SAMPLE (see above).
 */
long double
variance_value ( const long double * const values, size_t n, int df );

/*
 Given an two array of doubles, return the covariance value.
 'df' is degrees-of-freedom. Use DF_POPULATION or DF_SAMPLE (see above).
 */
long double
covariance_value ( const long double * const valuesA,
                   const long double * const valuesB, size_t n, int df );

/*
 Given an two array of doubles, return the Pearson correlation coefficient
 */
long double
pearson_corr_value ( const long double * const valuesA,
                     const long double * const valuesB, size_t n, int df);

/*
 Given an array of doubles, return the standard-deviation value.
 'df' is degrees-of-freedom. Use DF_POPULATION or DF_SAMPLE (see above).
 */
long double
stdev_value ( const long double * const values, size_t n, int df );

/*
 Given an array of doubles, return the skewness
 'df' is degrees-of-freedom. Use DF_POPULATION or DF_SAMPLE (see above).
 */
long double
skewness_value ( const long double * const values, size_t n, int df );

/* Standard error of skewness (SES), given the sample size 'n' */
long double
SES_value ( size_t n );

/* Skewness Test statistics Z = ( sample skewness / SES ) */
long double skewnessZ_value ( const long double * const values, size_t n);

/*
 Given an array of doubles, return the excess kurtosis
 'df' is degrees-of-freedom. Use DF_POPULATION or DF_SAMPLE (see above).
 */
long double
excess_kurtosis_value ( const long double * const values, size_t n, int df );

/* Standard error of kurtisos (SEK), given the sample size 'n' */
long double
SEK_value ( size_t n );

/* Kurtosis Test statistics Z = ( sample kurtosis / SEK ) */
long double
kurtosisZ_value ( const long double * const values, size_t n);

/*
 Chi-Squared - Cumulative distribution function,
 for the special case of DF=2.
 Equivalent to the R function 'pchisq (x,df=2)'.
*/
long double
pchisq_df2 (long double x);

/*
 D'Agostino-Perason omnibus test for normality.
 returns the p-value,
 where the null-hypothesis is normal distribution.
*/
long double
dagostino_pearson_omnibus_pvalue (const long double * const values, size_t n);



/*
 Given an array of doubles, return the p-Value
 Of the Jarque-Bera Test for normality
   http://en.wikipedia.org/wiki/Jarque%E2%80%93Bera_test
 Equivalent to R's "jarque.test ()" function in the "moments" library.
 */
long double
jarque_bera_pvalue (const long double * const values, size_t n);


enum MODETYPE
{
  MODE=1,
  ANTIMODE
};

/*
 Given an array of doubles, return the mode/anti-mode values.
 */
long double
mode_value ( const long double * const values, size_t n, enum MODETYPE type);

/*
 Given an array of doubles, return the mode/anti-mode values.
 */
long double
trimmed_mean_value ( const long double * const values, size_t n,
		     const long double trimmed_mean_percent);


/*
 comparison functions, to be used with 'qsort ()'
 */
int
cmpstringp (const void *p1, const void *p2);

int
cmpstringp_nocase (const void *p1, const void *p2);

int
cmp_long_double (const void *p1, const void *p2);

bool
hash_compare_strings (void const *x, void const *y);


/* Return the number of characters FROM THE END of 's'
   that match a guessed file extension.
   Return 0 if no extension is found.
   's' does not need to be NULL terminated. */
size_t
guess_file_extension (const char*s, size_t len);


/* Extract the first detected number from a string 's'
   (skipping non-number characters).
   's' does not need to be NUL-terminated. */
enum extract_number_type
  {
   ENT_NATURAL = 0,
   ENT_INTEGER,
   ENT_HEX,
   ENT_OCT,
   ENT_POSITIVE_DECIMAL,
   ENT_DECIMAL
  };
long double
extract_number (const char* s, size_t len, enum extract_number_type type);



/* returns non-zero if the input is equavalent to
   zero (or negative zero) */
static inline bool
is_zero (const long double a)
{
  return !((a>0)-(a<0));
}

/* returns non-zero if the input is negative zero */
static inline bool
is_signed_zero (const long double a)
{
  return signbit (a) && is_zero (a);
}

/* if the input is negative-zero, returns positive zero.
   otherwise, returns the input value. */
static inline long double
pos_zero (const long double a)
{
  return is_signed_zero (a) ? 0 : a;
}


/* On some systems (e.g. Cygwin) nanl is not defined,
   and gnulib does not yet provide a replacment
   (though it does provide 'isnanl' replacement) */
#ifndef nanl
  #define nanl nan
#endif


#endif

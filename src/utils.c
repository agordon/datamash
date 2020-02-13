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
#include <config.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "system.h"
#include "xalloc.h"
#include "size_max.h"

#include "utils.h"

bool _GL_ATTRIBUTE_PURE
is_na (const char* value, const size_t len)
{
  assert (value != NULL); /* LCOV_EXCL_LINE */
  return (len==2 && (strncasecmp (value,"NA",2)==0))
          || (len==3 && (strncasecmp (value,"N/A",3)==0))
          || (len==3 && (strncasecmp (value,"NAN",3)==0));
}


/* Compare two flowting-point variables, while avoiding '==' .
see:
http://www.gnu.org/software/libc/manual/html_node/Comparison-Functions.html */
int _GL_ATTRIBUTE_PURE
cmp_long_double (const void *p1, const void *p2)
{
  const long double *a = (const long double *)p1;
  const long double *b = (const long double *)p2;
  return ( *a > *b ) - (*a < *b);
}

long double _GL_ATTRIBUTE_PURE
median_value (const long double * const values, size_t n)
{
#if 0
  return percentile_value (values, n, 2.0/4.0);
#else
  /* Equivalent to the above, but slightly faster */
  return (n&0x01)
    ?values[n/2]
    :( (values[n/2-1] + values[n/2]) / 2.0 );
#endif
}

/* This implementation follows R's summary () and quantile (type=7) functions.
   See discussion here:
   http://tolstoy.newcastle.edu.au/R/e17/help/att-1067/Quartiles_in_R.pdf */
long double _GL_ATTRIBUTE_PURE
percentile_value (const long double * const values,
                  const size_t n, const double percentile)
{
  const double h = ( (n-1) * percentile ) ;
  const size_t fh = floor (h);

  /* Error in the calling parameters, should not happen */
  assert (n>0 && percentile>=0.0 && percentile<=100.0); /* LCOV_EXCL_LINE */

  if (n==1)
    return values[0];

  return values[fh] + (h-fh) * ( values[fh+1] - values[fh] ) ;
}


/* Given a sorted array of doubles, return the MAD value
   (median absolute deviation), with scale constant 'scale' */
long double _GL_ATTRIBUTE_PURE
mad_value (const long double * const values, size_t n, double scale)
{
  const long double median = median_value (values,n);
  long double *mads = xnmalloc (n,sizeof (long double));
  long double mad = 0 ;
  for (size_t i=0; i<n; ++i)
    mads[i] = fabsl (median - values[i]);
  qsortfl (mads,n);
  mad = median_value (mads,n);
  free (mads);
  return mad * scale;
}

long double _GL_ATTRIBUTE_PURE
arithmetic_mean_value (const long double * const values, const size_t n)
{
  long double sum=0;
  long double mean;
  for (size_t i = 0; i < n; i++)
    sum += values[i];
  mean = sum / n ;
  return mean;
}

long double _GL_ATTRIBUTE_PURE
variance_value (const long double * const values, size_t n, int df)
{
  long double sum=0;
  long double mean;
  long double variance;

  assert (df>=0); /* LCOV_EXCL_LINE */
  if ( (size_t)df == n )
    return nanl ("");

  mean = arithmetic_mean_value (values, n);

  sum = 0 ;
  for (size_t i = 0; i < n; i++)
    sum += (values[i] - mean) * (values[i] - mean);

  variance = sum / ( n - df );

  return variance;
}

long double _GL_ATTRIBUTE_PURE
covariance_value ( const long double * const valuesA,
                   const long double * const valuesB, size_t n, int df )
{
  long double sum=0;
  long double meanA, meanB;
  long double covariance;

  assert (df>=0); /* LCOV_EXCL_LINE */
  if ( (size_t)df == n )
    return nanl ("");

  meanA = arithmetic_mean_value (valuesA, n);
  meanB = arithmetic_mean_value (valuesB, n);

  sum = 0 ;
  for (size_t i = 0; i < n; i++)
    sum += (valuesA[i] - meanA) * (valuesB[i] - meanB);

  covariance = sum / ( n - df );

  return covariance;
}

long double
pearson_corr_value ( const long double * const valuesA,
                     const long double * const valuesB, size_t n, int df)
{
  long double meanA, meanB, sumA=0, sumB=0, sumCo=0;
  long double sdA, sdB;
  long double covariance;
  long double cor;

  assert (df>=0); /* LCOV_EXCL_LINE */
  if ( (size_t)df == n )
    return nanl ("");

  meanA = arithmetic_mean_value (valuesA, n);
  meanB = arithmetic_mean_value (valuesB, n);

  for (size_t i = 0; i < n; i++)
    {
      const long double a = (valuesA[i] - meanA);
      const long double b = (valuesB[i] - meanB);
      sumA += a*a;
      sumB += b*b;
      sumCo += a*b;
    }

  covariance = sumCo/(n-df);
  sdA = sqrtl (sumA/(n-df));
  sdB = sqrtl (sumB/(n-df));

  cor = covariance / ( sdA * sdB );
  return cor;
}


long double
stdev_value (const long double * const values, size_t n, int df)
{
  return sqrtl ( variance_value ( values, n, df ) );
}

/*
 Given an array of doubles, return the skewness
 'df' is degrees-of-freedom. Use DF_POPULATION or DF_SAMPLE (see above).
 */
long double
skewness_value (const long double * const values, size_t n, int df)
{
  long double moment2=0;
  long double moment3=0;
  long double mean;
  long double skewness;

  if (n<=1)
    return nanl ("");

  mean = arithmetic_mean_value (values, n);

  for (size_t i = 0; i < n; i++)
    {
      const long double t = (values[i] - mean);
      moment2 += t*t;
      moment3 += t*t*t;
    }
  moment2 /= n;
  moment3 /= n;

  /* can't use 'powl (moment2,3.0/2.0)' - not all systems have powl */
  skewness = moment3 / sqrtl (moment2*moment2*moment2);
  if ( df == DF_SAMPLE )
    {
      if (n<=2)
        return nanl ("");
      skewness = ( sqrtl (n*(n-1)) / (n-2) ) * skewness;
    }

  return skewness;
}

/* Standard error of skewness (SES), given the sample size 'n' */
long double
SES_value (size_t n)
{
  if (n<=2)
    return nanl ("");
  return sqrtl ( (long double)(6.0*n*(n-1))
                  / ((long double)(n-2)*(n+1)*(n+3)) );
}

/* Skewness Test statistics Z = ( sample skewness / SES ) */
long double
skewnessZ_value (const long double * const values, size_t n)
{
  const long double skew = skewness_value (values,n,DF_SAMPLE);
  const long double SES = SES_value (n);
  if (isnan (skew) || isnan (SES) )
    return nanl ("");
  return skew/SES;
}


/*
 Given an array of doubles, return the excess kurtosis
 'df' is degrees-of-freedom. Use DF_POPULATION or DF_SAMPLE (see above).
 */
long double _GL_ATTRIBUTE_PURE
excess_kurtosis_value (const long double * const values, size_t n, int df)
{
  long double moment2=0;
  long double moment4=0;
  long double mean;
  long double excess_kurtosis;

  if (n<=1)
    return nanl ("");

  mean = arithmetic_mean_value (values, n);

  for (size_t i = 0; i < n; i++)
    {
      const long double t = (values[i] - mean);
      moment2 += t*t;
      moment4 += t*t*t*t;
    }
  moment2 /= n;
  moment4 /= n;

  excess_kurtosis = moment4 / (moment2*moment2) - 3;

  if ( df == DF_SAMPLE )
    {
      if (n<=3)
        return nanl ("");
      excess_kurtosis = ( ((long double)n-1)
                          / (((long double)n-2)*((long double)n-3)) ) *
                          ( (n+1)*excess_kurtosis + 6 ) ;
    }

  return excess_kurtosis;
}

/* Standard error of kurtisos (SEK), given the sample size 'n' */
long double
SEK_value (size_t n)
{
  const long double ses = SES_value (n);

  if (n<=3)
    return nanl ("");

   return 2 * ses * sqrtl ( (long double)(n*n-1)
                            / ((long double)((n-3)*(n+5)))  );
}

/* Kurtosis Test statistics Z = ( sample kurtosis / SEK ) */
long double
kurtosisZ_value (const long double * const values, size_t n)
{
  const long double kurt = excess_kurtosis_value (values,n,DF_SAMPLE);
  const long double SEK = SEK_value (n);
  if (isnan (kurt) || isnan (SEK) )
    return nanl ("");
  return kurt/SEK;
}

/*
 Chi-Squared - Cumulative distribution function,
 for the special case of DF=2.
 Equivalent to the R function 'pchisq (x,df=2)'.
*/
long double
pchisq_df2 (long double x)
{
  return 1.0 - expl (-x/2);
}

/*
 Given an array of doubles, return the p-Value
 Of the Jarque-Bera Test for normality
   http://en.wikipedia.org/wiki/Jarque%E2%80%93Bera_test
 Equivalent to R's "jarque.test ()" function in the "moments" library.
 */
long double
jarque_bera_pvalue (const long double * const values, size_t n )
{
  const long double k = excess_kurtosis_value (values,n,DF_POPULATION);
  const long double s = skewness_value (values,n,DF_POPULATION);
  const long double jb = (long double)(n*(s*s + k*k/4))/6.0 ;
  const long double pval = 1.0 - pchisq_df2 (jb);
  if (n<=1 || isnan (k) || isnan (s))
    return nanl ("");
  return pval;
}

/*
 D'Agostino-Perason omnibus test for normality.
 returns the p-value,
 where the null-hypothesis is normal distribution.
*/
long double
dagostino_pearson_omnibus_pvalue (const long double * const values, size_t n)
{
  const long double z_skew = skewnessZ_value (values, n);
  const long double z_kurt = kurtosisZ_value (values, n);
  const long double DP = z_skew*z_skew + z_kurt*z_kurt;
  const long double pval = 1.0 - pchisq_df2 (DP);

  if (isnan (z_skew) || isnan (z_kurt))
    return nanl ("");
  return pval;
}


long double _GL_ATTRIBUTE_PURE
mode_value ( const long double * const values, size_t n, enum MODETYPE type)
{
  /* not ideal implementation but simple enough */
  /* Assumes 'values' are already sorted, find the longest sequence */
  long double last_value = values[0];
  size_t seq_size=1;
  size_t best_seq_size= (type==MODE)?1:SIZE_MAX;
  long double best_value = values[0];

  for (size_t i=1; i<n; i++)
    {
      bool eq = (cmp_long_double (&values[i],&last_value)==0);

      if (eq)
        seq_size++;

      if ( ((type==MODE) && (seq_size > best_seq_size))
           || ((type==ANTIMODE) && (seq_size < best_seq_size)))
        {
          best_seq_size = seq_size;
          best_value = last_value;
        }

      if (!eq)
          seq_size = 1;

      last_value = values[i];
    }
  return best_value;
}

long double  _GL_ATTRIBUTE_PURE
trimmed_mean_value ( const long double * const values, size_t n,
		     const long double trimmed_mean_percent)
{
  assert (trimmed_mean_percent >= 0); /* LCOV_EXCL_LINE */
  assert (trimmed_mean_percent <= 0.5); /* LCOV_EXCL_LINE */

  /* For R compatability:
     mean (x,trim=0.5) in R is equivalent to median (x).  */
  if (trimmed_mean_percent >= 0.5)
    return median_value (values, n);

  /* number of element to skip from each end */
  size_t c = pos_zero (floorl (trimmed_mean_percent * n));

  long double v = 0;
  for (size_t i=c; i< (n-c); i++)
    v += values[i];

  return v / (long double)(n-c*2);
}


int _GL_ATTRIBUTE_PURE
cmpstringp (const void *p1, const void *p2)
{
  /* The actual arguments to this function are "pointers to
   * pointers to char", but strcmp (3) arguments are "pointers
   * to char", hence the following cast plus dereference */
  return strcmp (* (char * const *) p1, * (char * const *) p2);
}

int _GL_ATTRIBUTE_PURE
cmpstringp_nocase (const void *p1, const void *p2)
{
  /* The actual arguments to this function are "pointers to
   * pointers to char", but strcmp (3) arguments are "pointers
   * to char", hence the following cast plus dereference */
  return strcasecmp (* (char * const *) p1, * (char * const *) p2);
}

/* Sorts (in-place) an array of long-doubles */
void qsortfl (long double *values, size_t n)
{
  qsort (values, n, sizeof (long double), cmp_long_double);
}

bool _GL_ATTRIBUTE_PURE
hash_compare_strings (void const *x, void const *y)
{
  assert (x != y); /* LCOV_EXCL_LINE */
  return STREQ (x, y) ? true : false;
}


static bool
is_add_on_extension (const char *s, size_t l)
{
  if ((l==4) \
      && (STREQ_LEN(s,".gpg",4) \
          || STREQ_LEN(s,".bz2",4) \
          || STREQ_LEN(s,".std",4)))
    return true;
  if ((l==3) \
      && (STREQ_LEN(s,".gz",3) || STREQ_LEN(s,".xz",3) || STREQ_LEN(s,".lz",3)))
    return true;
  return false;
}

size_t _GL_ATTRIBUTE_PURE
guess_file_extension (const char*s, size_t len)
{
  size_t prev_extension = 0;

  if (len==0)
    return 0;

  size_t l = len - 1 ;

 next_extension:
  while (l && c_isalnum (s[l]))
    --l;
  if (l>0 && s[l] == '.')
    {
      /* Found an extension, check if it's a known "add-on" one */

      /* check the string until to previous extension (if there was one),
         or the end of the string. */
      size_t ext_len = (prev_extension==0)?(len-l):(prev_extension-l);
      if (is_add_on_extension (&s[l],ext_len))
        {
          prev_extension = l;
          --l;
          goto next_extension;
        }

      return len-l;
    }

  /* In a case such as "foo.gpg", the first detected extension '.gpg'
     is found, but the second iteration fails (as 'foo' is not an extension).
     In such cases, revert to reporting the previously found extension. */
  if (prev_extension)
    return len-prev_extension;

  return 0;
}


struct EXTRACT_NUMBER_TYPE
{
  char *pattern;
  int base;
  bool floating_point;
};

/* NOTE: order must match 'enum extract_number_type' */
static struct EXTRACT_NUMBER_TYPE extract_number_types[] =
  {
   /* Natural Number (nonnegative integers), including ZERO */
   {"0123456789", 10,  false},

   /* Integers */
   {"-+0123456789", 10, false},

   /* Positive Hex number */
   {"0123456789abcdefABCDEF", 16, false},

   /* Positive Octal number */
   {"01234567", 8, false},

   /* Simple positive decimal point (including zero) */
   {".0123456789", 10, true},

   /* Simple decimal point (positive and negative) */
   {"+-.0123456789", 10, true}
  };


long double
extract_number (const char* s, size_t len, enum extract_number_type type)
{
  static char* buf, *endptr;
  static size_t buf_alloc;

  long double r = 0;
  char *pattern;
  int base ;
  bool fp;

  pattern = extract_number_types[type].pattern;
  base = extract_number_types[type].base;
  fp = extract_number_types[type].floating_point;

  if (len+1 > buf_alloc)
    {
      buf = xrealloc (buf, len+1);
      buf_alloc = len + 1;
    }

  size_t skip = strcspn (s, pattern);
  size_t span = strspn (s+skip, pattern);

  memcpy (buf, s+skip, span);
  buf[span] = '\0';

  if (!fp)
    {
      errno = 0;
      long long int val = strtoll (buf, &endptr, base);
      if (errno != 0)
        {
          /* failed to parse value */
          val = 0;
        }
      r = val;
    }
  else
    {
      errno = 0;
      r = strtold (buf, &endptr);
      if (errno != 0)
        r = 0;
    }

  return r;
}

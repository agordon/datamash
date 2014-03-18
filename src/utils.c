/* compute - perform simple calculation on input data
   Copyright (C) 2014 Assaf Gordon.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Assaf Gordon */
#include <config.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "size_max.h"

#include "utils.h"

/* Compare two flowting-point variables, while avoiding '==' .
see:
http://www.gnu.org/software/libc/manual/html_node/Comparison-Functions.html */
int
cmp_long_double (const void *p1, const void *p2)
{
  const long double *a = (const long double *)p1;
  const long double *b = (const long double *)p2;
  return ( *a > *b ) - (*a < *b);
}

long double
median_value ( const long double * const values, size_t n )
{
  /* Assumes 'values' are already sorted, returns the median value */
  return (n&0x01)
    ?values[n/2]
    :( (values[n/2-1] + values[n/2]) / 2.0 );
}

long double
variance_value ( const long double * const values, size_t n, int df )
{
  long double sum=0;
  long double mean;
  long double variance;

  for (size_t i = 0; i < n; i++)
    sum += values[i];
  mean = sum / n ;

  sum = 0 ;
  for (size_t i = 0; i < n; i++)
    sum += (values[i] - mean) * (values[i] - mean);

  variance = sum / ( n - df );

  return variance;
}

long double
stdev_value ( const long double * const values, size_t n, int df )
{
  return sqrtl ( variance_value ( values, n, df ) );
}



long double
mode_value ( const long double * const values, size_t n, enum MODETYPE type)
{
  /* not ideal implementation but simple enough */
  /* Assumes 'values' are already sorted, find the longest sequence */
  long double last_value = values[0];
  size_t seq_size=1;
  size_t best_seq_size= (type==MODE)?1:SIZE_MAX;
  size_t best_value = values[0];

  for (size_t i=1; i<n; i++)
    {
      bool eq = (cmp_long_double(&values[i],&last_value)==0);

      if (eq)
        seq_size++;

      if ( ((type==MODE) && (seq_size > best_seq_size))
           ||
           ((type==ANTIMODE) && (seq_size < best_seq_size)))
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

int
cmpstringp(const void *p1, const void *p2)
{
  /* The actual arguments to this function are "pointers to
   * pointers to char", but strcmp(3) arguments are "pointers
   * to char", hence the following cast plus dereference */
  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

int
cmpstringp_nocase(const void *p1, const void *p2)
{
  /* The actual arguments to this function are "pointers to
   * pointers to char", but strcmp(3) arguments are "pointers
   * to char", hence the following cast plus dereference */
  return strcasecmp(* (char * const *) p1, * (char * const *) p2);
}

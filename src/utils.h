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
#ifndef __UTILS_H__
#define __UTILS_H__

/*
 Generate Utility Functions module.
 */

/*
 Given an array of doubles, return the median value
 */
long double median_value ( const long double * const values, size_t n );

enum degrees_of_freedom
{
  DF_POPULATION = 0,
  DF_SAMPLE = 1
};

/*
 Given an array of doubles, return the variance value.
 'df' is degrees-of-freedom. Use DF_POPULATION or DF_SAMPLE (see above).
 */
long double variance_value ( const long double * const values, size_t n, int df );

/*
 Given an array of doubles, return the standard-deviation value.
 'df' is degrees-of-freedom. Use DF_POPULATION or DF_SAMPLE (see above).
 */
long double stdev_value ( const long double * const values, size_t n, int df );

enum MODETYPE
{
  MODE=1,
  ANTIMODE
};

/*
 Given an array of doubles, return the mode/anti-mode values.
 */
long double mode_value ( const long double * const values, size_t n, enum MODETYPE type);


/*
 comparison functions, to be used with 'qsort()'
 */
int cmpstringp(const void *p1, const void *p2);
int cmpstringp_nocase(const void *p1, const void *p2);
int cmp_long_double (const void *p1, const void *p2);

#endif

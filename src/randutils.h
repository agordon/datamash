/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2022 Tim Rice <trice@posteo.net>

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

/* Written by Tim Rice */
#ifndef __RANDUTILS_H__
#define __RANDUTILS_H__

#include <stdbool.h>

enum distribution
{
  DISTR_INVALID = -1,
  DISTR_UNIF    = 0,  /* Continuous uniform distribution U (a,b)    */
  DISTR_EXP,          /* Exponential distribution Exp (λ)           */
  DISTR_NORM,         /* Normal distribution N (μ,σ)                */
  DISTR_N_DISTR       /* Not a distribution, just counts the enum.  */
};

struct distribution_definition
{
  const char *name;
  enum distribution distr;
  void (*generator)(long reps);
};

struct distribution_parameter_definition
{
  char *name;
  bool is_set;
  long double value;
};

enum distribution_parameter
{
  not_a_parameter = -1,
  alpha = 0,
  beta,
  degf,
  degf2,
  mean,
  min,
  max,
  number,
  population,
  prob,
  rate,
  scale,
  stdev,
  successes,
  n_parameters
};

extern struct distribution_parameter_definition
distribution_parameters[n_parameters];
extern struct distribution_definition
distributions[DISTR_N_DISTR];

/* Initialize random number source */
void
init_random (bool force_seed, unsigned long seed);

#endif // __RANDUTILS_H__

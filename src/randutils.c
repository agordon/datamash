/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2021 Assaf Gordon <assafgordon@gmail.com>
   Copyright (C) 2022-     Tim Rice     <trice@posteo.net>

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

#include <config.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <sys/random.h>

#include "system.h"
#include "die.h"
#include "text-options.h"
#include "randutils.h"

/* The names should match enum distribution_parameter */
struct distribution_parameter_definition
distribution_parameters[n_parameters] = {
  { "alpha",      false,  0.0 },
  { "beta",       false,  0.0 },
  { "degf",       false,  0.0 },
  { "degf2",      false,  0.0 },
  { "mean",       false,  0.0 },
  { "min",        false,  0.0 },
  { "max",        false,  1.0 },
  { "number",     false,  1.0 },
  { "population", false,  1.0 },
  { "prob",       false,  0.0 },
  { "rate",       false,  1.0 },
  { "scale",      false,  0.0 },
  { "stdev",      false,  1.0 },
  { "successes",  false,  1.0 }
};

void runif (long reps);
void rexp  (long reps);
void rnorm (long reps);

struct distribution_definition distributions[DISTR_N_DISTR] =
{
  {"unif",        DISTR_UNIF,   &runif  },
  {"exp",         DISTR_EXP,    &rexp   },
  {"norm",        DISTR_NORM,   &rnorm  },
};

void
init_random (bool force_seed, unsigned long seed)
{
  if (!force_seed)
    {
      errno = 0;
      ssize_t nbytes = getrandom (&seed, sizeof (seed), 0);
      if (nbytes == -1 || errno != 0)
        {
          fprintf (stderr, "Error %d: %s\n", errno, strerror (errno));
        }
    }
  srandom (seed);
}

/*
  Some probability distributions, like Uniform and Exponential, are "primitives"
  required to simulate other distributions. We break their functions up into a
  "private" function prefixed with underscore, and a more user-facing function
  which does the printing.
*/
static long double
_runif ()
{
  return (long double)random ()/RAND_MAX;
}

static long double
_rexp ()
{
  return -logl (_runif ());
}

/* Box-Muller [1] mixes two Uniforms to gain two Normals.
   Since the output is array-valued, we can't return it in the usual way.
   [1] https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform */
static void
_rnorm (long double retvals[2])
{
  long double radius = sqrt (2*_rexp ());
  long double angle = 2*M_PI*_runif ();
  retvals[0] = radius*cos (angle);
  retvals[1] = radius*sin (angle);
}

void
runif (long reps)
{
  long double length = distribution_parameters[max].value
                      - distribution_parameters[min].value;
  for (long r = 0; r < reps; ++r)
    {
      printf ("%Lf", distribution_parameters[min].value + length*_runif ());
      fputc (eolchar, stdout);
    }
}

void
rexp (long reps)
{
  /* We allow either rate or mean to parametrize the exponential distribution,
     so long as not both are given. */
  if (distribution_parameters[rate].is_set
      && distribution_parameters[mean].is_set)
    die (EXIT_FAILURE, 0, _("only one of rate and mean may parametrize " \
                            "the exponential distribution"));
  long double r = distribution_parameters[rate].value;
  if (distribution_parameters[mean].is_set)
    r = 1.0/distribution_parameters[mean].value;
  for (long rep = 0; rep < reps; ++rep)
    {
      printf ("%Lf", _rexp ()/r);
      fputc (eolchar, stdout);
    }
}

void
rnorm (long reps)
{
  long double N[2]; /* Will be populated with two RVs at a time by Box-Muller */
  long double m = distribution_parameters[mean].value;
  long double s = distribution_parameters[stdev].value;
  long r = 0;
  for (r = 0; r < reps-1; r += 2)
    {
      _rnorm (N);
      printf ("%Lf%c%Lf%c", m+s*N[0], eolchar, m+s*N[1], eolchar);
    }
  if (r != reps) /* For odd-numbered reps */
    {
      _rnorm (N);
      printf ("%Lf", m+s*N[0]);
      fputc (eolchar, stdout);
    }
}

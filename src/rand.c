/* rand -- auxiliary program for simulating random numbers
   Copyright (C) 2022 Timothy Rice <trice@posteo.net>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */
#include <config.h>

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <inttypes.h>
#include <ctype.h>
#include <math.h>
#include <locale.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>

#include "system.h"
#include "error.h"
#include "closeout.h"
#include "stdnoreturn.h"
#include "linebuffer.h"
#include "die.h"
#define Version VERSION
#include "version-etc.h"
#include "sh-quote.h"

#include "text-options.h"
#include "randutils.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "rand"

#define AUTHORS \
  proper_name ("Tim Rice")

/* Until someone better comes along */
const char version_etc_copyright[] = "Copyright %s %d Timothy Rice" ;

char eolchar = '\n';

static char const shortopts[] = "a:A:b:B:f:F:k:m:n:N:p:r:s:S:t:hVz";

static struct option const longopts[] =
{
  /* Set a specific seed, eg for tests */
  {"seed",            required_argument,  NULL, 'S'},

  /* The following should match randutils.h enum parameters */
  {"alpha",           required_argument,  NULL, 'A'}, /* Shape parameter */
  {"beta",            required_argument,  NULL, 'B'}, /* Second shape param */
  {"degf",            required_argument,  NULL, 'f'}, /* For chi-sq, F distrs */
  {"degf2",           required_argument,  NULL, 'F'}, /* For F-distribution */
  {"mean",            required_argument,  NULL, 'm'},
  {"min",             required_argument,  NULL, 'a'}, /* The a for U (a,b) */
  {"max",             required_argument,  NULL, 'b'}, /* The b for U (a,b) */
  {"number",          required_argument,  NULL, 'n'}, /* The n for Bin (n,p) */
  {"population",      required_argument,  NULL, 'N'}, /* For hypergeometric */
  {"prob",            required_argument,  NULL, 'p'}, /* Bernoulli (p) etc */
  {"rate",            required_argument,  NULL, 'r'}, /* λ for Poisson etc */
  {"scale",           required_argument,  NULL, 't'}, /* Eg gamma distr */
  {"stdev",           required_argument,  NULL, 's'}, /* Standard deviation */
  {"successes",       required_argument,  NULL, 'k'}, /* Eg hypergeometric */

  {"zero-terminated", no_argument,        NULL, 'z'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [PARAMETERS] distribution [NUMBER]\n\
"),
              program_name);

      putchar ('\n');
      fputs (_("\
Generates pseudo-random numbers from popular probability distributions.\n"),
             stdout);
      fputs ("\n\n", stdout);
      fputs (_("\
'distribution' is the short name of a probability distribution.\n"),
             stdout);
      fputs (_("\
'NUMBER' defaults to 1 and simulates that many random variables from \
the chosen distribution.\n"),
             stdout);
      fputs ("\n\n", stdout);
      fputs (_("Available distribution short names:\n"), stdout);
      fputs ("  unif, exp, norm\n", stdout);
      putchar ('\n');

      fputs (_("Options:\n"), stdout);
      fputs ("\n", stdout);
      fputs (_("Options to set distribution parameters:\n"),stdout);
      putchar ('\n');
      fputs (_("\
  (Note that not all of these are currently used; they are reserved for future \
  implementation of associated distributions.)\n\
"), stdout);
      fputs (_("\
  -A, --alpha               Shape parameter\n\
"), stdout);
      fputs (_("\
  -B, --beta                Second shape parameter, eg for beta distribution\n\
"), stdout);
      fputs (_("\
  -f, --degf                Degrees of freedom for chi-square and F\n\
                              distributions\n\
"), stdout);
      fputs (_("\
  -F, --degf2               Second degree of freedom for F-distribution\n\
"), stdout);
      fputs (_("\
  -m, --mean                The mean for Normal and Exponential distributions\n\
"), stdout);
      fputs (_("\
  -a, --min                 The minimum of a continuous Uniform distribution\n\
"), stdout);
      fputs (_("\
  -b, --max                 The maximum of a continuous Uniform distribution\n\
"), stdout);
      fputs (_("\
  -n, --number              The sample size or number of draws for Binomial\n\
                              and Hypergeometric distributions\n\
"), stdout);
      fputs (_("\
  -N, --population          The population size for the Hypergeometric\n\
                              distribution\n\
"), stdout);
      fputs (_("\
  -p, --prob                The probability of each success in distributions\n\
                              based on Bernoulli trials\n\
"), stdout);
      fputs (_("\
  -r, --rate                The \"λ\" for distributions based on Poisson\n\
                              processes\n\
"), stdout);
      fputs (_("\
  -t, --scale               The scale, used for example by Gamma distribution\n\
"), stdout);
      fputs (_("\
  -s, --stdev               The standard deviation for Normal distributions\n\
"), stdout);
      fputs (_("\
  -k, --successes           The number of available \"success states\" for\n\
                              the Hypergeometric distribution\n\
"), stdout);

      fputs (_("General Options:\n"),stdout);
      fputs (_("\
  -z, --zero-terminated     end lines with 0 byte, not newline\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      putchar ('\n');

      fputs (_("Examples:"), stdout);
      fputs ("\n\n", stdout);
      fputs (_("Simulate the sample mean and standard deviation for 10 \
standard normal iidrvs:"), stdout);
      printf ("\n\
  $ rand norm 10 | datamash mean 1 sstdev 1\n\
  -0.2336997      0.99112189348592\n\
\n");

      fputs (_("For detailed usage information and examples, see\n"),stdout);
      printf ("  man %s\n", PROGRAM_NAME);
      fputs (_("The manual and more examples are available at\n"), stdout);
      fputs ("  " PACKAGE_URL "\n\n", stdout);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int opt;
  enum distribution_parameter p = not_a_parameter;
  bool force_seed = false;
  unsigned long int seed = 0;
  long reps = 0;

  openbsd_pledge ();

  set_program_name (argv[0]);

#ifdef FORCE_C_LOCALE
  /* Used on mingw/windows system */
  setlocale (LC_ALL, "C");
#else
  setlocale (LC_ALL, "");
#endif // FORCE_C_LOCALE

  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((opt = getopt_long (argc, argv, shortopts, longopts, NULL)) != -1)
    {
      p = not_a_parameter;
      switch (opt)
        {
        case 'S':
          force_seed = true;
          char *endptr;
          errno = 0;
          long proposed_seed = strtol (optarg, &endptr, 10);
          if (errno || *endptr != '\0' || proposed_seed < 0)
            die (EXIT_FAILURE, 0,
                 _("invalid seed"));
          seed = proposed_seed;
          break;

        case 'A':
          p = alpha;
          break;

        case 'B':
          p = beta;
          break;

        case 'f':
          p = degf;
          break;

        case 'F':
          p = degf2;
          break;

        case 'm':
          p = mean;
          break;

        case 'a':
          p = min;
          break;

        case 'b':
          p = max;
          break;

        case 'n':
          p = number;
          break;

        case 'N':
          p = population;
          break;

        case 'p':
          p = prob;
          break;

        case 'r':
          p = rate;
          break;

        case 's':
          p = stdev;
          break;

        case 'k':
          p = successes;
          break;

        case 't':
          p = scale;
          break;

        case 'z':
          eolchar = 0;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (SORT_FAILURE);
        }

      int erange_detected = 0;
      bool nan = false;
      long double argval = 0.0;
      switch (opt)
        {
        case 'A': case 'B': case 'a': case 'b': case 'f': case 'F': case 'k':
        case 'm': case 'n': case 'N': case 'p': case 'r': case 's': case 't': ;
          char *endptr;
          errno = 0;
          argval = strtold (optarg, &endptr);
          erange_detected = errno;
          if (*endptr != '\0')
            nan = true;
          distribution_parameters[p].value = argval;
          distribution_parameters[p].is_set = true;
          break;

        default:
          break;
        }

      if (erange_detected != 0)
        {
          fprintf (stderr, "rand: Warning: overflow or underflow detected \
when processing option -%c/--%s: '%s' became '%Lg'\n",
                  opt, distribution_parameters[p].name, optarg, argval);
        }

      if (nan)
        die (EXIT_FAILURE, 0,
             _("Non-numeric argument detected to parameter '%s': %s"),
             distribution_parameters[p].name, optarg);

      if (opt == 'r' && argval <= 0)
        die (EXIT_FAILURE, 0,
            _("not a valid rate: %Lf"), argval);
    }

  if (distribution_parameters[min].is_set
      || distribution_parameters[max].is_set)
    {
      long double lower = distribution_parameters[min].value;
      long double upper = distribution_parameters[max].value;
      if (lower > upper)
        die (EXIT_FAILURE, 0,
             _("min and max contradict: %Lf > %Lf"), lower, upper);
    }
  switch (argc-optind)
    {
      case 1:
        reps = 1;
        break;
      case 2: ;
        char *endptr;
        int e = 0;
        errno = 0;
        reps = strtol (argv[optind+1], &endptr, 10);
        e = errno;
        if (*endptr != '\0')
          die (EXIT_FAILURE, 0,
               _("Non-numeric argument detected to parameter '%s': %s"),
               distribution_parameters[p].name, optarg);
        if (e != 0)
          die (EXIT_FAILURE, 0,
               _("Error #%d: %s\n"), e, strerror (e));
        break;
      default:
        error (0, 0, _("invalid distribution specifier"));
        usage (EXIT_FAILURE);
        break;
    }

  init_random (force_seed, seed);

  enum distribution d = 0;
  for (d = 0; d < DISTR_N_DISTR; ++d)
  {
    if (STREQ (distributions[d].name, argv[optind]))
      break;
  }
  if (d == DISTR_N_DISTR)
    die (EXIT_FAILURE, 0,
        _("not a valid distribution: %s"), argv[0]);
  distributions[d].generator (reps);

  return EXIT_SUCCESS;
}

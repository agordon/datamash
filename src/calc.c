#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "system.h"
#include "config.h"
#include "linebuffer.h"
#include "error.h"
#include "quote.h"
#include "version.h"
#include "version-etc.h"


/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "calc"

#define AUTHORS proper_name ("Assaf Gordon")

/* Until someone better comes along */
const char version_etc_copyright[] = "Copyright %s %d Assaf Gordon" ;

enum
{
  DEBUG_OPTION = CHAR_MAX + 1,
};

static char const short_options[] = "zk:t:";

static struct option const long_options[] =
{
  {"", no_argument, NULL, 'b'},
  {"zero-terminated", no_argument, NULL, 'z'},
  {"field-separator", required_argument, NULL, 't'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0},
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION] op1 col1 [op2 col2]\n\
"),
              program_name);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
    }
  exit (status);
}



int main(int argc, char* argv[])
{
  int optc;

  set_program_name (argv[0]);

  while ((optc = getopt_long (argc, argv, short_options, long_options, NULL))
         != -1)
    {
      switch (optc)
        {
        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }

    }
}

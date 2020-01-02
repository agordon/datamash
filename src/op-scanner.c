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
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#include "system.h"

#include "die.h"
#include "op-scanner.h"

/* Used by other modules */
uintmax_t scan_val_int;
long double scan_val_float;
char* scanner_identifier;
bool scanner_keep_whitespace = false;

/* Internal */
static char* scanner_input;
static char* scan_pos;
static size_t scanner_identifier_len;
static bool have_peek;
static enum TOKEN scan_peek;

static inline void
set_identifier (const char* data, size_t n)
{
  if (n>=scanner_identifier_len)
    {
      scanner_identifier = xrealloc (scanner_identifier,n+1);
      scanner_identifier_len = n+1;
    }
  memcpy (scanner_identifier, data, n);
  scanner_identifier[n]=0;
}

/* Concatante argv into one (space separated) string */
void
scanner_set_input_from_argv (int argc, const char* argv[])
{
  assert (scanner_input == NULL);                /* LCOV_EXCL_LINE */

  size_t len = 1; /* +1 for NUL */
  for (int i=0;i<argc;++i)
    len += strlen (argv[i])+1; /* +1 for space */

  char *p = scan_pos = scanner_input = XCALLOC (len, char);
  for (int i=0; i<argc; ++i)
  {
      if (i>0)
        p = stpcpy (p, " ");
      p = stpcpy (p, argv[i]);
  }
}

void
scanner_free ()
{
  free (scanner_identifier);
  scanner_identifier = NULL;
  scanner_identifier_len = 0;

  free (scanner_input);
  scanner_input = NULL;
  scan_pos = NULL;
  have_peek = false;
}

enum TOKEN
scanner_peek_token ()
{
  if (have_peek)
    return scan_peek;

  scan_peek = scanner_get_token ();
  have_peek = true;
  return scan_peek;
}

enum TOKEN
scanner_get_token ()
{
  char ident[MAX_IDENTIFIER_LENGTH];
  char *pend;

  if (have_peek)
    {
      have_peek = false;
      return scan_peek;
    }

  assert (scan_pos != NULL);                      /* LCOV_EXCL_LINE */

  if (*scan_pos == '\0')
    return TOK_END;

  /* White space */
  if (c_isspace (*scan_pos))
    {
      while ( c_isspace (*scan_pos) )
        ++scan_pos;
      if (scanner_keep_whitespace)
        {
          if (*scan_pos == '\0')
            return TOK_END;
          return TOK_WHITESPACE;
        }
    }

  /* special characters */
  if (*scan_pos == ',')
    {
      ++scan_pos;
      set_identifier (",", 1);
      return TOK_COMMA;
    }
  if (*scan_pos == '-')
    {
      ++scan_pos;
      set_identifier ("-", 1);
      return TOK_DASH;
    }
  if (*scan_pos == ':')
    {
      ++scan_pos;
      set_identifier (":", 1);
      return TOK_COLONS;
    }

  /* Integer or floating-point value */
  if (c_isdigit (*scan_pos))
    {
      enum TOKEN rc = TOK_INTEGER;
      errno = 0;
      scan_val_int = strtol (scan_pos, &pend, 10);

      if (*pend == '.')
        {
          /* a floating-point value */
          scan_val_float = strtold (scan_pos, &pend);
          rc = TOK_FLOAT;
        }
      if ((c_isalpha (*pend) || *pend=='_') || (errno == ERANGE))
        die (EXIT_FAILURE, 0, _("invalid numeric value '%s'"),
                                scan_pos);

      set_identifier (scan_pos, pend-scan_pos);
      scan_pos = pend;
      return rc;
    }

  /* a valid identifier ( [a-z_][a-z0-9_]+ ),
     also backslash-CHAR is accepted as part of the identifier,
     to allow dash, colons, */
  if (c_isalpha (*scan_pos) || *scan_pos == '_' || *scan_pos == '\\')
    {
      int len = 0;
      char *v = scan_pos;
      while (1)
        {
          if (c_isalpha (*v) || c_isdigit (*v) || *v=='_' )
            {
              // Accept charracter
            }
          else if (*v == '\\')
            {
              ++v;
              if (!*v)
                die (EXIT_FAILURE, 0, _("backslash at end of identifier"));

              // Accept following character
            }
          else
            break;

          if (len >= (MAX_IDENTIFIER_LENGTH-1))
            die (EXIT_FAILURE, 0, _("identifier name too long"));

          ident[len++] = *v;
          ++v;
        }
      ident[len] = '\0';

      set_identifier (ident, len);
      scan_pos = v;

      return TOK_IDENTIFIER;
    }

  die (EXIT_FAILURE, 0, _("invalid operand %s"), quote (scan_pos));
  return TOK_END;
}


#ifdef SCANNER_TEST_MAIN
/*
 Trivial scanner tester.
 To compile:
    cc -D_STANDALONE_ -DSCANNER_TEST_MAIN \
       -I. \
       -std=c99 -Wall -Wextra -Werror -g -O0 \
       -o dm-scanner ./src/op-scanner.c
 Test:
    ./dm-scanner groupby 1,2 sum 4-7
    ./dm-scanner ppearson 1:6
    ./dm-scanner foo bar 9.5f
*/
#define TESTMAIN main
int TESTMAIN (int argc, const char* argv[])
{
  if (argc<2)
    die (EXIT_FAILURE, 0, _("missing script (among arguments)"));

  scanner_set_input_from_argv (argc-1, argv+1);

  enum TOKEN tok;
  while ( (tok = scanner_get_token ()) != TOK_END )
  {
    switch (tok)
    {
    case TOK_IDENTIFIER:
      printf ("TOK_IDENTIFIER: '%s'\n", scanner_identifier);
      break;

    case TOK_INTEGER:
      printf ("TOK_INTEGER: %lu ('%s')\n", scan_val_int, scanner_identifier);
      break;

    case TOK_FLOAT:
      printf ("TOK_FLOAT: %Lf ('%s')\n", scan_val_float, scanner_identifier);
      break;

    case TOK_COMMA:
      printf ("TOK_COMMA\n");
      break;

    case TOK_DASH:
      printf ("TOK_DASH\n");
      break;

    case TOK_COLONS:
      printf ("TOK_COLONS\n");
      break;

    default:
      die (EXIT_FAILURE, 0 ,_("unknown token %d\n"),tok);
    }
  }

  return 0;
}
#endif

/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=2: */
/* vim: set expandtab: */

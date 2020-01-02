/* system.h: system-dependent declarations.

   Common definitions, copied from coreutils' src/system.h .
   The original Copyright is below:

   system-dependent definitions for coreutils
   Copyright (C) 1989-2013 Free Software Foundation, Inc.

   Modifications for GNU Datamash are
   Copyright (C) 2014-2020 Assaf Gordon <assafgordon@gmail.com>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef __DATAMASH__SYSTEM_H__
#define __DATAMASH__SYSTEM_H__

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _STANDALONE_

/* compiling 'standalone' (bypassing gnulib) - add (unsafe) stubs */
#  define quote(x) (x)
#  define xrealloc(x,n) realloc (x,n)
#  define xrealloc(x,n) realloc (x,n)
#  define XCALLOC(n,t)  calloc (n,sizeof (t))
#  define XZALLOC(t)    calloc (1,sizeof (t))
#  define xstrdup(c)    strdup (c)
static inline void*
x2nrealloc (void *p, size_t *pn, size_t s)
{
  size_t n = (*pn==0)?1000:( (*pn)*2 );
  if (!p) { n = 1000 ; }
  *pn = n;
  return realloc (p, n*s);
}
#  define _(x) (x)
#  define N_(x) (x)

#  include <err.h>
#  define error(x,y,z,...) errx(x,z,##__VA_ARGS__)
#  define program_name "datamash"

#  define c_isalpha(c) isalpha((int)c)
#  define c_isspace(c) isspace((int)c)
#  define c_isdigit(c) isdigit((int)c)

#else /* !_STANDALONE_ */

#  include "xalloc.h"
#  include "c-ctype.h"
#  include "quote.h"
#  include "error.h"

/* Take care of NLS matters.  */
#include "gettext.h"
#if ! ENABLE_NLS
# undef textdomain
# define textdomain(Domainname) /* empty */
# undef bindtextdomain
# define bindtextdomain(Domainname, Dirname) /* empty */
#endif

#define _(msgid) gettext (msgid)
#define N_(msgid) msgid

/* Define away proper_name (leaving proper_name_utf8, which affects far
   fewer programs), since it's not worth the cost of adding ~17KB to
   the x86_64 text size of every single program.  This avoids a 40%
   (almost ~2MB) increase in the on-disk space utilization for the set
   of the 100 binaries. */
#define proper_name(x) (x)

#include "progname.h"

#endif /* !_STANDALONE_ */

#define STREQ(a, b) (strcmp (a, b) == 0)
#define STREQ_LEN(a, b, n) (strncmp (a, b, n) == 0)
#define STRPREFIX(a, b) (strncmp(a, b, strlen (b)) == 0)

#define case_GETOPT_VERSION_CHAR(Program_name, Authors)			\
  case GETOPT_VERSION_CHAR:						\
    version_etc (stdout, Program_name, PACKAGE_NAME, Version, Authors,	\
                 (char *) NULL);					\
    exit (EXIT_SUCCESS);						\
    break;

/* Factor out some of the common --help and --version processing code.  */

/* These enum values cannot possibly conflict with the option values
   ordinarily used by commands, including CHAR_MAX + 1, etc.  Avoid
   CHAR_MIN - 1, as it may equal -1, the getopt end-of-options value.  */
enum
{
  GETOPT_HELP_CHAR = (CHAR_MIN - 2),
  GETOPT_VERSION_CHAR = (CHAR_MIN - 3)
};

#define GETOPT_HELP_OPTION_DECL \
  "help", no_argument, NULL, GETOPT_HELP_CHAR
#define GETOPT_VERSION_OPTION_DECL \
  "version", no_argument, NULL, GETOPT_VERSION_CHAR

#define case_GETOPT_HELP_CHAR			\
  case GETOPT_HELP_CHAR:			\
    usage (EXIT_SUCCESS);			\
    break;

#define HELP_OPTION_DESCRIPTION \
  _("      --help     display this help and exit\n")
#define VERSION_OPTION_DESCRIPTION \
  _("      --version  output version information and exit\n")

static inline void
emit_try_help (void)
{
  fprintf (stderr, _("Try '%s --help' for more information.\n"), program_name);
}

#ifndef ATTRIBUTE_NORETURN
# define ATTRIBUTE_NORETURN __attribute__ ((__noreturn__))
#endif

/* Convert a possibly-signed character to an unsigned character.  This is
   a bit safer than casting to unsigned char, since it catches some type
   errors that the cast doesn't.  */
static inline unsigned char to_uchar (char ch) { return ch; }

#ifndef MAX
# define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
# define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

/* Use this to suppress gcc's '...may be used before initialized' warnings. */
#ifdef lint
# define IF_LINT(Code) Code
#else
# define IF_LINT(Code) /* empty */
#endif

/* ISDIGIT differs from isdigit, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char
     or EOF.
   - It's typically faster.
   POSIX says that only '0' through '9' are digits.  Prefer ISDIGIT to
   isdigit unless it's important to use the locale's definition
   of 'digit' even when the host does not conform to POSIX.  */
#define ISDIGIT(c) ((unsigned int) (c) - '0' <= 9)

/* Return a value that pluralizes the same way that N does, in all
   languages we know of.  */
static inline unsigned long int
select_plural (uintmax_t n)
{
  /* Reduce by a power of ten, but keep it away from zero.  The
     gettext manual says 1000000 should be safe.  */
  enum { PLURAL_REDUCER = 1000000 };
  return (n <= ULONG_MAX ? n : n % PLURAL_REDUCER + PLURAL_REDUCER);
}

#define __STRX__(x) #x
#define __STR__(x) __STRX__(x)
#define __STR_LINE__ __STR__(__LINE__)
#define internal_error(x) assert(!#x)

#endif /* __DATAMASH__SYSTEM_H__ */

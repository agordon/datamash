/* key-compare.c - Key-comparison common functions
   Copyright (C) 2014-2020 Free Software Foundation, Inc.

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

/* Extracted from sort.c, modified by Assaf Gordon */

/* define the following to enable extra key options:
    KEY_COMPARE_RANDOM       - sort by random order (-k1R,1)
    KEY_COMPARE_REVERSE      - reverse sort order   (-k1r,1)
    KEY_COMPARE_VERSION      - Version sort order   (-k1V,1)
    KEY_COMPARE_HUMAN_NUMERIC- Human sizes order    (-k1h,1)
    KEY_COMPARE_MONTH        - Month sort (Jan>Feb) (-K1M,1)
    KEY_COMPARE_NONPRINTING  - Only non-printables  (-K1i,1)

  If these are not defined, specifing them will generate an error.

  See 'set_ordering ()' and 'key_to_opts ()' in this file,
  and "src_uniq_CPPFLAGS" in "src/local.mk" for usage examples.
 */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "die.h"
#ifdef KEY_COMPARE_VERSION
//#include "filevercmp.h"
#endif
#include "hard-locale.h"
#ifdef KEY_COMPARE_RANDOM
//#include "md5.h"
//#include "randread.h"
#endif
#include "quote.h"
//#include "strnumcmp.h"
//#include "xmemcoll.h"
#include "xstrtol.h"
#include "xalloc.h"
//#include "mbswidth.h"
#include "key-compare.h"

#if HAVE_LANGINFO_CODESET
# include <langinfo.h>
#endif

#include <inttypes.h>
#include <intprops.h>
#include <ctype.h>

/* '\n' is considered a field separator with  --zero-terminated.  */
static inline bool
field_sep (unsigned char ch)
{
  return isblank (ch) || ch == '\n';
}

/* The representation of the decimal point in the current locale.  */
int decimal_point = 0;

/* Thousands separator; if -1, then there isn't one.  */
int thousands_sep = 0;

/* Nonzero if the corresponding locales are hard.  */
bool hard_LC_COLLATE = false;
#if HAVE_NL_LANGINFO
bool hard_LC_TIME = 0;
#endif


/* FIXME: None of these tables work with multibyte character sets.
   Also, there are many other bugs when handling multibyte characters.
   One way to fix this is to rewrite 'sort' to use wide characters
   internally, but doing this with good performance is a bit
   tricky.  */

/* Table of blanks.  */
static bool blanks[UCHAR_LIM];

#ifdef KEY_COMPARE_NONPRINTING
/* Table of non-printing characters. */
static bool nonprinting[UCHAR_LIM];
#endif

/* Table of non-dictionary characters (not letters, digits, or blanks). */
static bool nondictionary[UCHAR_LIM];

/* Translation table folding lower case to upper.  */
static char fold_toupper[UCHAR_LIM];

#ifdef KEY_COMPARE_MONTH
struct month
{
  char const *name;
  int val;
};

#define MONTHS_PER_YEAR 12

#endif /* KEY_COMPARE_MONTH */

/* Tab character separating fields.  If TAB_DEFAULT, then fields are
   separated by the empty string between a non-blank character and a blank
   character. */
int tab = TAB_DEFAULT;

/* List of key field comparisons to be tried.  */
struct keyfield *keylist = NULL;


/* Initialize the character class tables. */
static void
inittables (void)
{
  size_t i;

  for (i = 0; i < UCHAR_LIM; ++i)
    {
      blanks[i] = field_sep (i);
#ifdef KEY_COMPARE_NONPRINTING
      nonprinting[i] = ! isprint (i);
#endif
      nondictionary[i] = ! isalnum (i) && ! field_sep (i);
      fold_toupper[i] = toupper (i);
    }
}

/* Return a pointer to the first character of the field specified
   by KEY in LINE. */

extern char * _GL_ATTRIBUTE_PURE
begfield (struct line const *line, struct keyfield const *key)
{
  char *ptr = line->text, *lim = ptr + line->length - 1;
  size_t sword = key->sword;
  size_t schar = key->schar;

  /* The leading field separator itself is included in a field when -t
     is absent.  */

  if (tab != TAB_DEFAULT)
    while (ptr < lim && sword--)
      {
        while (ptr < lim && *ptr != tab)
          ++ptr;
        if (ptr < lim)
          ++ptr;
      }
  else
    while (ptr < lim && sword--)
      {
        while (ptr < lim && blanks[to_uchar (*ptr)])
          ++ptr;
        while (ptr < lim && !blanks[to_uchar (*ptr)])
          ++ptr;
      }

  /* If we're ignoring leading blanks when computing the Start
     of the field, skip past them here.  */
  if (key->skipsblanks)
    while (ptr < lim && blanks[to_uchar (*ptr)])
      ++ptr;

  /* Advance PTR by SCHAR (if possible), but no further than LIM.  */
  ptr = MIN (lim, ptr + schar);

  return ptr;
}

/* Return the limit of (a pointer to the first character after) the field
   in LINE specified by KEY. */

extern char * _GL_ATTRIBUTE_PURE
limfield (struct line const *line, struct keyfield const *key)
{
  char *ptr = line->text, *lim = ptr + line->length - 1;
  size_t eword = key->eword, echar = key->echar;

  if (echar == 0)
    eword++; /* Skip all of end field.  */

  /* Move PTR past EWORD fields or to one past the last byte on LINE,
     whichever comes first.  If there are more than EWORD fields, leave
     PTR pointing at the beginning of the field having zero-based index,
     EWORD.  If a delimiter character was specified (via -t), then that
     'beginning' is the first character following the delimiting TAB.
     Otherwise, leave PTR pointing at the first 'blank' character after
     the preceding field.  */
  if (tab != TAB_DEFAULT)
    while (ptr < lim && eword--)
      {
        while (ptr < lim && *ptr != tab)
          ++ptr;
        if (ptr < lim && (eword || echar))
          ++ptr;
      }
  else
    while (ptr < lim && eword--)
      {
        while (ptr < lim && blanks[to_uchar (*ptr)])
          ++ptr;
        while (ptr < lim && !blanks[to_uchar (*ptr)])
          ++ptr;
      }

#ifdef POSIX_UNSPECIFIED
  /* The following block of code makes GNU sort incompatible with
     standard Unix sort, so it's ifdef'd out for now.
     The POSIX spec isn't clear on how to interpret this.
     FIXME: request clarification.

     From: kwzh@gnu.ai.mit.edu (Karl Heuer)
     Date: Thu, 30 May 96 12:20:41 -0400
     [Translated to POSIX 1003.1-2001 terminology by Paul Eggert.]

     [...]I believe I've found another bug in 'sort'.

     $ cat /tmp/sort.in
     a b c 2 d
     pq rs 1 t
     $ textutils-1.15/src/sort -k1.7,1.7 </tmp/sort.in
     a b c 2 d
     pq rs 1 t
     $ /bin/sort -k1.7,1.7 </tmp/sort.in
     pq rs 1 t
     a b c 2 d

     Unix sort produced the answer I expected: sort on the single character
     in column 7.  GNU sort produced different results, because it disagrees
     on the interpretation of the key-end spec "M.N".  Unix sort reads this
     as "skip M-1 fields, then N-1 characters"; but GNU sort wants it to mean
     "skip M-1 fields, then either N-1 characters or the rest of the current
     field, whichever comes first".  This extra clause applies only to
     key-ends, not key-starts.
     */

  /* Make LIM point to the end of (one byte past) the current field.  */
  if (tab != TAB_DEFAULT)
    {
      char *newlim;
      newlim = memchr (ptr, tab, lim - ptr);
      if (newlim)
        lim = newlim;
    }
  else
    {
      char *newlim;
      newlim = ptr;
      while (newlim < lim && blanks[to_uchar (*newlim)])
        ++newlim;
      while (newlim < lim && !blanks[to_uchar (*newlim)])
        ++newlim;
      lim = newlim;
    }
#endif

  if (echar != 0) /* We need to skip over a portion of the end field.  */
    {
      /* If we're ignoring leading blanks when computing the End
         of the field, skip past them here.  */
      if (key->skipeblanks)
        while (ptr < lim && blanks[to_uchar (*ptr)])
          ++ptr;

      /* Advance PTR by ECHAR (if possible), but no further than LIM.  */
      ptr = MIN (lim, ptr + echar);
    }

  return ptr;
}



/* Insert a malloc'd copy of key KEY_ARG at the end of the key list.  */

extern struct keyfield*
insertkey (struct keyfield *key_arg)
{
  struct keyfield **p;
  struct keyfield *key = xmemdup (key_arg, sizeof *key);

  for (p = &keylist; *p; p = &(*p)->next)
    continue;
  *p = key;
  key->next = NULL;
  return key;
}

/* Report a bad field specification SPEC, with extra info MSGID.  */
extern void
badfieldspec (char const *spec, char const *msgid)
{
  die (SORT_FAILURE, 0, _("%s: invalid field specification %s"),
         _(msgid), quote (spec));
  abort ();
}

/* Parse the leading integer in STRING and store the resulting value
   (which must fit into size_t) into *VAL.  Return the address of the
   suffix after the integer.  If the value is too large, silently
   substitute SIZE_MAX.  If MSGID is NULL, return NULL after
   failure; otherwise, report MSGID and exit on failure.  */

extern char const *
parse_field_count (char const *string, size_t *val, char const *msgid)
{
  char *suffix;
  uintmax_t n;

  switch (xstrtoumax (string, &suffix, 10, &n, ""))
    {
    case LONGINT_OK:
    case LONGINT_INVALID_SUFFIX_CHAR:
      *val = n;
      if (*val == n)
        break;
      FALLTHROUGH;
    case LONGINT_OVERFLOW:
    case LONGINT_OVERFLOW | LONGINT_INVALID_SUFFIX_CHAR:
      *val = SIZE_MAX;
      break;

    case LONGINT_INVALID:
      if (msgid)
        die (SORT_FAILURE, 0, _("%s: invalid count at start of %s"),
               _(msgid), quote (string));
      return NULL;

    default:
      abort ();
    }

  return suffix;
}


/* Set the ordering options for KEY specified in S.
   Return the address of the first character in S that
   is not a valid ordering option.
   BLANKTYPE is the kind of blanks that 'b' should skip. */

extern char *
set_ordering (char const *s, struct keyfield *key, enum blanktype blanktype)
{
  while (*s)
    {
      switch (*s)
        {
        case 'b':
          if (blanktype == bl_start || blanktype == bl_both)
            key->skipsblanks = true;
          if (blanktype == bl_end || blanktype == bl_both)
            key->skipeblanks = true;
          break;
        case 'd':
          key->ignore = nondictionary;
          break;
        case 'f':
          key->translate = fold_toupper;
          break;
        case 'g':
          key->general_numeric = true;
          break;
#ifdef KEY_COMPARE_HUMAN_NUMERIC
        case 'h':
          key->human_numeric = true;
          break;
#endif
#ifdef KEY_COMPARE_NONPRINTING
        case 'i':
          /* Option order should not matter, so don't let -i override
             -d.  -d implies -i, but -i does not imply -d.  */
          if (! key->ignore)
            key->ignore = nonprinting;
          break;
#endif
#ifdef KEY_COMPARE_MONTH
        case 'M':
          key->month = true;
          break;
#endif
        case 'n':
          key->numeric = true;
          break;
#ifdef KEY_COMPARE_RANDOM
        case 'R':
          key->random = true;
          break;
#endif
#ifdef KEY_COMPARE_REVERSE
        case 'r':
          key->reverse = true;
          break;
#endif
#ifdef KEY_COMPARE_VERSION
        case 'V':
          key->version = true;
          break;
#endif
        default:
          return (char *) s;
        }
      ++s;
    }
  return (char *) s;
}

/* Initialize KEY.  */

extern struct keyfield *
key_init (struct keyfield *key)
{
  memset (key, 0, sizeof *key);
  key->eword = SIZE_MAX;
  return key;
}



extern void
init_key_spec (void)
{
  hard_LC_COLLATE = hard_locale (LC_COLLATE);
#if HAVE_NL_LANGINFO
  hard_LC_TIME = hard_locale (LC_TIME);
#endif

  /* Get locale's representation of the decimal point.  */
  {
    struct lconv const *locale = localeconv ();

    /* If the locale doesn't define a decimal point, or if the decimal
       point is multibyte, use the C locale's decimal point.  FIXME:
       add support for multibyte decimal points.  */
    decimal_point = to_uchar (locale->decimal_point[0]);
    if (! decimal_point || locale->decimal_point[1])
      decimal_point = '.';

    /* FIXME: add support for multibyte thousands separators.  */
    thousands_sep = to_uchar (*locale->thousands_sep);
    if (! thousands_sep || locale->thousands_sep[1])
      thousands_sep = -1;
  }

  inittables ();
}


extern char*
debug_keyfield (const struct keyfield *key)
{
  size_t len = (INT_BUFSIZE_BOUND (uintmax_t))*4 /* up to 4 numbers */
    + strlen ("-k,.")  /* argument syntax */
    + strlen ("bbdfghiMnRrV") /* all possible options */
    + ( (key->decorate_fn)?30:0 )
    + ( (key->decorate_cmd)?strlen (key->decorate_cmd):0)
    + 100 + 1 ; /* extra for good luck, and NULL */

  char* buf = xcalloc (len,1);
  char *p = buf;

  p = stpcpy (p,"-k");
  p += sprintf (p, "%zu", key->sword+1);
  if (key->schar)
    p += sprintf (p,".%zu", key->schar+1);
  if (key->skipsblanks)
    p = stpcpy (p,"b");
  if (key->eword != SIZE_MAX)
    {
      p += sprintf (p,",%zu",key->eword+1);
      if (key->echar)
        p += sprintf (p,".%zu", key->echar);
    }
  if (key->skipeblanks)
    p = stpcpy (p,"b");
  if (key->ignore == nondictionary)
    p = stpcpy (p,"d");
  if (key->translate == fold_toupper)
    p = stpcpy (p,"f");
  if (key->general_numeric)
    p = stpcpy (p,"g");
  if (key->human_numeric)
    p = stpcpy (p,"h");
  if (key->ignore == nonprinting)
    p = stpcpy (p,"i");
  if (key->month)
    p = stpcpy (p,"M");
  if (key->numeric)
    p = stpcpy (p,"n");
  if (key->random)
    p = stpcpy (p,"R");
  if (key->reverse)
    p = stpcpy (p,"r");
  if (key->version)
    p = stpcpy (p,"V");

  if (key->decorate_fn)
    p += sprintf (p,":[decorate %p]", key->decorate_fn);
  if (key->decorate_cmd)
    p += sprintf (p,"@%s", key->decorate_cmd);

  return buf;
}

extern void
debug_keylist (FILE *stream)
{
  struct keyfield const *key = keylist;

  do {
    char *p = debug_keyfield (key);
    fputs (p, stream);
    fputc ('\n', stream);
    free (p);
  }  while (key && ((key = key->next)));
}

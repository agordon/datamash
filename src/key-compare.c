/* key-spec-parsing.c - Key-comparison common functions
   Copyright (C) 2013 Free Software Foundation, Inc.

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
    KEY_SPEC_RANDOM       - sort by random order (-k1R,1)
    KEY_SPEC_REVERSE      - reverse sort order   (-k1r,1)
    KEY_SPEC_VERSION      - Version sort order   (-k1V,1)
    KEY_SPEC_HUMAN_NUMERIC- Human sizes order    (-k1h,1)
    KEY_SPEC_MONTH        - Month sort (Jan>Feb) (-K1M,1)
    KEY_SPEC_NONPRINTING  - Only non-printables  (-K1i,1)

  If these are not defined, specifing them will generate an error.

  See 'set_ordering()' and 'key_to_opts()' in this file,
  and "src_uniq_CPPFLAGS" in "src/local.mk" for usage examples.
 */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include <ctype.h>
#include <locale.h>
#include "minmax.h"
#include "error.h"
#ifdef KEY_SPEC_VERSION
#include "filevercmp.h"
#endif
#include "hard-locale.h"
#ifdef KEY_SPEC_RANDOM
#include "randread.h"
#include "md5.h"
#endif
#include "gettext.h"
#include "intprops.h"
#include "inttostr.h"
#include "quote.h"
#include "quotearg.h"
#include "strnumcmp.h"
#include "xmemcoll.h"
#include "xalloc.h"
#include "xstrtol.h"
#include "mbswidth.h"
#include "ignore-value.h"
#include "key-compare.h"

#if HAVE_LANGINFO_CODESET
# include <langinfo.h>
#endif


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

#ifdef KEY_SPEC_NONPRINTING
/* Table of non-printing characters. */
static bool nonprinting[UCHAR_LIM];
#endif

/* Table of non-dictionary characters (not letters, digits, or blanks). */
static bool nondictionary[UCHAR_LIM];

/* Translation table folding lower case to upper.  */
static char fold_toupper[UCHAR_LIM];

#ifdef KEY_SPEC_MONTH
struct month
{
  char const *name;
  int val;
};

#define MONTHS_PER_YEAR 12

/* Table mapping month names to integers.
   Alphabetic order allows binary search. */
static struct month monthtab[] =
{
  {"APR", 4},
  {"AUG", 8},
  {"DEC", 12},
  {"FEB", 2},
  {"JAN", 1},
  {"JUL", 7},
  {"JUN", 6},
  {"MAR", 3},
  {"MAY", 5},
  {"NOV", 11},
  {"OCT", 10},
  {"SEP", 9}
};
#endif /* KEY_SPEC_MONTH */

/* Tab character separating fields.  If TAB_DEFAULT, then fields are
   separated by the empty string between a non-blank character and a blank
   character. */
int tab = TAB_DEFAULT;

/* List of key field comparisons to be tried.  */
struct keyfield *keylist = NULL;

/* Report MESSAGE for FILE, then clean up and exit.
   If FILE is null, it represents standard output.  */

void
die (char const *message, char const *file)
{
  error (0, errno, "%s: %s", message, file ? file : _("standard output"));
  exit (SORT_FAILURE);
}


#if HAVE_NL_LANGINFO
#ifdef KEY_SPEC_MONTH
static int
struct_month_cmp (void const *m1, void const *m2)
{
  struct month const *month1 = m1;
  struct month const *month2 = m2;
  return strcmp (month1->name, month2->name);
}
#endif
#endif

/* Initialize the character class tables. */

static void
inittables (void)
{
  size_t i;

  for (i = 0; i < UCHAR_LIM; ++i)
    {
      blanks[i] = !! isblank (i);
#ifdef KEY_SPEC_NONPRINTING
      nonprinting[i] = ! isprint (i);
#endif
      nondictionary[i] = ! isalnum (i) && ! isblank (i);
      fold_toupper[i] = toupper (i);
    }

#if HAVE_NL_LANGINFO
#ifdef KEY_SPEC_MONTH
  /* If we're not in the "C" locale, read different names for months.  */
  if (hard_LC_TIME)
    {
      for (i = 0; i < MONTHS_PER_YEAR; i++)
        {
          char const *s;
          size_t s_len;
          size_t j, k;
          char *name;

          s = nl_langinfo (ABMON_1 + i);
          s_len = strlen (s);
          monthtab[i].name = name = xmalloc (s_len + 1);
          monthtab[i].val = i + 1;

          for (j = k = 0; j < s_len; j++)
            if (! isblank (to_uchar (s[j])))
              name[k++] = fold_toupper[to_uchar (s[j])];
          name[k] = '\0';
        }
      qsort (monthtab, MONTHS_PER_YEAR, sizeof *monthtab, struct_month_cmp);
    }
#endif
#endif
}


/* Return a pointer to the first character of the field specified
   by KEY in LINE. */

char * _GL_ATTRIBUTE_PURE
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

char * _GL_ATTRIBUTE_PURE
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

/* Table that maps characters to order-of-magnitude values.  */
static char const unit_order[UCHAR_LIM] =
  {
#if ! ('K' == 75 && 'M' == 77 && 'G' == 71 && 'T' == 84 && 'P' == 80 \
     && 'E' == 69 && 'Z' == 90 && 'Y' == 89 && 'k' == 107)
    /* This initializer syntax works on all C99 hosts.  For now, use
       it only on non-ASCII hosts, to ease the pain of porting to
       pre-C99 ASCII hosts.  */
    ['K']=1, ['M']=2, ['G']=3, ['T']=4, ['P']=5, ['E']=6, ['Z']=7, ['Y']=8,
    ['k']=1,
#else
    /* Generate the following table with this command:
       perl -e 'my %a=(k=>1, K=>1, M=>2, G=>3, T=>4, P=>5, E=>6, Z=>7, Y=>8);
       foreach my $i (0..255) {my $c=chr($i); $a{$c} ||= 0;print "$a{$c}, "}'\
       |fmt  */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 3,
    0, 0, 0, 1, 0, 2, 0, 0, 5, 0, 0, 0, 4, 0, 0, 0, 0, 8, 7, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#endif
  };

#ifdef KEY_SPEC_HUMAN_NUMERIC
/* Return an integer that represents the order of magnitude of the
   unit following the number.  The number may contain thousands
   separators and a decimal point, but it may not contain leading blanks.
   Negative numbers get negative orders; zero numbers have a zero order.  */

static int _GL_ATTRIBUTE_PURE
find_unit_order (char const *number)
{
  bool minus_sign = (*number == '-');
  char const *p = number + minus_sign;
  int nonzero = 0;
  unsigned char ch;

  /* Scan to end of number.
     Decimals or separators not followed by digits stop the scan.
     Numbers ending in decimals or separators are thus considered
     to be lacking in units.
     FIXME: add support for multibyte thousands_sep and decimal_point.  */

  do
    {
      while (ISDIGIT (ch = *p++))
        nonzero |= ch - '0';
    }
  while (ch == thousands_sep);

  if (ch == decimal_point)
    while (ISDIGIT (ch = *p++))
      nonzero |= ch - '0';

  if (nonzero)
    {
      int order = unit_order[ch];
      return (minus_sign ? -order : order);
    }
  else
    return 0;
}

/* Compare numbers A and B ending in units with SI or IEC prefixes
       <none/unknown> < K/k < M < G < T < P < E < Z < Y  */

static int
human_numcompare (char const *a, char const *b)
{
  while (blanks[to_uchar (*a)])
    a++;
  while (blanks[to_uchar (*b)])
    b++;

  int diff = find_unit_order (a) - find_unit_order (b);
  return (diff ? diff : strnumcmp (a, b, decimal_point, thousands_sep));
}
#endif /* KEY_SPEC_HUMAN_NUMERIC */

/* Compare strings A and B as numbers without explicitly converting them to
   machine numbers.  Comparatively slow for short strings, but asymptotically
   hideously fast. */

static int
numcompare (char const *a, char const *b)
{
  while (blanks[to_uchar (*a)])
    a++;
  while (blanks[to_uchar (*b)])
    b++;

  return strnumcmp (a, b, decimal_point, thousands_sep);
}

/* Work around a problem whereby the long double value returned by glibc's
   strtold ("NaN", ...) contains uninitialized bits: clear all bytes of
   A and B before calling strtold.  FIXME: remove this function once
   gnulib guarantees that strtold's result is always well defined.  */
static int
nan_compare (char const *sa, char const *sb)
{
  long_double a;
  memset (&a, 0, sizeof a);
  a = strtold (sa, NULL);

  long_double b;
  memset (&b, 0, sizeof b);
  b = strtold (sb, NULL);

  return memcmp (&a, &b, sizeof a);
}

static int
general_numcompare (char const *sa, char const *sb)
{
  /* FIXME: maybe add option to try expensive FP conversion
     only if A and B can't be compared more cheaply/accurately.  */

  char *ea;
  char *eb;
  long_double a = strtold (sa, &ea);
  long_double b = strtold (sb, &eb);

  /* Put conversion errors at the start of the collating sequence.  */
  if (sa == ea)
    return sb == eb ? 0 : -1;
  if (sb == eb)
    return 1;

  /* Sort numbers in the usual way, where -0 == +0.  Put NaNs after
     conversion errors but before numbers; sort them by internal
     bit-pattern, for lack of a more portable alternative.  */
  return (a < b ? -1
          : a > b ? 1
          : a == b ? 0
          : b == b ? -1
          : a == a ? 1
          : nan_compare (sa, sb));
}

#ifdef KEY_SPEC_MONTH
/* Return an integer in 1..12 of the month name MONTH.
   Return 0 if the name in S is not recognized.  */

static int
getmonth (char const *month, char **ea)
{
  size_t lo = 0;
  size_t hi = MONTHS_PER_YEAR;

  while (blanks[to_uchar (*month)])
    month++;

  do
    {
      size_t ix = (lo + hi) / 2;
      char const *m = month;
      char const *n = monthtab[ix].name;

      for (;; m++, n++)
        {
          if (!*n)
            {
              if (ea)
                *ea = (char *) m;
              return monthtab[ix].val;
            }
          if (to_uchar (fold_toupper[to_uchar (*m)]) < to_uchar (*n))
            {
              hi = ix;
              break;
            }
          else if (to_uchar (fold_toupper[to_uchar (*m)]) > to_uchar (*n))
            {
              lo = ix + 1;
              break;
            }
        }
    }
  while (lo < hi);

  return 0;
}
#endif /* KEY_SPEC_MONTH */

#ifdef KEY_SPEC_RANDOM
/* A randomly chosen MD5 state, used for random comparison.  */
static struct md5_ctx random_md5_state;

/* Initialize the randomly chosen MD5 state.  */

void
random_md5_state_init (char const *random_source)
{
  unsigned char buf[MD5_DIGEST_SIZE];
  struct randread_source *r = randread_new (random_source, sizeof buf);
  if (! r)
    die (_("open failed"), random_source);
  randread (r, buf, sizeof buf);
  if (randread_free (r) != 0)
    die (_("close failed"), random_source);
  md5_init_ctx (&random_md5_state);
  md5_process_bytes (buf, sizeof buf, &random_md5_state);
}

/* This is like strxfrm, except it reports any error and exits.  */

static size_t
xstrxfrm (char *restrict dest, char const *restrict src, size_t destsize)
{
  errno = 0;
  size_t translated_size = strxfrm (dest, src, destsize);

  if (errno)
    {
      error (0, errno, _("string transformation failed"));
      error (0, 0, _("set LC_ALL='C' to work around the problem"));
      error (SORT_FAILURE, 0,
             _("the untransformed string was %s"),
             quotearg_n_style (0, locale_quoting_style, src));
    }

  return translated_size;
}

/* Compare the keys TEXTA (of length LENA) and TEXTB (of length LENB)
   using one or more random hash functions.  TEXTA[LENA] and
   TEXTB[LENB] must be zero.  */

static int
compare_random (char *restrict texta, size_t lena,
                char *restrict textb, size_t lenb)
{
  /* XFRM_DIFF records the equivalent of memcmp on the transformed
     data.  This is used to break ties if there is a checksum
     collision, and this is good enough given the astronomically low
     probability of a collision.  */
  int xfrm_diff = 0;

  char stackbuf[4000];
  char *buf = stackbuf;
  size_t bufsize = sizeof stackbuf;
  void *allocated = NULL;
  uint32_t dig[2][MD5_DIGEST_SIZE / sizeof (uint32_t)];
  struct md5_ctx s[2];
  s[0] = s[1] = random_md5_state;

  if (hard_LC_COLLATE)
    {
      char const *lima = texta + lena;
      char const *limb = textb + lenb;

      while (true)
        {
          /* Transform the text into the basis of comparison, so that byte
             strings that would otherwise considered to be equal are
             considered equal here even if their bytes differ.

             Each time through this loop, transform one
             null-terminated string's worth from TEXTA or from TEXTB
             or both.  That way, there's no need to store the
             transformation of the whole line, if it contains many
             null-terminated strings.  */

          /* Store the transformed data into a big-enough buffer.  */

          /* A 3X size guess avoids the overhead of calling strxfrm
             twice on typical implementations.  Don't worry about
             size_t overflow, as the guess need not be correct.  */
          size_t guess_bufsize = 3 * (lena + lenb) + 2;
          if (bufsize < guess_bufsize)
            {
              bufsize = MAX (guess_bufsize, bufsize * 3 / 2);
              free (allocated);
              buf = allocated = malloc (bufsize);
              if (! buf)
                {
                  buf = stackbuf;
                  bufsize = sizeof stackbuf;
                }
            }

          size_t sizea =
            (texta < lima ? xstrxfrm (buf, texta, bufsize) + 1 : 0);
          bool a_fits = sizea <= bufsize;
          size_t sizeb =
            (textb < limb
             ? (xstrxfrm ((a_fits ? buf + sizea : NULL), textb,
                          (a_fits ? bufsize - sizea : 0))
                + 1)
             : 0);

          if (! (a_fits && sizea + sizeb <= bufsize))
            {
              bufsize = sizea + sizeb;
              if (bufsize < SIZE_MAX / 3)
                bufsize = bufsize * 3 / 2;
              free (allocated);
              buf = allocated = xmalloc (bufsize);
              if (texta < lima)
                strxfrm (buf, texta, sizea);
              if (textb < limb)
                strxfrm (buf + sizea, textb, sizeb);
            }

          /* Advance past NULs to the next part of each input string,
             exiting the loop if both strings are exhausted.  When
             exiting the loop, prepare to finish off the tiebreaker
             comparison properly.  */
          if (texta < lima)
            texta += strlen (texta) + 1;
          if (textb < limb)
            textb += strlen (textb) + 1;
          if (! (texta < lima || textb < limb))
            {
              lena = sizea; texta = buf;
              lenb = sizeb; textb = buf + sizea;
              break;
            }

          /* Accumulate the transformed data in the corresponding
             checksums.  */
          md5_process_bytes (buf, sizea, &s[0]);
          md5_process_bytes (buf + sizea, sizeb, &s[1]);

          /* Update the tiebreaker comparison of the transformed data.  */
          if (! xfrm_diff)
            {
              xfrm_diff = memcmp (buf, buf + sizea, MIN (sizea, sizeb));
              if (! xfrm_diff)
                xfrm_diff = (sizea > sizeb) - (sizea < sizeb);
            }
        }
    }

  /* Compute and compare the checksums.  */
  md5_process_bytes (texta, lena, &s[0]); md5_finish_ctx (&s[0], dig[0]);
  md5_process_bytes (textb, lenb, &s[1]); md5_finish_ctx (&s[1], dig[1]);
  int diff = memcmp (dig[0], dig[1], sizeof dig[0]);

  /* Fall back on the tiebreaker if the checksums collide.  */
  if (! diff)
    {
      if (! xfrm_diff)
        {
          xfrm_diff = memcmp (texta, textb, MIN (lena, lenb));
          if (! xfrm_diff)
            xfrm_diff = (lena > lenb) - (lena < lenb);
        }

      diff = xfrm_diff;
    }

  free (allocated);

  return diff;
}
#endif /* KEY_SPEC_RANDOM */


/* Convert a key to the short options used to specify it.  */

void
key_to_opts (struct keyfield const *key, char *opts)
{
  if (key->skipsblanks || key->skipeblanks)
    *opts++ = 'b';/* either disables global -b  */
  if (key->ignore == nondictionary)
    *opts++ = 'd';
  if (key->translate)
    *opts++ = 'f';
  if (key->general_numeric)
    *opts++ = 'g';
#ifdef KEY_SPEC_HUMAN_NUMERIC
  if (key->human_numeric)
    *opts++ = 'h';
#endif
#ifdef KEY_SPEC_NONPRINTING
  if (key->ignore == nonprinting)
    *opts++ = 'i';
#endif
#ifdef KEY_SPEC_MONTH
  if (key->month)
    *opts++ = 'M';
#endif
  if (key->numeric)
    *opts++ = 'n';
#ifdef KEY_SPEC_RANDOM
  if (key->random)
    *opts++ = 'R';
#endif
#ifdef KEY_SPEC_REVERSE
  if (key->reverse)
    *opts++ = 'r';
#endif
#ifdef KEY_SPEC_VERSION
  if (key->version)
    *opts++ = 'V';
#endif
  *opts = '\0';
}

/* Compare two lines A and B trying every key in sequence until there
   are no more keys or a difference is found. */

int
keycompare (struct line const *a, struct line const *b)
{
  struct keyfield *key = keylist;

  /* For the first iteration only, the key positions have been
     precomputed for us. */
  char *texta = a->keybeg;
  char *textb = b->keybeg;
  char *lima = a->keylim;
  char *limb = b->keylim;

  int diff;

  while (true)
    {
      char const *translate = key->translate;
      bool const *ignore = key->ignore;

      /* Treat field ends before field starts as empty fields.  */
      lima = MAX (texta, lima);
      limb = MAX (textb, limb);

      /* Find the lengths. */
      size_t lena = lima - texta;
      size_t lenb = limb - textb;

      if (hard_LC_COLLATE || key_numeric (key)
#ifdef KEY_SPEC_MONTH
          || key->month
#endif
#ifdef KEY_SPEC_RANDOM
          || key->random
#endif
#ifdef KEY_SPEC_VERSION
          || key->version
#endif
          )
        {
          char *ta;
          char *tb;
          size_t tlena;
          size_t tlenb;

          char enda IF_LINT (= 0);
          char endb IF_LINT (= 0);
          void *allocated IF_LINT (= NULL);
          char stackbuf[4000];

          if (ignore || translate)
            {
              /* Compute with copies of the keys, which are the result of
                 translating or ignoring characters, and which need their
                 own storage.  */

              size_t i;

              /* Allocate space for copies.  */
              size_t size = lena + 1 + lenb + 1;
              if (size <= sizeof stackbuf)
                ta = stackbuf, allocated = NULL;
              else
                ta = allocated = xmalloc (size);
              tb = ta + lena + 1;

              /* Put into each copy a version of the key in which the
                 requested characters are ignored or translated.  */
              for (tlena = i = 0; i < lena; i++)
                if (! (ignore && ignore[to_uchar (texta[i])]))
                  ta[tlena++] = (translate
                                 ? translate[to_uchar (texta[i])]
                                 : texta[i]);
              ta[tlena] = '\0';

              for (tlenb = i = 0; i < lenb; i++)
                if (! (ignore && ignore[to_uchar (textb[i])]))
                  tb[tlenb++] = (translate
                                 ? translate[to_uchar (textb[i])]
                                 : textb[i]);
              tb[tlenb] = '\0';
            }
          else
            {
              /* Use the keys in-place, temporarily null-terminated.  */
              ta = texta; tlena = lena; enda = ta[tlena]; ta[tlena] = '\0';
              tb = textb; tlenb = lenb; endb = tb[tlenb]; tb[tlenb] = '\0';
            }

          if (key->numeric)
            diff = numcompare (ta, tb);
          else if (key->general_numeric)
            diff = general_numcompare (ta, tb);
#ifdef KEY_SPEC_HUMAN_NUMERIC
          else if (key->human_numeric)
            diff = human_numcompare (ta, tb);
#endif
#ifdef KEY_SPEC_MONTH
          else if (key->month)
            diff = getmonth (ta, NULL) - getmonth (tb, NULL);
#endif
#ifdef KEY_SPEC_RANDOM
          else if (key->random)
            diff = compare_random (ta, tlena, tb, tlenb);
#endif
#ifdef KEY_SPEC_VERSION
          else if (key->version)
            diff = filevercmp (ta, tb);
#endif
          else
            {
              /* Locale-dependent string sorting.  This is slower than
                 C-locale sorting, which is implemented below.  */
              if (tlena == 0)
                diff = - NONZERO (tlenb);
              else if (tlenb == 0)
                diff = 1;
              else
                diff = xmemcoll0 (ta, tlena + 1, tb, tlenb + 1);
            }

          if (ignore || translate)
            free (allocated);
          else
            {
              ta[tlena] = enda;
              tb[tlenb] = endb;
            }
        }
      else if (ignore)
        {
#define CMP_WITH_IGNORE(A, B)						\
  do									\
    {									\
          while (true)							\
            {								\
              while (texta < lima && ignore[to_uchar (*texta)])		\
                ++texta;						\
              while (textb < limb && ignore[to_uchar (*textb)])		\
                ++textb;						\
              if (! (texta < lima && textb < limb))			\
                break;							\
              diff = to_uchar (A) - to_uchar (B);			\
              if (diff)							\
                goto not_equal;						\
              ++texta;							\
              ++textb;							\
            }								\
                                                                        \
          diff = (texta < lima) - (textb < limb);			\
    }									\
  while (0)

          if (translate)
            CMP_WITH_IGNORE (translate[to_uchar (*texta)],
                             translate[to_uchar (*textb)]);
          else
            CMP_WITH_IGNORE (*texta, *textb);
        }
      else if (lena == 0)
        diff = - NONZERO (lenb);
      else if (lenb == 0)
        goto greater;
      else
        {
          if (translate)
            {
              while (texta < lima && textb < limb)
                {
                  diff = (to_uchar (translate[to_uchar (*texta++)])
                          - to_uchar (translate[to_uchar (*textb++)]));
                  if (diff)
                    goto not_equal;
                }
            }
          else
            {
              diff = memcmp (texta, textb, MIN (lena, lenb));
              if (diff)
                goto not_equal;
            }
          diff = lena < lenb ? -1 : lena != lenb;
        }

      if (diff)
        goto not_equal;

      key = key->next;
      if (! key)
        break;

      /* Find the beginning and limit of the next field.  */
      if (key->eword != SIZE_MAX)
        lima = limfield (a, key), limb = limfield (b, key);
      else
        lima = a->text + a->length - 1, limb = b->text + b->length - 1;

      if (key->sword != SIZE_MAX)
        texta = begfield (a, key), textb = begfield (b, key);
      else
        {
          texta = a->text, textb = b->text;
          if (key->skipsblanks)
            {
              while (texta < lima && blanks[to_uchar (*texta)])
                ++texta;
              while (textb < limb && blanks[to_uchar (*textb)])
                ++textb;
            }
        }
    }

  return 0;

 greater:
  diff = 1;
 not_equal:
  return
#ifdef KEY_SPEC_REVERSE
    key->reverse ? -diff :
#endif
    diff;
}

/* Insert a malloc'd copy of key KEY_ARG at the end of the key list.  */

void
insertkey (struct keyfield *key_arg)
{
  struct keyfield **p;
  struct keyfield *key = xmemdup (key_arg, sizeof *key);

  for (p = &keylist; *p; p = &(*p)->next)
    continue;
  *p = key;
  key->next = NULL;
}

/* Report a bad field specification SPEC, with extra info MSGID.  */

void
badfieldspec (char const *spec, char const *msgid)
{
  error (SORT_FAILURE, 0, _("%s: invalid field specification %s"),
         _(msgid), quote (spec));
  abort ();
}


/* Parse the leading integer in STRING and store the resulting value
   (which must fit into size_t) into *VAL.  Return the address of the
   suffix after the integer.  If the value is too large, silently
   substitute SIZE_MAX.  If MSGID is NULL, return NULL after
   failure; otherwise, report MSGID and exit on failure.  */

char const *
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
      /* Fall through.  */
    case LONGINT_OVERFLOW:
    case LONGINT_OVERFLOW | LONGINT_INVALID_SUFFIX_CHAR:
      *val = SIZE_MAX;
      break;

    case LONGINT_INVALID:
      if (msgid)
        error (SORT_FAILURE, 0, _("%s: invalid count at start of %s"),
               _(msgid), quote (string));
      return NULL;
    }

  return suffix;
}


/* Set the ordering options for KEY specified in S.
   Return the address of the first character in S that
   is not a valid ordering option.
   BLANKTYPE is the kind of blanks that 'b' should skip. */

char *
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
#ifdef KEY_SPEC_HUMAN_NUMERIC
        case 'h':
          key->human_numeric = true;
          break;
#endif
#ifdef KEY_SPEC_NONPRINTING
        case 'i':
          /* Option order should not matter, so don't let -i override
             -d.  -d implies -i, but -i does not imply -d.  */
          if (! key->ignore)
            key->ignore = nonprinting;
          break;
#endif
#ifdef KEY_SPEC_MONTH
        case 'M':
          key->month = true;
          break;
#endif
        case 'n':
          key->numeric = true;
          break;
#ifdef KEY_SPEC_RANDOM
        case 'R':
          key->random = true;
          break;
#endif
#ifdef KEY_SPEC_REVERSE
        case 'r':
          key->reverse = true;
          break;
#endif
#ifdef KEY_SPEC_VERSION
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

struct keyfield *
key_init (struct keyfield *key)
{
  memset (key, 0, sizeof *key);
  key->eword = SIZE_MAX;
  return key;
}

/* Return the printable width of the block of memory starting at
   TEXT and ending just before LIM, counting each tab as one byte.
   FIXME: Should we generally be counting non printable chars?  */

size_t
debug_width (char const *text, char const *lim)
{
  size_t width = mbsnwidth (text, lim - text, 0);
  while (text < lim)
    width += (*text++ == '\t');
  return width;
}

/* For debug mode, "underline" a key at the
   specified offset and screen width.  */

void
mark_key (size_t offset, size_t width)
{
  while (offset--)
    putchar (' ');

  if (!width)
    printf (_("^ no match for key\n"));
  else
    {
      do
        putchar ('_');
      while (--width);

      putchar ('\n');
    }
}

/* For LINE, output a debugging line that underlines KEY in LINE.
   If KEY is null, underline the whole line.  */

void
debug_key (struct line const *line, struct keyfield const *key)
{
  char *text = line->text;
  char *beg = text;
  char *lim = text + line->length - 1;

  if (key)
    {
      if (key->sword != SIZE_MAX)
        beg = begfield (line, key);
      if (key->eword != SIZE_MAX)
        lim = limfield (line, key);

      if (key->skipsblanks
#ifdef KEY_SPEC_MONTH
          || key->month
#endif
          || key_numeric (key))
        {
          char saved = *lim;
          *lim = '\0';

          while (blanks[to_uchar (*beg)])
            beg++;

          char *tighter_lim = beg;

          if (lim < beg)
            tighter_lim = lim;
#ifdef KEY_SPEC_MONTH
          else if (key->month)
            getmonth (beg, &tighter_lim);
#endif
          else if (key->general_numeric)
            ignore_value (strtold (beg, &tighter_lim));
          else if (key->numeric
#ifdef KEY_SPEC_HUMAN_NUMERIC
                   || key->human_numeric
#endif
                   )
            {
              char *p = beg + (beg < lim && *beg == '-');
              bool found_digit = false;
              unsigned char ch;

              do
                {
                  while (ISDIGIT (ch = *p++))
                    found_digit = true;
                }
              while (ch == thousands_sep);

              if (ch == decimal_point)
                while (ISDIGIT (ch = *p++))
                  found_digit = true;

              if (found_digit)
                tighter_lim = p
#ifdef KEY_SPEC_HUMAN_NUMERIC
                                - ! (key->human_numeric && unit_order[ch])
#endif
                               ;
                                                 }
          else
            tighter_lim = lim;

          *lim = saved;
          lim = tighter_lim;
        }
    }

  size_t offset = debug_width (text, beg);
  size_t width = debug_width (beg, lim);
  mark_key (offset, width);
}

/* Debug LINE by underlining its keys.  */

/* XXX: unique&stable are globals in 'src/sort.c', pass them as parameters */
void
debug_line (struct line const *line, const bool unique, const bool stable)
{
  struct keyfield const *key = keylist;

  do
    debug_key (line, key);
  while (key && ((key = key->next) || ! (unique || stable)));
}

/* Return whether sorting options specified for key.  */

bool _GL_ATTRIBUTE_PURE
default_key_compare (struct keyfield const *key)
{
  return ! (key->ignore
            || key->translate
            || key->skipsblanks
            || key->skipeblanks
            || key_numeric (key)
#ifdef KEY_SPEC_MONTH
            || key->month
#endif
#ifdef KEY_SPEC_VERSION
            || key->version
#endif
#ifdef KEY_SPEC_RANDOM
            || key->random
#endif
            /* || key->reverse */
           );
}

/* Output data independent key warnings to stderr.  */

void
key_warnings (struct keyfield const *gkey, bool gkey_only,
              const bool unique, const bool stable)
{
  struct keyfield const *key;
  struct keyfield ugkey = *gkey;
  unsigned long keynum = 1;

  for (key = keylist; key; key = key->next, keynum++)
    {
      if (key->obsolete_used)
        {
          size_t sword = key->sword;
          size_t eword = key->eword;
          char tmp[INT_BUFSIZE_BOUND (uintmax_t)];
          /* obsolescent syntax +A.x -B.y is equivalent to:
               -k A+1.x+1,B.y   (when y = 0)
               -k A+1.x+1,B+1.y (when y > 0)  */
          char obuf[INT_BUFSIZE_BOUND (sword) * 2 + 4]; /* +# -#  */
          char nbuf[INT_BUFSIZE_BOUND (sword) * 2 + 5]; /* -k #,#  */
          char *po = obuf;
          char *pn = nbuf;

          if (sword == SIZE_MAX)
            sword++;

          po = stpcpy (stpcpy (po, "+"), umaxtostr (sword, tmp));
          pn = stpcpy (stpcpy (pn, "-k "), umaxtostr (sword + 1, tmp));
          if (key->eword != SIZE_MAX)
            {
              stpcpy (stpcpy (po, " -"), umaxtostr (eword + 1, tmp));
              stpcpy (stpcpy (pn, ","),
                      umaxtostr (eword + 1
                                 + (key->echar == SIZE_MAX), tmp));
            }
          error (0, 0, _("obsolescent key %s used; consider %s instead"),
                 quote_n (0, obuf), quote_n (1, nbuf));
        }

      /* Warn about field specs that will never match.  */
      if (key->sword != SIZE_MAX && key->eword < key->sword)
        error (0, 0, _("key %lu has zero width and will be ignored"), keynum);

      /* Warn about significant leading blanks.  */
      bool implicit_skip = key_numeric (key)
#ifdef KEY_SPEC_MONTH
        || key->month
#endif
        ;
      bool maybe_space_aligned = !hard_LC_COLLATE && default_key_compare (key)
                                 && !(key->schar || key->echar);
      bool line_offset = key->eword == 0 && key->echar != 0; /* -k1.x,1.y  */
      if (!gkey_only && tab == TAB_DEFAULT && !line_offset
          && ((!key->skipsblanks && !(implicit_skip || maybe_space_aligned))
              || (!key->skipsblanks && key->schar)
              || (!key->skipeblanks && key->echar)))
        error (0, 0, _("leading blanks are significant in key %lu; "
                       "consider also specifying 'b'"), keynum);

      /* Warn about numeric comparisons spanning fields,
         as field delimiters could be interpreted as part
         of the number (maybe only in other locales).  */
      if (!gkey_only && key_numeric (key))
        {
          size_t sword = key->sword + 1;
          size_t eword = key->eword + 1;
          if (!sword)
            sword++;
          if (!eword || sword < eword)
            error (0, 0, _("key %lu is numeric and spans multiple fields"),
                   keynum);
        }

      /* Flag global options not copied or specified in any key.  */
      if (ugkey.ignore && (ugkey.ignore == key->ignore))
        ugkey.ignore = NULL;
      if (ugkey.translate && (ugkey.translate == key->translate))
        ugkey.translate = NULL;
      ugkey.skipsblanks &= !key->skipsblanks;
      ugkey.skipeblanks &= !key->skipeblanks;
#ifdef KEY_SPEC_MONTH
      ugkey.month &= !key->month;
#endif
      ugkey.numeric &= !key->numeric;
      ugkey.general_numeric &= !key->general_numeric;
#ifdef KEY_SPEC_HUMAN_NUMERIC
      ugkey.human_numeric &= !key->human_numeric;
#endif
#ifdef KEY_SPEC_RANDOM
      ugkey.random &= !key->random;
#endif
#ifdef KEY_SPEC_VERSION
      ugkey.version &= !key->version;
#endif
#ifdef KEY_SPEC_REVERSE
      ugkey.reverse &= !key->reverse;
#endif
    }

  /* TODO: verify this logic with all he #ifdef's */

  /* Warn about ignored global options flagged above.
     Note if gkey is the only one in the list, all flags are cleared.  */
  if (!default_key_compare (&ugkey)
#ifdef KEY_SPEC_REVERSE
      || (
          ugkey.reverse && (stable || unique) && keylist)
#endif
      )
    {
#ifdef KEY_SPEC_REVERSE
      bool ugkey_reverse = ugkey.reverse;
      if (!(stable || unique))
        ugkey.reverse = false;
#endif
      /* The following is too big, but guaranteed to be "big enough".  */
      char opts[32]; /*XXX was: opt[sizeof short_options] */
      key_to_opts (&ugkey, opts);
      error (0, 0,
             ngettext ("option '-%s' is ignored",
                       "options '-%s' are ignored",
                       select_plural (strlen (opts))), opts);
#ifdef KEY_SPEC_REVERSE
      ugkey.reverse = ugkey_reverse;
#endif
    }
#ifdef KEY_SPEC_REVERSE
  if (ugkey.reverse && !(stable || unique) && keylist)
    error (0, 0, _("option '-r' only applies to last-resort comparison"));
#endif
}



void
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

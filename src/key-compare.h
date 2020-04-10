/* Key Comparison functions

   Copyright (C) 2014 Free Software Foundation, Inc.

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

#ifndef KEY_COMPARE_H
# define KEY_COMPARE_H

#define UCHAR_LIM (UCHAR_MAX + 1)


/* The representation of the decimal point in the current locale.  */
extern int decimal_point;

/* Thousands separator; if -1, then there isn't one.  */
extern int thousands_sep;

/* Nonzero if the corresponding locales are hard.  */
extern bool hard_LC_COLLATE;
#if HAVE_NL_LANGINFO
extern bool hard_LC_TIME;
#endif

#define NONZERO(x) ((x) != 0)

/* The kind of blanks for '-b' to skip in various options. */
enum blanktype { bl_start, bl_end, bl_both };

/* Lines are held in core as counted strings. */
struct line
{
  char *text;			/* Text of the line. */
  size_t length;		/* Length including final newline. */
  char *keybeg;			/* Start of first key. */
  char *keylim;			/* Limit of first key. */
};

/* Sort key.  */
struct keyfield
{
  size_t sword;			/* Zero-origin 'word' to start at. */
  size_t schar;			/* Additional characters to skip. */
  size_t eword;			/* Zero-origin last 'word' of key. */
  size_t echar;			/* Additional characters in field. */
  bool const *ignore;		/* Boolean array of characters to ignore. */
  char const *translate;	/* Translation applied to characters. */
  bool skipsblanks;		/* Skip leading blanks when finding start.  */
  bool skipeblanks;		/* Skip leading blanks when finding end.  */
  bool numeric;			/* Flag for numeric comparison.  Handle
                                   strings of digits with optional decimal
                                   point, but no exponential notation. */
#ifdef KEY_COMPARE_RANDOM
  bool random;			/* Sort by random hash of key.  */
#endif
  bool general_numeric;		/* Flag for general, numeric comparison.
                                   Handle numbers in exponential notation. */
#ifdef KEY_COMPARE_HUMAN_NUMERIC
  bool human_numeric;		/* Flag for sorting by human readable
                                   units with either SI xor IEC prefixes. */
#endif
#ifdef KEY_COMPARE_MONTH
  bool month;			/* Flag for comparison by month name. */
#endif
#ifdef KEY_COMPARE_REVERSE
  bool reverse;			/* Reverse the sense of comparison. */
#endif
#ifdef KEY_COMPARE_VERSION
  bool version;			/* sort by version number */
#endif
#ifdef KEY_COMPARE_DECORATION
  bool (*decorate_fn)(const char* in);
  const char* decorate_cmd;
#endif
  bool traditional_used;	/* Traditional key option format is used. */
  struct keyfield *next;	/* Next keyfield to try. */
};

/* If TAB has this value, blanks separate fields.  */
enum { TAB_DEFAULT = CHAR_MAX + 1 };

/* Tab character separating fields.  If TAB_DEFAULT, then fields are
   separated by the empty string between a non-blank character and a blank
   character. */
extern int tab;

/* List of key field comparisons to be tried.  */
extern struct keyfield *keylist;

/* Return a pointer to the first character of the field specified
   by KEY in LINE. */

char *
begfield (struct line const *line, struct keyfield const *key);

/* Return the limit of (a pointer to the first character after) the field
   in LINE specified by KEY. */

char *
limfield (struct line const *line, struct keyfield const *key);

/* Insert a malloc'd copy of key KEY_ARG at the end of the key list.  */

extern struct keyfield*
insertkey (struct keyfield *key_arg);

/* Report a bad field specification SPEC, with extra info MSGID.  */
void badfieldspec (char const *, char const *)
     ATTRIBUTE_NORETURN;

/* Parse the leading integer in STRING and store the resulting value
   (which must fit into size_t) into *VAL.  Return the address of the
   suffix after the integer.  If the value is too large, silently
   substitute SIZE_MAX.  If MSGID is NULL, return NULL after
   failure; otherwise, report MSGID and exit on failure.  */

char const *
parse_field_count (char const *string, size_t *val, char const *msgid);

/* Set the ordering options for KEY specified in S.
   Return the address of the first character in S that
   is not a valid ordering option.
   BLANKTYPE is the kind of blanks that 'b' should skip. */

char *
set_ordering (char const *s, struct keyfield *key, enum blanktype blanktype);

/* Initialize KEY.  */
struct keyfield *
key_init (struct keyfield *key);

/* print the key spec as a parameter */
void
debug_keylist (FILE* stream);

char*
debug_keyfield (const struct keyfield *key);



/* Initializes 'common' key-comparison global variables:
    thousand_sep
    decimal_point
    hard_LC_COLLATE
    hard_LC_TIME
    blanks, months, nonprintable tables (calls inittables).

    This function should be called once from main .
 */
void
init_key_spec (void);

#endif /* KEY_COMPARE_H */

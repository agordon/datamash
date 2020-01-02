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
#ifndef __TEXT_OPTIONS_H__
#define __TEXT_OPTIONS_H__

/*
 Text Processing options, used by several modules.
 */

/* The character marking end of line. Default to \n. */
extern char eolchar;

/* If TAB has this value, blanks separate fields.  */
enum { TAB_WHITESPACE = CHAR_MAX + 1 };

/* Tab character separating fields.  If TAB_WHITESPACE, then fields are
   separated by the empty string between a non-blank character and a blank
   character. */
extern int in_tab ;
/* The output field separator character, defaults to a TAB (ASCII 9) */
extern int out_tab ;

/* Global case-sensitivity option. Defaults to 'true' . */
extern bool case_sensitive ;

/* Numeric output format (default: "%.14Lg" */
extern char* numeric_output_format;
/* number of bytes to allocate for output buffer */
extern int   numeric_output_bufsize;

/* The character used to separate collapsed/uniqued strings */
extern char collapse_separator;

/* Should NA/NaN/empty values be silengtly ignored? */
extern bool remove_na_values;

/* if true, 'transpose' and 'reverse' require every line to have
   the exact same number of fields. Otherwise, the program
   will fail with non-zero exit code. */
extern bool strict;

/* if 'strict' is false, lines with fewer-than-expected fields
   will be filled with this value */
extern char* missing_field_filler;

/* if true, skip comments line (lines starting with optional whitespace
   followed by '#' or ';'. See line_record_is_comment.  */
extern bool skip_comments;

#define UCHAR_LIM (UCHAR_MAX + 1)
extern bool blanks[UCHAR_LIM];

/* Initializes the 'blanks' table. */
void
init_blank_table (void);

static inline void
print_field_separator ()
{
  putchar (out_tab);
}

static inline void
print_line_separator ()
{
  putchar (eolchar);
}


void
set_numeric_output_precision (const char* digits);

void
set_numeric_printf_format (const char* format);

#endif

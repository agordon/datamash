/* compute - perform simple calculation on input data

   Copyright (C) 2013,2014 Assaf Gordon <assafgordon@gmail.com>

   This file is part of Compute.

   Compute is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Compute is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Compute.  If not, see <http://www.gnu.org/licenses/>.
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

#define UCHAR_LIM (UCHAR_MAX + 1)
extern bool blanks[UCHAR_LIM];

/* Initializes the 'blanks' table. */
void init_blank_table (void);

static inline void print_field_separator()
{
  putchar( out_tab  );
}

static inline void print_line_separator()
{
  putchar( eolchar );
}

#endif

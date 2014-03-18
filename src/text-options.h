/* compute - perform simple calculation on input data
   Copyright (C) 2014 Assaf Gordon.

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

/* Written by Assaf Gordon */
#ifndef __TEXT_OPTIONS_H__
#define __TEXT_OPTIONS_H__

/*
 Text Processing options, used by several modules.
 */

/* The character marking end of line. Default to \n. */
extern char eolchar;

/* If TAB has this value, blanks separate fields.  */
enum { TAB_DEFAULT = CHAR_MAX + 1 };

/* Tab character separating fields.  If TAB_DEFAULT, then fields are
   separated by the empty string between a non-blank character and a blank
   character. */
extern int tab ;

/* Global case-sensitivity option. Defaults to 'true' . */
extern bool case_sensitive ;

#define UCHAR_LIM (UCHAR_MAX + 1)
extern bool blanks[UCHAR_LIM];

/* Initializes the 'blanks' table. */
void init_blank_table (void);

inline void print_field_separator()
{
  putchar( (tab==TAB_DEFAULT)?' ':tab );
}

inline void print_line_separator()
{
  putchar( eolchar );
}

#endif

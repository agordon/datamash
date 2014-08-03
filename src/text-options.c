/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2014 Assaf Gordon <assafgordon@gmail.com>

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
   along with GNU Datamash.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Written by Assaf Gordon */
#include <config.h>

#include <ctype.h>
#include <stdbool.h>

#include "system.h"

#include "text-options.h"

/* The character marking end of line. Default to \n. */
char eolchar = '\n';

/* Tab character separating fields.  If TAB_WHITESPACE, then fields are
   separated by the empty string between a non-blank character and a blank
   character. */
int in_tab = '\t';
int out_tab= '\t';

/* Global case-sensitivity option. Defaults to 'true' . */
bool case_sensitive = true;

#define UCHAR_LIM (UCHAR_MAX + 1)
bool blanks[UCHAR_LIM];

void
init_blank_table (void)
{
  size_t i;

  for (i = 0; i < UCHAR_LIM; ++i)
    {
      blanks[i] = !! isblank (i);
    }
}

/* Force generation of these inline'd symbols, needed to avoid
   "undefined reference" when compiling with coverage instrumentation.
   See: http://stackoverflow.com/a/16245669 */
void print_field_separator();
void print_line_separator();

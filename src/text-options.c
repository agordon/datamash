/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2018 Assaf Gordon <assafgordon@gmail.com>

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

/* In the future: allow users to change this */
int numeric_output_precision = 14;

/* The character used to separate collapsed/uniqued strings */
/* In the future: allow users to change this */
char collapse_separator = ',';

/* Should NA/NaN/empty values be silengtly ignored? */
bool remove_na_values = false;

/* if true, 'transpose' and 'reverse' require every line to have
   the exact same number of fields. Otherwise, the program
   will fail with non-zero exit code. */
bool strict = true;

/* if 'strict' is false, lines with fewer-than-expected fields
   will be filled with this value */
char* missing_field_filler = "N/A";

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
void print_field_separator ();
void print_line_separator ();

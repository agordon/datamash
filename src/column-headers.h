/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2014-2020 Assaf Gordon <assafgordon@gmail.com>

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
#ifndef __COLUMN_HEADERS_H__
#define __COLUMN_HEADERS_H__

/*
 Column Headers Module
*/

/*
  Given a parsed line (representing the header line),
  sets the column names.

  if 'store_names' is true,
      stores the name of each field as the column header.
  if 'store_names' is false,
      simply counts the number of fields in the input line.
 */
void
build_input_line_headers (const struct line_record_t *lr, bool store_names);

/*
 returns the number of fields as extracted by 'build_input_line_headers ()'
 */
size_t
get_num_column_headers ();

/*
 returns the name of column 'field_num' (1 == first field).

 If 'store_names' (above) was true, returns the name of the column as
    appeared in the first input line.
 If 'store_names' (above) was false, returns 'field-X'.

 The returned string must not be modified (or free'd).
*/
const char*
get_input_field_name (size_t field_num);


/* returns field number (1== first field)
   which matches the given field name.

   returns ZERO if no such field found. */
size_t
get_input_field_number (const char* field_name);

void
free_column_headers ();

#endif

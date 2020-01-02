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
#ifndef __CROSSTAB_H__
#define __CROSSTAB_H__

struct crosstab
{
  Hash_table *rows;
  Hash_table *columns;
  Hash_table *data;
};

struct crosstab_datacell
{
  const char* row_name;
  const char* col_name;
  const char* data;
};

struct crosstab_data_cell*
crosstab_new_datacell (const char* row, const char* col, const char* data);

struct crosstab*
crosstab_init ();

void
crosstab_add_result (struct crosstab* ct,
                      const char* row, const char* col, const char* data);

void
crosstab_print ();

void
crosstab_free (struct crosstab* ct);

#endif /* __CROSSTAB_H__ */

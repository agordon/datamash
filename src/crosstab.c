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
#include <config.h>

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include "ignore-value.h"
#include "hash.h"
#include "hash-pjw.h"
#include "xalloc.h"

#include "system.h"
#include "crosstab.h"
#include "utils.h"
#include "text-options.h"


static bool _GL_ATTRIBUTE_PURE
str_comparator (const void* a, const void* b)
{
  assert (a!=NULL && b!=NULL);                   /* LCOV_EXCL_LINE */
  if (a==b)
    return true;
  return (STREQ ((const char*)a, (const char*)b));
}

static size_t _GL_ATTRIBUTE_PURE
hash_crosstab_data_cell (void const *x, size_t tablesize)
{
  struct crosstab_datacell *dc = (struct crosstab_datacell*)x;

  const char *s;
  size_t h = 0;
#define SIZE_BITS (sizeof (size_t) * CHAR_BIT)

  for (s = dc->row_name; *s; s++)
    h = *s + ((h << 9) | (h >> (SIZE_BITS - 9)));
  for (s = dc->col_name; *s; s++)
    h = *s + ((h << 9) | (h >> (SIZE_BITS - 9)));

  return h % tablesize;
}

static bool _GL_ATTRIBUTE_PURE
crosstab_datacell_comparator (const void* a, const void* b)
{
  assert (a!=NULL && b!=NULL);                   /* LCOV_EXCL_LINE */
  if (a==b)
    return true;
  const struct crosstab_datacell *da = (struct crosstab_datacell*)a;
  const struct crosstab_datacell *db = (struct crosstab_datacell*)b;
  return (STREQ (da->row_name, db->row_name)
          && STREQ (da->col_name, db->col_name));
}


struct crosstab_datacell*
new_datacell (const char* row, const char* col, const char* data)
{
  struct crosstab_datacell *dc = xmalloc (sizeof (struct crosstab_datacell));
  dc->row_name = row;
  dc->col_name = col;
  dc->data = data;
  return dc;
}

static void
crosstab_datacell_free (void *a)
{
  struct crosstab_datacell *dc = (struct crosstab_datacell*)a;
  dc->row_name = NULL;
  dc->col_name = NULL;
  dc->data = NULL;
  free (dc);
}

/* Setup needed variables for the cross-tabulation */
struct crosstab*
crosstab_init ()
{
  struct crosstab *ct = XMALLOC (struct crosstab);

  ct->rows    = hash_initialize (1000,NULL,hash_pjw,str_comparator,free);
  ct->columns = hash_initialize (1000,NULL,hash_pjw,str_comparator,free);
  ct->data    = hash_initialize (1000,NULL,hash_crosstab_data_cell,
                                crosstab_datacell_comparator,
                                crosstab_datacell_free);
  return ct;
}

void
crosstab_free (struct crosstab* ct)
{
  assert (ct!=NULL);                             /* LCOV_EXCL_LINE */
  hash_free (ct->rows);
  ct->rows = NULL;
  hash_free (ct->columns);
  ct->columns = NULL;
  hash_free (ct->data);
  ct->data = NULL;
  free (ct);
}

/* Add new cross-tabulation result */
void
crosstab_add_result (struct crosstab* ct,
                      const char* row, const char* col, const char* data)
{
  const char* r = hash_lookup (ct->rows, row);
  if (r==NULL)
    r = hash_insert (ct->rows, xstrdup (row));

  const char* c = hash_lookup (ct->columns, col);
  if (c==NULL)
    c = hash_insert (ct->columns, xstrdup (col));

  struct crosstab_datacell *ctdc = new_datacell (r,c,xstrdup (data));
  ignore_value (hash_insert (ct->data, ctdc));
}


/* Print table */
void
crosstab_print (const struct crosstab* ct)
{
  const size_t n_rows = hash_get_n_entries (ct->rows);
  char** rows_list = XNMALLOC (n_rows,char*);
  hash_get_entries (ct->rows, (void**)rows_list, n_rows);
  qsort (rows_list, n_rows, sizeof (char*), cmpstringp);

  const size_t n_cols = hash_get_n_entries (ct->columns);
  char** cols_list = XNMALLOC (n_cols,char*);
  hash_get_entries (ct->columns, (void**)cols_list, n_cols);
  qsort (cols_list, n_cols, sizeof (char*), cmpstringp);

  /* Print columns */
  for (size_t c = 0; c < n_cols; ++c)
    {
      print_field_separator ();
      fputs (cols_list[c], stdout);
    }
  print_line_separator ();

  /* Print rows */
  for (size_t r = 0; r < n_rows; ++r)
    {
      fputs (rows_list[r], stdout);

      for (size_t c = 0; c < n_cols; ++c)
        {
          struct crosstab_datacell curr;
          curr.row_name = rows_list[r];
          curr.col_name = cols_list[c];

          const struct crosstab_datacell *dc = hash_lookup (ct->data, &curr);
          print_field_separator ();
          fputs ((dc==NULL)?missing_field_filler:dc->data, stdout);
        }

      print_line_separator ();
    }

  IF_LINT (free (rows_list));
  IF_LINT (free (cols_list));
}
/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=8: */
/* vim: set expandtab: */

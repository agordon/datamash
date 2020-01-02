/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2014-2020 Assaf Gordon.

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
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <strings.h>
#include <inttypes.h>

#include "system.h"
#include "xalloc.h"
#include "linebuffer.h"
#include "ignore-value.h"
#include "intprops.h"

#include "text-options.h"
#include "text-lines.h"
#include "column-headers.h"

static size_t num_input_column_headers = 0 ;
static char** input_column_headers;

void free_column_headers ()
{
  for (size_t i = 0; i < num_input_column_headers; ++i)
    {
      free (input_column_headers[i]);
      input_column_headers[i] = NULL;
    }
  free (input_column_headers);
  input_column_headers = NULL;
}

size_t _GL_ATTRIBUTE_PURE
get_num_column_headers ()
{
  return num_input_column_headers;
}

const char* _GL_ATTRIBUTE_PURE
get_input_field_name (size_t field_num)
{
  assert (field_num > 0                              /* LCOV_EXCL_LINE */
          && field_num <= num_input_column_headers); /* LCOV_EXCL_LINE */
  return input_column_headers[field_num-1];
}

size_t _GL_ATTRIBUTE_PURE
get_input_field_number (const char* field_name)
{
  assert (field_name != NULL); /* LCOV_EXCL_LINE */
  assert (*field_name != 0);   /* LCOV_EXCL_LINE */
  for (size_t i = 0 ; i < num_input_column_headers ; ++i)
    {
      if (STREQ (field_name,input_column_headers[i]))
        return i+1;
    }
  return 0;
}

void
build_input_line_headers (const struct line_record_t *lr, bool store_names)
{
  char *str;
  size_t len = 0;
  const size_t num_fields = line_record_num_fields (lr);
  const size_t field_name_buf_size = 7+INT_BUFSIZE_BOUND (size_t)+1;

  num_input_column_headers = num_fields;
  input_column_headers = XNMALLOC (num_fields, char*);

  for (size_t i = 1; i <= num_fields; ++i)
    {
      if (!store_names)
        {
          str = xmalloc ( field_name_buf_size );
          ignore_value (snprintf (str, field_name_buf_size,
                                  "field-%"PRIuMAX,(uintmax_t)i));
        }
      else
        {
          const char* tmp = NULL;
          line_record_get_field (lr, i, &tmp, &len);
          str = xmalloc ( len+1 );
          memcpy (str, tmp, len);
          str[len] = 0;
        }

      input_column_headers[i-1] = str;
    }
}

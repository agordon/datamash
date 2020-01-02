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

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>

#include "system.h"
#ifndef MAX
#include "minmax.h"
#endif
#include "linebuffer.h"
#include "xalloc.h"

#include "text-options.h"
#include "text-lines.h"

void
line_record_init (struct line_record_t* lr)
{
  initbuffer (&lr->lbuf);
  lr->alloc_fields = 10 ;
  lr->num_fields = 0;
  lr->fields = XNMALLOC (lr->alloc_fields, struct field_record_t);
}

#if 0
static void
line_record_debug_print_fields (const struct line_record_t *lr)
{
  fputs ("line_record_t = {\n",stderr);
  fprintf (stderr, "  linebuffer.length = %zu\n", lr->lbuf.length);
  fputs ("  linebuffer.buffer = '",stderr);
    fwrite (lr->lbuf.buffer,lr->lbuf.length,sizeof (char),stderr);
  fputs ("'\n",stderr);

  fprintf (stderr, "  num_fields = %zu\n", lr->num_fields);
  fprintf (stderr, "  alloc_fields = %zu\n", lr->alloc_fields);

  for (size_t i=0; i<lr->num_fields; ++i)
    {
      fprintf (stderr, "  field[%zu] = '", i);
      fwrite (lr->fields[i].buf,lr->fields[i].len,sizeof (char),stderr);
      fprintf (stderr, "' (len = %zu)\n", lr->fields[i].len);
    }
  fputs ("}\n", stderr);
}
#endif

/* Force NUL-termination of the string in the linebuffer struct.
   gnulib's readlinebuffer_delim () ALWAYS adds a delimiter to the buffer.
   Change the delimiter into a NUL.
*/
static void
linebuffer_nullify (struct linebuffer *line)
{
  assert (line->length > 0); /* LCOV_EXCL_LINE */
  line->buffer[line->length-1] = 0; /* make it NUL terminated */
  --line->length;
}

static inline void
line_record_reserve_fields (struct line_record_t* lr, const size_t n)
{
  if (lr->alloc_fields <= n)
    {
      lr->alloc_fields = MAX (n,lr->alloc_fields)*2;
      lr->fields = xnrealloc (lr->fields, lr->alloc_fields,
                              sizeof (struct field_record_t));
    }
}

static void
line_record_parse_fields (struct line_record_t *lr, int field_delim)
{
  size_t num_fields = 0;
  size_t pos = 0;
  const size_t buflen = line_record_length (lr);
  const char* fptr = line_record_buffer (lr);

  /* Move 'fptr' to point to the beginning of 'field' */
  if (field_delim != TAB_WHITESPACE)
    {
      while (buflen && pos<=buflen)
        {
          /* scan buffer until next delimiter */
          const char* field_beg = fptr;
          while ( (pos<buflen) && (*fptr != field_delim))
            {
              ++fptr;
              ++pos;
            }

          /* Add new field */
          line_record_reserve_fields (lr, num_fields);
          lr->fields[num_fields].buf = field_beg;
          lr->fields[num_fields].len = fptr - field_beg;
          ++num_fields;

          /* Skip the delimiter */
          ++pos;
          ++fptr;
        }
      lr->num_fields = num_fields;
    }
  else
    {
      /* delimiter is white-space transition
         (multiple whitespaces are one delimiter) */
      while (pos<buflen)
        {
          /* Skip leading whitespace */
          while ( (pos<buflen) && blanks[to_uchar (*fptr)])
            {
              ++fptr;
              ++pos;
            }

          /* Scan buffer until next whitespace */
          const char* field_beg = fptr;
          size_t flen = 0;
          while ( (pos<buflen) && !blanks[to_uchar (*fptr)])
            {
              ++fptr;
              ++pos;
              ++flen;
            }

          /* Add new field */
          line_record_reserve_fields (lr, num_fields);
          lr->fields[num_fields].buf = field_beg;
          lr->fields[num_fields].len = flen;
          ++num_fields;
        }
      lr->num_fields = num_fields;
    }
}


static bool
line_record_is_comment (const struct line_record_t* lr)
{
  const char* pch = line_record_buffer (lr);

  /* Skip white space at beginning of line */
  size_t s = strspn (pch, " \t");
  /* First non-whitespace character */
  char c = pch[s];
  return (c=='#' || c==';');
}

bool
line_record_fread (struct /* in/out */ line_record_t* lr,
                  FILE *stream, char delimiter, bool skip_comments)
{
  do {
    if (readlinebuffer_delim (&lr->lbuf, stream, delimiter) == 0)
      return false;
    linebuffer_nullify (&lr->lbuf);
  } while (skip_comments && line_record_is_comment (lr));


  line_record_parse_fields (lr, in_tab);
  return true;
}

void
line_record_free (struct line_record_t* lr)
{
  freebuffer (&lr->lbuf);
  lr->lbuf.buffer = NULL;
  free (lr->fields);
  lr->fields = NULL;
  lr->alloc_fields = 0;
  lr->num_fields = 0;
}

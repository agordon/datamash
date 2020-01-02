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
#ifndef __TEXT_LINES_H__
#define __TEXT_LINES_H__

struct field_record_t
{
  size_t len;
  const char*  buf;
};

struct line_record_t
{
  /* buffer of the entire line, as created with gnulib's
     readlinbuffer_delim */
  struct linebuffer lbuf;

  /* array of fields. Each valid field is a pointer to 'lbuf' */
  struct field_record_t *fields;
  size_t num_fields;    /* number of fields in this line */
  size_t alloc_fields;  /* number of fields allocated */
};

static inline size_t
line_record_length (const struct line_record_t *lr)
{
  return lr->lbuf.length;
}

static inline const char*
line_record_buffer (const struct line_record_t *lr)
{
  return lr->lbuf.buffer;
}

static inline size_t
line_record_num_fields (const struct line_record_t *lr)
{
  return lr->num_fields;
}

static inline const struct field_record_t*
line_record_field_unsafe (const struct line_record_t *lr, const size_t n)
{
  return &lr->fields[n-1];
}

static inline bool
line_record_get_field (const struct line_record_t *lr, const size_t n,
                       const char ** /* out */ pptr, size_t* /*out*/ plen)
{
  assert (n!=0); /* LCOV_EXCL_LINE */
  if (line_record_num_fields (lr) < n)
    return false;

  *pptr = lr->fields[n-1].buf;
  *plen = lr->fields[n-1].len;
  return true;
}

void
line_record_init (struct line_record_t* lr);

bool
line_record_fread (struct /* in/out */ line_record_t* lr,
                   FILE *stream, char delimiter, bool skip_comments);

void
line_record_free (struct line_record_t* lr);

#endif

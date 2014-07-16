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
#include "linebuffer.h"

#include "text-options.h"
#include "text-lines.h"


/* Force NUL-termination of the string in the linebuffer struct.
   gnulib's readlinebuffer_delim() ALWAYS adds a delimiter to the buffer.
   Change the delimiter into a NUL.
*/
void
linebuffer_nullify (struct linebuffer *line)
{
  if (line->length==0)
    return; /* LCOV_EXCL_LINE */
  line->buffer[line->length-1] = 0; /* make it NUL terminated */
}

void
get_field (const struct linebuffer *line, size_t field,
               const char** /* OUT*/ _ptr, size_t /*OUT*/ *_len)
{
  size_t pos = 0;
  size_t flen = 0;
  const size_t buflen = line->length;
  char* fptr = line->buffer;
  /* Move 'fptr' to point to the beginning of 'field' */
  if (in_tab != TAB_WHITESPACE)
    {
      /* delimiter is explicit character */
      while ((pos<buflen) && --field)
        {
          while ( (pos<buflen) && (*fptr != in_tab))
            {
              ++fptr;
              ++pos;
            }
          if ( (pos<buflen) && (*fptr == in_tab))
            {
              ++fptr;
              ++pos;
            }
        }
    }
  else
    {
      /* delimiter is white-space transition
         (multiple whitespaces are one delimiter) */
      while ((pos<buflen) && --field)
        {
          while ( (pos<buflen) && !blanks[to_uchar(*fptr)])
            {
              ++fptr;
              ++pos;
            }
          while ( (pos<buflen) && blanks[to_uchar(*fptr)])
            {
              ++fptr;
              ++pos;
            }
        }
    }

  /* Find the length of the field (until the next delimiter/eol) */
  if (in_tab != TAB_WHITESPACE)
    {
      while ( (pos+flen<buflen) && (*(fptr+flen) != in_tab) )
        flen++;
    }
  else
    {
      while ( (pos+flen<buflen) && !blanks[to_uchar(*(fptr+flen))] )
        flen++;
    }

  /* Chomp field if needed */
  if ( (flen>1) && ((*(fptr + flen -1) == 0) || (*(fptr+flen-1)==eolchar)) )
    flen--;

  *_len = flen;
  *_ptr = fptr;
}

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
#include <config.h>
#include <stddef.h>
#include <stdbool.h>

#include "system.h"
#include "xalloc.h"
#include "error.h"
#include "ignore-value.h"
#include "intprops.h"

#include "text-options.h"
#include "column-headers.h"

struct columnheader
{
        char *name;
        struct columnheader *next;
};

static size_t num_input_column_headers = 0 ;
static struct columnheader* input_column_headers = NULL;

void free_column_headers()
{
  struct columnheader *p = input_column_headers;
  while (p)
    {
      struct columnheader *n = p->next;
      free(p->name);
      free(p);
      p = n;
    }
  input_column_headers = NULL ;
  num_input_column_headers = 0;
}

size_t get_num_column_headers()
{
  return num_input_column_headers;
}

static void add_named_input_column_header(const char* buffer, size_t len)
{
  struct columnheader *h = XZALLOC(struct columnheader);
  h->name = xmalloc(len+1);
  bcopy(buffer,h->name,len);
  h->name[len]=0;

  if (input_column_headers != NULL)
    {
      struct columnheader *p = input_column_headers;
      while (p->next != NULL)
        p = p->next;
      p->next = h;
    }
  else
    input_column_headers = h;
}

/* Returns a pointer to the name of the field.
 * DO NOT FREE or reuse the returned buffer.
 * Might return a pointer to a static buffer - not thread safe.*/
const char* get_input_field_name(size_t field_num)
{
  static char tmp[6+INT_BUFSIZE_BOUND(size_t)+1];
  if (input_column_headers==NULL)
    {
       ignore_value(snprintf(tmp,sizeof(tmp),"field-%zu",field_num));
       return tmp;
    }
  else
    {
       const struct columnheader *h = input_column_headers;
       if (field_num > num_input_column_headers)
         error(EXIT_FAILURE,0,_("not enough input fields (field %zu requested, input has only %zu fields)"), field_num, num_input_column_headers);

       while (--field_num)
         h = h->next;
       return h->name;
    }
}

void
build_input_line_headers(const char*line, size_t len, bool store_names)
{
  const char* ptr = line;
  const char* lim = line+len;

  while (ptr<lim)
   {
     const char *end = ptr;

     /* Find the end of the input field, starting at 'ptr' */
     if (tab != TAB_DEFAULT)
       {
         while (end < lim && *end != tab)
           ++end;
       }
     else
       {
         while (end < lim && !blanks[to_uchar (*end)])
           ++end;
       }

     if (store_names)
       add_named_input_column_header(ptr, end-ptr);
     ++num_input_column_headers;

#ifdef ENABLE_DEBUG
     if (debug)
       {
         fprintf(stderr,"input line header = ");
         fwrite(ptr,sizeof(char),end-ptr,stderr);
         fputc(eolchar,stderr);
       }
#endif

     /* Find the begining of the next field */
     ptr = end ;
     if (tab != TAB_DEFAULT)
       {
         if (ptr < lim)
           ++ptr;
       }
     else
       {
         while (ptr < lim && blanks[to_uchar (*ptr)])
           ++ptr;
       }
    }
}

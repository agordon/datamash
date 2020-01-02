/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2018-2020 Assaf Gordon <assafgordon@gmail.com>
   Copyright (C) 1994-2018 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
Portions of this function were copied from GNU coreutils' seq.c,
hence FSF copyright.
*/


#include <config.h>

#include "system.h"
#include "die.h"
#include "quote.h"
#include "xalloc.h"


char*
validate_double_format (char const *fmt)
{
  size_t i;
  size_t len;
  char *out;

  len = strlen (fmt);

  /* extra space for NUL and 'L' printf-modifier */
  out = xmalloc (len+2);

  for (i = 0; ! (fmt[i] == '%' && fmt[i + 1] != '%'); i += (fmt[i] == '%') + 1)
    if (!fmt[i])
      die (EXIT_FAILURE, 0,
	   _("format %s has no %% directive"), quote (fmt));

  i++;
  i += strspn (fmt + i, "-+#0 '");
  i += strspn (fmt + i, "0123456789");
  if (fmt[i] == '.')
    {
      i++;
      i += strspn (fmt + i, "0123456789");
    }

  if (!fmt[i])
    die (EXIT_FAILURE, 0,
         _("format %s missing valid type after '%%'"), quote (fmt));

  if (! strchr ("efgaEFGA", fmt[i]))
    die (EXIT_FAILURE, 0,
         _("format %s has unknown/invalid type %%%c directive"),
	 quote (fmt), fmt[i]);

  /* Copy characters until the type character, add 'L', then the type,
     then the rest of the format string. */
  memcpy (out, fmt, i);
  out[i] = 'L';
  out[i+1] = fmt[i];
  memcpy (out+i+2, fmt+i+1, len-i);
  out[len+1] = '\0';

  for (i++; fmt[i] ; i += (fmt[i] == '%') + 1)
    if (fmt[i] == '%' && fmt[i + 1] != '%')
      die (EXIT_FAILURE, 0, _("format %s has too many %% directives"),
           quote (fmt));

  return out;
}

/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2021 Assaf Gordon <assafgordon@gmail.com>
   Copyright (C) 2022-     Tim Rice     <trice@posteo.net>

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

#include <config.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "randutils.h"

/* long mix function, from:
   Robert Jenkins' 96 bit Mix Function
   http://burtleburtle.net/bob/hash/doobs.html */
static unsigned long
mix (unsigned long a, unsigned long b, unsigned long c)
{
  a=a-b;  a=a-c;  a=a^(c >> 13);
  b=b-c;  b=b-a;  b=b^(a << 8);
  c=c-a;  c=c-b;  c=c^(b >> 13);
  a=a-b;  a=a-c;  a=a^(c >> 12);
  b=b-c;  b=b-a;  b=b^(a << 16);
  c=c-a;  c=c-b;  c=c^(b >> 5);
  a=a-b;  a=a-c;  a=a^(c >> 3);
  b=b-c;  b=b-a;  b=b^(a << 10);
  c=c-a;  c=c-b;  c=c^(b >> 15);
  return c;
}

void
init_random (void)
{
  unsigned long seed = mix (clock (), time (NULL), getpid ());
  srandom (seed);
}

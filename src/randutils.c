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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/random.h>

#include "randutils.h"

void
init_random (bool force_seed, unsigned long seed)
{
  if (!force_seed)
    {
      errno = 0;
      ssize_t nbytes = getrandom (&seed, sizeof (seed), 0);
      if (nbytes == -1 || errno != 0)
        {
          fprintf (stderr, "Error %d: %s\n", errno, strerror (errno));
        }
    }
  srandom (seed);
}

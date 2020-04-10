/* Decorate functions

   Copyright (C) 2020 Assaf Gordon <assafgordon@gmail.com>

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

#include <config.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <intprops.h>
#include <stdbool.h>
#include <error.h>

#include "system.h"
#include "die.h"
#include "decorate-functions.h"


bool
decorate_as_is (const char* in)
{
  fprintf (stdout, "%s", in);
  return true;
}

bool
decorate_strlen (const char* in)
{
  uintmax_t u = (uintmax_t)strlen (in);
  printf ("%0*"PRIuMAX, (int)INT_BUFSIZE_BOUND (u), u);
  return true;
}

_GL_ATTRIBUTE_CONST int
roman_numeral_to_value (char c)
{
  switch (c)
    {
    case 'M': return 1000;
    case 'D': return 500;
    case 'C': return 100;
    case 'L': return 50;
    case 'X': return 10;
    case 'V': return 5;
    case 'I': return 1;
    default:  return 0;
    }
}


/* Naive implementation of Roman numerals conversion.
   Does not support alternative forms such as
    XIIX,IIXX for 18,
    IIC for 98.  */
bool
decorate_roman_numerals (const char* in)
{
  intmax_t result = 0;
  intmax_t cur,last = 0;
  if (*in=='\0')
    {
      error (0, 0, _("invalid empty roman numeral"));
      return false;
    }
  while (*in)
    {
      cur = roman_numeral_to_value (*in);
      if (!cur)
        {
          error (0, 0, _("invalid roman numeral '%c' in %s"),  *in, quote (in));
          return false;
        }

      if (last)
        {
          if (last >= cur)
            {
              result += last;
            }
          else
            {
              result += (cur - last);
              cur = 0;
            }
        }

      last = cur;
      ++in;
    }

  result += last;
  printf ("%0*"PRIiMAX, (int)INT_BUFSIZE_BOUND (result), result);
  return true;
}

bool
decorate_ipv4_inet_addr (const char* in)
{
  struct in_addr adr;
  int s;

  s = inet_aton (in, &adr);

  if (s == 0)
    {
      error (0, 0, _("invalid IPv4 address %s"), quote (in));
      return false;
    }


  printf ("%08X", ntohl (adr.s_addr));
  return true;
}


bool
decorate_ipv4_dot_decimal (const char* in)
{
  struct in_addr adr;
  int s;

  s = inet_pton (AF_INET, in, &adr);

  if (s < 0)
    die (SORT_FAILURE, errno, _("inet_pton (AF_INET) failed"));

  if (s == 0)
    {
      error (0, 0, _("invalid dot-decimal IPv4 address %s"), quote (in));
      return false;
    }

  printf ("%08X", ntohl (adr.s_addr));
  return true;
}


bool
decorate_ipv6 (const char* in)
{
  struct in6_addr adr;
  int s;

  s = inet_pton (AF_INET6, in, &adr);

  if (s < 0)
    die (SORT_FAILURE, errno, _("inet_pton (AF_INET6) failed"));

  if (s == 0)
    {
      error (0, 0, _("invalid IPv6 address %s"), quote (in));
      return false;
    }

  /* A portable way to print IPv6 binary representation. */
  for (int i=0;i<16;i+=2)
    {
      printf ("%02X%02X", adr.s6_addr[i], adr.s6_addr[i+1]);
      if (i != 14)
        fputc (':', stdout);
    }

  return true;
}



struct conversions_t builtin_conversions[] = {
  { "as-is",    "copy as-is", decorate_as_is },     /* for debugging */
  { "roman",    "roman numerals", decorate_roman_numerals },
  { "strlen",   "length (in bytes) of the specified field", decorate_strlen },
  { "ipv4",     "dotted-decimal IPv4 addresses", decorate_ipv4_dot_decimal },
  { "ipv6",     "IPv6 addresses", decorate_ipv6 },
  { "ipv4inet", "number-and-dots IPv4 addresses (incl. octal, hex values)",
    decorate_ipv4_inet_addr },
  { NULL,       NULL, 0 }
};

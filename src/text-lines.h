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
#ifndef __TEXT_LINES_H__
#define __TEXT_LINES_H__

/*
 Text-Line and Field-Extraction module
 */

void linebuffer_nullify (struct linebuffer *line);

/* Given a linebuffer textual line, returns pointer (and length) of field 'field'.
   NOTE:
       'line' doesn't need to be NULL-terminated,
       and the returned _ptr will not be NULL-terminated either.
*/
void get_field (const struct linebuffer *line, size_t field,
               const char** /* OUT*/ _ptr, size_t /*OUT*/ *_len);

#endif

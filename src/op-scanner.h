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
#ifndef __OP_SCANNER_H__
#define __OP_SCANNER_H__

#define MAX_IDENTIFIER_LENGTH 512

enum TOKEN
{
  TOK_END=0,
  TOK_IDENTIFIER,
  TOK_INTEGER,
  TOK_FLOAT,
  TOK_COMMA,
  TOK_DASH,
  TOK_COLONS,
  TOK_WHITESPACE
};

extern uintmax_t scan_val_int;
extern long double scan_val_float;
extern char* scanner_identifier;
extern bool scanner_keep_whitespace;

/* Initialize the scanner from argc/argv pair.
   note: argv should contain only the actual input: remove
         any other program parameters (including progname/argv[0]) */
void
scanner_set_input_from_argv (int argc, const char* argv[]);

/* Free any data/memory associated with the scanner */
void
scanner_free ();

enum TOKEN
scanner_get_token ();

enum TOKEN
scanner_peek_token ();

#endif

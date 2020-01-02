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
#ifndef __OP_PARSER_H__
#define __OP_PARSER_H__

struct group_column_t
{
  size_t num;       /* 1 = first field */
  bool   by_name;   /* true if the user gave a column name */
  char*  name;      /* column name - to be converted to number after
                       header line is read */
};

struct op_column_t
{
  size_t num;                /* 1 = first field */
  bool   by_name;            /* true if the user gave a column name */
  char*  name;               /* column name - to be converted to number after
                                header line is read */
  enum   field_operation op;
};

struct mode_check_params_t
{
  uintmax_t n_lines;         /* If not zero, require this number of lines */
  uintmax_t n_fields;        /* if not zero, require this number of fields */
};

struct datamash_ops
{
  enum processing_mode mode; /* the processing mode */
  bool header_required;      /* true if any of the fields (groups/operations)
                                used a named column instead of a number. */

  struct group_column_t *grps; /* group-by columns */
  size_t num_grps;
  size_t alloc_grps;

  struct fieldop    *ops;  /* field operations */
  size_t num_ops;
  size_t alloc_ops;

  /* Additional parameters for mode operatons
     (i.e. ones relating to the operation mode,
     not to specific field-ops) */
  union
  {
    struct mode_check_params_t check_params;
  } mode_params;
};

/* Parse the operations, return new datamash_ops structure.
   This function assumes new syntax:
   1. The first word is either a mode (e.g. transpose/groupby/reverse)
      or an operation (e.g. sum/min/max) - implying a 'group-by' mode.
   2. The rest of the parameters are operations. */
struct datamash_ops*
datamash_ops_parse ( int argc, const char* argv[] );

/* Parse the operations, return new datamash_ops structure.
   This function assumes old syntax:
    The user already specified "-g X,Y,Z" - the processing mode is known,
    and the grouping text 'X,Y,Z' is known.
   The function will only accept operations (e.g. sum/min/max). */
struct datamash_ops*
datamash_ops_parse_premode ( enum processing_mode pm,
                             const char* grouping_spec,
                             int argc, const char* argv[] );

void
datamash_ops_debug_print ( const struct datamash_ops* p );

void
datamash_ops_free (struct datamash_ops *p);

#endif

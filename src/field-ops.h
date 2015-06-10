/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2015 Assaf Gordon <assafgordon@gmail.com>

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
#ifndef __FIELD_OPS_H__
#define __FIELD_OPS_H__

/*
 Operations Module
 */

enum accumulation_type
{
  NUMERIC_SCALAR = 0,
  NUMERIC_VECTOR,
  STRING_SCALAR,
  STRING_VECTOR
};

enum operation_result_type
{
  NUMERIC_RESULT = 0,
  STRING_RESULT
};

enum operation_first_value
{
  AUTO_SET_FIRST = true,
  IGNORE_FIRST = false
};

enum FIELD_OP_COLLECT_RESULT
{
  FLOCR_OK = 0,
  FLOCR_OK_KEEP_LINE,
  FLOCR_OK_SKIPPED,
  FLOCR_INVALID_NUMBER,
  FLOCR_INVALID_BASE64
};


#define field_op_ok(X) \
  (((X)==FLOCR_OK)||((X)==FLOCR_OK_KEEP_LINE)||((X)==FLOCR_OK_SKIPPED))

const char*
field_op_collect_result_name (const enum FIELD_OP_COLLECT_RESULT flocr);

struct operation_data
{
  enum accumulation_type acc_type;
  enum operation_first_value auto_first;
  //enum operation_mode mode;
  enum operation_result_type res_type;
};

extern struct operation_data operations[];

/* Operation on a field */
struct fieldop
{
    /* operation 'class' information */
  enum field_operation op;
  enum accumulation_type acc_type;
  enum operation_result_type res_type;
  bool numeric;
  bool auto_first; /* if true, automatically set 'value' if 'first' */

  /* Instance information */
  size_t field; /* field number.  1 = first field in input file. */
  bool   field_by_name; /* if true, user gave field name (instead of number),
			   which needs to be resolved AFTER the header line
			   is loaded */
  char* field_name;


  /* Collected Data */
  bool first;   /* true if this is the first item in a new group */

  /* NUMERIC_SCALAR operations */
  size_t count; /* number of items collected so far in a group */
  long double value; /* for single-value operations (sum, min, max, absmin,
                        absmax, mean) - this is the accumulated value */

  /* NUMERIC_VECTOR operations */
  long double *values;     /* array for multi-valued ops (median,mode,stdev) */
  size_t      num_values;  /* number of used values */
  size_t      alloc_values;/* number of allocated values */

  /* String buffer for STRING_VECTOR operations */
  char *str_buf;   /* points to the beginning of the buffer */
  size_t str_buf_used; /* number of bytes used in the buffer */
  size_t str_buf_alloc; /* number of bytes allocated in the buffer */

  /* Output buffer containing the final results of an operation,
     set by 'summarize' functions.
     also used for line operations (md5/sha1/256/512/base64). */
  char *out_buf;
  size_t out_buf_used;
  size_t out_buf_alloc;

  struct fieldop *next;
};

extern struct fieldop* field_ops ;

/* Add a numeric value to the values vector, allocating memory as needed */
void field_op_add_value (struct fieldop *op, long double val);

/* Returns an array of string-pointers (char*),
   each pointing to a string in the string buffer
   (added by field_op_add_string ).

   The returned pointer must be free'd.

   The returned pointer will have 'op->count+1' elements,
   pointing to 'op->count' strings + one last NULL.
*/
const char ** field_op_get_string_ptrs ( struct fieldop *op,
                                         bool sort, bool sort_case_sensitive );

/* Sort the numeric values vector in a fieldop structure */
void field_op_sort_values (struct fieldop *op);

/* Allocate a new fieldop, initialize it based on 'oper',
   and add it to the linked-list of operations */
struct fieldop *
new_field_op (enum field_operation oper, bool by_name, size_t num, const char* name);

/* Add a value (from input) to the current field operation.
   'str' does not need to be null-terminated.

  Returns true if the operation was successful.
  Returns false if the input was invalid numeric value.
*/
enum FIELD_OP_COLLECT_RESULT
field_op_collect (struct fieldop *op, const char* str, size_t slen);


/* creates a list of unique strings from op->str_buf .
   results are stored in op->out_buf. */
void
unique_value ( struct fieldop *op, bool case_sensitive );

/* stores the hexadecimal representation of 'buffer' in op->out_buf */
void
field_op_to_hex ( struct fieldop *op, const char *buffer, const size_t inlen );

/* Prints to stdout the result of the field operation,
   based on collected values */
void
field_op_summarize (struct fieldop *op);

/* Prints to stdout the result of the field operation
   when there are no input values.
   'no values' can happen with '--narm' and input of all N/As.
   The printed results are consistent as much as possible with R */
void
field_op_summarize_empty (struct fieldop *op);

void
summarize_field_ops ();

void
reset_field_ops ();

void
free_field_ops ();

/* Output precision, to be used with "printf ("%.*Lg",)" */
extern int field_op_output_precision;

/* Initialize random number source */
void
init_random (void);


/* returns TRUE if any of the configured fields was using
   a named column, therefore requiring a header line. */
bool
field_op_have_named_fields ();

/* Tries to find the column number for operations using
   a named column (instead of a number) */
void
field_op_find_named_columns ();

/* Helper function to print to stdout the 'empty value' of a numeric
   operation (e.g. what's printed by 'OP_MEAN' with empty input).
   Used in some of the tests. */
void
field_op_print_empty_value (enum field_operation mode);

#endif

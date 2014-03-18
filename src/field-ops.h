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
#ifndef __FIELD_OPS_H__
#define __FIELD_OPS_H__

/*
 Operations Module
 */

enum operation
{
  OP_COUNT = 0,
  OP_SUM,
  OP_MIN,
  OP_MAX,
  OP_ABSMIN,
  OP_ABSMAX,
  OP_MEAN,
  OP_MEDIAN,
  OP_PSTDEV,    /* Population Standard Deviation */
  OP_SSTDEV,    /* Sample Standard Deviation */
  OP_PVARIANCE, /* Population Variance */
  OP_SVARIANCE, /* Sample Variance */
  OP_MODE,
  OP_ANTIMODE,
  OP_UNIQUE,        /* Collapse Unique string into comma separated values */
  OP_COLLAPSE,      /* Collapse strings into comma separated values */
  OP_COUNT_UNIQUE   /* count number of unique values */
};

enum operation_type
{
  NUMERIC_SCALAR = 0,
  NUMERIC_VECTOR,
  STRING_SCALAR,
  STRING_VECTOR
};

enum operation_first_value
{
  AUTO_SET_FIRST = true,
  IGNORE_FIRST = false
};

struct operation_data
{
  const char* name;
  enum operation_type type;
  enum operation_first_value auto_first;
};

extern struct operation_data operations[];

/* Operation on a field */
struct fieldop
{
    /* operation 'class' information */
  enum operation op;
  enum operation_type type;
  const char* name;
  bool numeric;
  bool auto_first; /* if true, automatically set 'value' if 'first' */

  /* Instance information */
  size_t field; /* field number.  1 = first field in input file. */

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

  struct fieldop *next;
};

extern struct fieldop* field_ops ;

/* Add a numeric value to the values vector, allocating memory as needed */
void field_op_add_value (struct fieldop *op, long double val);

/* Returns an array of string-pointers (char*),
   each pointing to a string in the string buffer (added by field_op_add_string() ).

   The returned pointer must be free()'d.

   The returned pointer will have 'op->count+1' elements,
   pointing to 'op->count' strings + one last NULL.
*/
const char ** field_op_get_string_ptrs ( struct fieldop *op,
			bool sort, bool sort_case_sensitive );

/* Sort the numeric values vector in a fieldop structure */
void field_op_sort_values (struct fieldop *op);

/* Allocate a new fieldop, initialize it based on 'oper',
   and add it to the linked-list of operations */
void new_field_op (enum operation oper, size_t field);

/* Add a value (from input) to the current field operation.
   If the operation is numeric, num_value should be used.
   If the operation is textual, str +slen should be used
     (str is not guarenteed to be null terminated).

   Return value (boolean, keep_line) isn't used at the moment. */
bool field_op_collect (struct fieldop *op,
                  const char* str, size_t slen,
                  const long double num_value);

/* Returns a nul-terimated string, composed of the unique values
   of the input strings. The return string must be free()'d. */
char* unique_value ( struct fieldop *op, bool case_sensitive );

/* Prints to stdout the result of the field operation,
   based on collected values */
void field_op_summarize (struct fieldop *op);

void summarize_field_ops ();

void reset_field_ops ();

void free_field_ops ();

enum operation get_operation (const char* op);

/* Extract the operation patterns from args START through ARGC - 1 of ARGV. */
void parse_operations (int argc, int start, char **argv);

#endif

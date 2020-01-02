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

struct operation_data
{
  enum accumulation_type acc_type;
  enum operation_first_value auto_first;
  enum operation_result_type res_type;
};

/* Operation on a field */
struct fieldop
{
    /* operation 'class' information */
  enum field_operation op;
  enum accumulation_type acc_type;
  enum operation_result_type res_type;
  bool numeric;
  bool auto_first; /* if true, automatically set 'value' if 'first' */
  bool master;     /* if true, this field_op uses another as a slave */
  bool slave;      /* if true, not used directly, but referenced by
                      another field_op */
  size_t slave_idx;
  struct fieldop* slave_op;

  /* Instance information */
  size_t field; /* field number.  1 = first field in input file. */
  bool   field_by_name; /* if true, user gave field name (instead of number),
			   which needs to be resolved AFTER the header line
			   is loaded */
  char* field_name;

  union {
    long double bin_bucket_size;
    size_t strbin_bucket_size;
    size_t percentile;
    long double trimmed_mean;
    enum extract_number_type get_num_type;
  } params;

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
};

/* Initializes a new field-op, using an *existing* (pre-allocated) struct. */
void
field_op_init (struct fieldop* /*in-out*/ op,
               enum field_operation oper,
               bool by_name, size_t num, const char* name);

/* Frees the internal structures in the field-op.
   Does *not* free 'op' itself */
void
field_op_free (struct fieldop* op);

/* Add a value (from input) to the current field operation.
   'str' does not need to be null-terminated.

  Returns true if the operation was successful.
  Returns false if the input was invalid numeric value.
*/
enum FIELD_OP_COLLECT_RESULT
field_op_collect (struct fieldop *op, const char* str, size_t slen);

/* Evaluates to true/false depending if the value returned from
   field_op_collect represents a successful operation. */
#define field_op_ok(X) \
  (((X)==FLOCR_OK)||((X)==FLOCR_OK_KEEP_LINE)||((X)==FLOCR_OK_SKIPPED))

/* If field_op_ok returned false, this function will return a textual
   error message of the error. The returned value is a static string,
   do not free it. */
const char*
field_op_collect_result_name (const enum FIELD_OP_COLLECT_RESULT flocr);


/* Called after all values in a group are collected in a field-op,
   to perform any (optional) finalizing steps
   (e.g. in OP_MEAN, calculate the mean).
   Result will be stored in op->out_buf. */
void
field_op_summarize (struct fieldop *op);

/* resets internal variables, should be called when starting a new
   group of values. */
void
field_op_reset (struct fieldop *op);

/* Output precision, to be used with "printf ("%.*Lg",)" */
extern int field_op_output_precision;

/* Initialize random number source */
void
init_random (void);

/* Helper function to print to stdout the 'empty value' of a numeric
   operation (e.g. what's printed by 'OP_MEAN' with empty input).
   Used in some of the tests. */
void
field_op_print_empty_value (enum field_operation mode);

#endif

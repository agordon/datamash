/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013,2014 Assaf Gordon <assafgordon@gmail.com>

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

enum operation
{
  OP_COUNT = 0,
  OP_SUM,
  OP_MIN,
  OP_MAX,
  OP_ABSMIN,
  OP_ABSMAX,
  OP_FIRST,
  OP_LAST,
  OP_RAND,
  OP_MEAN,
  OP_MEDIAN,
  OP_QUARTILE_1,
  OP_QUARTILE_3,
  OP_IQR,       /* Inter-quartile range */
  OP_PSTDEV,    /* Population Standard Deviation */
  OP_SSTDEV,    /* Sample Standard Deviation */
  OP_PVARIANCE, /* Population Variance */
  OP_SVARIANCE, /* Sample Variance */
  OP_MAD,       /* MAD - Median Absolute Deviation, with adjustment constant of
                   1.4826 for normal distribution */
  OP_MADRAW,    /* MAD (same as above), with constant=1 */
  OP_S_SKEWNESS,/* Sample Skewness */
  OP_P_SKEWNESS,/* Population Skewness */
  OP_S_EXCESS_KURTOSIS, /* Sample Excess Kurtosis */
  OP_P_EXCESS_KURTOSIS, /* Population Excess Kurtosis */
  OP_JARQUE_BERA,   /* Jarque-Bera test of normality */
  OP_DP_OMNIBUS,    /* D'Agostino-Pearson omnibus test of normality */
  OP_MODE,
  OP_ANTIMODE,
  OP_UNIQUE,        /* Collapse Unique string into comma separated values */
  OP_COLLAPSE,      /* Collapse strings into comma separated values */
  OP_COUNT_UNIQUE,  /* count number of unique values */
  OP_TRANSPOSE,     /* transpose */
  OP_REVERSE,       /* reverse fields in each line */
  OP_BASE64,        /* Encode Field to Base64 */
  OP_DEBASE64,      /* Decode Base64 field */
  OP_MD5,           /* Calculate MD5 of a field */
  OP_SHA1,          /* Calculate SHA1 of a field */
  OP_SHA256,        /* Calculate SHA256 of a field */
  OP_SHA512,        /* Calculate SHA512 of a field */
};

enum accumulation_type
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

enum operation_mode
{
	UNKNOWN_MODE = 0,
	GROUPING_MODE,
	TRANSPOSE_MODE,
	REVERSE_FIELD_MODE,
	LINE_MODE
};


struct operation_data
{
  const char* name;
  enum accumulation_type acc_type;
  enum operation_first_value auto_first;
  enum operation_mode mode;
};

extern struct operation_data operations[];

/* Operation on a field */
struct fieldop
{
    /* operation 'class' information */
  enum operation op;
  enum accumulation_type acc_type;
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

  /* Output buffer for line operations (md5/sha1/256/512/base64) */
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
   (added by field_op_add_string() ).

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
struct fieldop *
new_field_op (enum operation oper, size_t field);

/* Add a value (from input) to the current field operation.
   'str' does not need to be null-terminated.

  Returns true if the operation was successful.
  Returns false if the input was invalid numeric value.
*/
bool field_op_collect (struct fieldop *op,
                  const char* str, size_t slen);


/* creates a list of unique strings from op->str_buf .
   results are stored in op->out_buf. */
void
unique_value ( struct fieldop *op, bool case_sensitive );

/* stores the hexadecimal representation of 'buffer' in op->out_buf */
void
field_op_to_hex ( struct fieldop *op, const char *buffer, const size_t inlen );

/* Prints to stdout the result of the field operation,
   based on collected values */
void field_op_summarize (struct fieldop *op);

void summarize_field_ops ();

void reset_field_ops ();

void free_field_ops ();

/*
enum operation_mode
get_operation_mode (const char* keyword);
*/

enum operation
get_operation (const char* keyword);

/* Extract the operation patterns from args START through ARGC - 1 of ARGV.
   Requires all operation to be of 'mode' - otherwise exits with an error.
 */
void
parse_operations (enum operation_mode mode,
		  int argc, int start, char **argv);

/* Extract the operation mode based on the first keyword.
   Possible modes are:
     transpose
     reverse (reverse fields)
     line mode
     grouping
   depending on the 'operation_mode' of the first operation keyword
   found.

  In grouping/line modes,
  calls 'parse_operations' to set the indivudual operaitons. */
enum operation_mode
parse_operation_mode (int argc, int start, char** argv);

/* Output precision, to be used with "printf("%.*Lg",)" */
extern int field_op_output_precision;

/* Initialize random number source */
void init_random();

#endif

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
#include <config.h>
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "error.h"
#include "minmax.h"
#include "system.h"
#include "xalloc.h"

#include "utils.h"
#include "text-options.h"
#include "field-ops.h"

struct operation_data operations[] =
{
  {"count",   STRING_SCALAR,  IGNORE_FIRST},   /* OP_COUNT */
  {"sum",     NUMERIC_SCALAR,  IGNORE_FIRST},   /* OP_SUM */
  {"min",     NUMERIC_SCALAR,  AUTO_SET_FIRST}, /* OP_MIN */
  {"max",     NUMERIC_SCALAR,  AUTO_SET_FIRST}, /* OP_MAX */
  {"absmin",  NUMERIC_SCALAR,  AUTO_SET_FIRST}, /* OP_ABSMIN */
  {"absmax",  NUMERIC_SCALAR,  AUTO_SET_FIRST}, /* OP_ABSMAX */
  {"mean",    NUMERIC_SCALAR,  IGNORE_FIRST},   /* OP_MEAN */
  {"median",  NUMERIC_VECTOR,  IGNORE_FIRST},   /* OP_MEDIAN */
  {"pstdev",  NUMERIC_VECTOR,  IGNORE_FIRST},   /* OP_PSTDEV */
  {"sstdev",  NUMERIC_VECTOR,  IGNORE_FIRST},   /* OP_SSTDEV */
  {"pvar",    NUMERIC_VECTOR,  IGNORE_FIRST},   /* OP_PVARIANCE */
  {"svar",    NUMERIC_VECTOR,  IGNORE_FIRST},   /* OP_SVARIANCE */
  {"mode",    NUMERIC_VECTOR,  IGNORE_FIRST},   /* OP_MODE */
  {"antimode",NUMERIC_VECTOR,  IGNORE_FIRST},   /* OP_ANTIMODE */
  {"unique",  STRING_VECTOR,   IGNORE_FIRST},   /* OP_UNIQUE */
  {"collapse",STRING_VECTOR,   IGNORE_FIRST},   /* OP_COLLAPSE */
  {"countunique",STRING_VECTOR,   IGNORE_FIRST},   /* OP_COUNT_UNIQUE */
  {NULL, 0, 0}
};

struct fieldop* field_ops = NULL;

enum { VALUES_BATCH_INCREMENT = 1024 };

/* The character used to separate collapsed/uniqued strings */
static char collapse_separator = ',';


/* Add a numeric value to the values vector, allocating memory as needed */
void
field_op_add_value (struct fieldop *op, long double val)
{
  if (op->num_values >= op->alloc_values)
    {
      op->alloc_values += VALUES_BATCH_INCREMENT;
      op->values = xnrealloc (op->values, op->alloc_values, sizeof (long double));
    }
  op->values[op->num_values] = val;
  op->num_values++;
}

/* Add a string to the strings vector, allocating memory as needed */
void
field_op_add_string (struct fieldop *op, const char* str, size_t slen)
{
  if (op->str_buf_used + slen+1 >= op->str_buf_alloc)
    {
      op->str_buf_alloc += MAX(VALUES_BATCH_INCREMENT,slen+1);
      op->str_buf = xrealloc (op->str_buf, op->str_buf_alloc);
    }

  /* Copy the string to the buffer */
  memcpy (op->str_buf + op->str_buf_used, str, slen);
  *(op->str_buf + op->str_buf_used + slen ) = 0;
  op->str_buf_used += slen + 1 ;
}

/* Returns an array of string-pointers (char*),
   each pointing to a string in the string buffer (added by field_op_add_string() ).

   The returned pointer must be free()'d.

   The returned pointer will have 'op->count+1' elements,
   pointing to 'op->count' strings + one last NULL.
*/
const char **
field_op_get_string_ptrs ( struct fieldop *op, bool sort, bool sort_case_sensitive )
{
  const char **ptrs = xnmalloc(op->count+1, sizeof(char*));
  char *p = op->str_buf;
  const char* pend = op->str_buf + op->str_buf_used;
  size_t idx=0;
  while (p < pend)
    {
      ptrs[idx++] = p;
      while ( p<pend && *p != '\0' )
        ++p;
      ++p;
    }
  ptrs[idx] = 0;

  if (sort)
    {
      /* Sort the string pointers */
      qsort ( ptrs, op->count, sizeof(char*), sort_case_sensitive
                                            ?cmpstringp
                                            :cmpstringp_nocase);
    }
  return ptrs;
}

/* Sort the numeric values vector in a fieldop structure */
void
field_op_sort_values (struct fieldop *op)
{
  qsort (op->values, op->num_values, sizeof (long double), cmp_long_double);
}

/* Allocate a new fieldop, initialize it based on 'oper',
   and add it to the linked-list of operations */
void
new_field_op (enum operation oper, size_t field)
{
  struct fieldop *op = XZALLOC(struct fieldop);

  op->op = oper;
  op->type = operations[oper].type;
  op->name = operations[oper].name;
  op->numeric = (op->type == NUMERIC_SCALAR || op->type == NUMERIC_VECTOR);
  op->auto_first = operations[oper].auto_first;

  op->field = field;
  op->first = true;
  op->value = 0;
  op->count = 0;

  op->next = NULL;

  if (field_ops != NULL)
    {
      struct fieldop *p = field_ops;
      while (p->next != NULL)
        p = p->next;
      p->next = op;
    }
  else
    field_ops = op;
}

/* Add a value (from input) to the current field operation.
   If the operation is numeric, num_value should be used.
   If the operation is textual, str +slen should be used
     (str is not guarenteed to be null terminated).

   Return value (boolean, keep_line) isn't used at the moment. */
bool
field_op_collect (struct fieldop *op,
                  const char* str, size_t slen,
                  const long double num_value)
{
  bool keep_line = false;

#ifdef ENABLE_DEBUG
  if (debug)
    {
      fprintf (stderr, "-- collect for %s(%zu) val='", op->name, op->field);
      fwrite (str, sizeof(char), slen, stderr);
      fprintf (stderr, "'\n");
    }
#endif

  op->count++;

  if (op->first && op->auto_first)
      op->value = num_value;

  switch (op->op)
    {
    case OP_SUM:
    case OP_MEAN:
      op->value += num_value;
      keep_line = op->first;
      break;

    case OP_COUNT:
      op->value++;
      keep_line = op->first;
      break;

    case OP_MIN:
      if (num_value < op->value)
        {
          keep_line = true;
          op->value = num_value;
        }
      break;

    case OP_MAX:
      if (num_value > op->value)
        {
          keep_line = true;
          op->value = num_value;
        }
      break;

    case OP_ABSMIN:
      if (fabsl(num_value) < fabsl(op->value))
        {
          keep_line = true;
          op->value = num_value;
        }
      break;

    case OP_ABSMAX:
      if (fabsl(num_value) > fabsl(op->value))
        {
          keep_line = true;
          op->value = num_value;
        }
      break;

    case OP_MEDIAN:
    case OP_PSTDEV:
    case OP_SSTDEV:
    case OP_PVARIANCE:
    case OP_SVARIANCE:
    case OP_MODE:
    case OP_ANTIMODE:
      field_op_add_value (op, num_value);
      break;

    case OP_UNIQUE:
    case OP_COLLAPSE:
    case OP_COUNT_UNIQUE:
      field_op_add_string (op, str, slen);
      break;

    default:
      /* Should never happen */
      error (EXIT_FAILURE, 0, _("internal error 3"));
    }

  if (op->first)
    op->first = false;

  return keep_line;
}

/* Returns a nul-terimated string, composed of the unique values
   of the input strings. The return string must be free()'d. */
char *
unique_value ( struct fieldop *op, bool case_sensitive )
{
  const char *last_str;
  char *buf, *pos;

  const char **ptrs = field_op_get_string_ptrs (op, true, case_sensitive);

  /* Uniquify them */
  pos = buf = xzalloc ( op->str_buf_used );

  /* Copy the first string */
  last_str = ptrs[0];
  strcpy (buf, ptrs[0]);
  pos += strlen(ptrs[0]);

  /* Copy the following strings, if they are different from the previous one */
  for (size_t i = 1; i < op->count; ++i)
    {
      const char *newstr = ptrs[i];

      if ((case_sensitive && (!STREQ(newstr, last_str)))
          ||
          (!case_sensitive && (strcasecmp(newstr, last_str)!=0)))
        {
          *pos++ = collapse_separator ;
          strcpy (pos, newstr);
          pos += strlen(newstr);
        }
      last_str = newstr;
    }

  free(ptrs);

  return buf;
}

/* Returns the number of unique string values in the given field operation */
size_t
count_unique_values ( struct fieldop *op, bool case_sensitive )
{
  const char *last_str, **cur_str;
  size_t count = 1 ;

  const char **ptrs = field_op_get_string_ptrs (op, true, case_sensitive);

  /* Copy the first string */
  cur_str = ptrs;
  last_str = *cur_str;
  ++cur_str;

  /* Copy the following strings, if they are different from the previous one */
  while ( *cur_str != 0 )
    {
      if ((case_sensitive && (!STREQ(*cur_str, last_str)))
          ||
          (!case_sensitive && (strcasecmp(*cur_str, last_str)!=0)))
        {
                ++count;
        }
      last_str = *cur_str;
      ++cur_str;
    }

  free(ptrs);

  return count;
}

/* Returns a nul-terimated string, composed of all the values
   of the input strings. The return string must be free()'d. */
char *
collapse_value ( struct fieldop *op )
{
  /* Copy the string buffer as-is */
  char *buf = xzalloc ( op->str_buf_used );
  memcpy (buf, op->str_buf, op->str_buf_used);

  /* convert every NUL to comma, except for the last one */
  for (size_t i=0; i < op->str_buf_used-1 ; i++)
      if (buf[i] == 0)
        buf[i] = collapse_separator ;

  return buf;
}

/* Prints to stdout the result of the field operation,
   based on collected values */
void
field_op_summarize (struct fieldop *op)
{
  bool print_numeric_result = true;
  long double numeric_result = 0 ;
  char *string_result = NULL;

#ifdef ENABLE_DEBUG
  if (debug)
    fprintf (stderr, "-- summarize for %s(%zu)\n", op->name, op->field);
#endif

  switch (op->op)
    {
    case OP_MEAN:
      numeric_result = op->value / op->count;
      break;

    case OP_SUM:
    case OP_COUNT:
    case OP_MIN:
    case OP_MAX:
    case OP_ABSMIN:
    case OP_ABSMAX:
      /* no summarization for these operations, just print the value */
      numeric_result = op->value;
      break;

    case OP_MEDIAN:
      field_op_sort_values (op);
      numeric_result = median_value ( op->values, op->num_values );
      break;

    case OP_PSTDEV:
      numeric_result = stdev_value ( op->values, op->num_values, DF_POPULATION);
      break;

    case OP_SSTDEV:
      numeric_result = stdev_value ( op->values, op->num_values, DF_SAMPLE);
      break;

    case OP_PVARIANCE:
      numeric_result = variance_value ( op->values, op->num_values,
                                        DF_POPULATION);
      break;

    case OP_SVARIANCE:
      numeric_result = variance_value ( op->values, op->num_values,
                                        DF_SAMPLE);
      break;

    case OP_MODE:
    case OP_ANTIMODE:
      field_op_sort_values (op);
      numeric_result = mode_value ( op->values, op->num_values,
                                    (op->op==OP_MODE)?MODE:ANTIMODE);
      break;

    case OP_UNIQUE:
      print_numeric_result = false;
      string_result = unique_value (op, case_sensitive);
      break;

    case OP_COLLAPSE:
      print_numeric_result = false;
      string_result = collapse_value (op);
      break;

   case OP_COUNT_UNIQUE:
      numeric_result = count_unique_values(op,case_sensitive);
      break;

    default:
      /* Should never happen */
      error (EXIT_FAILURE, 0, _("internal error 2"));
    }


#ifdef ENABLE_DEBUG
  if (debug)
    {
      if (print_numeric_result)
        fprintf (stderr, "%s(%zu) = %Lg\n", op->name, op->field, numeric_result);
      else
        fprintf (stderr, "%s(%zu) = '%s'\n", op->name, op->field, string_result);
    }
#endif

  if (print_numeric_result)
    printf ("%Lg", numeric_result);
  else
    printf ("%s", string_result);

  free (string_result);
}

/* reset operation values for next group */
void
reset_field_op (struct fieldop *op)
{
  op->first = true;
  op->count = 0 ;
  op->value = 0;
  op->num_values = 0 ;
  op->str_buf_used = 0;
  /* note: op->str_buf and op->str_alloc are not free'd, and reused */
}

/* reset all field operations, for next group */
void
reset_field_ops ()
{
  for (struct fieldop *p = field_ops; p ; p = p->next)
    reset_field_op (p);
}

/* Frees all memory associated with a field operation struct.
   returns the 'next' field operation, or NULL */
static void
free_field_op (struct fieldop *op)
{
  free (op->values);
  op->num_values = 0 ;
  op->alloc_values = 0;

  free(op->str_buf);
  op->str_buf = NULL;
  op->str_buf_alloc = 0;
  op->str_buf_used = 0;

  free(op);
}

void
free_field_ops ()
{
  struct fieldop *p = field_ops;
  while (p)
    {
      struct fieldop *n = p->next;
      free_field_op(p);
      p = n;
    }
}

/* Given a string with operation name, returns the operation enum.
   exits with an error message if the string is not a valid/known operation. */
enum operation
get_operation (const char* op)
{
  for (size_t i = 0; operations[i].name ; i++)
      if ( STREQ(operations[i].name, op) )
        return (enum operation)i;

  error (EXIT_FAILURE, 0, _("invalid operation '%s'"), op);
  return 0; /* never reached LCOV_EXCL_LINE */
}

/* Converts a string to number (field number).
   Exits with an error message (using 'op') on invalid field number. */
static size_t
safe_get_field_number(enum operation op, const char* field_str)
{
  size_t val;
  char *endptr;
  errno = 0 ;
  val = strtoul (field_str, &endptr, 10);
  /* NOTE: can't use xstrtol_fatal - it's too tightly-coupled
     with getopt command-line processing */
  if (errno != 0 || endptr == field_str)
    error (EXIT_FAILURE, 0, _("invalid column '%s' for operation " \
                               "'%s'"), field_str,
                               operations[op].name);
  return val;
}

/* Extract the operation patterns from args START through ARGC - 1 of ARGV. */
void
parse_operations (int argc, int start, char **argv)
{
  int i = start;	/* Index into ARGV. */
  size_t field;
  enum operation op;

  while ( i < argc )
    {
      op = get_operation (argv[i]);
      i++;
      if ( i >= argc )
        error (EXIT_FAILURE, 0, _("missing field number after " \
                                  "operation '%s'"), argv[i-1] );
      field = safe_get_field_number (op, argv[i]);
      i++;

      new_field_op (op, field);
    }
}

void
summarize_field_ops ()
{
  for (struct fieldop *p = field_ops; p ; p=p->next)
    {
      field_op_summarize (p);

      /* print field separator */
      if (p->next)
        print_field_separator ();
    }

  /* print end-of-line */
  print_line_separator ();
}

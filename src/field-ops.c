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
#include <config.h>
#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "error.h"
#include "minmax.h"
#include "quote.h"
#include "system.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "sha512.h"
#include "base64.h"
#include "xalloc.h"

#include "utils.h"
#include "text-options.h"
#include "field-ops.h"

/* In the future: allow users to change this */
int field_op_output_precision = 14 ;

struct operation_data operations[] =
{
  /* OP_COUNT */
  {"count",   STRING_SCALAR,  IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_SUM */
  {"sum",     NUMERIC_SCALAR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_MIN */
  {"min",     NUMERIC_SCALAR, AUTO_SET_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_MAX */
  {"max",     NUMERIC_SCALAR, AUTO_SET_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_ABSMIN */
  {"absmin",  NUMERIC_SCALAR, AUTO_SET_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_ABSMAX */
  {"absmax",  NUMERIC_SCALAR, AUTO_SET_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_FIRST */
  {"first",   STRING_SCALAR,  IGNORE_FIRST, GROUPING_MODE, STRING_RESULT},
  /* OP_LAST */
  {"last",    STRING_SCALAR,  IGNORE_FIRST, GROUPING_MODE, STRING_RESULT},
  /* OP_RAND */
  {"rand",    STRING_SCALAR,  IGNORE_FIRST, GROUPING_MODE, STRING_RESULT},
  /* OP_MEAN */
  {"mean",    NUMERIC_SCALAR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_MEDIAN */
  {"median",  NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_QUARTILE_1 */
  {"q1",      NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_QUARTILE_3 */
  {"q3",      NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_IQR */
  {"iqr",     NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_PSTDEV */
  {"pstdev",  NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_SSTDEV */
  {"sstdev",  NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_PVARIANCE */
  {"pvar",    NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_SVARIANCE */
  {"svar",    NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_MAD */
  {"mad",     NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_MADRAW */
  {"madraw",  NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_S_SKEWNESS */
  {"sskew",   NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_P_SKEWNESS */
  {"pskew",   NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_S_EXCESS_KURTOSIS */
  {"skurt",   NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_P_EXCESS_KURTOSIS */
  {"pkurt",   NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_JARQUE_BETA */
  {"jarque",  NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_DP_OMNIBUS */
  {"dpo",     NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_MODE */
  {"mode",    NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_ANTIMODE */
  {"antimode",NUMERIC_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_UNIQUE */
  {"unique",  STRING_VECTOR,  IGNORE_FIRST, GROUPING_MODE, STRING_RESULT},
  /* OP_COLLAPSE */
  {"collapse",STRING_VECTOR,  IGNORE_FIRST, GROUPING_MODE, STRING_RESULT},
  /* OP_COUNT_UNIQUE */
  {"countunique",STRING_VECTOR, IGNORE_FIRST, GROUPING_MODE, NUMERIC_RESULT},
  /* OP_TRANSPOSE */
  {"transpose",STRING_SCALAR, IGNORE_FIRST, TRANSPOSE_MODE, STRING_RESULT},
  /* OP_REVERSE */
  {"reverse", STRING_SCALAR, IGNORE_FIRST, REVERSE_FIELD_MODE, STRING_RESULT},
  /* OP_BASE64 */
  {"base64",  STRING_SCALAR, IGNORE_FIRST, LINE_MODE, STRING_RESULT},
  /* OP_DEBASE64 */
  {"debase64",STRING_SCALAR, IGNORE_FIRST, LINE_MODE, STRING_RESULT},
  /* OP_MD5 */
  {"md5",     STRING_SCALAR, IGNORE_FIRST, LINE_MODE, STRING_RESULT},
  /* OP_SHA1 */
  {"sha1",    STRING_SCALAR, IGNORE_FIRST, LINE_MODE, STRING_RESULT},
  /* OP_SHA256 */
  {"sha256",  STRING_SCALAR, IGNORE_FIRST, LINE_MODE, STRING_RESULT},
  /* OP_SHA512 */
  {"sha512",  STRING_SCALAR, IGNORE_FIRST, LINE_MODE, STRING_RESULT},
  {NULL, 0, 0, UNKNOWN_MODE, NUMERIC_RESULT}
};

const char* operation_mode_name[] = {
  "(unknown)",   /* UNKNOWN_MODE */
  "grouping",    /* GROUPING_MODE */
  "transpose",   /* TRANSPOSE_MODE */
  "reverse",     /* REVERSE_FIELD_MODE */
  "line"         /* LINE_MODE */
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
      op->values = xnrealloc (op->values, op->alloc_values,
                              sizeof (long double));
    }
  op->values[op->num_values] = val;
  op->num_values++;
}

static void
field_op_reserve_out_buf (struct fieldop *op, const size_t minsize)
{
  if (op->out_buf_alloc < minsize)
    {
      op->out_buf = xrealloc (op->out_buf, minsize);
      op->out_buf_alloc = minsize;
    }
}

void
field_op_to_hex ( struct fieldop* op, const char *buffer, const size_t inlen )
{
  static const char hex_digits[] =
  {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };
  size_t len = inlen*2+1;
  const char* inp = buffer;
  field_op_reserve_out_buf (op, len);
  char* ptr = op->out_buf;
  for (size_t i = 0 ; i < inlen; ++i)
   {
     *ptr = hex_digits[ ((*inp)>>4) & 0xf ] ;
     ++ptr;
     *ptr = hex_digits[  (*inp)     & 0xf ] ;
     ++ptr;
     ++inp;
   }
  *ptr = 0 ;
}

/* Add a string to the strings vector, allocating memory as needed */
void
field_op_add_string (struct fieldop *op, const char* str, size_t slen)
{
  if (op->str_buf_used + slen+1 >= op->str_buf_alloc)
    {
      op->str_buf_alloc += MAX (VALUES_BATCH_INCREMENT,slen+1);
      op->str_buf = xrealloc (op->str_buf, op->str_buf_alloc);
    }

  /* Copy the string to the buffer */
  memcpy (op->str_buf + op->str_buf_used, str, slen);
  *(op->str_buf + op->str_buf_used + slen ) = 0;
  op->str_buf_used += slen + 1 ;
}

/* Replace the current string in the string buffer.
   This function assumes only one string is stored in the buffer. */
void
field_op_replace_string (struct fieldop *op, const char* str, size_t slen)
{
  if (slen+1 >= op->str_buf_alloc)
    {
      op->str_buf_alloc += MAX (VALUES_BATCH_INCREMENT,slen+1);
      op->str_buf = xrealloc (op->str_buf, op->str_buf_alloc);
    }

  /* Copy the string to the buffer */
  memcpy (op->str_buf, str, slen);
  *(op->str_buf + slen ) = 0;
  op->str_buf_used = slen + 1 ;
}

/* Returns an array of string-pointers (char*),
   each pointing to a string in the string buffer
   (added by field_op_add_string () ).

   The returned pointer must be free'd.

   The returned pointer will have 'op->count+1' elements,
   pointing to 'op->count' strings + one last NULL.
*/
const char **
field_op_get_string_ptrs ( struct fieldop *op, bool sort,
                           bool sort_case_sensitive )
{
  const char **ptrs = xnmalloc (op->count+1, sizeof (char*));
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
      qsort ( ptrs, op->count, sizeof (char*), sort_case_sensitive
                                            ?cmpstringp
                                            :cmpstringp_nocase);
    }
  return ptrs;
}

/* Sort the numeric values vector in a fieldop structure */
void
field_op_sort_values (struct fieldop *op)
{
  qsortfl (op->values, op->num_values);
}

/* Allocate a new fieldop, initialize it based on 'oper',
   and add it to the linked-list of operations */
struct fieldop *
new_field_op (enum operation oper, size_t field)
{
  struct fieldop *op = XZALLOC (struct fieldop);

  op->op = oper;
  op->acc_type = operations[oper].acc_type;
  op->res_type = operations[oper].res_type;
  op->name = operations[oper].name;
  op->numeric = (op->acc_type == NUMERIC_SCALAR
                 || op->acc_type == NUMERIC_VECTOR);
  op->auto_first = operations[oper].auto_first;

  op->field = field;
  op->first = true;
  op->value = 0;
  op->count = 0;

  op->next = NULL;

  if (op->res_type == STRING_RESULT)
    {
      op->out_buf_alloc = 1024;
      op->out_buf = xmalloc (op->out_buf_alloc);
    }
  op->out_buf_used = 0 ;

  if (field_ops != NULL)
    {
      struct fieldop *p = field_ops;
      while (p->next != NULL)
        p = p->next;
      p->next = op;
    }
  else
    field_ops = op;

  return op;
}

/* Add a value (from input) to the current field operation. */
bool
field_op_collect (struct fieldop *op,
                  const char* str, size_t slen)
{
  char *endptr=NULL;
  long double num_value = 0;
#ifdef HAVE_BROKEN_STRTOLD
  char tmpbuf[512];
#endif

  assert (str != NULL); /* LCOV_EXCL_LINE */

  op->count++;

  if (op->numeric)
    {
      errno = 0;
#ifdef HAVE_BROKEN_STRTOLD
      /* On Cygwin, strtold doesn't stop at a tab character,
         and returns invalid value.
         Make a copy of the input buffer and NULL-terminate it */
      if (slen >= sizeof (tmpbuf))
        error (EXIT_FAILURE, 0,
                "internal error: input field too long (%zu)", slen);
      memcpy (tmpbuf,str,slen);
      tmpbuf[slen]=0;
      num_value = strtold (tmpbuf, &endptr);
      if (errno==ERANGE || endptr==tmpbuf || endptr!=(tmpbuf+slen))
        return false;
#else
      if (slen == 0)
        return false;
      num_value = strtold (str, &endptr);
      if (errno==ERANGE || endptr==str || endptr!=(str+slen))
        return false;
#endif
    }

  if (op->first && op->auto_first && op->numeric)
      op->value = num_value;

  switch (op->op)
    {
    case OP_SUM:
    case OP_MEAN:
      op->value += num_value;
      break;

    case OP_COUNT:
      op->value++;
      break;

    case OP_MIN:
      if (num_value < op->value)
        {
          op->value = num_value;
        }
      break;

    case OP_MAX:
      if (num_value > op->value)
        {
          op->value = num_value;
        }
      break;

    case OP_ABSMIN:
      if (fabsl (num_value) < fabsl (op->value))
        {
          op->value = num_value;
        }
      break;

    case OP_ABSMAX:
      if (fabsl (num_value) > fabsl (op->value))
        {
          op->value = num_value;
        }
      break;

    case OP_FIRST:
      if (op->first)
        field_op_replace_string (op, str, slen);
      break;

    case OP_DEBASE64:
    case OP_BASE64:
    case OP_MD5:
    case OP_SHA1:
    case OP_SHA256:
    case OP_SHA512:
    case OP_LAST:
      /* Replace the 'current' string with the latest one */
      field_op_replace_string (op, str, slen);
      break;

    case OP_RAND:
      {
        /* Reservoir sampling,
           With a simpler case were "k=1" */
        unsigned long i = random ()%op->count;
        if (op->first || i==0)
          field_op_replace_string (op, str, slen);
      }
      break;

    case OP_MEDIAN:
    case OP_QUARTILE_1:
    case OP_QUARTILE_3:
    case OP_IQR:
    case OP_PSTDEV:
    case OP_SSTDEV:
    case OP_PVARIANCE:
    case OP_SVARIANCE:
    case OP_MAD:
    case OP_MADRAW:
    case OP_S_SKEWNESS:
    case OP_P_SKEWNESS:
    case OP_S_EXCESS_KURTOSIS:
    case OP_P_EXCESS_KURTOSIS:
    case OP_JARQUE_BERA:
    case OP_DP_OMNIBUS:
    case OP_MODE:
    case OP_ANTIMODE:
      field_op_add_value (op, num_value);
      break;

    case OP_UNIQUE:
    case OP_COLLAPSE:
    case OP_COUNT_UNIQUE:
      field_op_add_string (op, str, slen);
      break;

    case OP_REVERSE:
    case OP_TRANSPOSE:
    default:
      /* Should never happen */
      internal_error ("bad op");     /* LCOV_EXCL_LINE */
    }

  if (op->first)
    op->first = false;

  return true;
}

/* Returns a nul-terimated string, composed of the unique values
   of the input strings. The return string must be free'd. */
void
unique_value ( struct fieldop *op, bool case_sensitive )
{
  const char *last_str;
  char *pos;

  const char **ptrs = field_op_get_string_ptrs (op, true, case_sensitive);

  /* Uniquify them */
  field_op_reserve_out_buf (op, op->str_buf_used);
  pos = op->out_buf ;

  /* Copy the first string */
  last_str = ptrs[0];
  strcpy (pos, ptrs[0]);
  pos += strlen (ptrs[0]);

  /* Copy the following strings, if they are different from the previous one */
  for (size_t i = 1; i < op->count; ++i)
    {
      const char *newstr = ptrs[i];

      if ((case_sensitive && (!STREQ (newstr, last_str)))
          || (!case_sensitive && (strcasecmp (newstr, last_str)!=0)))
        {
          *pos++ = collapse_separator ;
          strcpy (pos, newstr);
          pos += strlen (newstr);
        }
      last_str = newstr;
    }

  free (ptrs);
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
      if ((case_sensitive && (!STREQ (*cur_str, last_str)))
          || (!case_sensitive && (strcasecmp (*cur_str, last_str)!=0)))
        {
          ++count;
        }
      last_str = *cur_str;
      ++cur_str;
    }

  free (ptrs);

  return count;
}

/* Returns a nul-terimated string, composed of all the values
   of the input strings. The return string must be free'd. */
void
collapse_value ( struct fieldop *op )
{
  /* Copy the string buffer as-is */
  field_op_reserve_out_buf (op, op->str_buf_used);
  char *buf = op->out_buf;
  memcpy (buf, op->str_buf, op->str_buf_used);

  /* convert every NUL to comma, except for the last one */
  for (size_t i=0; i < op->str_buf_used-1 ; i++)
      if (buf[i] == 0)
        buf[i] = collapse_separator ;
}

/* Prints to stdout the result of the field operation,
   based on collected values */
void
field_op_summarize (struct fieldop *op)
{
  long double numeric_result = 0 ;
  char tmpbuf[64]; /* 64 bytes - enough to hold sha512 */

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

    case OP_FIRST:
    case OP_LAST:
    case OP_RAND:
      /* Only one string is returned in the buffer, return it */
      field_op_reserve_out_buf (op, op->str_buf_used);
      memcpy (op->out_buf, op->str_buf, op->str_buf_used);
      break;

    case OP_MEDIAN:
      field_op_sort_values (op);
      numeric_result = median_value ( op->values, op->num_values );
      break;

    case OP_QUARTILE_1:
      field_op_sort_values (op);
      numeric_result = quartile1_value ( op->values, op->num_values );
      break;

    case OP_QUARTILE_3:
      field_op_sort_values (op);
      numeric_result = quartile3_value ( op->values, op->num_values );
      break;

    case OP_IQR:
      field_op_sort_values (op);
      numeric_result = quartile3_value ( op->values, op->num_values )
                       - quartile1_value ( op->values, op->num_values );
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

    case OP_MAD:
      field_op_sort_values (op);
      numeric_result = mad_value ( op->values, op->num_values, 1.4826 );
      break;

    case OP_MADRAW:
      field_op_sort_values (op);
      numeric_result = mad_value ( op->values, op->num_values, 1.0 );
      break;

    case OP_S_SKEWNESS:
      numeric_result = skewness_value ( op->values, op->num_values,
                                        DF_SAMPLE );
      break;

    case OP_P_SKEWNESS:
      numeric_result = skewness_value ( op->values, op->num_values,
                                        DF_POPULATION );
      break;

    case OP_S_EXCESS_KURTOSIS:
      numeric_result = excess_kurtosis_value ( op->values, op->num_values,
                                               DF_SAMPLE );
      break;

    case OP_P_EXCESS_KURTOSIS:
      numeric_result = excess_kurtosis_value ( op->values, op->num_values,
                                               DF_POPULATION );
      break;

    case OP_JARQUE_BERA:
      numeric_result = jarque_bera_pvalue ( op->values, op->num_values );
      break;

    case OP_DP_OMNIBUS:
      numeric_result = dagostino_pearson_omnibus_pvalue ( op->values,
                                                          op->num_values );
      break;

    case OP_MODE:
    case OP_ANTIMODE:
      field_op_sort_values (op);
      numeric_result = mode_value ( op->values, op->num_values,
                                    (op->op==OP_MODE)?MODE:ANTIMODE);
      break;

    case OP_UNIQUE:
      unique_value (op, case_sensitive);
      break;

    case OP_COLLAPSE:
      collapse_value (op);
      break;

    case OP_COUNT_UNIQUE:
      numeric_result = count_unique_values (op,case_sensitive);
      break;

    case OP_BASE64:
      field_op_reserve_out_buf (op, BASE64_LENGTH (op->str_buf_used-1)+1 ) ;
      base64_encode ( op->str_buf, op->str_buf_used-1,
		      op->out_buf, BASE64_LENGTH (op->str_buf_used-1)+1 );
      break;

    case OP_DEBASE64:
      {
	size_t decoded_size = op->str_buf_used ;
        field_op_reserve_out_buf (op, decoded_size);
	if (!base64_decode ( op->str_buf, op->str_buf_used-1,
			op->out_buf, &decoded_size ))
		error (EXIT_FAILURE, 0, _("base64 decoding failed"));
	op->out_buf[decoded_size]=0;
      }
      break;

    case OP_MD5:
      md5_buffer (op->str_buf, op->str_buf_used-1, tmpbuf);
      field_op_to_hex (op, tmpbuf, 16);
      break;

    case OP_SHA1:
      sha1_buffer (op->str_buf, op->str_buf_used-1, tmpbuf);
      field_op_to_hex (op, tmpbuf, 20);
      break;

    case OP_SHA256:
      sha256_buffer (op->str_buf, op->str_buf_used-1, tmpbuf);
      field_op_to_hex (op, tmpbuf, 32);
      break;

    case OP_SHA512:
      sha512_buffer (op->str_buf, op->str_buf_used-1, tmpbuf);
      field_op_to_hex (op, tmpbuf, 64);
      break;

    case OP_TRANSPOSE: /* not handled here */
    case OP_REVERSE:   /* not handled here */
    default:
      /* Should never happen */
      internal_error ("bad op");     /* LCOV_EXCL_LINE */
    }

  if (op->res_type==NUMERIC_RESULT)
    printf ("%.*Lg", field_op_output_precision, numeric_result);
  else
    printf ("%s", op->out_buf);
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
  op->out_buf_used = 0;
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

  free (op->str_buf);
  op->str_buf = NULL;
  op->str_buf_alloc = 0;
  op->str_buf_used = 0;

  free (op->out_buf);
  op->out_buf = NULL;
  op->out_buf_alloc = 0;
  op->out_buf_used = 0;

  free (op);
}

void
free_field_ops ()
{
  struct fieldop *p = field_ops;
  while (p)
    {
      struct fieldop *n = p->next;
      free_field_op (p);
      p = n;
    }
}

/* Given a string with operation name, returns the operation enum.
   exits with an error message if the string is not a valid/known operation. */
enum operation
get_operation (const char* keyword)
{
  for (size_t i = 0; operations[i].name ; i++)
      if ( STREQ (operations[i].name, keyword) )
        return (enum operation)i;

  error (EXIT_FAILURE, 0, _("invalid operation '%s'"), keyword);
  return 0; /* never reached LCOV_EXCL_LINE */
}

/* Converts a string to number (field number).
   Exits with an error message (using 'op') on invalid field number. */
static size_t
safe_get_field_number (enum operation op, const char* field_str)
{
  long int val;
  char *endptr;
  errno = 0 ;
  val = strtol (field_str, &endptr, 10);
  /* NOTE: can't use xstrtol_fatal - it's too tightly-coupled
     with getopt command-line processing */
  if (errno != 0 || endptr == field_str || val < 1
      || endptr == NULL || *endptr != 0)
    error (EXIT_FAILURE, 0, _("invalid column '%s' for operation " \
                               "'%s'"), field_str,
                               operations[op].name);
  return (size_t)val;
}

/* Extract the operation patterns from args START through ARGC - 1 of ARGV. */
void
parse_operations (enum operation_mode mode,
	int argc, int start, char **argv)
{
  int i = start;	/* Index into ARGV. */
  size_t field;
  enum operation op;

  /* From here on, by default we assume it's a "groupby" operation */
  while ( i < argc )
    {
      op = get_operation (argv[i]);
      if (operations[op].mode != mode)
	error (EXIT_FAILURE, 0, _("conflicting operation found: "\
		"expecting %s operations, but found %s operation %s"),
		operation_mode_name[mode],
		operation_mode_name[operations[op].mode],
		quote (operations[op].name));

      i++;
      if ( i >= argc )
        error (EXIT_FAILURE, 0, _("missing field number after " \
                                  "operation '%s'"), argv[i-1] );
      field = safe_get_field_number (op, argv[i]);
      i++;

      new_field_op (op, field);
    }
}

enum operation_mode
parse_operation_mode (int argc, int start, char** argv)
{
  assert (start < argc); /* LCOV_EXCL_LINE */

  const enum operation op = get_operation ( argv[start] );
  const enum operation_mode om = operations[op].mode;
  switch (om)
    {
    case TRANSPOSE_MODE:
    case REVERSE_FIELD_MODE:
      if ( start+1 < argc )
	error (EXIT_FAILURE, 0, _("extra operands after '%s'"), argv[start]);
      break;

    case LINE_MODE:
    case GROUPING_MODE:
      /* parse individual operations */
      parse_operations (om, argc, start, argv);
      break;

    case UNKNOWN_MODE:
    default:
      internal_error ("unknown mode"); /* LCOV_EXCL_LINE */
    }

  return om;
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

/* long mix function, from:
   Robert Jenkins' 96 bit Mix Function
   http://burtleburtle.net/bob/hash/doobs.html */
static unsigned long
mix (unsigned long a, unsigned long b, unsigned long c)
{
    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);
    return c;
}

void
init_random (void)
{
  unsigned long seed = mix (clock (), time (NULL), getpid ());
  srandom (seed);
}

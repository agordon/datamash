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
#include <config.h>
#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <libgen.h> /* for dirname & POSIX version of basename */

#include "die.h"
#include "minmax.h"
#include "linebuffer.h"
#include "system.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "sha512.h"
#include "base64.h"
#include "xalloc.h"
#include "hash-pjw-bare.h"

#include "utils.h"
#include "text-options.h"
#include "text-lines.h"
#include "column-headers.h"
#include "op-defs.h"
#include "field-ops.h"

struct operation_data operations[] =
{
  /* OP_COUNT */
  {STRING_SCALAR,  IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_SUM */
  {NUMERIC_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_MIN */
  {NUMERIC_SCALAR, AUTO_SET_FIRST, NUMERIC_RESULT},
  /* OP_MAX */
  {NUMERIC_SCALAR, AUTO_SET_FIRST, NUMERIC_RESULT},
  /* OP_ABSMIN */
  {NUMERIC_SCALAR, AUTO_SET_FIRST, NUMERIC_RESULT},
  /* OP_ABSMAX */
  {NUMERIC_SCALAR, AUTO_SET_FIRST, NUMERIC_RESULT},
  /* OP_RANGE */
  {NUMERIC_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_FIRST */
  {STRING_SCALAR,  IGNORE_FIRST, STRING_RESULT},
  /* OP_LAST */
  {STRING_SCALAR,  IGNORE_FIRST, STRING_RESULT},
  /* OP_RAND */
  {STRING_SCALAR,  IGNORE_FIRST, STRING_RESULT},
  /* OP_MEAN */
  {NUMERIC_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_MEDIAN */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_QUARTILE_1 */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_QUARTILE_3 */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_IQR */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_PERCENTILE */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_PSTDEV */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_SSTDEV */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_PVARIANCE */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_SVARIANCE */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_MAD */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_MADRAW */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_S_SKEWNESS */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_P_SKEWNESS */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_S_EXCESS_KURTOSIS */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_P_EXCESS_KURTOSIS */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_JARQUE_BETA */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_DP_OMNIBUS */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_MODE */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_ANTIMODE */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_UNIQUE */
  {STRING_VECTOR,  IGNORE_FIRST, STRING_RESULT},
  /* OP_COLLAPSE */
  {STRING_VECTOR,  IGNORE_FIRST, STRING_RESULT},
  /* OP_COUNT_UNIQUE */
  {STRING_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_BASE64 */
  {STRING_SCALAR, IGNORE_FIRST, STRING_RESULT},
  /* OP_DEBASE64 */
  {STRING_SCALAR, IGNORE_FIRST, STRING_RESULT},
  /* OP_MD5 */
  {STRING_SCALAR, IGNORE_FIRST, STRING_RESULT},
  /* OP_SHA1 */
  {STRING_SCALAR, IGNORE_FIRST, STRING_RESULT},
  /* OP_SHA256 */
  {STRING_SCALAR, IGNORE_FIRST, STRING_RESULT},
  /* OP_SHA512 */
  {STRING_SCALAR, IGNORE_FIRST, STRING_RESULT},
  /* OP_P_COVARIANCE */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_S_COVARIANCE */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_P_PEARSON_COR */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_S_PEARSON_COR */
  {NUMERIC_VECTOR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_BIN_BUCKETS */
  {NUMERIC_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_STRBIN */
  {STRING_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_FLOOR */
  {NUMERIC_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_CEIL */
  {NUMERIC_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_ROUND */
  {NUMERIC_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_TRUNCATE */
  {NUMERIC_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_FRACTION */
  {NUMERIC_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_TRIMMED_MEAN */
  {NUMERIC_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_DIRNAME */
  {STRING_SCALAR, IGNORE_FIRST, STRING_RESULT},
  /* OP_BASENAME */
  {STRING_SCALAR, IGNORE_FIRST, STRING_RESULT},
  /* OP_EXTNAME */
  {STRING_SCALAR, IGNORE_FIRST, STRING_RESULT},
  /* OP_BARENAME */
  {STRING_SCALAR, IGNORE_FIRST, STRING_RESULT},
  /* OP_GETNUM */
  {STRING_SCALAR, IGNORE_FIRST, NUMERIC_RESULT},
  /* OP_CUT */
  {STRING_SCALAR, IGNORE_FIRST, STRING_RESULT},
  {0, 0, NUMERIC_RESULT}
};

//struct fieldop* field_ops = NULL;

enum { VALUES_BATCH_INCREMENT = 1024 };

/* Add a numeric value to the values vector, allocating memory as needed */
static void
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

/* stores the hexadecimal representation of 'buffer' in op->out_buf */
static void
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
static const char **
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
static void
field_op_sort_values (struct fieldop *op)
{
  qsortfl (op->values, op->num_values);
}

void
field_op_init (struct fieldop* /*out*/ op,
               enum field_operation oper,
               bool by_name, size_t num, const char* name)
{
  assert (op != NULL); /* LCOV_EXCL_LINE */
  memset (op, 0, sizeof *op);

  op->op = oper;
  op->acc_type = operations[oper].acc_type;
  op->res_type = operations[oper].res_type;
  op->numeric = (op->acc_type == NUMERIC_SCALAR
                 || op->acc_type == NUMERIC_VECTOR);
  op->auto_first = operations[oper].auto_first;
  op->slave = false;
  op->slave_op = NULL;

  op->field = num;
  op->field_by_name = by_name;
  op->field_name = (by_name)?xstrdup (name):NULL;
  op->first = true;
  if (op->res_type == STRING_RESULT)
    {
      op->out_buf_alloc = 1024;
      op->out_buf = xmalloc (op->out_buf_alloc);
    }
}

/* Ensure this (master) fieldop has the same number of values as
   as it's slave fieldop. */
static void
verify_slave_num_values (const struct fieldop *op)
{
  assert (op && !op->slave && op->slave_op);     /* LCOV_EXCL_LINE */

  if (op->num_values != op->slave_op->num_values)
    die (EXIT_FAILURE, 0, _("input error for operation %s: \
fields %"PRIuMAX",%"PRIuMAX" have different number of items"),
                            quote (get_field_operation_name (op->op)),
                            (uintmax_t)op->slave_op->field,
                            (uintmax_t)op->field);
}

/* Add a value (from input) to the current field operation. */
enum FIELD_OP_COLLECT_RESULT
field_op_collect (struct fieldop *op,
                  const char* str, size_t slen)
{
  char *endptr=NULL;
  long double num_value = 0;
#ifdef HAVE_BROKEN_STRTOLD
  char tmpbuf[512];
#endif
  enum FIELD_OP_COLLECT_RESULT rc = FLOCR_OK;

  assert (str != NULL); /* LCOV_EXCL_LINE */

  if (remove_na_values && is_na (str,slen))
    return FLOCR_OK_SKIPPED;

  if (op->numeric)
    {
      errno = 0;
#ifdef HAVE_BROKEN_STRTOLD
      /* On Cygwin, strtold doesn't stop at a tab character,
         and returns invalid value.
         Make a copy of the input buffer and NULL-terminate it */
      if (slen >= sizeof (tmpbuf))
        die (EXIT_FAILURE, 0,
                "internal error: input field too long (%zu)", slen);
      memcpy (tmpbuf,str,slen);
      tmpbuf[slen]=0;
      num_value = strtold (tmpbuf, &endptr);
      if (errno==ERANGE || endptr==tmpbuf || endptr!=(tmpbuf+slen))
        return FLOCR_INVALID_NUMBER;
#else
      if (slen == 0)
        return FLOCR_INVALID_NUMBER;
      num_value = strtold (str, &endptr);
      if (errno==ERANGE || endptr==str || endptr!=(str+slen))
        return FLOCR_INVALID_NUMBER;
#endif
    }

  op->count++;

  if (op->first && op->auto_first && op->numeric)
      op->value = num_value;

  switch (op->op)                                /* LCOV_EXCL_BR_LINE */
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
          rc = FLOCR_OK_KEEP_LINE;
        }
      break;

    case OP_MAX:
      if (num_value > op->value)
        {
          op->value = num_value;
          rc = FLOCR_OK_KEEP_LINE;
        }
      break;

    case OP_ABSMIN:
      if (fabsl (num_value) < fabsl (op->value))
        {
          op->value = num_value;
          rc = FLOCR_OK_KEEP_LINE;
        }
      break;

    case OP_ABSMAX:
      if (fabsl (num_value) > fabsl (op->value))
        {
          op->value = num_value;
          rc = FLOCR_OK_KEEP_LINE;
        }
      break;

    case OP_RANGE:
      /* Upon the first value, we store it twice
         (once for min, once for max).
         For subsequence values, we update the min/max entries directly. */
      if (op->first)
        {
          field_op_add_value (op, num_value);
          field_op_add_value (op, num_value);
        }
      else
        {
          if (num_value < op->values[0])
            op->values[0] = num_value;
          if (num_value > op->values[1])
            op->values[1] = num_value;
        }
      break;

    case OP_FIRST:
      if (op->first)
        {
          field_op_replace_string (op, str, slen);
          rc = FLOCR_OK_KEEP_LINE;
        }
      break;

    case OP_LAST:
      /* Replace the 'current' string with the latest one */
      field_op_replace_string (op, str, slen);
      rc = FLOCR_OK_KEEP_LINE;
      break;

    case OP_DEBASE64:
      /* Base64 decoding is a special case: we decode during collection,
         and report any errors back to the caller. */
      {
        /* safe to assume decoded base64 is never larger than encoded base64 */
        size_t decoded_size = slen;
        field_op_reserve_out_buf (op, decoded_size);
        if (!base64_decode ( str, slen, op->out_buf, &decoded_size ))
          return FLOCR_INVALID_BASE64;
        op->out_buf[decoded_size]=0;
      }
      break;

    case OP_BASE64:
    case OP_MD5:
    case OP_SHA1:
    case OP_SHA256:
    case OP_SHA512:
    case OP_DIRNAME:
    case OP_BASENAME:
    case OP_EXTNAME:
    case OP_BARENAME:
      /* Replace the 'current' string with the latest one */
      field_op_replace_string (op, str, slen);
      break;

    case OP_RAND:
      {
        /* Reservoir sampling,
           With a simpler case were "k=1" */
        unsigned long i = random ()%op->count;
        if (op->first || i==0)
          {
            field_op_replace_string (op, str, slen);
            rc = FLOCR_OK_KEEP_LINE;
          }
      }
      break;

    case OP_MEDIAN:
    case OP_QUARTILE_1:
    case OP_QUARTILE_3:
    case OP_IQR:
    case OP_PERCENTILE:
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
    case OP_P_COVARIANCE:
    case OP_S_COVARIANCE:
    case OP_P_PEARSON_COR:
    case OP_S_PEARSON_COR:
    case OP_TRIMMED_MEAN:
      field_op_add_value (op, num_value);
      break;

    case OP_UNIQUE:
    case OP_COLLAPSE:
    case OP_COUNT_UNIQUE:
      field_op_add_string (op, str, slen);
      break;

    case OP_BIN_BUCKETS:
      {
        const long double val = num_value / op->params.bin_bucket_size;
        modfl (val, & op->value);
        /* signbit will take care of negative-zero as well. */
        if (signbit (op->value))
          --op->value;
        op->value *= op->params.bin_bucket_size;
      }
      break;

    case OP_STRBIN:
      op->value = hash_pjw_bare (str,slen) % (op->params.strbin_bucket_size);
      break;

    case OP_FLOOR:
      op->value = pos_zero (floorl (num_value));
      break;

    case OP_CEIL:
      op->value = pos_zero (ceill (num_value));
      break;

    case OP_ROUND:
      op->value = pos_zero (roundl (num_value));
      break;

    case OP_TRUNCATE:
      modfl (num_value, &op->value);
      op->value = pos_zero (op->value);
      break;

    case OP_FRACTION:
      {
        long double dummy;
        op->value = pos_zero (modfl (num_value, &dummy));
      };
      break;

    case OP_GETNUM:
      op->value = extract_number (str, slen, op->params.get_num_type);
      break;

    case OP_CUT:
      field_op_replace_string (op, str, slen);
      break;

    case OP_INVALID:                 /* LCOV_EXCL_LINE */
    default:                         /* LCOV_EXCL_LINE */
      /* Should never happen */
      internal_error ("bad op");     /* LCOV_EXCL_LINE */
    }

  op->first = false;

  return rc;
}

/* creates a list of unique strings from op->str_buf .
   results are stored in op->out_buf. */
static void
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

/* stores in op->out_buf the result of the field operation
   when there are no input values.
   'no values' can happen with '--narm' and input of all N/As.
   The printed results are consistent as much as possible with R */
static void
field_op_summarize_empty (struct fieldop *op)
{
  long double numeric_result = 0 ;

  switch (op->op)                                /* LCOV_EXCL_BR_LINE */
    {
    case OP_MEAN:
    case OP_S_SKEWNESS:
    case OP_P_SKEWNESS:
    case OP_S_EXCESS_KURTOSIS:
    case OP_P_EXCESS_KURTOSIS:
    case OP_JARQUE_BERA:
    case OP_DP_OMNIBUS:
    case OP_MEDIAN:
    case OP_QUARTILE_1:
    case OP_QUARTILE_3:
    case OP_IQR:
    case OP_PERCENTILE:
    case OP_MAD:
    case OP_MADRAW:
    case OP_PSTDEV:
    case OP_SSTDEV:
    case OP_PVARIANCE:
    case OP_SVARIANCE:
    case OP_MODE:
    case OP_ANTIMODE:
    case OP_P_COVARIANCE:
    case OP_S_COVARIANCE:
    case OP_P_PEARSON_COR:
    case OP_S_PEARSON_COR:
    case OP_BIN_BUCKETS:
    case OP_STRBIN:
    case OP_FLOOR:
    case OP_CEIL:
    case OP_ROUND:
    case OP_TRUNCATE:
    case OP_FRACTION:
    case OP_RANGE:
    case OP_TRIMMED_MEAN:
    case OP_GETNUM:
      numeric_result = nanl ("");
      break;

    case OP_SUM:
    case OP_COUNT:
    case OP_COUNT_UNIQUE:
      numeric_result = 0;
      break;

    case OP_MIN:
    case OP_ABSMIN:
      numeric_result = -HUGE_VALL;
      break;

    case OP_MAX:
    case OP_ABSMAX:
      numeric_result = HUGE_VALL;
      break;

    case OP_FIRST:
    case OP_LAST:
    case OP_RAND:
    case OP_CUT:
      field_op_reserve_out_buf (op, 4);
      strcpy (op->out_buf, "N/A");
      break;

    case OP_UNIQUE:
    case OP_COLLAPSE:
    case OP_BASE64:
    case OP_DEBASE64:
    case OP_MD5:
    case OP_SHA1:
    case OP_SHA256:
    case OP_SHA512:
    case OP_DIRNAME:
    case OP_BASENAME:
    case OP_EXTNAME:
    case OP_BARENAME:
      field_op_reserve_out_buf (op, 1);
      strcpy (op->out_buf, "");
      break;

    case OP_INVALID:                 /* LCOV_EXCL_LINE */
    default:                         /* LCOV_EXCL_LINE */
      /* Should never happen */
      internal_error ("bad op");     /* LCOV_EXCL_LINE */
    }

  if (op->res_type==NUMERIC_RESULT)
    {
      field_op_reserve_out_buf (op, numeric_output_bufsize);
      snprintf (op->out_buf, op->out_buf_alloc,
                numeric_output_format, numeric_result);
    }
}

/* Prints to stdout the result of the field operation,
   based on collected values */
void
field_op_summarize (struct fieldop *op)
{
  long double numeric_result = 0 ;
  char tmpbuf[64]; /* 64 bytes - enough to hold sha512 */

  /* In case of no values, each operation returns a specific result.
     'no values' can happen with '--narm' and input of all N/As. */
  if (op->count==0)
    {
      field_op_summarize_empty (op);
      return ;
    }

  switch (op->op)                                /* LCOV_EXCL_BR_LINE */
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
    case OP_BIN_BUCKETS:
    case OP_STRBIN:
    case OP_FLOOR:
    case OP_CEIL:
    case OP_ROUND:
    case OP_TRUNCATE:
    case OP_FRACTION:
    case OP_GETNUM:
      /* no summarization for these operations, just print the value */
      numeric_result = op->value;
      break;

    case OP_FIRST:
    case OP_LAST:
    case OP_RAND:
    case OP_CUT:
      /* Only one string is returned in the buffer, return it */
      field_op_reserve_out_buf (op, op->str_buf_used);
      memcpy (op->out_buf, op->str_buf, op->str_buf_used);
      break;

    case OP_RANGE:
      numeric_result = op->values[1] - op->values[0];
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

    case OP_PERCENTILE:
      field_op_sort_values (op);
      numeric_result = percentile_value ( op->values, op->num_values,
                                          op->params.percentile / 100.0 );
      break;

    case OP_TRIMMED_MEAN:
      field_op_sort_values (op);
      numeric_result = trimmed_mean_value ( op->values, op->num_values,
					    op->params.trimmed_mean);
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

    case OP_P_COVARIANCE:
    case OP_S_COVARIANCE:
      assert (!op->slave);                       /* LCOV_EXCL_LINE */
      assert (op->slave_op);                     /* LCOV_EXCL_LINE */
      verify_slave_num_values (op);
      numeric_result = covariance_value (op->values, op->slave_op->values,
                                         op->num_values,
                                         (op->op==OP_P_COVARIANCE)?
                                                DF_POPULATION:DF_SAMPLE );
      break;

    case OP_P_PEARSON_COR:
    case OP_S_PEARSON_COR:
      assert (!op->slave);                       /* LCOV_EXCL_LINE */
      assert (op->slave_op);                     /* LCOV_EXCL_LINE */
      verify_slave_num_values (op);
      numeric_result = pearson_corr_value (op->values, op->slave_op->values,
                                           op->num_values,
                                           (op->op==OP_P_PEARSON_COR)?
                                                DF_POPULATION:DF_SAMPLE);
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
      /* Decoding base64 is a special case: decoding (and error checking) was
         done in field_op_collect.  op->out_buf already contains the decoded
         value. */
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

    case OP_DIRNAME:
      {
        op->str_buf[op->str_buf_used] = 0;
        char *t = dirname (op->str_buf);
        field_op_reserve_out_buf (op, op->str_buf_used);
        strcpy (op->out_buf,t);
      }
      break;

    case OP_BASENAME:
    case OP_EXTNAME:
    case OP_BARENAME:
      {
        if (op->str_buf_used==1)
          {
            /* Empty string, containing only NUL */
            field_op_reserve_out_buf (op, 1);
            op->out_buf[0] = '\0';
            break;
          }

        op->str_buf[op->str_buf_used] = 0;
        char *t = basename (op->str_buf);
        field_op_reserve_out_buf (op, op->str_buf_used);

        if (op->op == OP_BASENAME)
          {
            /* Just copy the extracted base name */
            strcpy (op->out_buf,t);
          }
        else
          {
            /* Guess the file extension */
            size_t tl = strlen (t);
            size_t l = guess_file_extension (t, tl);

            if (op->op == OP_EXTNAME)
              {
                /* Store the extension */
                if (l>0)
                  {
                    memcpy (op->out_buf, t+(tl-l+1), l-1);
                    op->out_buf[l-1] = '\0';
                  }
                else
                  {
                    op->out_buf[0] = '\0';
                  }
              }
            else
              {
                /* Store the basename without the extension */
                memcpy (op->out_buf, t, tl-l);
                op->out_buf[tl-l] = '\0';
              }
          }
      }
      break;

    case OP_INVALID:                 /* LCOV_EXCL_LINE */
    default:                         /* LCOV_EXCL_LINE */
      /* Should never happen */
      internal_error ("bad op");     /* LCOV_EXCL_LINE */
    }

  if (op->res_type==NUMERIC_RESULT)
    {
      field_op_reserve_out_buf (op, numeric_output_bufsize);
      snprintf (op->out_buf, op->out_buf_alloc,
                numeric_output_format, numeric_result);
    }
}

/* reset operation values for next group */
void
field_op_reset (struct fieldop *op)
{
  op->first = true;
  op->count = 0 ;
  op->value = 0;
  op->num_values = 0 ;
  op->str_buf_used = 0;
  op->out_buf_used = 0;
  /* note: op->str_buf and op->str_alloc are not free'd, and reused */
}

void
field_op_free (struct fieldop* op)
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

  free (op->field_name);
  op->field_name = NULL;
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

const char*
field_op_collect_result_name (const enum FIELD_OP_COLLECT_RESULT flocr)
{
  switch (flocr)                                 /* LCOV_EXCL_BR_LINE */
   {
   case FLOCR_INVALID_NUMBER:
     return _("invalid numeric value");
   case FLOCR_INVALID_BASE64:
     return _("invalid base64 value");
   case FLOCR_OK:                                /* LCOV_EXCL_LINE */
   case FLOCR_OK_KEEP_LINE:                      /* LCOV_EXCL_LINE */
   case FLOCR_OK_SKIPPED:                        /* LCOV_EXCL_LINE */
   default:
     internal_error ("op_collect_result_name");  /* LCOV_EXCL_LINE */
     return "";                                  /* LCOV_EXCL_LINE */
   }
}

void
field_op_print_empty_value (enum field_operation mode)
{
  struct fieldop op;
  memset (&op, 0, sizeof op);
  op.op = mode;
  op.res_type = NUMERIC_RESULT;
  field_op_summarize_empty (&op);
  fputs (op.out_buf, stdout);
}

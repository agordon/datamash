/* calc - perform simple calculation on input data
   Copyright (C) 2013 Assaf Gordon.

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

#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>

#include "system.h"

#include "closeout.h"
#include "error.h"
#include "linebuffer.h"
#include "ignore-value.h"
#include "minmax.h"
#include "intprops.h"
#include "quote.h"
#include "size_max.h"
#include "inttostr.h"
#include "version-etc.h"
#include "version.h"
#include "xalloc.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "calc"

#define AUTHORS proper_name ("Assaf Gordon")

/* Until someone better comes along */
const char version_etc_copyright[] = "Copyright %s %d Assaf Gordon" ;

/* enable debugging */
static bool debug = false;

/* The character marking end of line. Default to \n. */
static char eolchar = '\n';

/* The character used to separate collapsed/uniqued strings */
static char collapse_separator = ',';

/* Line number in the input file */
static size_t line_number = 0 ;

/* Lines in the current group */
static size_t lines_in_group = 0 ;

/* Print Output Header */
static bool output_header = false;

/* Input file has a header line */
static bool input_header = false;

/* If true, print the entire input line. Otherwise, print only the key fields */
static bool print_full_line = false;

/* If TAB has this value, blanks separate fields.  */
enum { TAB_DEFAULT = CHAR_MAX + 1 };

/* Tab character separating fields.  If TAB_DEFAULT, then fields are
   separated by the empty string between a non-blank character and a blank
   character. */
static int tab = TAB_DEFAULT;

#define UCHAR_LIM (UCHAR_MAX + 1)
static bool blanks[UCHAR_LIM];

static size_t *group_columns = NULL;
static size_t num_group_colums = 0;

static bool pipe_through_sort = false;
static FILE* pipe_input = NULL;

struct columnheader
{
        char *name;
        struct columnheader *next;
};

static size_t num_input_column_headers = 0 ;
static struct columnheader* input_column_headers = NULL;

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
  OP_UNIQUE_NOCASE, /* Collapse Unique strings, ignoring-case */
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

static inline void print_field_separator()
{
  putchar( (tab==TAB_DEFAULT)?' ':tab );
}

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
  {"uniquenc",STRING_VECTOR,   IGNORE_FIRST},   /* OP_UNIQUE_NOCASE */
  {"collapse",STRING_VECTOR,   IGNORE_FIRST},   /* OP_COLLAPSE */
  {"countunique",STRING_VECTOR,   IGNORE_FIRST},   /* OP_COUNT_UNIQUE */
  {NULL, 0, 0}
};

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

static struct fieldop* field_ops = NULL;

void add_named_input_column_header(const char* buffer, size_t len)
{
  struct columnheader *h = XZALLOC(struct columnheader);
  h->name = xmalloc(len+1);
  bcopy(buffer,h->name,len);
  h->name[len]=0;

  if (input_column_headers != NULL)
    {
      struct columnheader *p = input_column_headers;
      while (p->next != NULL)
        p = p->next;
      p->next = h;
    }
  else
    input_column_headers = h;
}

/* Returns a pointer to the name of the field.
 * DO NOT FREE or reuse the returned buffer.
 * Might return a pointer to a static buffer - not thread safe.*/
const char const * get_input_field_name(size_t field_num)
{
  static char tmp[6+INT_BUFSIZE_BOUND(size_t)+1];
  if (input_column_headers==NULL)
    {
       ignore_value(snprintf(tmp,sizeof(tmp),"field-%zu",field_num));
       return tmp;
    }
  else
    {
       const struct columnheader *h = input_column_headers;
       if (field_num > num_input_column_headers)
         error(EXIT_FAILURE,0,_("Not enough input fields (field %zu requested, input has only %zu"), field_num, num_input_column_headers);

       while (--field_num)
         h = h->next;
       return h->name;
    }
}

enum { VALUES_BATCH_INCREMENT = 1024 };

/* Add a numeric value to the values vector, allocating memory as needed */
static void
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
static void
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

static int
cmpstringp(const void *p1, const void *p2);

static int
cmpstringp_nocase(const void *p1, const void *p2);

/* Returns an array of string-pointers (char*),
   each pointing to a string in the string buffer (added by field_op_add_string() ).

   The returned pointer must be free()'d.

   The returned pointer will have 'op->count+1' elements,
   pointing to 'op->count' strings + one last NULL.
*/
static const char **
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

/* Compare two flowting-point variables, while avoiding '==' .
see:
http://www.gnu.org/software/libc/manual/html_node/Comparison-Functions.html */
static int
cmp_long_double (const void *p1, const void *p2)
{
  const long double *a = (const long double *)p1;
  const long double *b = (const long double *)p2;
  return ( *a > *b ) - (*a < *b);
}

/* Sort the numeric values vector in a fieldop structure */
static void
field_op_sort_values (struct fieldop *op)
{
  qsort (op->values, op->num_values, sizeof (long double), cmp_long_double);
}

/* Allocate a new fieldop, initialize it based on 'oper',
   and add it to the linked-list of operations */
static void
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
static bool
field_op_collect (struct fieldop *op,
                  const char* str, size_t slen,
                  const long double num_value)
{
  bool keep_line = false;

  if (debug)
    {
      fprintf (stderr, "-- collect for %s(%zu) val='", op->name, op->field);
      fwrite (str, sizeof(char), slen, stderr);
      fprintf (stderr, "'\n");
    }

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
    case OP_UNIQUE_NOCASE:
    case OP_COLLAPSE:
    case OP_COUNT_UNIQUE:
      field_op_add_string (op, str, slen);
      break;

    default:
      break;
    }

  if (op->first)
    op->first = false;

  return keep_line;
}

inline static long double
median_value ( const long double * const values, size_t n )
{
  /* Assumes 'values' are already sorted, returns the median value */
  return (n&0x01)
    ?values[n/2]
    :( (values[n/2-1] + values[n/2]) / 2.0 );
}

enum degrees_of_freedom
{
  DF_POPULATION = 0,
  DF_SAMPLE = 1
};

inline static long double
variance_value ( const long double * const values, size_t n, int df )
{
  long double sum=0;
  long double mean;
  long double variance;

  for (size_t i = 0; i < n; i++)
    sum += values[i];
  mean = sum / n ;

  sum = 0 ;
  for (size_t i = 0; i < n; i++)
    sum += (values[i] - mean) * (values[i] - mean);

  variance = sum / ( n - df );

  return variance;
}

inline static long double
stdev_value ( const long double * const values, size_t n, int df )
{
  return sqrtl ( variance_value ( values, n, df ) );
}


enum MODETYPE
{
  MODE=1,
  ANTIMODE
};

inline static long double
mode_value ( const long double * const values, size_t n, enum MODETYPE type)
{
  /* not ideal implementation but simple enough */
  /* Assumes 'values' are already sorted, find the longest sequence */
  long double last_value = values[0];
  size_t seq_size=1;
  size_t best_seq_size= (type==MODE)?1:SIZE_MAX;
  size_t best_value = values[0];

  for (size_t i=1; i<n; i++)
    {
      bool eq = (cmp_long_double(&values[i],&last_value)==0);

      if (eq)
        seq_size++;

      if ( ((type==MODE) && (seq_size > best_seq_size))
           ||
           ((type==ANTIMODE) && (seq_size < best_seq_size)))
        {
          best_seq_size = seq_size;
          best_value = last_value;
        }

      if (!eq)
          seq_size = 1;

      last_value = values[i];
    }
  return best_value;
}

static int
cmpstringp(const void *p1, const void *p2)
{
  /* The actual arguments to this function are "pointers to
   * pointers to char", but strcmp(3) arguments are "pointers
   * to char", hence the following cast plus dereference */
  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

static int
cmpstringp_nocase(const void *p1, const void *p2)
{
  /* The actual arguments to this function are "pointers to
   * pointers to char", but strcmp(3) arguments are "pointers
   * to char", hence the following cast plus dereference */
  return strcasecmp(* (char * const *) p1, * (char * const *) p2);
}



/* Returns a nul-terimated string, composed of the unique values
   of the input strings. The return string must be free()'d. */
static char *
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

      if ((case_sensitive && (strcmp(newstr, last_str)!=0))
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
      if ((case_sensitive && (strcmp(*cur_str, last_str)!=0))
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
static char *
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
static void
field_op_summarize (struct fieldop *op)
{
  bool print_numeric_result = true;
  long double numeric_result = 0 ;
  char *string_result = NULL;

  if (debug)
    fprintf (stderr, "-- summarize for %s(%zu)\n", op->name, op->field);

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
    case OP_UNIQUE_NOCASE:
      print_numeric_result = false;
      string_result = unique_value (op, (op->op==OP_UNIQUE));
      break;

    case OP_COLLAPSE:
      print_numeric_result = false;
      string_result = collapse_value (op);
      break;

   case OP_COUNT_UNIQUE:
      numeric_result = count_unique_values(op,true); //true: for now - always case sensitive
      break;

    default:
      error (EXIT_FAILURE, 0, _("internal error 2"));
      break;
    }


  if (debug)
    {
      if (print_numeric_result)
        fprintf (stderr, "%s(%zu) = %Lg\n", op->name, op->field, numeric_result);
      else
        fprintf (stderr, "%s(%zu) = '%s'\n", op->name, op->field, string_result);
    }

  if (print_numeric_result)
    printf ("%Lg", numeric_result);
  else
    printf ("%s", string_result);

  free (string_result);
}

static void
summarize_field_ops ()
{
  for (struct fieldop *p = field_ops; p ; p=p->next)
    {
      field_op_summarize (p);

      /* print field separator */
      if (p->next)
        putchar( (tab==TAB_DEFAULT)?' ':tab );
    }

    /* print end-of-line */
    putchar(eolchar);
}

/* reset operation values for next group */
static void
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
static void
reset_field_ops ()
{
  for (struct fieldop *p = field_ops; p ; p = p->next)
    reset_field_op (p);
}

/* Frees all memory associated with a field operation struct.
   returns the 'next' field operation, or NULL */
static struct fieldop *
free_field_op (struct fieldop *op)
{
  struct fieldop *next;

  if (!op)
    return NULL;

  next = op->next;

  if (op->values)
      free (op->values);
  op->num_values = 0 ;
  op->alloc_values = 0;

  free(op->str_buf);
  op->str_buf = NULL;
  op->str_buf_alloc = 0;
  op->str_buf_used = 0;

  free(op);

  return next;
}

static void
free_field_ops ()
{
  struct fieldop *p = field_ops;
  while (p)
    p = free_field_op(p);
}

enum
{
  DEBUG_OPTION = CHAR_MAX + 1,
  INPUT_HEADER_OPTION,
  OUTPUT_HEADER_OPTION
};

static char const short_options[] = "sfzg:t:TH";

static struct option const long_options[] =
{
  {"zero-terminated", no_argument, NULL, 'z'},
  {"field-separator", required_argument, NULL, 't'},
  {"groups", required_argument, NULL, 'g'},
  {"header-in", no_argument, NULL, INPUT_HEADER_OPTION},
  {"header-out", no_argument, NULL, OUTPUT_HEADER_OPTION},
  {"headers", no_argument, NULL, 'H'},
  {"full", no_argument, NULL, 'f'},
  {"sort", no_argument, NULL, 's'},
  {"debug", no_argument, NULL, DEBUG_OPTION},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0},
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION] op col [op col ...]\n\
"),
              program_name);
      fputs (_("\
Performs numeric/string operations on input from stdin.\n\
"), stdout);
      fputs("\
\n\
'op' is the operation to perform on field 'col'.\n\
\n\
Numeric operations:\n\
  count      count number of elements in the input group\n\
  sum        print the sum the of values\n\
  min        print the minimum value\n\
  max        print the maximum value\n\
  absmin     print the minimum of abs(values)\n\
  absmax     print the maximum of abs(values)\n\
  mean       print the mean of the values\n\
  median     print the median value\n\
  mode       print the mode value (most common value)\n\
  antimode   print the anti-mode value (least common value)\n\
  pstdev     print the population standard deviation\n\
  sstdev     print the sample standard deviation\n\
  pvar       print the population variance\n\
  svar       print the sample variance\n\
\n\
String operations:\n\
  unique     print comma-separated sorted list of unique values\n\
  uniquenc   Same as above, while ignoring upper/lower case letters.\n\
  collapse   print comma-separed list of all input values\n\
\n\
",stdout);
      fputs (_("\
\n\
General options:\n\
  -f, --full                Print entire input line before op results\n\
                            (default: print only the groupped keys)\n\
  -g, --groups=X[,Y,Z,]     Group via fields X,[Y,X]\n\
                            This is a short-cut for --key:\n\
                            '-g5,6' is equivalent to '-k5,5 -k6,6'\n\
  --header-in               First input line is column headers\n\
  --header-out              Print column headers as first line\n\
  -H, --headers             Same as '--header-in --header-out'\n\
  -s, --sort                Sort the input before grouping\n\
                            Removes the need to manually pipe the input through 'sort'\n\
  -t, --field-separator=X   use X instead of whitespace for field delimiter\n\
  -T                        Use tab as field separator\n\
                            Same as -t $'\\t'\n\
  -z, --zero-terminated     end lines with 0 byte, not newline\n\
"), stdout);

      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);

      printf (_("\
\n\
Examples:\n\
\n\
Print the mean and the median of values from column 1:\n\
\n\
  $ seq 10 | %s mean 1 median 1\n\
\n\
Group input based on field 1, and sum values (per group) on field 2:\n\
\n\
  $ printf \"%%s %%d\\n\" A 10 A 5 B 9 | %s -g1 sum 2\n\
\n\
Unsorted input must be sorted:\n\
\n\
  $ cat INPUT.TXT | %s -s -g1 mean 2\n\
\n\
  Which is equivalent to:\n\
  $ cat INPUT.TXT | sort -k1,1 | %s -g1 mean 2\n\
\n\
\n\
"), program_name, program_name, program_name, program_name);

    }
  exit (status);
}

/* Given a string with operation name, returns the operation enum.
   exits with an error message if the string is not a valid/known operation. */
static enum operation
get_operation (const char* op)
{
  for (size_t i = 0; operations[i].name ; i++)
      if ( STREQ(operations[i].name, op) )
        return (enum operation)i;

  error (EXIT_FAILURE, 0, _("invalid operation '%s'"), op);
  return 0; /* never reached */
}

/* Converts a string to number (field number).
   Exits with an error message (using 'op') on invalid field number. */
static size_t
get_field_number(enum operation op, const char* field_str)
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
static void
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
      field = get_field_number (op, argv[i]);
      i++;

      new_field_op (op, field);
    }
}

/* Force NUL-termination of the string in the linebuffer struct.
   NOTE 1: The buffer is assumed to contain NUL later on in the program,
           and is used in 'strtoul()'.
   NOTE 2: The buffer can not be simply chomp'd (by derementing length),
           because sort's "keycompare()" function assume the last valid index
           is one PAST the last character of the line (i.e. there is an EOL
           charcter in the buffer). */
inline static void
linebuffer_nullify (struct linebuffer *line)
{
  if (line->buffer[line->length-1]==eolchar)
    {
      line->buffer[line->length-1] = 0; /* make it NUL terminated */
    }
  else
    {
      /* TODO: verify this is safe, and the allocated buffer is large enough */
      line->buffer[line->length] = 0;
    }
}

static void
get_field (const struct linebuffer *line, size_t field,
               const char** /* OUT*/ _ptr, size_t /*OUT*/ *_len)
{
  size_t pos = 0;
  size_t flen = 0;
  const size_t buflen = line->length;
  char* fptr = line->buffer;
  /* Move 'fptr' to point to the beginning of 'field' */
  if (tab != TAB_DEFAULT)
    {
      /* delimiter is explicit character */
      while ((pos<buflen) && --field)
        {
          while ( (pos<buflen) && (*fptr != tab))
            {
              ++fptr;
              ++pos;
            }
          if ( (pos<buflen) && (*fptr == tab))
            {
              ++fptr;
              ++pos;
            }
        }
    }
  else
    {
      /* delimiter is white-space transition
         (multiple whitespaces are one delimiter) */
      while ((pos<buflen) && --field)
        {
          while ( (pos<buflen) && !blanks[to_uchar(*fptr)])
            {
              ++fptr;
              ++pos;
            }
          while ( (pos<buflen) && blanks[to_uchar(*fptr)])
            {
              ++fptr;
              ++pos;
            }
        }
    }

  /* Find the length of the field (until the next delimiter/eol) */
  if (tab != TAB_DEFAULT)
    {
      while ( (pos+flen<buflen) && (*(fptr+flen) != tab) )
        flen++;
    }
  else
    {
      while ( (pos+flen<buflen) && !blanks[to_uchar(*(fptr+flen))] )
        flen++;
    }

  /* Chomp field if needed */
  if ( (flen>1) && ((*(fptr + flen -1) == 0) || (*(fptr+flen-1)==eolchar)) )
    flen--;

  *_len = flen;
  *_ptr = fptr;
}

inline static long double
safe_strtold ( const char *str, size_t len, size_t field )
{
  char *endptr=NULL;

  errno = 0;
  long double val = strtold (str, &endptr);
  if (errno==ERANGE || endptr==str)
    {
      char *tmp = strdup(str);
      tmp[len] = 0 ;
      /* TODO: make invalid input error or warning or silent */
      error (EXIT_FAILURE, 0,
          _("invalid numeric input in line %zu field %zu: '%s'"),
          line_number, field, tmp);
    }
  return val;
}

/* returns TRUE if the lines are different, false if identical.
 * comparison is based on the specified keys */
/* copied from coreutils's src/uniq.c (in the key-spec branch) */
static bool
different (const struct linebuffer* l1, const struct linebuffer* l2)
{
  for (size_t *key = group_columns; key && *key ; ++key )
    {
      const char *str1,*str2;
      size_t len1,len2;
      get_field(l1,*key,&str1,&len1);
      get_field(l2,*key,&str2,&len2);
      if (debug)
        {
          fprintf(stderr,"diff, key column = %zu, str1='", *key);
          fwrite(str1,sizeof(char),len1,stderr);
          fprintf(stderr,"' str2='");
          fwrite(str2,sizeof(char),len2,stderr);
          fputs("\n",stderr);
        }
      if (len1 != len2)
        return true;
      if (memcmp(str1,str2,len1) != 0)
        return true;
    }
  return false;
}

/* For a given line, extract all requested fields and process the associated
   operations on them */
static void
process_line (const struct linebuffer *line)
{
  long double val=0;
  const char *str;
  size_t len;

  struct fieldop *op = field_ops;
  while (op)
    {
      get_field (line, op->field, &str, &len);
      if (debug)
        {
          fprintf(stderr,"getfield(%zu) = len %zu: '", op->field,len);
          fwrite(str,sizeof(char),len,stderr);
          fprintf(stderr,"'\n");
        }

      if (op->numeric)
        val = safe_strtold ( str, len, op->field );

      field_op_collect (op, str, len, val);

      op = op->next;
    }
}

/* Print the input line representing the summarized group.
   if '--full' - print the entire line.
   if not full, print only the keys used for grouping.
 */
static void
print_input_line (const struct linebuffer* lb)
{
  if (print_full_line)
    {
      size_t len = lb->length;
      const char *buf = lb->buffer;
      if (buf[len-1]==eolchar || buf[len-1]==0)
        len--;
      fwrite (buf, sizeof(char), len, stdout);
      print_field_separator();
    }
  else
    {
      for (size_t *key = group_columns; key && *key != 0 ; ++key)
        {
          const char *str;
          size_t len;
          get_field(lb,*key, &str, &len);
          fwrite(str,sizeof(char),len,stdout);
          print_field_separator();
        }
    }
}

#define SWAP_LINES(A, B)			\
  do						\
    {						\
      struct linebuffer *_tmp;			\
      _tmp = (A);				\
      (A) = (B);				\
      (B) = _tmp;				\
    }						\
  while (0)

static void
init_blank_table (void)
{
  size_t i;

  for (i = 0; i < UCHAR_LIM; ++i)
    {
      blanks[i] = !! isblank (i);
    }
}

static void
build_input_line_headers(struct linebuffer *lb, bool store_names)
{
  const char* ptr = lb->buffer;
  const char* lim = lb->buffer + lb->length;

  while (ptr<lim)
   {
     const char *end = ptr;

     /* Find the end of the input field, starting at 'ptr' */
     if (tab != TAB_DEFAULT)
       {
         while (end < lim && *end != tab)
           ++end;
       }
     else
       {
         while (end < lim && !blanks[to_uchar (*end)])
           ++end;
       }

     if (store_names)
       add_named_input_column_header(ptr, end-ptr);
     ++num_input_column_headers;

     if (debug)
       {
         fprintf(stderr,"input line header = ");
         fwrite(ptr,sizeof(char),end-ptr,stderr);
         fputc(eolchar,stderr);
       }

     /* Find the begining of the next field */
     ptr = end ;
     if (tab != TAB_DEFAULT)
       {
         if (ptr < lim)
           ++ptr;
       }
     else
       {
         while (ptr < lim && blanks[to_uchar (*ptr)])
           ++ptr;
       }
    }
}

static void
print_column_headers()
{
  if (print_full_line)
    {
      for (size_t n=1; n<=num_input_column_headers; ++n)
        {
          fputs(get_input_field_name(n), stdout);
          print_field_separator();
        }
    }
  else
    {
      for (size_t *key = group_columns; *key != 0; ++key)
        {
          printf("GroupBy(%s)",get_input_field_name(*key));
          print_field_separator();
        }
    }

  for (struct fieldop *op = field_ops; op ; op=op->next)
    {
      printf("%s(%s)",op->name, get_input_field_name(op->field));

      if (op->next)
        print_field_separator();
    }

  /* print end-of-line */
  putchar(eolchar);
}

/*
    Process each line in the input.

    If the key fo the current line is different from the prevopis one,
    summarize the previous group and start a new one.
 */
static void
process_file ()
{
  struct linebuffer lb1, lb2;
  struct linebuffer *thisline, *group_first_line;

  thisline = &lb1;
  group_first_line = &lb2;

  initbuffer (thisline);
  initbuffer (group_first_line);

  while (!feof (pipe_input))
    {
      bool new_group = false;

      if (readlinebuffer_delim (thisline, pipe_input, eolchar) == 0)
        break;
      linebuffer_nullify (thisline);
      line_number++;

      /* Process header, only after reading the first line,
         in case we need to know either the column names or
         the number of columns in the input. */
      if ((input_header || output_header) && line_number==1)
        {
          build_input_line_headers(thisline,input_header);

          if (output_header)
            print_column_headers();
          if (input_header)
            continue; //this line does not contain numeric values, only headers.
        }

      /* If no keys are given, the entire input is considered one group */
      if (num_group_colums)
        {
          new_group = (group_first_line->length == 0
                       || different (thisline, group_first_line));

          if (debug)
            {
              fprintf(stderr,"group_first_line = '");
              fwrite(group_first_line->buffer,sizeof(char),group_first_line->length,stderr);
              fprintf(stderr,"'\n");
              fprintf(stderr,"thisline = '");
              fwrite(thisline->buffer,sizeof(char),thisline->length,stderr);
              fprintf(stderr,"'\n");
              fprintf(stderr, "newgroup = %d\n", new_group);
            }

          if (new_group && lines_in_group>0)
            {
              print_input_line (group_first_line);
              summarize_field_ops ();
              reset_field_ops ();
              lines_in_group = 0 ;
              group_first_line->length = 0;
            }
        }
      else
        {
          /* The entire line is a "group", if it's the first line, keep it */
          new_group = (group_first_line->length==0);
        }

      lines_in_group++;
      process_line (thisline);

      if (new_group)
        {
          SWAP_LINES (group_first_line, thisline);
        }
    }

  /* summarize last group */
  if (lines_in_group)
    {
      print_input_line (group_first_line);
      summarize_field_ops ();
      reset_field_ops ();
    }

  free (lb1.buffer);
  free (lb2.buffer);
}

static void
open_input()
{
  if (pipe_through_sort && group_columns)
    {
      char tmp[INT_BUFSIZE_BOUND(size_t)*2+5];
      char cmd[1024];
      bzero(cmd,sizeof(cmd));

      if (input_header)
        strcat(cmd,"sed -u 1q;");
      strcat(cmd,"LC_ALL=C sort ");
      if (tab != TAB_DEFAULT)
        {
          snprintf(tmp,sizeof(tmp),"-t '%c' ",tab);
          strcat(cmd,tmp);
        }
      for (size_t *key = group_columns; key && *key; ++key)
        {
          snprintf(tmp,sizeof(tmp),"-k%zu,%zu ",*key,*key);
          if (strlen(tmp)+strlen(cmd)+1 >= sizeof(cmd))
            error(EXIT_FAILURE, 0, _("sort command too-long (please report this bug)"));
          strcat(cmd,tmp);
        }

      if (debug)
        fprintf(stderr,"sort cmd = '%s'\n", cmd);

      pipe_input = popen(cmd,"r");
      if (pipe_input == NULL)
        error (EXIT_FAILURE, 0, _("failed to run 'sort': popen failed"));
    }
  else
    {
      pipe_input = stdin;
    }
}

static void
close_input()
{
  int i;

  if (ferror (pipe_input))
    error (EXIT_FAILURE, 0, _("read error"));

  if (pipe_through_sort)
    i = pclose(pipe_input);
  else
    i = fclose(pipe_input);

  if (i != 0)
    error (EXIT_FAILURE, 0, _("read error (on close)"));
}

static void
free_sort_keys ()
{
  free(group_columns);
}

/* Parse the "--group=X[,Y,Z]" parameter, populating 'keylist' */
static void
parse_group_spec ( char* spec )
{
  size_t val;
  size_t idx;
  char *endptr;

  /* Count number of groups parameters, by number of commas */
  num_group_colums = 1;
  if (debug)
    fprintf(stderr,"parse_group_spec (spec='%s')\n", spec);
  endptr = spec;
  while ( *endptr != '\0' )
    {
      if (*endptr == ',')
        num_group_colums++;
      ++endptr;
    }

  /* Allocate the group_columns */
  group_columns = xnmalloc(num_group_colums+1, sizeof(size_t));

  errno = 0 ;
  idx=0;
  while (1)
    {
      val = strtoul (spec, &endptr, 10);
      if (errno != 0 || endptr == spec)
        error (EXIT_FAILURE, 0, _("invalid field value for grouping '%s'"),
                                        spec);
      if (val==0)
        error (EXIT_FAILURE, 0, _("invalid field value (zero) for grouping"));

      group_columns[idx] = val;
      idx++;

      if (endptr==NULL || *endptr==0)
        break;
      if (*endptr != ',')
        error (EXIT_FAILURE, 0, _("invalid grouping parameter '%s'"), endptr);
      endptr++;
      spec = endptr;
    }
  group_columns[idx] = 0 ; /* marker for the last element */

  if (debug)
    {
      fprintf(stderr,"group columns (%p) num=%zu = ", group_columns,num_group_colums);
      for (size_t *key=group_columns;key && *key; ++key)
        {
          fprintf(stderr,"%zu ", *key);
        }
      fputs("\n",stderr);
    }
}

int main(int argc, char* argv[])
{
  int optc;

  set_program_name (argv[0]);

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  init_blank_table();

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, short_options, long_options, NULL))
         != -1)
    {
      switch (optc)
        {
        case 'f':
          print_full_line = true;
          break;

        case 'g':
          parse_group_spec (optarg);
          break;

        case 'z':
          eolchar = 0;
          break;

        case DEBUG_OPTION:
          debug = true;
          break;

        case INPUT_HEADER_OPTION:
          input_header = true;
          break;

        case OUTPUT_HEADER_OPTION:
          output_header = true;
          break;

        case 'H':
          input_header = output_header = true;
          break;

        case 's':
          pipe_through_sort = true;
          break;

        case 't':
          /* Interpret -t '' to mean 'use the NUL byte as the delimiter.'  */
          if (optarg[0] != '\0' && optarg[1] != '\0')
            error (EXIT_FAILURE, 0,
                   _("the delimiter must be a single character"));
          tab = optarg[0];
          break;

        case 'T':
          tab = '\t';
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  if (argc <= optind)
    {
      error (0, 0, _("missing operation specifiers"));
      usage (EXIT_FAILURE);
    }

  parse_operations (argc, optind, argv);
  open_input ();
  process_file ();
  free_field_ops ();
  free_sort_keys ();
  close_input ();

  return EXIT_SUCCESS;
}

/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=8: */
/* vim: set expandtab: */

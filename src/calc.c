#include <stdio.h>

#include "system.h"
#include "config.h"
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include "linebuffer.h"
#include "minmax.h"
#include "error.h"
#include "quote.h"
#include "version.h"
#include "size_max.h"
#include "version-etc.h"
#include "xalloc.h"

#include "key-compare.h"

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

/* If delimiter has this value, blanks separate fields.  */
enum { DELIMITER_DEFAULT = CHAR_MAX + 1 };
static int delimiter = DELIMITER_DEFAULT; /* field separator */

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
  OP_COLLAPSE       /* Collapse strings into comma separated values */
};

enum operation_type__
{
  NUMERIC_SCALAR = 0,
  NUMERIC_VECTOR,
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
  enum operation_type__ type;
  enum operation_first_value auto_first;
};

struct operation_data operations[] =
{
  {"count",   NUMERIC_SCALAR,  IGNORE_FIRST},   /* OP_COUNT */
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

  {NULL}
};

/* Operation on a field */
struct fieldop
{
    /* operation 'class' information */
  enum operation op;
  enum operation_type__ type;
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
  char **str_ptr;  /* array of string pointers, into 'str_buf' */
  size_t str_ptr_used; /* number of strings pointers */
  size_t str_ptr_alloc; /* number of string pointers allocated */

  struct fieldop *next;
};

static struct fieldop* field_ops = NULL;

enum { VALUES_BATCH_INCREMENT = 1024 };

/* Re-Allocate the 'op->values' buffer, by VALUES_BATCH_INCREMENT elements.
   If 'op->values' is NULL, allocate the first batch */
static void
field_op_reserve_values (struct fieldop *op)
{
  op->alloc_values += VALUES_BATCH_INCREMENT;
  op->values = xnrealloc (op->values, op->alloc_values, sizeof (long double));
}

/* Re-Allocate the 'op->collapsed_buf' buffer,
   by MAX(size+1,VALUES_BATCH_INCREMENT) bytes. */
static void
field_op_add_string (struct fieldop *op, const char* str, size_t slen)
{
  if (op->str_buf_used + slen+1 >= op->str_buf_alloc)
    {
      op->str_buf_alloc += MAX(VALUES_BATCH_INCREMENT,slen+1);
      op->str_buf = xrealloc (op->str_buf, op->str_buf_alloc);
    }
  if (op->str_ptr_used + 1 >= op->str_ptr_alloc)
    {
      op->str_ptr_alloc += VALUES_BATCH_INCREMENT;
      op->str_ptr = xrealloc (op->str_ptr, op->str_ptr_alloc);
    }

  /* Set the string pointer */
  op->str_ptr[op->str_ptr_used] = op->str_buf + op->str_buf_used;
  op->str_ptr_used++;

  /* Copy the string to the buffer */
  memcpy (op->str_buf + op->str_buf_used, str, slen);
  op->str_buf_used += slen;
  *(op->str_buf + op->str_buf_used) = 0;
  op->str_buf_used++;
}

/* see:
  http://www.gnu.org/software/libc/manual/html_node/Comparison-Functions.html */
static int
cmp_long_double (const void *p1, const void *p2)
{
  const long double *a = (const long double *)p1;
  const long double *b = (const long double *)p2;

  return ( *a > *b ) - (*a < *b);

}

static void
field_op_sort_values (struct fieldop *op)
{
  qsort (op->values, op->num_values, sizeof (long double), cmp_long_double);
}

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
  if (op->type == NUMERIC_VECTOR)
    field_op_reserve_values (op);

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
      if (op->num_values >= op->alloc_values)
        field_op_reserve_values(op);
      op->values[op->num_values] = num_value;
      op->num_values++;
      break;

    case OP_UNIQUE:
    case OP_UNIQUE_NOCASE:
    case OP_COLLAPSE:
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
  /* TODO: handle n==0 */

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
   *               pointers to char", but strcmp(3) arguments are "pointers
   *                             to char", hence the following cast plus dereference */

  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

static int
cmpstringp_nocase(const void *p1, const void *p2)
{
  /* The actual arguments to this function are "pointers to
   *               pointers to char", but strcmp(3) arguments are "pointers
   *                             to char", hence the following cast plus dereference */

  return strcasecmp(* (char * const *) p1, * (char * const *) p2);
}



static char *
unique_value ( struct fieldop *op, bool case_sensitive )
{
  const char *last_str;
  char *buf, *pos;

  /* Sort the string pointers */
  qsort ( op->str_ptr, op->str_ptr_used, sizeof(char*), case_sensitive
                                                          ?cmpstringp
                                                          :cmpstringp_nocase);

  /* Uniquify them */
  pos = buf = xzalloc ( op->str_buf_used );

  /* Copy the first string */
  last_str = op->str_ptr[0];
  strcpy (buf, op->str_ptr[0]);
  pos += strlen(op->str_ptr[0]);

  /* Copy the following strings, if they are different from the previous one */
  for (size_t i = 1; i < op->str_ptr_used ; ++i)
    {
      const char *newstr = op->str_ptr[i];

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

  return buf;
}

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

static void
field_op_summarize (struct fieldop *op)
{
  long double numeric_result = 0 ;
  char *string_result = NULL;

  if (debug)
    fprintf (stderr, "-- summarize for %s(%zu)\n", op->name, op->field);

  switch (op->op)
    {
    case OP_MEAN:
      /* Calculate mean */
      /* TODO: check for division-by-zero */
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
      /* TODO: check for no values at all */
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
      string_result = unique_value (op, (op->op==OP_UNIQUE));
      break;

    case OP_COLLAPSE:
      string_result = collapse_value (op);
      break;


    default:
      error (EXIT_FAILURE, 0, _("internal error 2"));
      break;
    }


  if (debug)
    {
      if (op->numeric)
        fprintf (stderr, "%s(%zu) = %Lg\n", op->name, op->field, numeric_result);
      else
        fprintf (stderr, "%s(%zu) = '%s'\n", op->name, op->field, string_result);
    }

  if (op->numeric)
    printf ("%Lg", numeric_result);
  else
    printf ("%s", string_result);

  free (string_result);

  /* reset values for next group */
  op->first = true;
  op->count = 0 ;
  op->value = 0;
  op->num_values = 0 ;
  op->str_ptr_used = 0;
  op->str_buf_used = 0;
}

static void
summarize_field_ops ()
{
  for (struct fieldop *p = field_ops; p ; p=p->next)
    {
      field_op_summarize (p);

      /* print field separator */
      if (p->next)
        putchar( (delimiter==DELIMITER_DEFAULT)?' ':delimiter );
    }

    /* print end-of-line */
    putchar(eolchar);
}

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

  free (op->str_ptr);
  op->str_ptr = NULL;
  op->str_ptr_used = 0;
  op->str_ptr_alloc = 0;

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
};

static char const short_options[] = "zk:t:";

static struct option const long_options[] =
{
  {"zero-terminated", no_argument, NULL, 'z'},
  {"field-separator", required_argument, NULL, 't'},
  {"key", required_argument, NULL, 'k'},
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
\n\
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
  -t, --field-separator=X    use X instead of whitespace for field delimiter\n\
  -z, --zero-terminated     end lines with 0 byte, not newline\n\
"), stdout);

      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);

      printf (_("\
\n\
Examples:\n\
  Print the mean and the median of values from column 1\n\
  $ seq 10 | %s mean 1 median 1\n\
\n\
"), program_name);

    }
  exit (status);
}

static enum operation
get_operation (const char* op)
{
  for (size_t i = 0; operations[i].name ; i++)
      if ( STREQ(operations[i].name, op) )
        return (enum operation)i;

  error (EXIT_FAILURE, 0, _("invalid operation '%s'"), op);
  return 0; /* never reached */
}

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

#if 0
static void
print_ops ()
{
  struct fieldop *p = field_ops;
  while (p)
    {
      printf ("%s(%zu)\n", p->name, p->field);
      p = p->next;
    }
}
#endif

inline static void
linebuffer_chomp (struct linebuffer *line)
{
  if (line->buffer[line->length-1]==eolchar)
    {
      line->buffer[line->length-1] = 0; /* make it NUL terminated */
      line->length--;
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
  if (delimiter != DELIMITER_DEFAULT)
    {
      /* delimiter is explicit character */
      while ((pos<buflen) && --field)
        {
          while ( (pos<buflen) && (*fptr != delimiter))
            {
              ++fptr;
              ++pos;
            }
          if ( (pos<buflen) && (*fptr == delimiter))
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
          while ( (pos<buflen) && !isblank (*fptr))
            {
              ++fptr;
              ++pos;
            }
          while ( (pos<buflen) && isblank (*fptr))
            {
              ++fptr;
              ++pos;
            }
        }
    }

  /* Find the length of the field (until the next delimiter/eol) */
  if (delimiter != DELIMITER_DEFAULT)
    {
      while ( (pos+flen<buflen) && (*(fptr+flen) != delimiter) )
        flen++;
    }
  else
    {
      while ( (pos+flen<buflen) && !isblank (*(fptr+flen)) )
        flen++;
    }

  *_len = flen;
  *_ptr = fptr;
}

#if 0
static bool
test_get_field_eq ( const char *str, size_t field, const char *expected )
{
  const char *ptr;
  size_t len;
  struct linebuffer l;

  l.buffer = (char*)str;
  l.length = strlen(l.buffer);

  get_field (&l, field, &ptr, &len);

  if (len != strlen(expected))
      return false;

  return (strncmp(ptr,expected,len)==0);
}

static void
test_get_field()
{
  /* white space delimiter */
  delimiter = DELIMITER_DEFAULT ;
  const char *s1 = "hello world   a\tb c";
  assert(test_get_field_eq(s1,1,"hello"));
  assert(test_get_field_eq(s1,2,"world"));
  assert(test_get_field_eq(s1,3,"a"));
  assert(test_get_field_eq(s1,4,"b"));
  assert(test_get_field_eq(s1,5,"c"));

  /* Explicit TAB delimiter */
  delimiter = '\t';
  const char *s2 = "hello\t\tworld   a\tb c";
  assert(test_get_field_eq(s2,1,"hello"));
  assert(test_get_field_eq(s2,2,""));
  assert(test_get_field_eq(s2,3,"world   a"));
  assert(test_get_field_eq(s2,4,"b c"));
}
#endif

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

/* Given a "struct linebuffer", initializes a sort-compatible "struct line"
 * (and finds the first key field) */
/* copied from coreutils's src/uniq.c (in the key-spec branch) */
static void
prepare_line (const struct linebuffer *linebuf, struct line* /*output*/ line,
              char eol_delimiter)
{
  size_t len = linebuf->length;

  line->text = linebuf->buffer;
  line->length = linebuf->length;
  line->keybeg = NULL; /* TODO: are these the right initializers? */
  line->keylim = NULL;

  if (line->text[line->length-1] == eol_delimiter)
    len--;

  line->keybeg = begfield (line, keylist);
  line->keylim = limfield (line, keylist);

  if (line->keybeg >= line->keylim)
    {
      /* TODO: is this the correct way to detect 'no limit' ?
       * (ie compare keys until the end of the line)*/
      line->keylim = line->keybeg + len - (line->keybeg - line->text);
    }
}

/* returns TRUE if the lines are different, false if identical.
 * comparison is based on the specified keys */
/* copied from coreutils's src/uniq.c (in the key-spec branch) */
static bool
different (const struct line* l1, const struct line* l2)
{
  int diff = keycompare (l1,l2);
  return (diff != 0);
}

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

      if (op->numeric)
        val = safe_strtold ( str, len, op->field );

      field_op_collect (op, str, len, val);

      op = op->next;
    }
}

static void
process_file ()
{
  struct linebuffer line;

  initbuffer (&line);
  while (readlinebuffer_delim (&line, stdin, eolchar))
    {
      line_number++;
      linebuffer_chomp (&line);

      if (line.length==0)
        {
          /* new group marker */
          if (lines_in_group>0)
            summarize_field_ops ();

          lines_in_group = 0;
          continue;
        }

      lines_in_group++;
      process_line (&line);
    }
  freebuffer (&line);

  if (lines_in_group>0)
    summarize_field_ops ();

  if (ferror (stdin))
    error (EXIT_FAILURE, 0, _("error reading input"));
}

int main(int argc, char* argv[])
{
  int optc;
  struct keyfield *key;
  struct keyfield key_buf;
  char const *s;

  set_program_name (argv[0]);

  setlocale (LC_ALL, "");

  init_key_spec ();

  while ((optc = getopt_long (argc, argv, short_options, long_options, NULL))
         != -1)
    {
      switch (optc)
        {
        case 'k':
          key = key_init (&key_buf);

          /* Get POS1. */
          s = parse_field_count (optarg, &key->sword,
                                 N_("invalid number at field start"));
          if (! key->sword--)
            {
              /* Provoke with 'sort -k0' */
              badfieldspec (optarg, N_("field number is zero"));
            }
          if (*s == '.')
            {
              s = parse_field_count (s + 1, &key->schar,
                                     N_("invalid number after '.'"));
              if (! key->schar--)
                {
                  /* Provoke with 'sort -k1.0' */
                  badfieldspec (optarg, N_("character offset is zero"));
                }
            }
          if (! (key->sword || key->schar))
            key->sword = SIZE_MAX;
          s = set_ordering (s, key, bl_start);
          if (*s != ',')
            {
              key->eword = SIZE_MAX;
              key->echar = 0;
            }
          else
            {
              /* Get POS2. */
              s = parse_field_count (s + 1, &key->eword,
                                     N_("invalid number after ','"));
              if (! key->eword--)
                {
                  /* Provoke with 'sort -k1,0' */
                  badfieldspec (optarg, N_("field number is zero"));
                }
              if (*s == '.')
                {
                  s = parse_field_count (s + 1, &key->echar,
                                         N_("invalid number after '.'"));
                }
              s = set_ordering (s, key, bl_end);
            }
          if (*s)
            badfieldspec (optarg, N_("stray character in field spec"));
          /* TODO: strange, sort's original code sets sword=SIZE_MAX for "-k1".
           * force override??? */
          if (key->sword==SIZE_MAX)
            key->sword=0;
          insertkey (key);
          break;

        case 'z':
          eolchar = 0;
          break;

        case DEBUG_OPTION:
          debug = true;
          break;

        case 't':
          /* Interpret -t '' to mean 'use the NUL byte as the delimiter.'  */
          if (optarg[0] != '\0' && optarg[1] != '\0')
            error (EXIT_FAILURE, 0,
                   _("the delimiter must be a single character"));
          delimiter = optarg[0];
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  if (argc <= optind)
    error (EXIT_FAILURE, 0, _("missing operations specifiers"));

  parse_operations (argc, optind, argv);
  process_file ();
  free_field_ops ();
}

/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=8: */
/* vim: set expandtab: */

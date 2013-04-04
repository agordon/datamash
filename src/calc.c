#include <stdio.h>

#include "system.h"
#include "config.h"
#include <getopt.h>
#include <math.h>
#include <stdlib.h>
#include "gl_avltree_oset.h"
#include "gl_xlist.h"
#include "gl_xoset.h"
#include "linebuffer.h"
#include "error.h"
#include "quote.h"
#include "version.h"
#include "version-etc.h"
#include "xalloc.h"
#include "xstrtod.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "calc"

#define AUTHORS proper_name ("Assaf Gordon")

/* Until someone better comes along */
const char version_etc_copyright[] = "Copyright %s %d Assaf Gordon" ;

/* enable debugging */
static bool debug = false;

/* The character marking end of line. Default to \n. */
static char eolchar = '\n';

static size_t line_number = 0 ;

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
  OP_UNIQUE_NOCASE  /* Collapse Unique strings, ignoring-case */
};

enum operation_type__
{
  NUMERIC_SCALAR = 0,
  NUMERIC_VECTOR,
  STRING_CUSTOM
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
  {"unique",  STRING_CUSTOM,   IGNORE_FIRST},   /* OP_UNIQUE */
  {"uniquenc",STRING_CUSTOM,   IGNORE_FIRST},   /* OP_UNIQUE_NOCASE */

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

  /* String container for OP_UNIQE / OP_UNIQUE_NOCASE*/
  gl_oset_t   str_oset;

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

  if (op->op == OP_UNIQUE)
    op->str_oset = gl_oset_create_empty (GL_AVLTREE_OSET,
                                         (gl_setelement_compar_fn)strcmp,
                                         (gl_setelement_dispose_fn)free);
  if (op->op == OP_UNIQUE_NOCASE)
    op->str_oset = gl_oset_create_empty (GL_AVLTREE_OSET,
                                         (gl_setelement_compar_fn)strcasecmp,
                                         (gl_setelement_dispose_fn)free);

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
                  const char* str_value, const long double num_value)
{
  bool keep_line = false;

  if (debug)
    fprintf (stderr, "-- collect for %s(%zu) val=%s\n",
        op->name, op->field, str_value);

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
      gl_oset_add (op->str_oset, strdup(str_value));
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

static char *
unique_value ( gl_oset_t str_oset )
{
  size_t buf_size = 0;
  const char* str;
  char *buf, *pos;

  gl_oset_iterator_t it = gl_oset_iterator (str_oset);
  while (gl_oset_iterator_next (&it, (const void**)&str))
    buf_size += strlen(str)+1;

  pos = buf = xzalloc (buf_size);

  it = gl_oset_iterator (str_oset);
  while (gl_oset_iterator_next (&it, (const void**)&str))
    {
      if (pos != buf)
          *pos++ = ',';
      size_t len = strlen(str);
      strcpy (pos, str);
      pos += len;
    }

  return buf;
}

static void
field_op_summarize (struct fieldop *op)
{
  long double numeric_result = 0 ;

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
      fprintf(stderr, "foo = %s\n", unique_value (op->str_oset));
      break;


    default:
      error (EXIT_FAILURE, 0, _("internal error 2"));
      break;
    }


  if (op->numeric)
    fprintf (stderr, "%s(%zu) = %Lg\n", op->name, op->field, numeric_result);

  /* reset values for next group */
  op->first = true;
  op->count = 0 ;
  op->value = 0;
}

static void
summarize_field_ops ()
{
  for (struct fieldop *p = field_ops; p ; p=p->next)
    field_op_summarize (p);
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
Usage: %s [OPTION] op [op ...] col [...]\n\
"),
              program_name);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
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
  strtol_error e = xstrtoul (field_str, NULL, 10, &val, NULL);
  /* NOTE: can't use xstrtol_fatal - it's too tightly-coupled
     with getopt command-line processing */
  if (e != LONGINT_OK)
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

static void
extract_field (const struct linebuffer *line, size_t field,
               char /*OUTPUT*/ **buf, size_t *buf_size)
{
  size_t len = line->length;

  (void)field;

  /* TODO: extract field, instead of whole line */
  if (line->buffer[len-1]==eolchar)
    len--; /* chomp */
  if (len+1>*buf_size)
    {
      *buf_size = len+1;
      xrealloc(*buf, *buf_size);
    }
  memcpy (*buf, line->buffer, len);
  (*buf)[len]=0; /* NUL-terminate, as readlinebuffer doesn't */
}

static void
process_line (const struct linebuffer *line)
{
  size_t buf_size = 1024 ;
  char *buf = xzalloc (buf_size);
  long double val;
  bool ok;

  struct fieldop *op = field_ops;
  while (op)
    {
      extract_field (line, op->field, &buf, &buf_size);

      if (op->numeric)
        {
          ok = xstrtold (buf, NULL, &val, strtold);
          /* TODO: make invalid input error or warning or silent */
          if (!ok)
            error (EXIT_FAILURE, 0,
                _("invalid numeric input in line %zu field %zu: '%s'"),
                line_number, op->field, buf);

          //fprintf (stderr, "buf=%s val = %Lg\n", buf, val);
        }
      else
        val = 0;

      field_op_collect (op, buf, val);

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
      process_line (&line);
    }
  freebuffer (&line);

  summarize_field_ops ();

  if (ferror (stdin))
    error (EXIT_FAILURE, 0, _("error reading input"));
}

int main(int argc, char* argv[])
{
  int optc;

  set_program_name (argv[0]);

  while ((optc = getopt_long (argc, argv, short_options, long_options, NULL))
         != -1)
    {
      switch (optc)
        {
        case 'z':
          eolchar = 0;
          break;

        case DEBUG_OPTION:
          debug = true;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  if (argc <= optind)
    error (0, 0, _("missing operations specifiers"));

  parse_operations (argc, optind, argv);
  process_file ();
  free_field_ops ();
}

/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=8: */
/* vim: set expandtab: */

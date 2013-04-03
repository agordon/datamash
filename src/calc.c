#include <stdio.h>

#include "system.h"
#include "config.h"
#include <getopt.h>
#include <math.h>
#include <stdlib.h>
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

enum operation_type
{
  OP_COUNT = 0,
  OP_SUM,
  OP_MIN,
  OP_MAX,
  OP_ABSMIN,
  OP_ABSMAX,
  OP_MEAN,
  OP_MEDIAN,
  OP_MODE,
  OP_STDEV,

  OP_LAST /* Must be last element */
};

static const char* operation_names[] =
{
  "count", "sum", "min", "max", "absmin", "absmax", "mean", "median", "mode",
  "stdev", NULL
};

static bool operation_numerics[] =
{
  true, true, true, true, true, true, /* count, sum, min, max, absmin, absmax */
  true, true, true, true /* mean, median, mode, stdev */
};

static bool operation_auto_firsts[] =
{
  false, false,  /* count, sum */
  true, true, true, true, /* min, max, absmin, absmax */
  false, false, false, false /* mean, median, mode, stdev */
};

/* Operation on a field */
struct fieldop
{
  /* operation 'class' information */
  enum operation_type type;
  const char* name;
  bool numeric;
  bool auto_first; /* if true, automatically set 'value' if 'first' */

  /* Instance information */
  size_t field; /* field number.  1 = first field in input file. */

  /* Collected Data */
  bool first;   /* true if this is the first item in a new group */
  size_t count; /* number of items collected so far in a group */
  long double value; /* for single-value operations (sum, min, max, absmin,
                        absmax, mean) - this is the accumulated value */

  struct fieldop *next;
};


static struct fieldop* field_ops = NULL;

static void
new_field_op (enum operation_type type, size_t field)
{
  struct fieldop *op = XZALLOC(struct fieldop);

  op->type = type;
  op->name = operation_names[type];
  op->numeric = operation_numerics[type];
  op->auto_first = operation_auto_firsts[type];

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

static bool
field_op_collect (struct fieldop *op,
                  const char* str_value, const long double num_value)
{
  bool keep_line = false;


  op->count++;

  if (op->first && op->auto_first)
      op->value = num_value;

  switch (op->type)
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
    case OP_MODE:
    case OP_STDEV:
      error (EXIT_FAILURE, 0, _("not implemented yet"));
      break;

    }

  if (op->first)
    op->first = false;

  return keep_line;
}

static void
field_op_summarize (struct fieldop *op)
{
  switch (op->type)
    {
    case OP_MEAN:
      /* Calculate mean, then fall-through */
      /* TODO: check for division-by-zero */
      op->value /= op->count;

    case OP_SUM:
    case OP_COUNT:
    case OP_MIN:
    case OP_MAX:
    case OP_ABSMIN:
    case OP_ABSMAX:
      fprintf (stderr, "%s(%zu) = %Lg\n", op->name, op->field, op->value);
      break;

    case OP_MEDIAN:
    case OP_MODE:
    case OP_STDEV:
      error (EXIT_FAILURE, 0, _("not implemented yet"));
      break;

    }


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

  switch (op->type)
    {
    case OP_MEAN:
    case OP_SUM:
    case OP_COUNT:
    case OP_MIN:
    case OP_MAX:
    case OP_ABSMIN:
    case OP_ABSMAX:
      /* nothing to free for these types */
      break;

    case OP_MEDIAN:
    case OP_MODE:
    case OP_STDEV:
      error (EXIT_FAILURE, 0, _("not implemented yet"));
      break;
    }

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

static enum operation_type
get_operation_type (const char* op)
{
  size_t i;
  for (i = 0; i < OP_LAST; ++i)
      if ( STREQ(operation_names[i], op) )
        return (enum operation_type)i;
  error (EXIT_FAILURE, 0, _("invalid operation '%s'"), op);
}

static size_t
get_field_number(enum operation_type op, const char* field_str)
{
  size_t val;
  strtol_error e = xstrtoul (field_str, NULL, 10, &val, NULL);
  /* NOTE: can't use xstrtol_fatal - it's too tightly-coupled
     with getopt command-line processing */
  if (e != LONGINT_OK)
    error (EXIT_FAILURE, 0, _("invalid column '%s' for operation " \
                               "'%s'"), field_str,
                               operation_names[op]);
  return val;
}

/* Extract the operation patterns from args START through ARGC - 1 of ARGV. */
static void
parse_operations (int argc, int start, char **argv)
{
  int i = start;	/* Index into ARGV. */
  size_t field;
  enum operation_type op;

  while ( i < argc )
    {
      op = get_operation_type (argv[i]);
      i++;
      if ( i >= argc )
        error (EXIT_FAILURE, 0, _("missing field number after " \
                                  "operation '%s'"), op );
      field = get_field_number (op, argv[i]);
      i++;

      new_field_op (op, field);
    }
}

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

static void
extract_field (const struct linebuffer *line, size_t field,
               char /*OUTPUT*/ **buf, size_t *buf_size)
{
  size_t len = line->length;

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
                line_number, 1, buf);

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

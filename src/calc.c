#include <stdio.h>
#include <stdlib.h>

#include "system.h"
#include "config.h"
#include <getopt.h>
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
  OP_MEAN,
  OP_MEDIAN,
  OP_MODE,
  OP_STDEV,
  OP_MIN,
  OP_MAX,

  OP_LAST /* Must be last element */
};

typedef void* (*op_init) ();
typedef bool  (*op_collect) (const char* str_value, const double *num_value);
typedef void  (*op_calc) (void *);
typedef void  (*op_free) (void *);

struct operation_data
{
  const char* name; /* operation name, e.g. "sum" */
  bool  numeric;    /* if true, expect only numeric input.
                       non-numeric input will trigger error/warning. */
};

enum
{
  NUMERIC = true,
  NON_NUMERIC = false
};

struct operation_data operations[] =
{
    /* OP_COUNT */ {"count", NON_NUMERIC },
    /* OP_SUM   */ {"sum",   NUMERIC },
    /* OP_MEAN  */ {"mean",  NUMERIC },
    /* OP_MEDIAN*/ {"median",NUMERIC },
    /* OP_MODE  */ {"mode",  NUMERIC },
    /* OP_STDEV */ {"stdev", NUMERIC },
    /* OP_MIN   */ {"min",   NUMERIC },
    /* OP_MAX   */ {"max",   NUMERIC }
};

/* Operation on a field */
struct fieldop
{
  size_t field; /* field number.  1 = first field in input file. */
  enum operation_type op;
  struct fieldop *next;
};

struct fieldop *field_ops = NULL;

static void
add_field_op (enum operation_type op, size_t field)
{
  struct fieldop *newop = XZALLOC(struct fieldop);
  newop->field = field;
  newop->op = op;

  if (field_ops == NULL)
    {
      field_ops = newop;
    }
  else
    {
      struct fieldop *p = field_ops;
      while (p->next != NULL)
        p = p->next;
      p->next = newop;
    }
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

static const char*
get_operation_name (enum operation_type op)
{
  return operations[op].name;
}

static enum operation_type
get_operation_type (const char* op)
{
  size_t i;
  for (i = 0; i < OP_LAST; ++i)
      if ( STREQ(operations[i].name, op) )
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
                               get_operation_name(op));
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

      add_field_op (op, field);
    }
}

static void
print_ops ()
{
  struct fieldop *p = field_ops;
  while (p)
    {
      printf ("%s(%zu)\n", get_operation_name(p->op),p->field);
      p = p->next;
    }
}

static void
process_file ()
{
  struct linebuffer line;
  long double val;
  bool ok;
  size_t buf_size = 1024 ;
  char *buf = xzalloc (buf_size);

  initbuffer (&line);
  while (readlinebuffer_delim (&line, stdin, eolchar))
    {
      line_number++;

      /* TODO: extract field, instead of whole line */
      if (line.buffer[line.length-1]==eolchar)
        line.length--; /* chomp */
      if (line.length+1>buf_size)
        {
          buf_size = line.length+1;
          xrealloc(buf, buf_size);
        }
      memcpy (buf, line.buffer, line.length);
      buf[line.length]=0; /* NUL-terminate, as readlinebuffer doesn't */

      /* TODO: convert to numeric only if any of the ops are numeric */
      ok = xstrtold (buf, NULL, &val, strtold);

      /* TODO: make invalid input error or warning or silent */
      if (!ok)
        error (EXIT_FAILURE, 0,
               _("invalid numeric input in line %zu field %zu: '%s'"),
                     line_number, 1, buf);
    }
  freebuffer (&line);

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
}

/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=8: */
/* vim: set expandtab: */

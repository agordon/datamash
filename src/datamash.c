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
#include <getopt.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <strings.h>

#include "system.h"

#include "die.h"
#include "fpucw.h"
#include "closeout.h"
#include "hash.h"
#include "hash-pjw.h"
#include "error.h"
#include "lib/intprops.h"
#include "quote.h"
#include "ignore-value.h"
#include "stdnoreturn.h"
#include "linebuffer.h"
#define Version VERSION
#include "version-etc.h"
#include "xstrndup.h"
#include "xalloc.h"

#include "text-options.h"
#include "text-lines.h"
#include "column-headers.h"
#include "op-defs.h"
#include "op-parser.h"
#include "utils.h"
#include "field-ops.h"
#include "crosstab.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "datamash"

#define AUTHORS proper_name ("Assaf Gordon")

/* Until someone better comes along */
const char version_etc_copyright[] = "Copyright %s %d Assaf Gordon" ;

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

/* processing mode, group fields, field operations */
static struct datamash_ops *dm = NULL;

static bool line_mode = false; /* if TRUE, handle each line as a group */

static bool crosstab_mode = false; /* if TRUE, do cross-tabulation */

static struct crosstab* crosstab = NULL;

static bool pipe_through_sort = false;
static FILE* input_stream = NULL;

/* Use large buffer for normal operation (will be reduced for testing) */
static size_t rmdup_initial_size = (1024*1024);

/* Explicit output delimiter with --output-delimiter */
static int explicit_output_delimiter = -1;

enum
{
  INPUT_HEADER_OPTION = CHAR_MAX + 1,
  OUTPUT_HEADER_OPTION,
  NO_STRICT_OPTION,
  REMOVE_NA_VALUES_OPTION,
  OUTPUT_DELIMITER_OPTION,
  CUSTOM_FORMAT_OPTION,
  UNDOC_PRINT_INF_OPTION,
  UNDOC_PRINT_NAN_OPTION,
  UNDOC_PRINT_PROGNAME_OPTION,
  UNDOC_RMDUP_TEST
};

static char const short_options[] = "sfF:izg:t:HWR:C";

static struct option const long_options[] =
{
  {"zero-terminated", no_argument, NULL, 'z'},
  {"field-separator", required_argument, NULL, 't'},
  {"whitespace", no_argument, NULL, 'W'},
  {"group", required_argument, NULL, 'g'},
  {"ignore-case", no_argument, NULL, 'i'},
  {"skip-comments", no_argument, NULL, 'C'},
  {"header-in", no_argument, NULL, INPUT_HEADER_OPTION},
  {"header-out", no_argument, NULL, OUTPUT_HEADER_OPTION},
  {"headers", no_argument, NULL, 'H'},
  {"full", no_argument, NULL, 'f'},
  {"filler", required_argument, NULL, 'F'},
  {"format", required_argument, NULL, CUSTOM_FORMAT_OPTION},
  {"output-delimiter", required_argument, NULL, OUTPUT_DELIMITER_OPTION},
  {"sort", no_argument, NULL, 's'},
  {"no-strict", no_argument, NULL, NO_STRICT_OPTION},
  {"narm", no_argument, NULL, REMOVE_NA_VALUES_OPTION},
  {"round", required_argument, NULL, 'R'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  /* Undocumented options */
  {"-print-inf", no_argument, NULL, UNDOC_PRINT_INF_OPTION},
  {"-print-nan", no_argument, NULL, UNDOC_PRINT_NAN_OPTION},
  {"-print-progname", no_argument, NULL, UNDOC_PRINT_PROGNAME_OPTION},
  {"-rmdup-test", no_argument, NULL, UNDOC_RMDUP_TEST},
  {NULL, 0, NULL, 0},
};

void
group_columns_find_named_columns ()
{
  for (size_t i = 0; i < dm->num_grps; ++i)
    {
      struct group_column_t *p = &dm->grps[i];

      if (!p->by_name)
        continue;

      p->num = get_input_field_number (p->name);
      if (p->num == 0)
        die (EXIT_FAILURE, 0,
                _("column name %s not found in input file"),
                quote (p->name));
      p->by_name = false;
    }
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("Usage: %s [OPTION] op [fld] [op fld ...]\n"),
          program_name);
      fputs ("\n", stdout);
      fputs (_("Performs numeric/string operations on input from stdin."),
          stdout);
      fputs ("\n\n", stdout);
      fputs (_("\
'op' is the operation to perform.  If a primary operation is used,\n\
it must be listed first, optionally followed by other operations.\n"), stdout);
      fputs (_("\
'fld' is the input field to use.  'fld' can be a number (1=first field),\n\
or a field name when using the -H or --header-in options.\n"), stdout);
      fputs (_("\
Multiple fields can be listed with a comma (e.g. 1,6,8).  A range of\n\
fields can be listed with a dash (e.g. 2-8).  Use colons for operations\n\
which require a pair of fields (e.g. 'pcov 2:6').\n"), stdout);
      fputs ("\n\n", stdout);
      fputs (_("Primary operations:\n"),stdout);
      fputs ("  groupby, crosstab, transpose, reverse, check\n",stdout);

      fputs (_("Line-Filtering operations:\n"),stdout);
      fputs ("  rmdup\n",stdout);

      fputs (_("Per-Line operations:\n"),stdout);
      fputs ("  base64, debase64, md5, sha1, sha256, sha512,\n", stdout);
      fputs ("  bin, strbin, round, floor, ceil, trunc, frac,\n", stdout);
      fputs ("  dirname, basename, barename, extname, getnum, cut\n", stdout);

      fputs (_("Numeric Grouping operations:\n"),stdout);
      fputs ("  sum, min, max, absmin, absmax, range\n",stdout);

      fputs (_("Textual/Numeric Grouping operations:\n"),stdout);
      fputs ("  count, first, last, rand, unique, collapse, countunique\n",
             stdout);

      fputs (_("Statistical Grouping operations:\n"),stdout);
      fputs ("\
  mean, trimmean, median, q1, q3, iqr, perc, mode, antimode, \n\
  pstdev, sstdev, pvar, svar, mad, madraw,\n\
  pskew, sskew, pkurt, skurt, dpo, jarque,\n\
  scov, pcov, spearson, ppearson\n\
\n", stdout);
      fputs ("\n", stdout);

      fputs (_("Grouping Options:\n"),stdout);
      fputs (_("\
  -C, --skip-comments       skip comment lines (starting with '#' or ';'\n\
                              and optional whitespace)\n\
"), stdout);
      fputs (_("\
  -f, --full                print entire input line before op results\n\
                              (default: print only the grouped keys)\n\
"), stdout);
      fputs (_("\
  -g, --group=X[,Y,Z]       group via fields X,[Y,Z];\n\
                              equivalent to primary operation 'groupby'\n\
"), stdout);
      fputs (_("\
      --header-in           first input line is column headers\n\
"), stdout);
      fputs (_("\
      --header-out          print column headers as first line\n\
"), stdout);
      fputs (_("\
  -H, --headers             same as '--header-in --header-out'\n\
"), stdout);
      fputs (_("\
  -i, --ignore-case         ignore upper/lower case when comparing text;\n\
                              this affects grouping, and string operations\n\
"), stdout);
      fputs (_("\
  -s, --sort                sort the input before grouping; this removes the\n\
                              need to manually pipe the input through 'sort'\n\
"), stdout);

      fputs (_("File Operation Options:\n"),stdout);
      fputs (_("\
      --no-strict           allow lines with varying number of fields\n\
"), stdout);
      fputs (_("\
      --filler=X            fill missing values with X (default %s)\n\
"), stdout);

      fputs ("\n", stdout);
      fputs (_("General Options:\n"),stdout);
      fputs (_("\
  -t, --field-separator=X   use X instead of TAB as field delimiter\n\
"), stdout);
      fputs (_("\
      --format=FORMAT       print numeric values with printf style\n\
                            floating-point FORMAT.\n\
"), stdout);
      fputs (_("\
      --output-delimiter=X  use X instead as output field delimiter\n\
                            (default: use same delimiter as -t/-W)\n\
"), stdout);
      fputs (_("\
      --narm                skip NA/NaN values\n\
"), stdout);
      fputs (_("\
  -R, --round=N             round numeric output to N decimal places\n\
"), stdout);
      fputs (_("\
  -W, --whitespace          use whitespace (one or more spaces and/or tabs)\n\
                              for field delimiters\n\
"), stdout);
      fputs (_("\
  -z, --zero-terminated     end lines with 0 byte, not newline\n\
"), stdout);

      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);

      fputs ("\n\n", stdout);

      fputs (_("Examples:"), stdout);
      fputs ("\n\n", stdout);
      fputs (_("Print the sum and the mean of values from column 1:"), stdout);
      printf ("\n\
  $ seq 10 | %s sum 1 mean 1\n\
  55  5.5\n\
\n", program_name);
      fputs (_("Transpose input:"), stdout);
      printf ("\n\
  $ seq 10 | paste - - | %s transpose\n\
  1    3    5    7    9\n\
  2    4    6    8    10\n\
\n", program_name);

      fputs (_("For detailed usage information and examples, see\n"),stdout);
      printf ("  man %s\n", PACKAGE_NAME);
      fputs (_("The manual and more examples are available at\n"), stdout);
      fputs ("  " PACKAGE_URL "\n\n", stdout);
    }
  exit (status);
}

static inline noreturn void
error_not_enough_fields (const size_t needed, const size_t found)
{
  error (0, 0, _("invalid input: field %"PRIuMAX" requested, " \
        "line %"PRIuMAX" has only %"PRIuMAX" fields"),
        (uintmax_t)needed, (uintmax_t)line_number, (uintmax_t)found);
  exit (EXIT_FAILURE);
}


static inline void
safe_line_record_get_field (const struct line_record_t *lr, const size_t n,
                            const char ** /* out */ pptr, size_t* /*out*/ plen)
{
  if (!line_record_get_field (lr, n, pptr, plen))
    error_not_enough_fields (n, line_record_num_fields (lr));
}

static inline void
safe_line_record_get_fieldz (const struct line_record_t *lr, const size_t n,
                             char * /* out */ buf, size_t len)
{
  const char *p;
  size_t l;
  safe_line_record_get_field (lr, n, &p, &l);
  l = MIN (len-1,l);
  memcpy (buf, p, l);
  buf[l] = 0;
}

/* returns TRUE if the lines are different, false if identical.
 * comparison is based on the specified keys */
/* copied from coreutils's src/uniq.c (in the key-spec branch) */
static bool
different (const struct line_record_t* l1, const struct line_record_t* l2)
{
  for (size_t i = 0; i < dm->num_grps; ++i)
    {
      const size_t col_num = dm->grps[i].num;
      const char *str1=NULL,*str2=NULL;
      size_t len1=0,len2=0;
      safe_line_record_get_field (l1, col_num, &str1, &len1);
      safe_line_record_get_field (l2, col_num, &str2, &len2);
      if (len1 != len2)
        return true;
      if ((case_sensitive && !STREQ_LEN (str1,str2,len1))
          || (!case_sensitive && (strncasecmp (str1,str2,len1)!=0)))
        return true;
    }
  return false;
}

/* For a given line, extract all requested fields and process the associated
   operations on them */
static bool
process_line (const struct line_record_t *line)
{
  const char *str = NULL;
  size_t len = 0;
  enum FIELD_OP_COLLECT_RESULT flocr;
  bool keep_line = false;

  for (size_t i=0; i<dm->num_ops; ++i)
    {
      struct fieldop *op = &dm->ops[i];
      safe_line_record_get_field (line, op->field, &str, &len);
      flocr = field_op_collect (op, str, len);
      if (!field_op_ok (flocr))
        {
          char *tmp = xmalloc (len+1);
          memcpy (tmp,str,len);
          tmp[len] = 0 ;
          die (EXIT_FAILURE, 0,
              _("%s in line %"PRIuMAX" field %"PRIuMAX": '%s'"),
              field_op_collect_result_name (flocr),
              (uintmax_t)line_number, (uintmax_t)op->field, tmp);
        }
      keep_line = keep_line || (flocr==FLOCR_OK_KEEP_LINE);
    }
  return keep_line;
}

/* Print the input line representing the summarized group.
   if '--full' - print the entire line.
   if not full, print only the keys used for grouping.
 */
static void
print_input_line (const struct line_record_t* lb)
{
  const char *str = NULL;
  size_t len = 0 ;
  if (print_full_line)
    {
      for (size_t i = 1; i <= line_record_num_fields (lb); ++i)
        {
          safe_line_record_get_field (lb, i, &str, &len);
          ignore_value (fwrite (str, sizeof (char), len, stdout));
          print_field_separator ();
        }
    }
  else
    {
      for (size_t i = 0; i < dm->num_grps; ++i)
        {
          const size_t col_num = dm->grps[i].num;
          safe_line_record_get_field (lb, col_num, &str, &len);
          ignore_value (fwrite (str,sizeof (char),len,stdout));
          print_field_separator ();
        }
    }
}

#define SWAP_LINES(A, B)			\
  do						\
    {						\
      struct line_record_t *_tmp;		\
      _tmp = (A);				\
      (A) = (B);				\
      (B) = _tmp;				\
    }						\
  while (0)


static void
print_column_headers ()
{
  if (print_full_line)
    {
      /* Print the headers of all the input fields */
      for (size_t n=1; n<=get_num_column_headers (); ++n)
        {
          fputs (get_input_field_name (n), stdout);
          print_field_separator ();
        }
    }
  else
    {
      /* print only the headers of the group-by fields, e.g
        'GroupBy (field-3)'  (without input headers), or
        'GroupBy (NAME)'     (with input headers)           */
      for (size_t i = 0; i < dm->num_grps; ++i)
        {
          const size_t col_num = dm->grps[i].num;
          if (col_num > get_num_column_headers ())
            error_not_enough_fields (col_num, get_num_column_headers ());
          printf ("GroupBy" "(%s)",get_input_field_name (col_num));
          print_field_separator ();
        }
    }

  /* add headers of the operations, e.g.
        'sum (field-3)'  (without input headers), or
        'sum (NAME)'     (with input headers)           */
  for (size_t i=0; i<dm->num_ops; ++i)
    {
      struct fieldop *op = &dm->ops[i];
      if (op->slave)
        continue;

      if (op->field > get_num_column_headers ())
        error_not_enough_fields (op->field, get_num_column_headers ());

      printf ("%s", get_field_operation_name (op->op));

      if (op->op == OP_PERCENTILE) {
        printf (":%"PRIuMAX, (uintmax_t)op->params.percentile);
      }
      if (op->op == OP_TRIMMED_MEAN) {
        printf (":%Lg", op->params.trimmed_mean);
      }

      printf ("(%s)", get_input_field_name (op->field));

      if (i != dm->num_ops-1)
        print_field_separator ();
    }

  /* print end-of-line */
  print_line_separator ();
}

void
field_op_find_named_columns ()
{
  for (size_t i=0; i<dm->num_ops; ++i)
    {
      struct fieldop *p = &dm->ops[i];
      if (!p->field_by_name)
        continue;
      p->field = get_input_field_number (p->field_name);
      if (p->field == 0)
        die (EXIT_FAILURE, 0,
                _("column name %s not found in input file"),
                quote (p->field_name));
      p->field_by_name = false;
    }
}

/*
    Process the input header line
*/
static void
process_input_header (FILE *stream)
{
  struct line_record_t lr;

  line_record_init (&lr);
  if (line_record_fread (&lr, stream, eolchar, skip_comments))
    {
      build_input_line_headers (&lr, true);
      line_number++;
      /* If using named-columns, find the column numbers after reading the
         header line. */
      field_op_find_named_columns ();
      group_columns_find_named_columns ();
    }
  line_record_free (&lr);
}

void
summarize_field_ops ()
{
  for (size_t i=0;i<dm->num_ops;++i)
    {
      struct fieldop *p = &dm->ops[i];
      if (p->slave)
        continue;

      field_op_summarize (p);
      fputs (p->out_buf, stdout);

      /* print field separator */
      if (i != dm->num_ops-1)
        print_field_separator ();
    }

  /* print end-of-line */
  print_line_separator ();
}

void
reset_field_ops ()
{
  for (size_t i=0;i<dm->num_ops;++i)
    field_op_reset (&dm->ops[i]);
}

/* Process a completed group of data lines
   (all with the same 'group by' keys). */
static void
process_group (const struct line_record_t* line)
{
  /* TODO: dynamically re-alloc if needed */
  char col_name[512];
  char row_name[512];

  if (lines_in_group>0)
    {
      if (crosstab_mode)
        {
          /* cross-tabulation mode - save results in a matrix, print later */
          const size_t row_field = dm->grps[0].num;
          safe_line_record_get_fieldz (line, row_field,
                                       row_name, sizeof row_name);

          const size_t col_field = dm->grps[1].num;
          safe_line_record_get_fieldz (line, col_field,
                                       col_name, sizeof col_name);

          field_op_summarize (&dm->ops[0]);
          const char* data = dm->ops[0].out_buf;

          crosstab_add_result (crosstab, row_name, col_name, data);
        }
      else
        {
          /* group-by/per-line mode - print results once available */
          print_input_line (line);
          summarize_field_ops ();
        }
    }
  lines_in_group = 0;
  reset_field_ops ();
}

/*
    Process each line in the input.

    If the key fo the current line is different from the prevopis one,
    summarize the previous group and start a new one.
 */
static void
process_file ()
{
  struct line_record_t lb1, lb2;
  struct line_record_t *thisline, *group_first_line;

  thisline = &lb1;
  group_first_line = &lb2;

  line_record_init (thisline);
  line_record_init (group_first_line);

  /* If there is an input header line, and it wasn't read already
     in 'open_input' - read it now */
  if (input_header && line_number==0)
    process_input_header (input_stream);

  /* If there is an input header line, and the user requested an output
     header line, and the input line was read successfully, print headers */
  if (input_header && output_header && line_number==1)
    print_column_headers ();


  while (true)
    {
      bool new_group = false;

      if (!line_record_fread (thisline, input_stream, eolchar, skip_comments))
        break;
      line_number++;

      /* If there's no input header line, and the user requested an output
         header line, then generate output header line based on the number
         of fields in the first (data, non-header) input line */
      if (line_number==1 && output_header && !input_header)
        {
          build_input_line_headers (thisline, false);
          print_column_headers ();
        }


      /* If no keys are given, the entire input is considered one group */
      if (dm->num_grps || line_mode)
        {
          new_group = (group_first_line->lbuf.length == 0 || line_mode
                       || different (thisline, group_first_line));

          if (new_group)
            {
              process_group (group_first_line);
              group_first_line->lbuf.length = 0;
            }
        }
      else
        {
          /* The entire line is a "group", if it's the first line, keep it */
          new_group = (group_first_line->lbuf.length==0);
        }

      lines_in_group++;
      bool keep_line = process_line (thisline);

      if (new_group || keep_line)
        {
          SWAP_LINES (group_first_line, thisline);
        }
    }

  /* summarize last group */
  process_group (group_first_line);

  line_record_free (&lb1);
  line_record_free (&lb2);
}

/*
    Transpose rows and columns in input file
 */
static void
transpose_file ()
{
  size_t num_lines = 0;
  size_t alloc_lines = 0;
  struct line_record_t *lines = NULL;

  size_t max_num_fields = 0 ;
  size_t prev_num_fields = 0 ;

  /* Read all input lines - but instead of reusing line_record_t,
     keep all lines in memory. */
  while (true)
    {
      if (num_lines+1 > alloc_lines)
        {
          alloc_lines += 1000;
          lines = xnrealloc ( lines, alloc_lines,
                              sizeof (struct line_record_t));
        }

      struct line_record_t *thisline = &lines[num_lines];
      num_lines++;
      line_record_init (thisline);

      if (!line_record_fread (thisline, input_stream, eolchar, skip_comments))
        break;
      line_number++;

      const size_t num_fields = line_record_num_fields (thisline);

      if (strict && line_number>1 && num_fields != prev_num_fields)
          die (EXIT_FAILURE, 0, _("transpose input error: line %"PRIuMAX" " \
                    "has %"PRIuMAX" fields (previous lines had %"PRIuMAX");\n" \
                    "see --help to disable strict mode"),
                    (uintmax_t)line_number, (uintmax_t)num_fields,
                    (uintmax_t)prev_num_fields);

      prev_num_fields = num_fields;
      max_num_fields = MAX (max_num_fields,num_fields);

    }

  /* Output all fields */
  for (size_t i = 1 ; i <= max_num_fields ; ++i)
    {
      for (size_t j = 0; j < line_number; ++j)
        {
          if (j>0)
            print_field_separator ();

          const struct line_record_t *line = &lines[j];
          const char* str;
          size_t len;
          if (line_record_get_field (line, i, &str, &len))
            fwrite (str, len, sizeof (char), stdout);
          else
            fputs (missing_field_filler, stdout);
        }
      print_line_separator ();
    }

  /* Free pointers */
  for (size_t i = 0; i < num_lines; ++i)
    line_record_free (&lines[i]);
  free (lines);
}

/*
    Reverse the fields in each line of the input file
 */
static void
reverse_fields_in_file ()
{
  struct line_record_t lr;
  struct line_record_t *thisline;
  size_t prev_num_fields = 0;
  thisline = &lr;
  line_record_init (thisline);

  while (true)
    {
      if (!line_record_fread (thisline, input_stream, eolchar, skip_comments))
        break;
      line_number++;

      const size_t num_fields = line_record_num_fields (thisline);

      if (strict && line_number>1 && num_fields != prev_num_fields)
          die (EXIT_FAILURE, 0, _("reverse-field input error: line " \
                       "%"PRIuMAX" has %"PRIuMAX" fields (previous lines had " \
                       "%"PRIuMAX");\n" \
                       "see --help to disable strict mode"),
                        (uintmax_t)line_number, (uintmax_t)num_fields,
                        (uintmax_t)prev_num_fields);

      prev_num_fields = num_fields;

      /* Special handling for header line */
      if (line_number == 1)
        {
          /* If there is an header line (first line), and the user did not
             request printing the header, skip it */
          if (input_header && !output_header)
            continue;

          /* There is no input header line (first line already contains data),
             and the user requested to generate output header line.
             Print dummy header lines. */
          if (!input_header && output_header)
            {
              build_input_line_headers (thisline, false);
              for (size_t i = num_fields; i > 0; --i)
                {
                  if (i < num_fields)
                    print_field_separator ();
                  fputs (get_input_field_name (i), stdout);
                }
              print_line_separator ();
            }
        }

      /* Print the line, reversing field order */
      for (size_t i = num_fields; i > 0; --i)
        {
          if (i < num_fields)
            print_field_separator ();

          const char* str = NULL;
          size_t len = 0 ;
          ignore_value (line_record_get_field (thisline, i, &str, &len));
          fwrite (str, len, sizeof (char), stdout);
        }
      print_line_separator ();
    }
  line_record_free (&lr);
}

/*
   read, parse, print file - without any additional operations
 */
static void
noop_file ()
{
  struct line_record_t lr;
  struct line_record_t *thisline;

  thisline = &lr;
  line_record_init (thisline);
  while (true)
    {
      if (!line_record_fread (thisline, input_stream, eolchar, skip_comments))
        break;
      line_number++;

      if (print_full_line)
        {
          ignore_value (fwrite (line_record_buffer (thisline),
                                line_record_length (thisline), sizeof (char),
                                stdout));
          print_line_separator ();
        }
    }
  line_record_free (&lr);
}

/*
   Read file, ensure it is in tabular format
   (same number of fields in each line)
 */
static void
tabular_check_file ()
{
  size_t prev_num_fields=0;
  struct line_record_t lb1, lb2;
  struct line_record_t *thisline, *prevline;

  const uintmax_t n_lines = dm->mode_params.check_params.n_lines;
  const uintmax_t n_fields = dm->mode_params.check_params.n_fields;

  thisline = &lb1;
  prevline = &lb2;

  line_record_init (thisline);
  line_record_init (prevline);

  while (true)
    {
      if (!line_record_fread (thisline, input_stream, eolchar, skip_comments))
        break;
      line_number++;

      const size_t num_fields = line_record_num_fields (thisline);

      /* Check if the number of fields is different than expected/requested
         on the command line (e.g. with 'datamash check 6 fields') */
      if (n_fields && n_fields != num_fields)
        {
          fprintf (stderr, _("line %"PRIuMAX" (%"PRIuMAX" fields):\n  "),
                       (uintmax_t)(line_number), (uintmax_t)num_fields);
          ignore_value (fwrite (line_record_buffer (thisline),
                                line_record_length (thisline), sizeof (char),
                                stderr));
          fputc ('\n', stderr);
          die (EXIT_FAILURE, 0, _("check failed: line " \
                       "%"PRIuMAX" has %"PRIuMAX" fields (expecting "\
                       "%"PRIuMAX")"),
                       (uintmax_t)line_number, (uintmax_t)num_fields,
                       (uintmax_t)n_fields);
        }

      /* Check if the number of fields changed from one line to the next
         (only if no expected number of fields specified on the command line).*/
      else if (line_number>1 && num_fields != prev_num_fields)
        {
          fprintf (stderr, _("line %"PRIuMAX" (%"PRIuMAX" fields):\n  "),
                       (uintmax_t)(line_number-1), (uintmax_t)prev_num_fields);
          ignore_value (fwrite (line_record_buffer (prevline),
                                line_record_length (prevline), sizeof (char),
                                stderr));
          fputc ('\n', stderr);
          fprintf (stderr, _("line %"PRIuMAX" (%"PRIuMAX" fields):\n  "),
                       (uintmax_t)(line_number), (uintmax_t)num_fields);
          ignore_value (fwrite (line_record_buffer (thisline),
                                line_record_length (thisline), sizeof (char),
                                stderr));
          fputc ('\n', stderr);
          die (EXIT_FAILURE, 0, _("check failed: line " \
                       "%"PRIuMAX" has %"PRIuMAX" fields (previous line had "\
                       "%"PRIuMAX")"),
                       (uintmax_t)line_number, (uintmax_t)num_fields,
                       (uintmax_t)prev_num_fields);
        }
      prev_num_fields = num_fields;

      SWAP_LINES (prevline, thisline);
    }

  /* Check if we read too many/few lines */
  if (n_lines && n_lines != line_number)
    {
      die (EXIT_FAILURE, 0, _("check failed: input had %"PRIuMAX" lines " \
                              "(expecting %"PRIuMAX")"),
           (uintmax_t)line_number, (uintmax_t)n_lines);
    }

  /* Print summary */
  printf (ngettext ("%"PRIuMAX" line", "%"PRIuMAX" lines",
          select_plural (line_number)), (uintmax_t)line_number);
  fputs (", ", stdout);
  printf (ngettext ("%"PRIuMAX" field", "%"PRIuMAX" fields",
          select_plural (prev_num_fields)), (uintmax_t)prev_num_fields);
  print_line_separator ();

  line_record_free (&lb1);
  line_record_free (&lb2);
}

/*
    Remove lines with duplicates keys from a file
 */
static void
remove_dups_in_file ()
{
  const char* str = NULL;
  size_t len = 0;
  struct line_record_t lr;
  struct line_record_t *thisline;
  Hash_table *ht;
  const size_t init_table_size = rmdup_initial_size;

  char* keys_buffer = NULL;
  size_t keys_buffer_alloc = 0;
  size_t next_key_pos = 0;

  char** buffer_list = NULL;
  size_t buffer_list_alloc = 0;
  size_t buffer_list_size = 0;

  thisline = &lr;
  line_record_init (thisline);
  ht = hash_initialize (init_table_size, NULL,
                        hash_pjw, hash_compare_strings, NULL);

  /* Allocate keys buffer */
  keys_buffer_alloc = rmdup_initial_size ;
  keys_buffer = xmalloc ( keys_buffer_alloc );

  /* List of allocated key-buffers */
  buffer_list = x2nrealloc (NULL, &buffer_list_alloc, sizeof (char*));
  buffer_list[0] = keys_buffer;
  buffer_list_size = 1 ;

  if (input_header)
    {
      if (line_record_fread (thisline, input_stream, eolchar, skip_comments))
        {
          line_number++;

          /* If using named-columns, find the column numbers after reading the
             header line. */
          if (dm->header_required)
            {
              build_input_line_headers (&lr, true);
              group_columns_find_named_columns ();
            }

          if (output_header)
            {
              ignore_value (fwrite (line_record_buffer (thisline),
                                    line_record_length (thisline),
                                    sizeof (char), stdout));
              print_line_separator ();
            }
        }
    }

  assert (dm->num_grps==1); /* LCOV_EXCL_LINE */
  const size_t key_col = dm->grps[0].num;

  /* TODO: handle (output_header && !input_header) by generating dummy headers
           after the first line is read, and the number of fields is known. */

  while (true)
    {
      if (!line_record_fread (thisline, input_stream, eolchar, skip_comments))
        break;
      line_number++;

       if (!line_record_get_field (thisline, key_col, &str, &len))
         error_not_enough_fields (key_col, line_record_num_fields (thisline));

       /* Add key to the key buffer */
       if (next_key_pos + len + 1 > keys_buffer_alloc)
         {
           keys_buffer = xmalloc ( keys_buffer_alloc ) ;
           next_key_pos = 0;

           /* Add new key-buffer to the list */
           if (buffer_list_size == buffer_list_alloc)
              buffer_list = x2nrealloc (buffer_list,
                                        &buffer_list_alloc, sizeof (char*));
           buffer_list[buffer_list_size++] = keys_buffer;
         }
       char *next_key = keys_buffer+next_key_pos;
       memcpy (next_key, str, len);
       next_key[len] = 0;

       /* Add key to buffer (if not found) */
       const int i = hash_insert_if_absent (ht, next_key, NULL);
       if ( i == -1 )
         die (EXIT_FAILURE, errno, _("hash memory allocation error"));

       if ( i == 1 )
         {
           /* This string was not found in the hash - new key */
           next_key_pos += len+1;
           ignore_value (fwrite (line_record_buffer (thisline),
                                 line_record_length (thisline), sizeof (char),
                                 stdout));
           print_line_separator ();
         }
    }
  line_record_free (&lr);
  hash_free (ht);
  for (size_t i = 0 ; i < buffer_list_size; ++i)
    free (buffer_list[i]);
  free (buffer_list);
}


static void
open_input ()
{
  if (pipe_through_sort && dm->num_grps>0)
    {
      char tmp[INT_BUFSIZE_BOUND (size_t)*2+5];
      char cmd[1024];
      memset (cmd,0,sizeof (cmd));

      if (input_header)
        {
          /* Set no-buffering, to ensure only the first line is consumed */
          setbuf (stdin,NULL);
          /* Read the header line from STDIN, and pass the rest of it to
             the 'sort' child-process */
          process_input_header (stdin);
        }
#ifdef SORT_WITHOUT_LOCALE
      /* For mingw/windows systems */
      strcat (cmd,"sort ");
#else
      strcat (cmd,"LC_ALL=C sort ");
#endif
      if (!case_sensitive)
        strcat (cmd,"-f ");
#ifdef HAVE_STABLE_SORT
      /* stable sort (-s) is needed to support first/last operations
         (prevent sort from re-ordering lines which are not part of the group.
         '-s' is not standard POSIX, but very commonly supported, including
         on GNU coreutils, Busybox, FreeBSD, MacOSX */
      strcat (cmd,"-s ");
#endif
      if (in_tab != TAB_WHITESPACE)
        {
          /* If the delimiter is a single-quote character, use
             double-quote to prevent shell quoting problems. */
          const char qc = (in_tab=='\'')?'"':'\'';
          snprintf (tmp,sizeof (tmp),"-t %c%c%c ",qc,in_tab,qc);
          strcat (cmd,tmp);
        }
      for (size_t i = 0; i < dm->num_grps; ++i)
        {
          const size_t col_num = dm->grps[i].num;
          snprintf (tmp,sizeof (tmp),"-k%"PRIuMAX",%"PRIuMAX" ",
                    (uintmax_t)col_num,(uintmax_t)col_num);
          if (strlen (tmp)+strlen (cmd)+1 >= sizeof (cmd))
            die (EXIT_FAILURE, 0,
                   _("sort command too-long (please report this bug)"));
          strcat (cmd,tmp);
        }

      input_stream = popen (cmd,"r");
      if (input_stream == NULL)
        die (EXIT_FAILURE, 0, _("failed to run 'sort': popen failed"));
    }
  else
    {
      /* without grouping, there's no need to sort */
      input_stream = stdin;
      pipe_through_sort = false;
    }
}

static void
close_input ()
{
  int i;

  if (ferror (input_stream))
    die (EXIT_FAILURE, errno, _("read error"));

  if (pipe_through_sort)
    i = pclose (input_stream);
  else
    i = fclose (input_stream);

  if (i != 0)
    die (EXIT_FAILURE, errno, _("read error (on close)"));
}

int main (int argc, char* argv[])
{
  int optc;
  enum processing_mode premode = MODE_INVALID;
  const char* premode_group_spec = NULL;

  DECL_LONG_DOUBLE_ROUNDING
  BEGIN_LONG_DOUBLE_ROUNDING ();

  set_program_name (argv[0]);

#ifdef FORCE_C_LOCALE
  /* Used on mingw/windows system */
  setlocale (LC_ALL, "C");
#else
  setlocale (LC_ALL, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  init_blank_table ();
  init_random ();

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, short_options, long_options, NULL))
         != -1)
    {
      switch (optc)
        {
	case 'C':
	  skip_comments = true;
	  break;

        case 'F':
          missing_field_filler = optarg;
          break;

        case 'f':
          print_full_line = true;
          break;

        case 'g':
          premode = MODE_GROUPBY;
          premode_group_spec = optarg;
          break;

        case 'i':
          case_sensitive = false;
          break;

        case 'z':
          eolchar = 0;
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

	case 'R':
	  set_numeric_output_precision (optarg);
	  break;

        case 's':
          pipe_through_sort = true;
          break;

        case NO_STRICT_OPTION:
          strict = false;
          break;

        case CUSTOM_FORMAT_OPTION:
          set_numeric_printf_format (optarg);
          break;

        case REMOVE_NA_VALUES_OPTION:
          remove_na_values = true;
          break;

        case 't':
          if (optarg[0] == '\0' || optarg[1] != '\0')
            die (EXIT_FAILURE, 0,
                   _("the delimiter must be a single character"));
          in_tab = out_tab = optarg[0];
          break;

	case OUTPUT_DELIMITER_OPTION:
          if (optarg[0] == '\0' || optarg[1] != '\0')
            die (EXIT_FAILURE, 0,
                   _("the delimiter must be a single character"));
	  explicit_output_delimiter = (char)optarg[0];
	  break;

        case 'W':
          in_tab = TAB_WHITESPACE;
          out_tab = '\t';
          break;

        case UNDOC_PRINT_INF_OPTION:
        case UNDOC_PRINT_NAN_OPTION:
          field_op_print_empty_value ( (optc==UNDOC_PRINT_INF_OPTION)
                                       ?OP_MAX:OP_MEAN );
          exit (EXIT_SUCCESS);
          break;

        case UNDOC_PRINT_PROGNAME_OPTION:
          printf ("%s", program_name);
          exit (EXIT_SUCCESS);
          break;

        case UNDOC_RMDUP_TEST:
          rmdup_initial_size = 1024;
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

  /* If --output-delimiter=X was used, override any previous output delimiter */
  if (explicit_output_delimiter != -1)
    out_tab = explicit_output_delimiter ;

  /* The rest of the parameters are the operations */
  if (premode == MODE_INVALID)
    dm = datamash_ops_parse (argc - optind, (const char**)argv+optind);
  else
    dm = datamash_ops_parse_premode (premode, premode_group_spec,
                            argc - optind, (const char**)argv+optind);

  /* If using named-columns, but no input header - abort */
  if (dm->header_required && !input_header)
    die (EXIT_FAILURE, 0,
           _("-H or --header-in must be used with named columns"));

  open_input ();
  switch (dm->mode)                              /* LCOV_EXCL_BR_LINE */
    {
    case MODE_PER_LINE:
      line_mode = true;
      /* fall through */
    case MODE_GROUPBY:
      process_file ();
      break;

    case MODE_NOOP:
      noop_file ();
      break;

    case MODE_TRANSPOSE:
      transpose_file ();
      break;

    case MODE_REVERSE:
      reverse_fields_in_file ();
      break;

    case MODE_REMOVE_DUPS:
      remove_dups_in_file ();
      break;

    case MODE_CROSSTAB:
      assert ( dm->num_grps== 2 ); /* LCOV_EXCL_LINE */
      assert ( dm->num_ops == 1 ); /* LCOV_EXCL_LINE */
      crosstab_mode = true;
      crosstab = crosstab_init ();
      process_file ();
      crosstab_print (crosstab);
      crosstab_free (crosstab);
      break;

    case MODE_TABULAR_CHECK:
      tabular_check_file ();
      break;

    case MODE_INVALID:                           /* LCOV_EXCL_LINE */
    default:                                     /* LCOV_EXCL_LINE */
      internal_error ("op mode");                /* LCOV_EXCL_LINE */
    }
  free_column_headers ();
  close_input ();
  datamash_ops_free (dm);

  END_LONG_DOUBLE_ROUNDING ();

  return EXIT_SUCCESS;
}

/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=8: */
/* vim: set expandtab: */

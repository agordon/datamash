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
#include <getopt.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "system.h"

#include "closeout.h"
#include "hash.h"
#include "hash-pjw.h"
#include "error.h"
#include "lib/intprops.h"
#include "ignore-value.h"
#include "stdnoreturn.h"
#include "linebuffer.h"
#define Version VERSION
#include "version-etc.h"
#include "xalloc.h"

#include "text-options.h"
#include "text-lines.h"
#include "column-headers.h"
#include "field-ops.h"
#include "utils.h"

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

static size_t *group_columns = NULL;
static size_t num_group_colums = 0;
static bool line_mode = false; /* if TRUE, handle each line as a group */

static bool pipe_through_sort = false;
static FILE* input_stream = NULL;

/* if true, 'transpose' and 'reverse' require every line to have
   the exact same number of fields. Otherwise, the program
   will fail with non-zero exit code. */
static bool strict = true;

/* if 'strict' is false, lines with fewer-than-expected fields
   will be filled with this value */
static char* missing_field_filler = "N/A";


enum
{
  INPUT_HEADER_OPTION = CHAR_MAX + 1,
  OUTPUT_HEADER_OPTION,
  NO_STRICT_OPTION
};

static char const short_options[] = "sfF:izg:t:HW";

static struct option const long_options[] =
{
  {"zero-terminated", no_argument, NULL, 'z'},
  {"field-separator", required_argument, NULL, 't'},
  {"whitespace", no_argument, NULL, 'W'},
  {"group", required_argument, NULL, 'g'},
  {"ignore-case", no_argument, NULL, 'i'},
  {"header-in", no_argument, NULL, INPUT_HEADER_OPTION},
  {"header-out", no_argument, NULL, OUTPUT_HEADER_OPTION},
  {"headers", no_argument, NULL, 'H'},
  {"full", no_argument, NULL, 'f'},
  {"filler", required_argument, NULL, 'F'},
  {"sort", no_argument, NULL, 's'},
  {"no-strict", no_argument, NULL, NO_STRICT_OPTION},
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
      printf (_("Usage: %s [OPTION] op [col] [op col ...]\n"),
          program_name);
      fputs ("\n", stdout);
      fputs (_("Performs numeric/string operations on input from stdin."),
          stdout);
      fputs ("\n\n", stdout);
      fputs (_("'op' is the operation to perform;\n"), stdout);
      fputs (_("\
For grouping,per-line operations 'col' is the input field to use.\
"), stdout);
      fputs ("\n\n", stdout);
      fputs (_("File operations:\n"),stdout);
      fputs ("  transpose, reverse\n",stdout);

      fputs (_("Line-Filtering operations:\n"),stdout);
      fputs ("  rmdup\n",stdout);

      fputs (_("Per-Line operations:\n"),stdout);
      fputs ("  base64, debase64, md5, sha1, sha256, sha512\n",stdout);

      fputs (_("Numeric Grouping operations:\n"),stdout);
      fputs ("  sum, min, max, absmin, absmax\n",stdout);

      fputs (_("Textual/Numeric Grouping operations:\n"),stdout);
      fputs ("  count, first, last, rand, unique, collapse, countunique\n",
             stdout);

      fputs (_("Statistical Grouping operations:\n"),stdout);
      fputs ("\
  mean, median, q1, q3, iqr, mode, antimode, pstdev, sstdev, pvar\n\
  svar, mad, madraw, pskew, sskew, pkurt, skurt, dpo, jarque\n\
\n", stdout);
      fputs ("\n", stdout);

      fputs (_("Options:\n"),stdout);
      fputs (_("Grouping Options:\n"),stdout);
      fputs (_("\
  -f, --full                print entire input line before op results\n\
                              (default: print only the grouped keys)\n\
"), stdout);
      fputs (_("\
  -g, --group=X[,Y,Z]       group via fields X,[Y,Z]\n\
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
      printf ("  man %s\n", program_name);
      fputs (_("The manual and more examples are available at\n"), stdout);
      fputs ("  " PACKAGE_URL "\n\n", stdout);
    }
  exit (status);
}

static inline noreturn void
error_not_enough_fields (const size_t needed, const size_t found)
{
  error (0, 0, _("invalid input: field %zu requested, " \
        "line %zu has only %zu fields"),
        needed, line_number, found);
  exit (EXIT_FAILURE);
}


static inline void
safe_line_record_get_field (const struct line_record_t *lr, const size_t n,
                            const char ** /* out */ pptr, size_t* /*out*/ plen)
{
  if (!line_record_get_field (lr, n, pptr, plen))
    error_not_enough_fields (n, line_record_num_fields (lr));
}

/* returns TRUE if the lines are different, false if identical.
 * comparison is based on the specified keys */
/* copied from coreutils's src/uniq.c (in the key-spec branch) */
static bool
different (const struct line_record_t* l1, const struct line_record_t* l2)
{
  assert (group_columns != NULL); /* LCOV_EXCL_LINE */
  for (size_t *key = group_columns; *key ; ++key )
    {
      const char *str1=NULL,*str2=NULL;
      size_t len1=0,len2=0;
      safe_line_record_get_field (l1, *key, &str1, &len1);
      safe_line_record_get_field (l2, *key, &str2, &len2);
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
static void
process_line (const struct line_record_t *line)
{
  const char *str = NULL;
  size_t len = 0;
  enum FIELD_OP_COLLECT_RESULT flocr;

  struct fieldop *op = field_ops;
  while (op)
    {
      safe_line_record_get_field (line, op->field, &str, &len);
      flocr = field_op_collect (op, str, len);
      if (flocr != FLOCR_OK)
        {
          char *tmp = xmalloc (len+1);
          memcpy (tmp,str,len);
          tmp[len] = 0 ;
          error (EXIT_FAILURE, 0,
              _("%s in line %zu field %zu: '%s'"),
              field_op_collect_result_name (flocr),
              line_number, op->field, tmp);
        }

      op = op->next;
    }
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
      for (size_t *key = group_columns; key && *key != 0 ; ++key)
        {
          safe_line_record_get_field (lb, *key, &str, &len);
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
      for (size_t n=1; n<=get_num_column_headers (); ++n)
        {
          fputs (get_input_field_name (n), stdout);
          print_field_separator ();
        }
    }
  else
    {
      for (size_t *key = group_columns; key && *key != 0; ++key)
        {
          if (*key > get_num_column_headers ())
            error_not_enough_fields (*key, get_num_column_headers ());
          printf ("GroupBy" "(%s)",get_input_field_name (*key));
          print_field_separator ();
        }
    }

  for (struct fieldop *op = field_ops; op ; op=op->next)
    {
      if (op->field > get_num_column_headers ())
        error_not_enough_fields (op->field, get_num_column_headers ());
      printf ("%s" "(%s)",op->name, get_input_field_name (op->field));

      if (op->next)
        print_field_separator ();
    }

  /* print end-of-line */
  print_line_separator ();
}

/*
    Process the input header line
*/
static void
process_input_header (FILE *stream)
{
  struct line_record_t lr;

  line_record_init (&lr);
  if (line_record_fread (&lr, stream, eolchar))
    {
      build_input_line_headers (&lr, true);
      line_number++;
    }
  line_record_free (&lr);
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

  if (input_header)
    process_input_header (input_stream);

  while (true)
    {
      bool new_group = false;

      if (!line_record_fread (thisline, input_stream, eolchar))
        break;

      /* If asked to print the output headers,
         and the input doesn't have headers,
         then count the number of fields in first input line.
         NOTE: 'input_header' might be false if 'sort piping' was used with
               header, but in that case, line_number will be 1. */
      if (line_number==0 && output_header && !input_header)
        build_input_line_headers (thisline, false);

      /* Print output header, only after reading the first line */
      if (output_header && line_number==1)
        print_column_headers ();

      line_number++;

      /* If no keys are given, the entire input is considered one group */
      if (num_group_colums || line_mode)
        {
          new_group = (group_first_line->lbuf.length == 0 || line_mode
                       || different (thisline, group_first_line));

          if (new_group && lines_in_group>0)
            {
              print_input_line (group_first_line);
              summarize_field_ops ();
              reset_field_ops ();
              lines_in_group = 0 ;
              group_first_line->lbuf.length = 0;
            }
        }
      else
        {
          /* The entire line is a "group", if it's the first line, keep it */
          new_group = (group_first_line->lbuf.length==0);
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

      if (!line_record_fread ( thisline, input_stream, eolchar))
        break;
      line_number++;

      const size_t num_fields = line_record_num_fields (thisline);

      if (strict && line_number>1 && num_fields != prev_num_fields)
          error (EXIT_FAILURE, 0, _("transpose input error: line %zu has " \
                       "%zu fields (previous lines had %zu);\n" \
                       "see --help to disable strict mode"),
                        line_number, num_fields, prev_num_fields);

      prev_num_fields = num_fields;

    }

  /* Output all fields */
  for (size_t i = 1 ; i <= prev_num_fields ; ++i)
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
    Revers the fields in each line of the input file
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
      if (!line_record_fread (thisline, input_stream, eolchar))
        break;
      line_number++;

      const size_t num_fields = line_record_num_fields (thisline);

      if (strict && line_number>1 && num_fields != prev_num_fields)
          error (EXIT_FAILURE, 0, _("reverse-field input error: line %zu has " \
                       "%zu fields (previous lines had %zu);\n" \
                       "see --help to disable strict mode"),
                         line_number, num_fields, prev_num_fields);

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
      if (!line_record_fread (thisline, input_stream, eolchar))
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
    Remove lines with duplicates keys from a file
 */
static void
remove_dups_in_file ()
{
  const char* str = NULL;
  size_t len = 0;
  struct line_record_t lr;
  struct line_record_t *thisline;
  const size_t key_col = field_ops->field;
  Hash_table *ht;
  const size_t init_table_size = 100000;

  char* keys_buffer = NULL;
  size_t keys_buffer_alloc = 0;
  size_t next_key_pos = 0;

  char** buffer_list = NULL;
  size_t buffer_list_alloc = 0;
  size_t buffer_list_size = 0;

  assert (field_ops != NULL); /* LCOV_EXCL_LINE */

  thisline = &lr;
  line_record_init (thisline);
  ht = hash_initialize (init_table_size, NULL,
                        hash_pjw, hash_compare_strings, NULL);

  /* Allocate keys buffer */
  keys_buffer_alloc = 1000000 ;
  keys_buffer = xmalloc ( keys_buffer_alloc );

  /* List of allocated key-buffers */
  buffer_list_alloc = 1000 ;
  buffer_list = xnmalloc (buffer_list_alloc, sizeof (char*));
  buffer_list[0] = keys_buffer;
  buffer_list_size = 1 ;

  if (input_header)
    {
      if (line_record_fread (thisline, input_stream, eolchar))
        {
          line_number++;
          if (output_header)
            {
              ignore_value (fwrite (line_record_buffer (thisline),
                                    line_record_length (thisline),
                                    sizeof (char), stdout));
              print_line_separator ();
            }
        }
    }
  /* TODO: handle (output_header && !input_header) by generating dummy headers
           after the first line is read, and the number of fields is known. */

  while (true)
    {
      if (!line_record_fread (thisline, input_stream, eolchar))
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
           if (buffer_list_size >= buffer_list_alloc)
             {
               buffer_list_alloc += 1000 ;
               buffer_list = xnrealloc (buffer_list, buffer_list_alloc,
                                        sizeof (char*));
             }
           buffer_list[buffer_list_size++] = keys_buffer;
         }
       char *next_key = keys_buffer+next_key_pos;
       memcpy (next_key, str, len);
       next_key[len] = 0;

       /* Add key to buffer (if not found) */
       const int i = hash_insert_if_absent (ht, next_key, NULL);
       if ( i == -1 )
         error (EXIT_FAILURE, errno, _("hash memory allocation error"));

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
  if (pipe_through_sort && group_columns)
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
          input_header = false;
        }
      strcat (cmd,"LC_ALL=C sort ");
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
          snprintf (tmp,sizeof (tmp),"-t '%c' ",in_tab);
          strcat (cmd,tmp);
        }
      for (size_t *key = group_columns; *key; ++key)
        {
          snprintf (tmp,sizeof (tmp),"-k%zu,%zu ",*key,*key);
          if (strlen (tmp)+strlen (cmd)+1 >= sizeof (cmd))
            error (EXIT_FAILURE, 0,
                   _("sort command too-long (please report this bug)"));
          strcat (cmd,tmp);
        }

      input_stream = popen (cmd,"r");
      if (input_stream == NULL)
        error (EXIT_FAILURE, 0, _("failed to run 'sort': popen failed"));
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
    error (EXIT_FAILURE, errno, _("read error"));

  if (pipe_through_sort)
    i = pclose (input_stream);
  else
    i = fclose (input_stream);

  if (i != 0)
    error (EXIT_FAILURE, errno, _("read error (on close)"));
}

static void
free_sort_keys ()
{
  free (group_columns);
}

/* Parse the "--group=X[,Y,Z]" parameter, populating 'keylist' */
static void
parse_group_spec ( char* spec )
{
  long int val;
  size_t idx;
  char *endptr;

  /* Count number of groups parameters, by number of commas */
  num_group_colums = 1;
  endptr = spec;
  while ( *endptr != '\0' )
    {
      if (*endptr == ',')
        num_group_colums++;
      ++endptr;
    }

  /* Allocate the group_columns */
  group_columns = xnmalloc (num_group_colums+1, sizeof (size_t));

  errno = 0 ;
  idx=0;
  while (1)
    {
      val = strtol (spec, &endptr, 10);
      if (errno != 0 || endptr == spec || val<=0)
        error (EXIT_FAILURE, 0, _("invalid field value for grouping '%s'"),
                                        spec);

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
}

int main (int argc, char* argv[])
{
  int optc;
  enum operation_mode op_mode;
  set_program_name (argv[0]);

  setlocale (LC_ALL, "");
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
        case 'F':
          missing_field_filler = optarg;
          break;

        case 'f':
          print_full_line = true;
          break;

        case 'g':
          parse_group_spec (optarg);
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

        case 's':
          pipe_through_sort = true;
          break;

        case NO_STRICT_OPTION:
          strict = false;
          break;

        case 't':
          if (optarg[0] != '\0' && optarg[1] != '\0')
            error (EXIT_FAILURE, 0,
                   _("the delimiter must be a single character"));
          in_tab = out_tab = optarg[0];
          break;

        case 'W':
          in_tab = TAB_WHITESPACE;
          out_tab = '\t';
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

  op_mode = parse_operation_mode (argc, optind, argv);

  open_input ();
  switch (op_mode)
    {
    case LINE_MODE:
      line_mode = true;
      /* fall through */
    case GROUPING_MODE:
      process_file ();
      break;

    case NOOP_MODE:
      noop_file ();
      break;

    case TRANSPOSE_MODE:
      transpose_file ();
      break;

    case REVERSE_FIELD_MODE:
      reverse_fields_in_file ();
      break;

    case REMOVE_DUPS_MODE:
      remove_dups_in_file ();
      break;

    case UNKNOWN_MODE:
    default:
      internal_error ("op mode"); /* LCOV_EXCL_LINE */
    }
  free_field_ops ();
  free_sort_keys ();
  free_column_headers ();
  close_input ();

  return EXIT_SUCCESS;
}

/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=8: */
/* vim: set expandtab: */

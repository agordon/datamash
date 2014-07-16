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

#include <ctype.h>
#include <getopt.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "system.h"

#include "closeout.h"
#include "error.h"
#include "lib/intprops.h"
#include "linebuffer.h"
#include "ignore-value.h"
#define Version VERSION
#include "version-etc.h"
#include "xalloc.h"

#include "text-options.h"
#include "text-lines.h"
#include "column-headers.h"
#include "field-ops.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "datamash"

#define AUTHORS proper_name ("Assaf Gordon")

/* Until someone better comes along */
const char version_etc_copyright[] = "Copyright %s %d Assaf Gordon" ;

#ifdef ENABLE_BUILTIN_DEBUG
/* enable debugging */
bool debug = false;
#endif

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

static bool pipe_through_sort = false;
static FILE* input_stream = NULL;

enum
{
  INPUT_HEADER_OPTION = CHAR_MAX + 1,
  OUTPUT_HEADER_OPTION,
#ifdef ENABLE_BUILTIN_DEBUG
  DEBUG_OPTION
#endif
};

static char const short_options[] = "sfizg:t:HW";

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
  {"sort", no_argument, NULL, 's'},
#ifdef ENABLE_BUILTIN_DEBUG
  {"debug", no_argument, NULL, DEBUG_OPTION},
#endif
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
      fputs (_("Performs numeric/string operations on input from stdin."),
             stdout);
      fputs ("\n\n", stdout);
      fputs (_("'op' is the operation to perform on field 'col'."),
             stdout);
      fputs ("\n\n", stdout);
      fputs (_("Numeric operations:\n"),stdout);
      fputs ("  sum, min, max, absmin, absmax\n",stdout);

      fputs (_("Textual/Numeric operations:\n"),stdout);
      fputs ("  count, first, last, rand \n", stdout);
      fputs ("  unique, collapse, countunique\n",stdout);

      fputs (_("Statistical operations:\n"),stdout);
      fputs ("  mean, median, q1, q3, iqr, mode, antimode\n", stdout);
      fputs ("  pstdev, sstdev, pvar, svar, mad, madraw\n", stdout);
      fputs ("  pskew, sskew, pkurt, skurt, pto, jarque\n", stdout);
      fputs ("\n", stdout);

      fputs (_("Options:\n"),stdout);
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
#ifdef ENABLE_BUILTIN_DEBUG
      fputs (_("\
      --debug               print helpful debugging information\n\
"), stdout);
#endif

      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);

      fputs ("\n", stdout);

      fputs (_("\
\n\
Example:\n\
\n\
Print the sum and the mean of values from column 1:\n\
\n\
"), stdout);

      printf ("\
  $ seq 10 | %s sum 1 mean 1\n\
  55  5.5\n\
\n", program_name);

      fputs (_("For detailed usage information and examples, see\n"),stdout);
      printf ("  man %s\n", program_name);
      fputs (_("The manual and more examples are available at\n"), stdout);
      fputs ("  " PACKAGE_URL "\n", stdout);
    }
  exit (status);
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
#ifdef ENABLE_BUILTIN_DEBUG
      if (debug)
        {
          fprintf(stderr,"diff, key column = %zu, str1='", *key);
          fwrite(str1,sizeof(char),len1,stderr);
          fprintf(stderr,"' str2='");
          fwrite(str2,sizeof(char),len2,stderr);
          fputs("\n",stderr);
        }
#endif
      if (len1 != len2)
        return true;
      if ((case_sensitive && (strncmp(str1,str2,len1)!=0))
          ||
          (!case_sensitive && (strncasecmp(str1,str2,len1)!=0)))
        return true;
    }
  return false;
}

/* For a given line, extract all requested fields and process the associated
   operations on them */
static void
process_line (const struct linebuffer *line)
{
  const char *str;
  size_t len;

  struct fieldop *op = field_ops;
  while (op)
    {
      get_field (line, op->field, &str, &len);
#ifdef ENABLE_BUILTIN_DEBUG
      if (debug)
        {
          fprintf(stderr,"getfield(%zu) = len %zu: '", op->field,len);
          fwrite(str,sizeof(char),len,stderr);
          fprintf(stderr,"'\n");
        }
#endif
      if (!field_op_collect (op, str, len))
        {
          char *tmp = xmalloc(len+1);
          memcpy(tmp,str,len);
          tmp[len] = 0 ;
          error (EXIT_FAILURE, 0,
              _("invalid numeric input in line %zu field %zu: '%s'"),
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
print_input_line (const struct linebuffer* lb)
{
  if (print_full_line)
    {
      size_t len = lb->length;
      const char *buf = lb->buffer;
      if (buf[len-1]==eolchar || buf[len-1]==0)
        len--;
      ignore_value(fwrite (buf, sizeof(char), len, stdout));
      print_field_separator();
    }
  else
    {
      for (size_t *key = group_columns; key && *key != 0 ; ++key)
        {
          const char *str;
          size_t len;
          get_field(lb,*key, &str, &len);
          ignore_value(fwrite(str,sizeof(char),len,stdout));
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
print_column_headers()
{
  if (print_full_line)
    {
      for (size_t n=1; n<=get_num_column_headers(); ++n)
        {
          fputs(get_input_field_name(n), stdout);
          print_field_separator();
        }
    }
  else
    {
      for (size_t *key = group_columns; key && *key != 0; ++key)
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
  print_line_separator();
}

/*
    Process the input header line
*/
static void
process_input_header(FILE *stream)
{
  struct linebuffer lb;

  initbuffer (&lb);
  if (readlinebuffer_delim (&lb, stream, eolchar) != 0)
    {
      linebuffer_nullify (&lb);
      build_input_line_headers(lb.buffer,lb.length,true);
      line_number++;
    }
  freebuffer(&lb);
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

  if (input_header)
    process_input_header(input_stream);

  while (!feof (input_stream))
    {
      bool new_group = false;

      if (readlinebuffer_delim (thisline, input_stream, eolchar) == 0)
        break;
      linebuffer_nullify (thisline);

      /* If asked to print the full-line, and the input doesn't have headers,
         then count the number of fields in first input line.
         NOTE: 'input_header' might be false if 'sort piping' was used with header,
                 but in that case, line_number will be 1. */
      if (line_number==0 && print_full_line && !input_header)
        build_input_line_headers(thisline->buffer,thisline->length,false);

      /* Print output header, only after reading the first line */
      if (output_header && line_number==1)
        print_column_headers();

      line_number++;

      /* If no keys are given, the entire input is considered one group */
      if (num_group_colums)
        {
          new_group = (group_first_line->length == 0
                       || different (thisline, group_first_line));

#ifdef ENABLE_BUILTIN_DEBUG
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
#endif

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
      memset(cmd,0,sizeof(cmd));

      if (input_header)
        {
          /* Set no-buffering, to ensure only the first line is consumed */
          setbuf(stdin,NULL);
          /* Read the header line from STDIN, and pass the rest of it to
             the 'sort' child-process */
          process_input_header(stdin);
          input_header = false;
        }
      strcat(cmd,"LC_ALL=C sort ");
      if (!case_sensitive)
        strcat(cmd,"-f ");
#ifdef HAVE_STABLE_SORT
      /* stable sort (-s) is needed to support first/last operations
         (prevent sort from re-ordering lines which are not part of the group.
         '-s' is not standard POSIX, but very commonly supported, including
         on GNU coreutils, Busybox, FreeBSD, MacOSX */
      strcat(cmd,"-s ");
#endif
      if (in_tab != TAB_WHITESPACE)
        {
          snprintf(tmp,sizeof(tmp),"-t '%c' ",in_tab);
          strcat(cmd,tmp);
        }
      for (size_t *key = group_columns; key && *key; ++key)
        {
          snprintf(tmp,sizeof(tmp),"-k%zu,%zu ",*key,*key);
          if (strlen(tmp)+strlen(cmd)+1 >= sizeof(cmd))
            error(EXIT_FAILURE, 0, _("sort command too-long (please report this bug)"));
          strcat(cmd,tmp);
        }

#ifdef ENABLE_BUILTIN_DEBUG
      if (debug)
        fprintf(stderr,"sort cmd = '%s'\n", cmd);
#endif

      input_stream = popen(cmd,"r");
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
close_input()
{
  int i;

  if (ferror (input_stream))
    error (EXIT_FAILURE, 0, _("read error"));

  if (pipe_through_sort)
    i = pclose(input_stream);
  else
    i = fclose(input_stream);

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
#ifdef ENABLE_BUILTIN_DEBUG
  if (debug)
    fprintf(stderr,"parse_group_spec (spec='%s')\n", spec);
#endif

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

#ifdef ENABLE_BUILTIN_DEBUG
  if (debug)
    {
      fprintf(stderr,"group columns (%p) num=%zu = ", group_columns,num_group_colums);
      for (size_t *key=group_columns;key && *key; ++key)
        {
          fprintf(stderr,"%zu ", *key);
        }
      fputs("\n",stderr);
    }
#endif
}

int main(int argc, char* argv[])
{
  int optc;

  set_program_name (argv[0]);

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  init_blank_table();
  init_random();

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

        case 'i':
          case_sensitive = false;
          break;

        case 'z':
          eolchar = 0;
          break;

#ifdef ENABLE_BUILTIN_DEBUG
        case DEBUG_OPTION:
          debug = true;
          break;
#endif
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

  parse_operations (argc, optind, argv);
  open_input ();
  process_file ();
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

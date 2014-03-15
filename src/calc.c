/* calc - perform simple calculation on input data
   Copyright (C) 2013-2014 Assaf Gordon.

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
#define PROGRAM_NAME "calc"

#define AUTHORS proper_name ("Assaf Gordon")

/* Until someone better comes along */
const char version_etc_copyright[] = "Copyright %s %d Assaf Gordon" ;

#ifdef ENABLE_DEBUG
/* enable debugging */
static bool debug = false;
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
static FILE* pipe_input = NULL;

enum
{
  INPUT_HEADER_OPTION = CHAR_MAX + 1,
  OUTPUT_HEADER_OPTION,
#ifdef ENABLE_DEBUG
  DEBUG_OPTION
#endif
};

static char const short_options[] = "sfizg:t:TH";

static struct option const long_options[] =
{
  {"zero-terminated", no_argument, NULL, 'z'},
  {"field-separator", required_argument, NULL, 't'},
  {"group", required_argument, NULL, 'g'},
  {"ignore-case", no_argument, NULL, 'i'},
  {"header-in", no_argument, NULL, INPUT_HEADER_OPTION},
  {"header-out", no_argument, NULL, OUTPUT_HEADER_OPTION},
  {"headers", no_argument, NULL, 'H'},
  {"full", no_argument, NULL, 'f'},
  {"sort", no_argument, NULL, 's'},
#ifdef ENABLE_DEBUG
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
  unique      print comma-separated sorted list of unique values\n\
  collapse    print comma-separed list of all input values\n\
  countunique print number of unique values\n\
\n\
",stdout);
      fputs (_("\
\n\
General options:\n\
  -f, --full                Print entire input line before op results\n\
                            (default: print only the groupped keys)\n\
  -g, --group=X[,Y,Z]       Group via fields X,[Y,Z]\n\
  --header-in               First input line is column headers\n\
  --header-out              Print column headers as first line\n\
  -H, --headers             Same as '--header-in --header-out'\n\
  -i, --ignore-case         Ignore upper/lower case when comparing text\n\
                            This affects grouping, and string operations\n\
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
Print the sum and the mean of values from column 1:\n\
\n\
  $ seq 10 | %s sum 1 mean 1\n\
  55  5.5\n\
\n"), program_name);

      printf (_("\
Group input based on field 1, and sum values (per group) on field 2:\n\
\n\
  $ cat example.txt\n\
  A  10\n\
  A  5\n\
  B  9\n\
  B  11\n\
  $ %s -g 1 sum 2 < example.txt \n\
  A  15\n\
  B  20\n\
\n"), program_name);

      printf (_("\
Unsorted input must be sorted (with '-s'):\n\
\n\
  $ cat example.txt\n\
  A  10\n\
  C  4\n\
  B  9\n\
  C  1\n\
  A  5\n\
  B  11\n\
  $ %s -s -g1 sum 2 < example.txt \n\
  A 15\n\
  B 20\n\
  C 5\n\
\n\
Which is equivalent to:\n\
  $ cat example.txt | sort -k1,1 | %s -g 1 sum 2\n\
\n\
\n\
"), program_name, program_name);

    }
  exit (status);
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
#ifdef ENABLE_DEBUG
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
  long double val=0;
  const char *str;
  size_t len;

  struct fieldop *op = field_ops;
  while (op)
    {
      get_field (line, op->field, &str, &len);
#ifdef ENABLE_DEBUG
      if (debug)
        {
          fprintf(stderr,"getfield(%zu) = len %zu: '", op->field,len);
          fwrite(str,sizeof(char),len,stderr);
          fprintf(stderr,"'\n");
        }
#endif
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
          build_input_line_headers(thisline->buffer,thisline->length,input_header);

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

#ifdef ENABLE_DEBUG
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
      bzero(cmd,sizeof(cmd));

      if (input_header)
        strcat(cmd,"sed -u 1q;");
      strcat(cmd,"LC_ALL=C sort ");
      if (!case_sensitive)
        strcat(cmd,"-f ");
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

#ifdef ENABLE_DEBUG
      if (debug)
        fprintf(stderr,"sort cmd = '%s'\n", cmd);
#endif

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
#ifdef ENABLE_DEBUG
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

#ifdef ENABLE_DEBUG
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

#ifdef ENABLE_DEBUG
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
  free_column_headers ();
  close_input ();

  return EXIT_SUCCESS;
}

/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=8: */
/* vim: set expandtab: */

/* decorate -- auxiliary program for sort preprocessing
   Copyright (C) 2020 Assaf Gordon <assafgordon@gmail.com>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */
#include <config.h>

#include <getopt.h>
#include <limits.h>
#include <inttypes.h>
#include <sys/wait.h>

#include "system.h"
//#include "stdio--.h"
//#include "fadvise.h"
#include "closeout.h"
#include "linebuffer.h"
#include "die.h"
#include "quote.h"
#include "key-compare.h"
#define Version VERSION
#include "version-etc.h"
#include "ignore-value.h"

#include "decorate-functions.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "decorate"

#define AUTHORS \
  proper_name ("Assaf Gordon")

static bool debug_print_pid = false;

static void _GL_ATTRIBUTE_FORMAT ((__printf__, 1, 2))
dbg_printf (char const *msg, ...)
{
  va_list args;
  /* TODO: use gnulib's 'program_name' instead?  */
  if (debug_print_pid)
    fprintf (stderr, "decorate (PID %i):", (int)getpid ());
  else
    fputs ("decorate: ", stderr);

  va_start (args, msg);
  vfprintf (stderr, msg, args);
  va_end (args);
  fputc ('\n', stderr);
}

/* Until someone better comes along */
const char version_etc_copyright[] = "Copyright %s %d Assaf Gordon" ;

/* if true, enable developer's debug messages */
static bool debug = false;

static char eol_delimiter = '\n';

/* List of key field to be decorated  */
static struct keyfield *decorate_keylist = NULL;

static char **sort_extra_args = NULL;
static size_t sort_extra_args_used = 0;
static size_t sort_extra_args_allocated = 0;

static long skip_header_lines = 0 ;
static FILE* header_out = NULL ;

enum
{
  DEBUG_OPTION = CHAR_MAX + 1,
  DECORATE_OPTION,
  UNDECORATE_OPTION,
  PRINT_SORT_ARGS_OPTION,

  /* Sort options */
  COMPRESS_PROGRAM_OPTION,
  CHECK_OPTION,
  NMERGE_OPTION,
  RANDOM_SOURCE_OPTION,
  HEADER_OPTION,
  PARALLEL_OPTION
};

static struct option const longopts[] =
{
  {"key", required_argument, NULL, 'k'},
  {"decorate", no_argument, NULL, DECORATE_OPTION},
  {"undecorate", required_argument, NULL, UNDECORATE_OPTION},
  {"-debug", no_argument, NULL, DEBUG_OPTION},
  {"zero-terminated", no_argument, NULL, 'z'},
  {"print-sort-args", no_argument, NULL, PRINT_SORT_ARGS_OPTION},
  {"header", required_argument, NULL, HEADER_OPTION},

  /* sort options, passed as-is to the sort process */
  {"check", optional_argument, NULL, CHECK_OPTION},
  {"compress-program", required_argument, NULL, COMPRESS_PROGRAM_OPTION},
  {"random-source", required_argument, NULL, RANDOM_SOURCE_OPTION},
  {"stable", no_argument, NULL, 's'},
  {"batch-size", required_argument, NULL, NMERGE_OPTION},
  {"buffer-size", required_argument, NULL, 'S'},
  {"field-separator", required_argument, NULL, 't'},
  {"temporary-directory", required_argument, NULL, 'T'},
  {"unique", no_argument, NULL, 'u'},
  {"parallel", required_argument, NULL, PARALLEL_OPTION},

  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};


void
add_sort_extra_args (char* s)
{
  if (sort_extra_args_used == sort_extra_args_allocated )
    sort_extra_args = x2nrealloc (sort_extra_args,
                                  &sort_extra_args_allocated,
                                  sizeof *sort_extra_args);
  sort_extra_args[sort_extra_args_used++] = xstrdup (s);
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [INPUT]\n\
   or: %s --decorate [OPTION]... [INPUT]\n\
   or: %s --undecorate N [OPTION]... [INPUT]\n\
\n\
"),
              program_name, program_name, program_name);
      fputs (_("\
Converts (and optionally sorts) fields of various formats\n\
"), stdout);

     putchar ('\n');

     fputs (_("\
With --decorate: adds the converted fields to the start\n\
of each line and prints and prints it to STDOUT; does not sort.\n\
"), stdout);

     putchar ('\n');

     fputs (_("\
With --undecorate: removes the first N fields from the input;\n\
Use as post-processing step after sort(1).\n\
"), stdout);

     putchar ('\n');

     fputs (_("\
Without --decorate and --undecorate: automatically decorates the input,\n\
runs sort(1) and undecorates the result; This is the easiest method to use.\n\
"), stdout);

     putchar ('\n');

     fputs (_("\
General Options:\n\
"), stdout);


     fputs (_("\
      --decorate             decorate/convert the specified fields and print\n\
                             the output to STDOUT. Does not automatically run\n\
                             sort(1) or undecorates the output\n\
"), stdout);
     fputs (_("\
      --header=N             does not decorate or sort the first N lines\n\
  -H                         same as --header=N\n\
"), stdout);
      fputs (_("\
  -k, --key=KEYDEF           key/field to sort; same syntax as sort(1),\n\
                             optionally followed by ':method' to convert\n\
                             to the field into a sortable value; see examples\n\
                             and available conversion below\n\
"), stdout);
     fputs (_("\
  -t, --field-separator=SEP  use SEP instead of non-blank to blank transition\n\
"), stdout);
     fputs (_("\
      --print-sort-args      print adjusted parameters for sort(1); Useful\n\
                             when using --decorate and then manually running\n\
                             sort(1)\n\
"), stdout);
     fputs (_("\
      --undecorate=N         removes the first N fields\n\
"), stdout);
      fputs (_("\
  -z, --zero-terminated      line delimiter is NUL, not newline\n\
"), stdout);
     fputs (HELP_OPTION_DESCRIPTION, stdout);
     fputs (VERSION_OPTION_DESCRIPTION, stdout);

     putchar ('\n');

     fputs (_("\
The following options are passed to sort(1) as-is:\n\
"), stdout);

     putchar ('\n');

     fputs (_("\
  -c, --check\n\
      --compress-program\n\
      --random-source\n\
  -s, --stable\n\
      --batch-size\n\
  -S, --buffer-size\n\
  -T, --temporary-directory\n\
  -u, --unique\n\
      --parallel\n\
"), stdout);


     putchar ('\n');

     fputs (_("\
Available conversions methods (use with -k):\n\
"), stdout);

     putchar ('\n');

     for (int i = 0 ; builtin_conversions[i].name; ++i)
       printf ("  %-10s   %s\n", builtin_conversions[i].name,
               builtin_conversions[i].description);

     putchar ('\n');

     fputs (_("\
Examples:\n\
"), stdout);

     putchar ('\n');

     fputs (_("\
The following two invocations are equivalent:\n\
"), stdout);

     putchar ('\n');

     printf ("   %s -k2,2:ipv4 -k3,3nr FILE.TXT\n", program_name);

     putchar ('\n');

     printf ("   %s --decorate -k2,2:ipv4 FILE.TXT | sort -k1,1 -k4,4nr \\\n \
       | %s --undecorate 1\n", program_name, program_name);

     putchar ('\n');

     fputs (_("\
Decorated output of roman numerals:\n\
"), stdout);

     putchar ('\n');

     printf ("  $ printf \"%%s\\n\" C V III IX XI | "
             "%s -k1,1:roman --decorate\n", program_name);
     fputs ("\
  0000100 C\n\
  0000005 V\n\
  0000003 III\n\
  0000009 IX\n\
  0000011 XI\n\
", stdout);


     putchar ('\n');


     fputs (_("For detailed usage information and examples, see\n"),stdout);
     printf ("  man %s\n", program_name);
     fputs (_("The manual and more examples are available at\n"), stdout);
     fputs ("  " PACKAGE_URL "\n\n", stdout);
    }
  exit (status);
}

static bool
decorate_fields (struct linebuffer *linebuf)
{
  bool ok = true;
  struct line l;   /* The struct used by key-compare */
  struct line *a;

  //size_t len = linebuf->length;
  struct keyfield *key = decorate_keylist;

  l.text = linebuf->buffer;
  l.length = linebuf->length;
  l.keybeg = NULL;
  l.keylim = NULL;



  l.keybeg = begfield (&l, decorate_keylist);
  l.keylim = limfield (&l, decorate_keylist);

  if (l.keybeg >= l.keylim)
    {
      if (l.length>0 && l.text[l.length-1]==eol_delimiter)
        {
          if (debug)
            dbg_printf ("chomp");
          l.length--;
        }

      /* TODO: is this the correct way to detect 'no limit' ?
       * (ie compare keys until the end of the line)*/
      l.keylim = l.keybeg + l.length - (l.keybeg - l.text);

      if (debug)
        dbg_printf ("keylim - keybeg = %zu", (l.keylim-l.keybeg));
    }



  a = &l;

  /* For the first iteration only, the key positions have been
     precomputed for us. */
  char *texta = a->keybeg;
  char *lima = a->keylim;

  while (true)
    {
      /* Treat field ends before field starts as empty fields.  */
      lima = MAX (texta, lima);

      /* Find the lengths. */
      size_t lena = lima - texta;

      char *ta;
      size_t tlena;
      char enda IF_LINT (= 0);

      /* Use the keys in-place, temporarily null-terminated.  */
      ta = texta; tlena = lena; enda = ta[tlena]; ta[tlena] = '\0';

      /* Process the extracted key in NUL-terminated string 'ta' */

      if (debug)
        dbg_printf ("extracted field: '%s'", ta);

      if (key->decorate_fn)
        {
          /* Innternal conversions */
          ok = ok && key->decorate_fn (ta);
         }
      else
        {
          /* run external command */
        }

      /* Print field delimiter */
      fputc ((tab == TAB_DEFAULT) ? ' ' : tab, stdout);

      /* restore the field-separator */
      ta[tlena] = enda;

      key = key->next;
      if (! key)
        break;

      /* Find the beginning and limit of the next field.  */
      if (key->eword != SIZE_MAX)
        lima = limfield (a, key);
      else
        lima = a->text + a->length - 1;

      if (key->sword != SIZE_MAX)
        texta = begfield (a, key);
      else
        {
          texta = a->text;
          if (key->skipsblanks)
            {
              //              while (texta < lima && blanks[to_uchar (*texta)])
              // ++texta;
            }
        }
    }
  return ok;
}

static void
decorate_file (const char *infile)
{
  intmax_t linenum = 0;
  struct linebuffer lb;

  if (! (STREQ (infile, "-") || freopen (infile, "r", stdin)))
    die (SORT_FAILURE, errno, "%s", quotef (infile));

  //fadvise (stdin, FADVISE_SEQUENTIAL);

  initbuffer (&lb);

  if (debug)
    dbg_printf ("starting decoreate_file read loop");

  while (!feof (stdin))
    {
      if (readlinebuffer_delim (&lb, stdin, eol_delimiter) == 0)
        break;

      ++linenum;

      if (skip_header_lines)
        {
          if (debug)
            dbg_printf ("skipping header line");

          if (fwrite (lb.buffer, 1, lb.length, header_out) != lb.length)
            die (SORT_FAILURE, errno, _("fwrite (header line) failed"));

          --skip_header_lines;

          /* if no more header lines are left - flush the header lines,
             to ensure they are passed to the output stream before all
             other lines */
          if ((skip_header_lines == 0) && fflush (header_out) == EOF)
            die (SORT_FAILURE, errno, _("fflush (header line) failed"));

          continue;
        }

      if (!decorate_fields (&lb))
        die (SORT_FAILURE, errno, _("conversion failed in line %zu"), linenum);

      if (debug)
        dbg_printf ("writing line: '%.*s'", (int)lb.length, lb.buffer);

      if (fwrite (lb.buffer, 1, lb.length, stdout) != lb.length)
        die (SORT_FAILURE, errno, _("decorate: fwrite failed"));

    }

  // closefiles:
  if (ferror (stdin) || fclose (stdin) != 0)
    die (SORT_FAILURE, 0, _("error reading %s"), quoteaf (infile));

  if (debug)
    dbg_printf ("end decorate_file read loop");

  /* stdout is handled via the atexit-invoked close_stdout function.  */
  free (lb.buffer);
}



static void
undecorate_file (const char *infile, int num_fields)
{
  intmax_t linenum = 0;
  struct linebuffer lb;
  struct keyfield key;
  struct line l;
  memset (&l, 0, sizeof (l));

  /* Create a dummy keyfield, encompasing the first N fields to skip */
  memset (&key, 0, sizeof (key));
  key.sword = num_fields;
  key.skipsblanks = true;

  if (! (STREQ (infile, "-") || freopen (infile, "r", stdin)))
    die (SORT_FAILURE, errno, "%s", quotef (infile));

  //fadvise (stdin, FADVISE_SEQUENTIAL);

  initbuffer (&lb);

  while (!feof (stdin))
    {
      if (readlinebuffer_delim (&lb, stdin, eol_delimiter) == 0)
        break;

      ++linenum;

      l.text = lb.buffer;
      l.length = lb.length;

      /* don't print EOL character, if any */
      if (lb.length>0 && lb.buffer[lb.length-1]==eol_delimiter)
        --l.length;

      /* skip the first N fields */
      char* p = begfield (&l, &key);
      size_t t = l.length - (p-l.text);

      if (debug)
        {
          dbg_printf ("input Line: %zu chars: '%.*s'", l.length,
                     (int)l.length, l.text);
          dbg_printf ("undecorated Line: %zu chars: '%.*s'", t, (int)t, p);
        }
      fwrite (p, 1, t, stdout);

      fputc (eol_delimiter, stdout);
    }

  if (ferror (stdin) || fclose (stdin) != 0)
    die (SORT_FAILURE, 0, _("error reading %s"), quoteaf (infile));

  /* stdout is handled via the atexit-invoked close_stdout function.  */
  free (lb.buffer);
}


/* this code was copied from src/sort.c */
struct keyfield*
parse_sort_key_arg (const char* optarg, char const** endpos)
{
  char const *s;
  struct keyfield key_buf;
  struct keyfield *key = NULL;

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

  /* TODO: strange, sort's original code sets sword=SIZE_MAX for "-k1".
   * force override??? */
  if (key->sword==SIZE_MAX)
    key->sword=0;

  key = insertkey (key); /* returns a newly malloc'd struct */

  *endpos = s;

  return key;
}


static void
parse_external_conversion_spec (const char* optarg, const char *s,
                                struct keyfield *key)
{
  ignore_value (key);

  ++s; /* skip the '@' */

  if (*s == '\0')
    badfieldspec (optarg, N_("missing external conversion command"));

  die (SORT_FAILURE, 0, "external commands are not implemented (yet)");
}


static void
parse_builtin_conversion_spec (const char* optarg, const char *s,
                               struct keyfield *key)
{
  bool found = false;

  ++s; /* skip the ':' */

  if (*s == '\0')
    badfieldspec (optarg, N_("missing internal conversion function"));

  for (int i = 0 ; builtin_conversions[i].name; ++i)
    {
      if (STREQ (s, builtin_conversions[i].name))
        {
          found = true;
          key->decorate_fn = builtin_conversions[i].decorate_fn;
          break;
        }
    }
  if (!found)
    badfieldspec (optarg, N_("invalid built-in conversion option"));
}

void check_allowed_key_flags (const struct keyfield* key)
{
  /* key->reverse is the only ordering flag allowed */
  if (key->skipsblanks || key->skipeblanks
      || (key->ignore != 0) || (key->translate != 0)
      || (key->general_numeric) || (key->human_numeric)
      || (key->month) || (key->numeric)
      || (key->random) || (key->version))
    badfieldspec (optarg, N_("ordering flags (b/d/i/h/n/g/M/R/V) can"
                             "not be combined with a conversion function"));
}

void parse_key_arg (const char* optarg)
{
  char const *s;
  struct keyfield *key = parse_sort_key_arg (optarg, &s);

  switch (*s)
    {
    case '\0':
      /* End of key-spec, this is a standard sort key spec
         to be passed as-is. */
      break;

    case ':':
      /* built-in conversion */
      check_allowed_key_flags (key);
      parse_builtin_conversion_spec (optarg, s, key);
      break;

    case '@':
      /* external command conversion */
      check_allowed_key_flags (key);
      parse_external_conversion_spec (optarg, s, key);
      break;

    default:
      /* invalid key spec character */
      badfieldspec (optarg, N_("invalid key specification"));
      break;

    }
}

void
insert_decorate_key (struct keyfield *key_arg)
{
  struct keyfield **p;
  struct keyfield *key = xmemdup (key_arg, sizeof *key);

  /* decorated fields always skip blanks */
  key->skipsblanks = key->skipeblanks = true;

  for (p = &decorate_keylist; *p; p = &(*p)->next)
    continue;
  *p = key;
  key->next = NULL;
}


int
adjust_key_fields ()
{
  struct keyfield *key = keylist;
  int cnt = 0;
  int idx = 0;

  do {
    if (key->decorate_fn || key->decorate_cmd)
      ++cnt;
  }  while (key && ((key = key->next)));

  if (debug)
    dbg_printf ("found %d decorated fields", cnt);

  key = keylist;
  do {
    if (key->decorate_fn || key->decorate_cmd)
      {
        /* Save the input keyfield spec */
        insert_decorate_key (key);

        /* when passing args to 'sort',
           move decorated key fields to the begining */
        key->sword = idx ; /* note: sword is ZERO based,
                              so 0 is the first column */
        if (key->eword != SIZE_MAX)
          key->eword = idx;

        key->decorate_fn = NULL ;
        key->decorate_cmd = NULL;
        ++idx;
      }
    else
      {
        /* shift all non-decorate key fields by the count
           of decorated fields (which will be inserted at the begining
           of each line)*/
        key->sword += cnt ;
        if (key->eword != SIZE_MAX)
          key->eword += cnt;
      }
  }  while (key && ((key = key->next)));

  return cnt;
}

char**
build_sort_process_args ()
{
  int argc = 2 ; /* one 'sort' program name (argv[0]), one for NULL */
  struct keyfield *key = keylist;

  /* step 1: count number of args */
  do {
    ++argc;
  }  while (key && ((key = key->next)));

  argc += sort_extra_args_used;

  /* Step 2: allocate and build argv.
     The terminating NULL is implicit thanks to calloc. */
  char** argv = xcalloc (argc, sizeof (char*));
  int i = 0;

  argv[i++] = xstrdup ("sort"); /* argv[0] */

  key = keylist;
  do {
    argv[i++] = debug_keyfield (key);
  }  while (key && ((key = key->next)));

  for (size_t j=0;j<sort_extra_args_used;++j)
    {
      argv[i++] = sort_extra_args[j];
    }

  return argv;
}


static void
do_decorate (int optind, int argc, char** argv)
{
  if (optind < argc)
    {
      for (int i = optind; i < argc; ++i)
        decorate_file (argv[i]);
    }
  else
    decorate_file ("-");
}

static void
do_undecorate (int optind, int argc, char** argv, int undecorate_fields)
{
  if (optind < argc)
    {
      for (int i = optind; i < argc; ++i)
        undecorate_file (argv[i], undecorate_fields);
    }
  else
    undecorate_file ("-", undecorate_fields);
}


int
main (int argc, char **argv)
{
  int opt;
  char *endp;
  int undecorate_fields = 0;
  int num_decorate_fields = 0;
  bool decorate_only = false;
  bool print_sort_args = false;

  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  init_key_spec ();

  //atexit (close_stdout);

  while ((opt = getopt_long (argc, argv, "cCHsS:T:uk:t:z",
                             longopts, NULL)) != -1)
    {
      switch (opt)
        {
        case 's':
          add_sort_extra_args ("-s");
          break;

        case 'u':
          add_sort_extra_args ("-u");
          break;

        case 'S':
          add_sort_extra_args ("-S");
          add_sort_extra_args (optarg);
          break;

        case 'T':
          add_sort_extra_args ("-T");
          add_sort_extra_args (optarg);
          break;

        case 'k':
          parse_key_arg (optarg);
          break;

        case 'H':
          skip_header_lines = 1;
          break;

        case HEADER_OPTION:
          errno = 0 ;
          skip_header_lines = strtol (optarg, &endp, 10);
          if (errno || (*endp != '\0'))
            die (SORT_FAILURE, 0, _("invalid number of header lines %s"),
                 quote (optarg));
          break;

          /* pass the various check options as-is to sort - do not validate */
        case CHECK_OPTION:
          if (!optarg)
            {
              add_sort_extra_args ("--check");
            }
          else
            {
              char *p = xcalloc (8 + strlen (optarg) + 1, 1);
              strcpy (p, "--check=");
              strcat (p, optarg);
              add_sort_extra_args (p);
              free (p);
            }
          break;

        case 'c':
          add_sort_extra_args ("-c");
          break;

        case 'C':
          add_sort_extra_args ("-C");
          break;


        case COMPRESS_PROGRAM_OPTION:
          add_sort_extra_args ("--compress-program");
          add_sort_extra_args (optarg);
          break;

        case RANDOM_SOURCE_OPTION:
          add_sort_extra_args ("--random-source");
          add_sort_extra_args (optarg);
          break;

        case NMERGE_OPTION:
          add_sort_extra_args ("--batch-size");
          add_sort_extra_args (optarg);
          break;

        case PARALLEL_OPTION:
          add_sort_extra_args ("--parallel");
          add_sort_extra_args (optarg);
          break;

        case 't':
          /* copied as-is from src/sort.c */
          {
            char newtab = optarg[0];
            if (! newtab)
              die (SORT_FAILURE, 0, _("empty tab"));
            if (optarg[1])
              {
                if (STREQ (optarg, "\\0"))
                  newtab = '\0';
                else
                  {
                    /* Provoke with 'sort -txx'.  Complain about
                       "multi-character tab" instead of "multibyte tab", so
                       that the diagnostic's wording does not need to be
                       changed once multibyte characters are supported.  */
                    die (SORT_FAILURE, 0, _("multi-character tab %s"),
                           quote (optarg));
                  }
              }
            if (tab != TAB_DEFAULT && tab != newtab)
              die (SORT_FAILURE, 0, _("incompatible tabs"));
            tab = newtab;
          }

          add_sort_extra_args ("-t");
          add_sort_extra_args (optarg);
          break;

        case 'z':
          add_sort_extra_args ("-z");
          eol_delimiter = '\0';
          break;

        case DECORATE_OPTION:
          decorate_only = true;
          break;

        case UNDECORATE_OPTION:
          errno = 0 ;
          undecorate_fields = strtol (optarg, &endp, 10);
          if (errno || (*endp != '\0') || (undecorate_fields <= 0))
            die (SORT_FAILURE, 0,
                 _("invalid number of fields to undecorate %s"),
                 quote (optarg));
          break;

        case PRINT_SORT_ARGS_OPTION:
          print_sort_args = true;
          break;

        case DEBUG_OPTION:
          debug = true;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (SORT_FAILURE);
        }
    }

  if (decorate_only && undecorate_fields)
    die (SORT_FAILURE, 0,
         _("--decorate and --undecorate options are mutually exclusive"));

  if (undecorate_fields && keylist)
    die (SORT_FAILURE, 0,
         _("--undecorate cannot be used with --keys or --decorate"));

  if (!keylist && !undecorate_fields)
    die (SORT_FAILURE, 0,
         _("missing -k/--key decoration or --undecorate options"));

  if (keylist)
    {
      if (debug)
        debug_keylist (stderr);
      num_decorate_fields = adjust_key_fields ();
      if (!num_decorate_fields)
        die (SORT_FAILURE, 0,
             _("no decorated keys specified. use sort instead"));
      if (debug)
        debug_keylist (stderr);
    }

  if (print_sort_args)
    {
      /* print and exit */
      char** sort_args = build_sort_process_args ();
      char **p = sort_args;
      while (*p)
        {
          /* TODO: shell quoting / escaping */
          fputs (*p, stdout);
          ++p;
          if (*p)
            fputc (' ', stdout);
        }
      fputc ('\n', stdout);
      exit (EXIT_SUCCESS);
    }

  if (decorate_only)
    {
      header_out = stdout;
      do_decorate (optind, argc, argv);
    }
  else if (undecorate_fields)
    {
      do_undecorate (optind, argc, argv, undecorate_fields);
    }
  else
    {
      /* decorate + sort + undecorate */
      debug_print_pid = true;

      int dec_sort_fds[2];
      int sort_undec_fds[2];
      if (pipe (dec_sort_fds) != 0)
        die (SORT_FAILURE, errno, _("failed to create dec-sort pipe"));
      if (pipe (sort_undec_fds) != 0)
        die (SORT_FAILURE, errno, _("failed to create sort-undec pipe"));

      pid_t undec_pid = -1, sort_pid = -1;

      /* fork for the 'decorate' process */
      undec_pid = fork ();
      if (undec_pid == -1)
        die (SORT_FAILURE, errno, _("failed to fork-1"));

      /* if this is the parent, fork again for the sort process */
      if (undec_pid != 0)
        {
          sort_pid = fork ();
          if (sort_pid == -1)
            die (SORT_FAILURE, errno, _("failed to fork-2"));
        }


      if (undec_pid!=0 && sort_pid!=0)
        {
          if (debug)
            dbg_printf ("main process starting\n");

          /* close the read-end pipe */
          if (close (sort_undec_fds[0])!=0)
            die (SORT_FAILURE, errno,
                 _("failed to close sort-undec read-pipe"));
          if (close (sort_undec_fds[1])!=0)
            die (SORT_FAILURE, errno,
                 _("failed to close sort-undec write-pipe"));
          if (close (dec_sort_fds[0])!=0)
            die (SORT_FAILURE, errno, _("failed to close dec-sort read-pipe"));

          /* If there are header lines to skip, they need to be printed
             directly to our current STDOUT, not to the sort-pipe.
             save STDOUT for later. */
          if (skip_header_lines)
            {
              int fd = dup (STDOUT_FILENO);
              if (fd==-1)
                die (SORT_FAILURE, errno, _("failed to dup STDOUT fd"));
              header_out = fdopen (fd, "w");
              if (!header_out)
                die (SORT_FAILURE, errno, _("fdopen (dup (stdout)) failed"));
            }

          /* replace the STDOUT of this process, to the write-end pipe */
          if (dup2 (dec_sort_fds[1], STDOUT_FILENO) == -1)
            die (SORT_FAILURE, errno, _("failed to reassign dec-sort STDOUT"));

          do_decorate (optind, argc, argv);

          fclose (stdout);
          close (dec_sort_fds[1]);

          if (debug)
            dbg_printf ("main process: waiting for children");

          /* TODO: pass exit codes from sort/undecorate */
          waitpid (undec_pid, NULL, 0);
          waitpid (sort_pid, NULL, 0);

          if (debug)
            dbg_printf ("main process: done");
        }
      else if (sort_pid==0)
        {
          /* This is the sort process */
          if (debug)
            dbg_printf ("sort process starting");

          if (close (dec_sort_fds[1])!=0)
            die (SORT_FAILURE, errno, _("failed to close dec-sort write-pipe"));
          if (close (sort_undec_fds[0])!=0)
            die (SORT_FAILURE, errno,
                 _("failed to close sort-undec read-pipe"));

          /* replace the STDIN of this process,
             to the read-end pipe of 'decorate' */
          if (dup2 (dec_sort_fds[0], STDIN_FILENO) == -1)
            die (SORT_FAILURE, errno, _("failed to reassign dec-sort STDIN"));

          /* replace the STDOUT of this process,
             to the write-end pipe of 'undecorate' */
          if (dup2 (sort_undec_fds[1], STDOUT_FILENO) == -1)
            die (SORT_FAILURE, errno,
                 _("failed to reassign sort-undec STDOUT"));

          char** sort_args = build_sort_process_args ();
          if (debug)
            {
              char **p = sort_args;
              while (*p)
                {
                  dbg_printf ("sort: argv[] = '%s'", *p);
                  ++p;
                }
            }
          execvp ("sort", sort_args);

          die (SORT_FAILURE, errno, _("failed to run the sort command"));
        }
      else
        {
          /* This is the undecorate process */
          if (debug)
            dbg_printf ("undecorate child starting");

          if (close (dec_sort_fds[0])!=0)
            die (SORT_FAILURE, errno, _("failed to close dec-sort read-pipe"));
          if (close (dec_sort_fds[1])!=0)
            die (SORT_FAILURE, errno, _("failed to close dec-sort write-pipe"));
          if (close (sort_undec_fds[1])!=0)
            die (SORT_FAILURE, errno,
                 _("failed to close sort-undec write-pipe"));

          /* replace the STDIN of this process, to the read-end pipe */
          if (dup2 (sort_undec_fds[0], STDIN_FILENO) == -1)
            die (SORT_FAILURE, errno, _("failed to reassign sort-undec STDIN"));

          /* undecorate from STDIN */
          do_undecorate (1, 0, NULL, num_decorate_fields);

          if (debug)
            dbg_printf ("undecorate child completed");
        }
    }

  return EXIT_SUCCESS;
}

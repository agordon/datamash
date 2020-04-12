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
#include <intprops.h>
#include <sys/wait.h>

#include "system.h"
//#include "stdio--.h"
//#include "fadvise.h"
#include "closeout.h"
#include "linebuffer.h"
#include "die.h"
#include "error.h"
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
  sort_extra_args[sort_extra_args_used++] = xstrdup(s);
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [INPUT [OUTPUT]]\n\
"),
              program_name);
      fputs (_("\
Convert fields to sortable format\n\
\n\
"), stdout);

      //emit_mandatory_arg_note ();
      fputs (_("\
  -k, --key=KEYDEF      convert field KEYDEF in the input file to sortable\n\
                        format; Converted form will be added to the end of\n\
                        each line;\n\
                        KEYDEF gives location and conversion function.\n\
"), stdout);

     fputs (_("\
  -t, --field-separator=SEP  use SEP instead of non-blank to blank transition\n\
"), stdout);
      fputs (_("\
  -z, --zero-terminated     line delimiter is NUL, not newline\n\
"), stdout);
     fputs (HELP_OPTION_DESCRIPTION, stdout);
     fputs (VERSION_OPTION_DESCRIPTION, stdout);
     fputs (_("\
\n\
A field is a run of blanks (usually spaces and/or TABs), then non-blank\n\
characters.  Fields are skipped before chars.\n\
"), stdout);
      fputs (_("\
\n\
KEYDEF is F[.C][,F[.C][:conversion][@command] for start and stop position,\n\
where F is a field number and C a character position in the field; both are\n\
origin 1, and the stop position defaults to the line's end.\n\
"), stdout);
      fputs (_("\
\n\
Available conversions:\n\
  roman        roman numerals\n\
  ipv4         dotted-decimal IPv4 addresses\n\
  ipv4inet     number-and-dots IPv4 addresses (incl. octal, hex values)\n\
  ipv6         IPv6 addresses\n\
  strlen       length (in bytes) of the specified field\n\
\n\
"), stdout);
      //emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

static void
decorate_fields (struct linebuffer *linebuf)
{
  struct line l;   /* The struct used by key-compare */
  struct line *a;

  size_t len = linebuf->length;
  struct keyfield *key = decorate_keylist;

  l.text = linebuf->buffer;
  l.length = linebuf->length;
  l.keybeg = NULL;
  l.keylim = NULL;

  l.keybeg = begfield (&l, decorate_keylist);
  l.keylim = limfield (&l, decorate_keylist);

  if (l.keybeg >= l.keylim)
    {
      /* TODO: is this the correct way to detect 'no limit' ?
       * (ie compare keys until the end of the line)*/
      l.keylim = l.keybeg + len - (l.keybeg - l.text);
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

      //fprintf(stderr,"extracted field: '%s'\n", ta);

      if (key->decorate_fn)
        {
          /* Innternal conversions */
          key->decorate_fn (ta);
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
}

static void
decorate_file (const char *infile)
{
  intmax_t linenum = 0;
  struct linebuffer lb;

  if (! (STREQ (infile, "-") || freopen (infile, "r", stdin)))
    die (EXIT_FAILURE, errno, "%s", quotef (infile));

  //fadvise (stdin, FADVISE_SEQUENTIAL);

  initbuffer (&lb);

  if (debug)
    fprintf(stderr,"decorate: starting read loop\n");

  while (!feof (stdin))
    {
      if (readlinebuffer_delim (&lb, stdin, eol_delimiter) == 0)
        break;

      ++linenum;

      size_t l = lb.length;

      /* don't print EOL character, if any */
      if (lb.length>0 && lb.buffer[lb.length-1]==eol_delimiter)
        --l;

      decorate_fields (&lb);

      if (debug)
        fprintf(stderr,"decorate: writing line: '%.*s'\n", (int)l, lb.buffer);

      if (fwrite (lb.buffer, 1, l, stdout) != l)
        die (EXIT_FAILURE, errno, _("decorate: fwrite failed"));

      fputc (eol_delimiter, stdout);
    }

  // closefiles:
  if (ferror (stdin) || fclose (stdin) != 0)
    die (EXIT_FAILURE, 0, _("error reading %s"), quoteaf (infile));

  if (debug)
    fprintf(stderr,"decorate: end loop\n");

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
  memset (&l, 0, sizeof(l));

  /* Create a dummy keyfield, encompasing the first N fields to skip */
  memset(&key, 0, sizeof(key));
  key.sword = num_fields;
  key.skipsblanks = true;

  if (! (STREQ (infile, "-") || freopen (infile, "r", stdin)))
    die (EXIT_FAILURE, errno, "%s", quotef (infile));

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
      char* p = begfield(&l, &key);
      size_t t = l.length - (p-l.text);

      if (debug)
        {
          fprintf(stderr,"Orig Line: %zu chars: '%.*s'\n", l.length, (int)l.length, l.text);
          fprintf(stderr,"Undec Line: %zu chars: '%.*s'\n", t, (int)t, p);
        }
      fwrite (p, 1, t, stdout);

      fputc (eol_delimiter, stdout);
    }

  // closefiles:
  if (ferror (stdin) || fclose (stdin) != 0)
    die (EXIT_FAILURE, 0, _("error reading %s"), quoteaf (infile));

  /* stdout is handled via the atexit-invoked close_stdout function.  */
  free (lb.buffer);
}


/* this code was copied from src/sort.c */
struct keyfield*
parse_sort_key_arg(const char* optarg, char const** endpos)
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
parse_external_conversion_spec (const char* optarg, const char *s, struct keyfield *key)
{
  ignore_value (key);

  if (*s == '\0')
    badfieldspec (optarg, N_("missing external conversion command"));

  die (SORT_FAILURE, 0, "external commands are not implemented (yet)");
}


static void
parse_builtin_conversion_spec (const char* optarg, const char *s, struct keyfield *key)
{
  bool found = false;
  /* built-in conversions */
  ++s;

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
    badfieldspec (optarg, N_("ordering flags (b/d/i/h/n/g/M/R/V) can not be combined with a conversion function"));
}

void parse_key_arg(const char* optarg)
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
adjust_key_fields()
{
  struct keyfield *key = keylist;
  int cnt = 0;
  int idx = 0;

  do {
    if (key->decorate_fn || key->decorate_cmd)
      ++cnt;
  }  while (key && ((key = key->next)));

  if (debug)
    fprintf(stderr, "Found %d decorate fields\n", cnt);

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
  char** argv = xcalloc (argc, sizeof(char*));
  int i = 0;

  argv[i++] = xstrdup("sort"); /* argv[0] */

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
do_decorate(int optind, int argc, char** argv)
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
do_undecorate(int optind, int argc, char** argv, int undecorate_fields)
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

  while ((opt = getopt_long (argc, argv, "csS:T:uk:t:z", longopts, NULL)) != -1)
    {
      switch (opt)
        {
        case 's':
          add_sort_extra_args("-s");
          break;

        case 'u':
          add_sort_extra_args("-u");
          break;

        case 'S':
          add_sort_extra_args("-S");
          add_sort_extra_args(optarg);
          break;

        case 'T':
          add_sort_extra_args("-T");
          add_sort_extra_args(optarg);
          break;

        case 'k':
          parse_key_arg (optarg);
          break;

        case CHECK_OPTION:
          add_sort_extra_args("--check");
          add_sort_extra_args(optarg);
          break;

        case COMPRESS_PROGRAM_OPTION:
          add_sort_extra_args("--compress-program");
          add_sort_extra_args(optarg);
          break;

        case RANDOM_SOURCE_OPTION:
          add_sort_extra_args("--random-source");
          add_sort_extra_args(optarg);
          break;

        case NMERGE_OPTION:
          add_sort_extra_args("--batch-size");
          add_sort_extra_args(optarg);
          break;

        case PARALLEL_OPTION:
          add_sort_extra_args("--parallel");
          add_sort_extra_args(optarg);
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

          add_sort_extra_args("-t");
          add_sort_extra_args(optarg);
          break;

        case 'z':
          add_sort_extra_args("-z");
          eol_delimiter = '\0';
          break;

        case DECORATE_OPTION:
          decorate_only = true;
          break;

        case UNDECORATE_OPTION:
          undecorate_fields = atoi(optarg);
          if (undecorate_fields <= 0)
            die (SORT_FAILURE, 0, _("invalid number of fields to undecorate %s"),
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
          usage (EXIT_FAILURE);
        }
    }

  if (decorate_only && undecorate_fields)
    die (SORT_FAILURE, 0, _("--decorate and --undecorate options are mutually exclusive"));

  if (undecorate_fields && keylist)
    die (SORT_FAILURE, 0, _("--undecorate cannot be used with --keys or --decorate"));

  if (!keylist && !undecorate_fields)
    die (SORT_FAILURE, 0, _("missing -k/--key decoration or --undecorate options"));


  if (keylist)
    {
      if (debug)
        debug_keylist (stderr);
      num_decorate_fields = adjust_key_fields ();
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
          fputs(*p, stdout);
          ++p;
          if (*p)
            fputc(' ', stdout);
        }
      fputc('\n', stdout);
      exit(EXIT_SUCCESS);
    }

  if (decorate_only)
    {
      do_decorate (optind, argc, argv);
    }
  else if (undecorate_fields)
    {
      do_undecorate (optind, argc, argv, undecorate_fields);
    }
  else
    {
      /* decorate + sort + undecorate */
      int dec_sort_fds[2];
      int sort_undec_fds[2];
      if (pipe(dec_sort_fds) != 0)
        die (SORT_FAILURE, errno, _("failed to create dec-sort pipe"));
      if (pipe(sort_undec_fds) != 0)
        die (SORT_FAILURE, errno, _("failed to create sort-undec pipe"));

      pid_t undec_pid = -1, sort_pid = -1;
      if (debug)
        fprintf(stderr,"=1= my PID = %i   undec_pid = %i, sort_pid = %i\n", getpid(), undec_pid, sort_pid);

      /* fork for the 'decorate' process */
      undec_pid = fork ();
      if (undec_pid == -1)
        die (SORT_FAILURE, errno, _("failed to fork-1"));

      if (debug)
        fprintf(stderr,"=2= my PID = %i   undec_pid = %i, sort_pid = %i\n", getpid(), undec_pid, sort_pid);

      /* if this is the parent, fork again for the sort process */
      if (undec_pid != 0)
        {
          sort_pid = fork ();
          if (sort_pid == -1)
            die (SORT_FAILURE, errno, _("failed to fork-2"));
        }

      if (debug)
        fprintf(stderr,"=3= my PID = %i   undec_pid = %i, sort_pid = %i\n", getpid(), undec_pid, sort_pid);

      if (undec_pid!=0 && sort_pid!=0)
        {
          if (debug)
            fprintf(stderr,"DECORATE process starting (PID %i)\n", getpid());

          /* close the read-end pipe */
          if (close(sort_undec_fds[0])!=0)
            die (SORT_FAILURE, errno, _("failed to close sort-undec read-pipe"));
          if (close(sort_undec_fds[1])!=0)
            die (SORT_FAILURE, errno, _("failed to close sort-undec write-pipe"));
          if (close(dec_sort_fds[0])!=0)
            die (SORT_FAILURE, errno, _("failed to close dec-sort read-pipe"));

          /* replace the STDOUT of this process, to the write-end pipe */
          if (dup2(dec_sort_fds[1], STDOUT_FILENO) == -1)
            die (SORT_FAILURE, errno, _("failed to reassign dec-sort STDOUT"));

          do_decorate (optind, argc, argv);

          if (debug)
            fprintf(stderr,"decorate: closing STDOUT\n");

          fclose(stdout);
          close(dec_sort_fds[1]);

          if (debug)
            fprintf(stderr,"decorate: waiting for children 1\n");

          waitpid (undec_pid, NULL, 0);

          if (debug)
            fprintf(stderr,"decorate: waiting for children 2\n");

          waitpid (sort_pid, NULL, 0);

          if (debug)
            fprintf(stderr,"decorate: done\n");
        }
      else if (sort_pid==0)
        {
          /* This is the sort process */
          if (debug)
            fprintf(stderr,"SORT child starting (pid %i)\n", getpid());

          if (close(dec_sort_fds[1])!=0)
            die (SORT_FAILURE, errno, _("failed to close dec-sort write-pipe"));
          if (close(sort_undec_fds[0])!=0)
            die (SORT_FAILURE, errno, _("failed to close sort-undec read-pipe"));

          /* replace the STDIN of this process, to the read-end pipe of 'decorate' */
          if (dup2(dec_sort_fds[0], STDIN_FILENO) == -1)
            die (SORT_FAILURE, errno, _("failed to reassign dec-sort STDIN"));

          /* replace the STDOUT of this process, to the write-end pipe of 'undecorate' */
          if (dup2(sort_undec_fds[1], STDOUT_FILENO) == -1)
            die (SORT_FAILURE, errno, _("failed to reassign sort-undec STDOUT"));

          char** sort_args = build_sort_process_args ();
          if (debug)
            {
              char **p = sort_args;
              while (*p)
                {
                  fprintf(stderr,"sort: argv[] = '%s'\n", *p);
                  ++p;
                }
            }
          execvp("sort", sort_args);

          die (SORT_FAILURE, errno, _("failed to run the sort command"));
        }
      else
        {
          /* This is the undecorate process */
          if (debug)
            fprintf(stderr,"UNDECORATE child starting (pid %d)\n", getpid());

          if (close(dec_sort_fds[0])!=0)
            die (SORT_FAILURE, errno, _("failed to close dec-sort read-pipe"));
          if (close(dec_sort_fds[1])!=0)
            die (SORT_FAILURE, errno, _("failed to close dec-sort write-pipe"));
          if (close(sort_undec_fds[1])!=0)
            die (SORT_FAILURE, errno, _("failed to close sort-undec write-pipe"));

          /* replace the STDIN of this process, to the read-end pipe */
          if (dup2(sort_undec_fds[0], STDIN_FILENO) == -1)
            die (SORT_FAILURE, errno, _("failed to reassign sort-undec STDIN"));

          /* undecorate from STDIN */
          do_undecorate (1, 0, NULL, num_decorate_fields);
        }
    }

  return EXIT_SUCCESS;
}

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
#include <sys/types.h>
#include <arpa/inet.h>
#include <limits.h>
#include <inttypes.h>

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

#define SORT_FAILURE EXIT_FAILURE

//#include "key-compare.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "decorate"

#define AUTHORS \
  proper_name ("Assaf Gordon")

/* Until someone better comes along */
const char version_etc_copyright[] = "Copyright %s %d Assaf Gordon" ;

/* if true, enable developer's debug messages */
static bool debug = false;

static char eol_delimiter = '\n';

enum
{
  DEBUG_OPTION = CHAR_MAX + 1
};

static struct option const longopts[] =
{
  {"key", required_argument, NULL, 'k'},
  {"check-chars", required_argument, NULL, 'w'},
  {"zero-terminated", no_argument, NULL, 'z'},
  {"-debug", no_argument, NULL, DEBUG_OPTION},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

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

static bool
decorate_as_is (const char* in)
{
  fprintf (stdout, "%s", in);
  return true;
}

static bool
decorate_strlen (const char* in)
{
  fprintf (stdout, "%"PRIuMAX, (uintmax_t)strlen (in));
  return true;
}

static int
roman_numeral_to_value (char c)
{
  switch (c)
    {
    case 'M': return 1000;
    case 'D': return 500;
    case 'C': return 100;
    case 'L': return 50;
    case 'X': return 10;
    case 'V': return 5;
    case 'I': return 1;
    default:  return 0;
    }
}


/* Naive implementation of Roman numerals conversion.
   Does not support alternative forms such as
    XIIX,IIXX for 18,
    IIC for 98.  */
static bool
decorate_roman_numerals (const char* in)
{
  intmax_t result = 0;
  intmax_t cur,last = 0;
  while (*in)
    {
      cur = roman_numeral_to_value (*in);
      if (!cur)
        {
          error (0, 0, _("invalid roman numeral '%c' in %s"),  *in, quote (in));
          return false;
        }

      if (last)
        {
          if (last >= cur)
            {
              result += last;
            }
          else
            {
              result += (cur - last);
              cur = 0;
            }
        }

      last = cur;
      ++in;
    }

  result += last;
  printf ("%"PRIiMAX, result);
  return true;
}

static bool
decorate_ipv4_inet_addr (const char* in)
{
  struct in_addr adr;
  int s;

  s = inet_aton (in, &adr);

  if (s == 0)
    {
      error (0, 0, _("invalid IPv4 address %s"), quote (in));
      return false;
    }

  printf ("%08X", ntohl (adr.s_addr));
  return true;
}


static bool
decorate_ipv4_dot_decimal (const char* in)
{
  struct in_addr adr;
  int s;

  s = inet_pton (AF_INET, in, &adr);

  if (s < 0)
    die (SORT_FAILURE, errno, _("inet_pton (AF_INET) failed"));

  if (s == 0)
    {
      error (0, 0, _("invalid dot-decimal IPv4 address %s"), quote (in));
      return false;
    }

  printf ("%08X", ntohl (adr.s_addr));
  return true;
}

static bool
decorate_ipv6 (const char* in)
{
  struct in6_addr adr;
  int s;

  s = inet_pton (AF_INET6, in, &adr);

  if (s < 0)
    die (SORT_FAILURE, errno, _("inet_pton (AF_INET6) failed"));

  if (s == 0)
    {
      error (0, 0, _("invalid IPv6 address %s"), quote (in));
      return false;
    }

  /* A portable way to print IPv6 binary representation. */
  for (int i=0;i<16;i+=2)
    {
      printf ("%02X%02X", adr.s6_addr[i], adr.s6_addr[i+1]);
      if (i != 14)
        fputc (':', stdout);
    }

  return true;
}

struct conversions_t
{
  const char* name;
  bool (*decorate_fn)(const char* in);
};

static struct conversions_t conversions[] = {
  { "as-is",    decorate_as_is },     /* for debugging */
  { "roman",    decorate_roman_numerals },
  { "strlen",   decorate_strlen },
  { "ipv4",     decorate_ipv4_dot_decimal },
  { "ip4",      decorate_ipv4_dot_decimal }, /* alias */
  { "ipv6",     decorate_ipv6 },
  { "ip6",      decorate_ipv6 },             /* alias */
  { "ipv4inet", decorate_ipv4_inet_addr },
  { "ip4inet",  decorate_ipv4_inet_addr },   /* alias */
  { NULL,       0 }
};

static void
parse_conversion_spec (const char* optarg, const char *s, struct keyfield *key)
{
  if (*s == '\0')
    badfieldspec (optarg, N_("missing conversion option (':' or '@')"));

  if (*s == '@')
    {
      /* external conversion */
      ++s;

      if (*s == '\0')
        badfieldspec (optarg, N_("missing external conversion command"));

      die (SORT_FAILURE, 0, "external commands are not implemented (yet)");
    }
  else if (*s == ':')
    {
      bool found = false;
      /* built-in conversions */
      ++s;

      if (*s == '\0')
        badfieldspec (optarg, N_("missing internal conversion function"));

      for (int i = 0 ; conversions[i].name; ++i)
        {
          if (STREQ (s, conversions[i].name))
            {
              found = true;
              key->decorate_fn = conversions[i].decorate_fn;
              break;
            }
        }
      if (!found)
        badfieldspec (optarg, N_("invalid built-in conversion option"));
    }
  else
    badfieldspec (optarg,
                  N_("invalid conversion option (expecting ':' or '@')"));
}

static void
decorate_fields (struct linebuffer *linebuf)
{
  struct line l;   /* The struct used by key-compare */
  struct line *a;

  size_t len = linebuf->length;
  struct keyfield *key = keylist;

  l.text = linebuf->buffer;
  l.length = linebuf->length;
  l.keybeg = NULL;
  l.keylim = NULL;

  l.keybeg = begfield (&l, keylist);
  l.keylim = limfield (&l, keylist);

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

      /* Print field delimiter */
      fputc ((tab == TAB_DEFAULT) ? ' ' : tab, stdout);

      if (key->decorate_fn)
        {
          /* Innternal conversions */
          key->decorate_fn (ta);
        }
      else
        {
          /* run external command */
        }

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

  while (!feof (stdin))
    {
      if (readlinebuffer_delim (&lb, stdin, eol_delimiter) == 0)
        break;

      ++linenum;

      size_t l = lb.length;

      /* don't print EOL character, if any */
      if (lb.length>0 && lb.buffer[lb.length-1]==eol_delimiter)
        --l;

      fwrite (lb.buffer, 1, l, stdout);

      decorate_fields (&lb);

      fputc (eol_delimiter, stdout);
    }

  // closefiles:
  if (ferror (stdin) || fclose (stdin) != 0)
    die (EXIT_FAILURE, 0, _("error reading %s"), quoteaf (infile));

  /* stdout is handled via the atexit-invoked close_stdout function.  */

  free (lb.buffer);
}



int
main (int argc, char **argv)
{
  struct keyfield *key = NULL;
  struct keyfield key_buf;
  char const *s;
  int opt;

  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  //init_key_spec ();

  atexit (close_stdout);

  while ((opt = getopt_long (argc, argv, "k:t:z", longopts, NULL)) != -1)
    {
      switch (opt)
        {
        /* this case was copied from src/sort.c */
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

          /* always skip blanks, equivalent to sort's -b option */
          key->skipsblanks = key->skipeblanks = true;

          /* which conversion/decoration to use */
          parse_conversion_spec (optarg, s, key);

          /* TODO: strange, sort's original code sets sword=SIZE_MAX for "-k1".
           * force override??? */
          if (key->sword==SIZE_MAX)
            key->sword=0;
          insertkey (key);
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
          break;

        case 'z':
          eol_delimiter = '\0';
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

  if (!keylist)
    die (SORT_FAILURE, 0, _("missing -k/--key decoration options"));

  if (optind < argc)
    {
      for (int i = optind; i < argc; ++i)
        decorate_file (argv[i]);
    }
  else
    decorate_file ("-");

  return EXIT_SUCCESS;
}

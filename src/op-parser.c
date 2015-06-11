/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013-2015 Assaf Gordon <assafgordon@gmail.com>

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
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "system.h"

#include "xalloc.h"
#include "quote.h"
#include "error.h"

#include "strcnt.h"

#include "op-defs.h"
#include "op-parser.h"
#include "field-ops.h"

static struct datamash_ops *dm= NULL;

static void
_alloc_ops ()
{
  dm = XZALLOC(struct datamash_ops);
  dm->mode = MODE_INVALID;
}

static void
add_group_col (bool by_name, size_t num, const char* name)
{
  if (dm->num_grps == dm->alloc_grps)
    dm->grps = x2nrealloc (dm->grps, &dm->alloc_grps, sizeof *dm->grps);
  struct group_column_t *p = &dm->grps[dm->num_grps++];

  p->num = num;
  p->name = NULL;
  p->by_name = by_name;
  if (by_name)
    {
      p->name = xstrdup (name);
      dm->header_required = true;
    }
}

static void
add_op (enum field_operation op, bool by_name, size_t num, const char* name)
{
  if (dm->num_ops == dm->alloc_ops)
    dm->ops = x2nrealloc (dm->ops, &dm->alloc_ops, sizeof *dm->ops);
  struct fieldop *p = &dm->ops[dm->num_ops++];

  if (by_name)
    dm->header_required = true;

  field_op_init (p, op, by_name, num, name);
}

static void
parse_mode_columns ( const char* s )
{
  long int val;
  size_t idx;
  char *tok, *endptr;
  char *spec = xstrdup(s);
  char *copy = spec;

  /* Count number of groups parameters, by number of commas */

  idx=0;
  while ( (tok = strsep (&spec, ",")) != NULL)
    {
      if (strlen (tok) == 0)
        error (EXIT_FAILURE, 0, _("invalid empty grouping parameter"));
      errno = 0 ;
      val = strtol (tok, &endptr, 10);
      if (errno == 0 && endptr != tok && *endptr == 0)
        {
          /* If strtol succeeded, it's a valid number. */
          if (val<1)
            error (EXIT_FAILURE, 0, _("invalid grouping parameter %s"),
                                    quote (tok));
          add_group_col (false, val, NULL);
        }
      else
        {
          /* If strrol failed, assume it's a named column - resolve it later. */
          add_group_col (true, 0, tok);
        }
      idx++;
    }

  free (copy);
}

static void
parse_operation_columns ( const char* s, enum field_operation op )
{
  long int val;
  size_t idx;
  char *tok, *endptr;
  char *spec = xstrdup(s);
  char *copy = spec;

  /* Count number of groups parameters, by number of commas */
//  int num_group_columns = strcnt (spec, ',')+1;

  idx=0;
  while ( (tok = strsep (&spec, ",")) != NULL)
    {
      if (strlen (tok) == 0)
        error (EXIT_FAILURE, 0, _("invalid empty column for operation %s"),
                                  quote (get_field_operation_name (op)));
      errno = 0 ;
      val = strtol (tok, &endptr, 10);
      if (errno == 0 && endptr != tok && *endptr == 0)
        {
          /* If strtol succeeded, it's a valid number. */
          if (val<1)
            error (EXIT_FAILURE, 0, _("invalid column %s for operation '%s'"),
                                    quote (tok),
                                    get_field_operation_name (op));
          add_op (op, false, val, NULL);
        }
      else
        {
          /* If strrol failed, assume it's a named column - resolve it later. */
          add_op (op, true, 0, tok);
        }
      idx++;
    }

  free (copy);
}

static inline bool
compatible_operation_modes (enum processing_mode current,
                             enum processing_mode added)
{
  return (current==added);
}


void
parse_field_operations( enum processing_mode mode,
                        int argc, const char* argv[] )
{
  enum processing_mode newmode;

  if (argc<=0)
    error(EXIT_FAILURE,0, _("missing operation. See --help for help"));

  enum field_operation fop = get_field_operation (argv[0], &newmode);
  if (fop==OP_INVALID)
    {
      /* detect mix-up of modes and operations, and give a friendly message */
      if (get_processing_mode (argv[0]) != MODE_INVALID)
        error(EXIT_FAILURE,0, _("conflicting operation %s"), quote (argv[0]));

      error(EXIT_FAILURE,0, _("invalid operation %s"), quote (argv[0]));
    }

  if (!compatible_operation_modes (mode,newmode))
    error (EXIT_FAILURE, 0, _("conflicting operation found: "\
           "expecting %s operations, but found %s operation %s"),
           get_processing_mode_name (mode),
           get_processing_mode_name (newmode),
           quote (argv[0]));

  if (argc<=1)
    error(EXIT_FAILURE,0, _("missing field number after operation %s"),
                          quote(argv[0]));

  parse_operation_columns (argv[1], fop);
}

void
parse_mode (int argc, const char* argv[])
{
  bool allow_operations = true;
  enum field_operation fop;
  if (argc<=0)
    error(EXIT_FAILURE,0, _("missing operation. See --help for help"));
  enum processing_mode pm = get_processing_mode (argv[0]);
  switch (pm)
  {
  case MODE_TRANSPOSE:
  case MODE_NOOP:
  case MODE_REVERSE:
      allow_operations = false;
      --argc;
      ++argv;
      break;

  case MODE_REMOVE_DUPS:
    allow_operations = false;
    /* fall through to get the group-by column */

  case MODE_GROUPBY:
    if (argc<=1)
      error(EXIT_FAILURE,0, _("missing field number for %s"), quote (argv[0]));
    parse_mode_columns (argv[1]);
    argc -= 2;
    argv += 2;
    break;

  case MODE_PER_LINE:
    error(EXIT_FAILURE,0, _("TODO: fix me, don't use these modes directly"));
    break;

  /* Implicit mode, determine by operation (e.g. 'sum' => groupby mode) */
  case MODE_INVALID:
    fop = get_field_operation (argv[0], &pm);
    if (fop==OP_INVALID)
      error(EXIT_FAILURE,0, _("invalid operation '%s'"), argv[0]);
    break;

  default:
    internal_error ("foo"); /* LCOV_EXCL_LINE */
    break;
  }
  dm->mode = pm;

  while (allow_operations && argc>0)
    {
      parse_field_operations (pm, argc, argv);
      argc -= 2;
      argv += 2;
    }

  if (argc>0)
    error(EXIT_FAILURE,0,_("extra operand %s"), quote(argv[0]));
}

struct datamash_ops*
datamash_ops_parse ( int argc, const char* argv[] )
{
  _alloc_ops ();
  parse_mode (argc, argv);
  return dm;
}

struct datamash_ops*
datamash_ops_parse_premode ( enum processing_mode pm,
                             const char* grouping,
                             int argc, const char* argv[] )
{
  _alloc_ops ();
  switch (pm)
  {
  case MODE_GROUPBY:
    parse_mode_columns (grouping);
    break;

  case MODE_REMOVE_DUPS:
  case MODE_PER_LINE:
  case MODE_TRANSPOSE:
  case MODE_NOOP:
  case MODE_REVERSE:
  case MODE_INVALID:
  default:
    internal_error ("foo"); /* LCOV_EXCL_LINE */
    break;
  }
  dm->mode = pm;

  while (argc>0)
    {
      parse_field_operations (pm, argc, argv);
      argc -= 2;
      argv += 2;
    }
  if (argc>0)
    error(EXIT_FAILURE,0,_("extra operand %s"), quote(argv[0]));
  return dm;
}


void
datamash_ops_debug_print ( const struct datamash_ops* p )
{
  assert (p != NULL );
  fprintf (stderr,"datamash_ops =\n\
  processing_mode = %s\n\
  header_required = %d\n",
    get_processing_mode_name (p->mode),
    (int)p->header_required);

  if (p->num_grps==0)
      fputs ("   no grouping specified\n", stderr);
  for (size_t i=0; i<p->num_grps; ++i)
    {
      const struct group_column_t *tmp = &p->grps[i];
      if (tmp->by_name)
        fprintf(stderr,"  group-by named column '%s'\n",tmp->name);
      else
        fprintf(stderr,"  group-by numeric column %zu\n",tmp->num);
    }

  for (size_t i=0; i<p->num_ops; ++i)
    {
      struct fieldop *o = &p->ops[i];
      if (o->field_by_name)
        fprintf(stderr,"  operation '%s' on named column '%s'\n",
                        get_field_operation_name (o->op), o->field_name);
      else
        fprintf(stderr,"  operation '%s' on numeric column %zu\n",
                        get_field_operation_name (o->op), o->field);
    }
}

void
datamash_ops_free ( struct datamash_ops* p )
{
  assert (p != NULL);
  for (size_t i=0; i<p->num_grps; ++i)
    free (p->grps[i].name);
  free (p->grps);
  p->grps = NULL;

  for (size_t i=0; i<p->num_ops; ++i)
    field_op_free (&p->ops[i]);
  free (p->ops);
  p->ops = NULL;

  free (p);
}

/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=8: */
/* vim: set expandtab: */

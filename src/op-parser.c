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

#include "op-defs.h"
#include "op-parser.h"
#include "field-ops.h"

static struct datamash_ops *dm= NULL;

static const char**  tokens = NULL;
static int     tokind = 0;
static int     tokcount = 0;

static void
set_tokens (int count, const char** _tokens)
{
  tokind = 0;
  tokcount = count;
  tokens = _tokens;
}

static inline bool
have_more_tokens ()
{
  return tokind < tokcount;
}

static inline const char* _GL_ATTRIBUTE_PURE
tok_peek ()
{
  assert (have_more_tokens ());                  /* LCOV_EXCL_LINE */
  return tokens[tokind];
}

static inline void
tok_consume ()
{
  ++tokind;
}

static void
_alloc_ops ()
{
  dm = XZALLOC (struct datamash_ops);
  dm->mode = MODE_INVALID;
}

/* Evalutates to TRUE if operation X (=enum field_operation)
   requires a paired field parameters (e.g. 1:2) */
#define OP_NEED_PAIR_PARAMS(x) (((x)==OP_P_COVARIANCE)||\
                                ((x)==OP_S_COVARIANCE))

#define ADD_NAMED_GROUP(name)  (add_group_col (true,0,(name)))
#define ADD_NUMERIC_GROUP(num) (add_group_col (false,num,NULL))
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

#define ADD_NAMED_OP(op,name)   (add_op (op,true,0,(name)))
#define ADD_NUMERIC_OP(op,num)  (add_op (op,false,num,NULL))
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

static inline bool
strtol_valid (const char* s, long int /*out*/ *val)
{
  assert (val != NULL);                          /* LCOV_EXCL_LINE */
  char *endptr;
  errno = 0 ;
  *val = strtol (s, &endptr, 10);
  return (errno == 0 && endptr != s && *endptr == 0);
}

static void
add_processing_mode_columns ( const char* s )
{
  long int val;
  char *tok;
  char *spec = xstrdup (s);
  char *copy = spec;

  while ( (tok = strsep (&spec, ",")) != NULL)
    {
      if (strlen (tok) == 0)
        error (EXIT_FAILURE, 0, _("invalid empty grouping parameter"));
      if (strtol_valid (tok, &val))
        {
          if (val<1)
            error (EXIT_FAILURE, 0, _("invalid grouping parameter %s"),
                                    quote (tok));
          ADD_NUMERIC_GROUP (val);
        }
      else
        {
          ADD_NAMED_GROUP (tok);
        }
    }

  free (copy);
}

static bool
try_add_single_field (const char* s, enum field_operation fop)
{
  long int val;
  if (!strtol_valid (s,&val))
    return false;
  if (val < 1)
    error (EXIT_FAILURE,0,_("invalid column %s for operation '%s'"),
                          quote (s), get_field_operation_name (fop));
  ADD_NUMERIC_OP (fop, val);
  return true;
}


static bool
try_add_field_range (const char* s, enum field_operation fop)
{
  char *endp;
  long int val1;
  long int val2;
  errno = 0 ;
  val1 = strtol (s, &endp, 10);
  if (errno != 0 || endp == s || endp == NULL || *endp != '-')
    return false;
  ++endp;

  /* TODO: informative error about invalid range specification? */
  if (!strtol_valid (endp,&val2)
      || val1 < 1 || val2 < 1 || (val1>val2))
    error (EXIT_FAILURE,0,_("invalid field range %s"), quote (s));

  for (long int i = val1; i<=val2; ++i)
    ADD_NUMERIC_OP (fop, i);

  return true;
}

static bool
try_add_field_pair (const char* s, enum field_operation fop)
{
  long int val;
  char *field1 = xstrdup (s);
  char *field2 = strchr (field1,':');
  struct fieldop *op=NULL;

  if (field2==NULL)
    return false;
  *field2 = 0;
  ++field2;

  /* empty first field or empty second field */
  if (*field1==0 || *field2==0)
    error (EXIT_FAILURE,0,_("invalid field pair %s"), quote (s));

  if (strtol_valid (field1, &val))
    ADD_NUMERIC_OP (fop, val);
  else
    ADD_NAMED_OP (fop, field1);

  const size_t slave_idx = dm->num_ops-1;
  op = &dm->ops[slave_idx];
  op->slave = true;

  if (strtol_valid (field2, &val))
    ADD_NUMERIC_OP (fop, val);
  else
    ADD_NAMED_OP (fop, field2);

  op = &dm->ops[dm->num_ops-1];
  op->master = true;
  op->slave_idx = slave_idx;

  free (field1);
  return true;
}


static void
add_named_field (const char* spec, enum field_operation fop)
{
  /* TODO: check 'isalpha (*spec)' to require names to start with a letter? */
  ADD_NAMED_OP (fop, spec);
}

static void
parse_operation_column (const char* spec, enum field_operation fop)
{
  if (OP_NEED_PAIR_PARAMS (fop))
    {
      if (!try_add_field_pair (spec, fop))
        error (EXIT_FAILURE,0,_("operation %s requires column pairs"),
                              quote (get_field_operation_name (fop)));
      return;
    }
  if (try_add_single_field (spec, fop)
      || try_add_field_range (spec, fop))
    return;

  add_named_field (spec, fop);
}

static void
parse_operation_column_list (enum field_operation fop)
{
  char *tok;
  char *spec, *copy;

  if (!have_more_tokens ())
    error (EXIT_FAILURE,0, _("missing field number after operation %s"),
                          quote (get_field_operation_name (fop)));

  copy = spec = xstrdup (tok_peek ());
  tok_consume ();

  while ( (tok = strsep (&spec, ",")) != NULL)
    {
      if (strlen (tok) == 0)
        error (EXIT_FAILURE, 0, _("invalid empty column for operation %s"),
                                  quote (get_field_operation_name (fop)));

      parse_operation_column (tok, fop);
    }

  free (copy);
}

static inline bool
compatible_operation_modes (enum processing_mode current,
                             enum processing_mode added)
{
  return ((current==MODE_CROSSTAB)&&(added==MODE_GROUPBY))||
         (current==added);
}

static void
parse_operation (enum processing_mode pm)
{
  enum processing_mode newmode;
  enum field_operation fop = get_field_operation (tok_peek (), &newmode);
  if (fop==OP_INVALID)
    {
      /* detect mix-up of modes and operations, and give a friendly message */
      if (get_processing_mode (tok_peek ()) != MODE_INVALID)
        error (EXIT_FAILURE,0, _("conflicting operation %s"),
                               quote (tok_peek ()));

      error (EXIT_FAILURE,0, _("invalid operation %s"), quote (tok_peek ()));
    }

  if (!compatible_operation_modes (pm,newmode))
    error (EXIT_FAILURE, 0, _("conflicting operation found: "\
           "expecting %s operations, but found %s operation %s"),
           get_processing_mode_name (pm),
           get_processing_mode_name (newmode),
           quote (tok_peek ()));
  tok_consume ();

  parse_operation_column_list (fop);
}

static void
parse_operations (enum processing_mode pm)
{
  while (have_more_tokens ())
    parse_operation (pm);

  /* After adding all operations, see of there are master/slave ops
     that need resolving - caching their pointer instead of index */
  for (size_t i=0; i<dm->num_ops; ++i)
    {
      if (!dm->ops[i].master)
        continue;
      const size_t si = dm->ops[i].slave_idx;
      assert (si<=dm->num_ops);                  /* LCOV_EXCL_LINE */
      dm->ops[i].slave_op = &dm->ops[si];
    }
}

static void
parse_processing_mode_column ()
{
  if (!have_more_tokens ())
        error (EXIT_FAILURE,0, _("missing field number"));
  add_processing_mode_columns (tok_peek ());
  tok_consume ();
}

void
parse_mode ()
{
  assert (have_more_tokens ());                    /* LCOV_EXCL_LINE */
  enum field_operation fop;
  enum processing_mode pm = get_processing_mode (tok_peek ());
  dm->mode = pm;
  if (pm != MODE_INVALID)
    tok_consume ();

  switch (pm)                                    /* LCOV_EXCL_BR_LINE */
  {
  case MODE_TRANSPOSE:
  case MODE_NOOP:
  case MODE_REVERSE:
  case MODE_TABULAR_CHECK:
    break;

  case MODE_REMOVE_DUPS:
    parse_processing_mode_column ();
    break;

  case MODE_CROSSTAB:
    parse_processing_mode_column ();
    parse_operations (pm);
    if (dm->num_grps!=2)
      error (EXIT_FAILURE,0, _("crosstab requires exactly 2 fields, " \
			       "found %zu"), dm->num_grps);
    /* if the user didn't specify an operation, print counts */
    if (dm->num_ops==0)
      ADD_NUMERIC_OP (OP_COUNT, dm->grps[0].num);
    break;

  case MODE_GROUPBY:
    parse_processing_mode_column ();
    parse_operations (pm);
    if (dm->num_ops==0)
      error (EXIT_FAILURE,0, _("missing operation"));
    break;

  case MODE_PER_LINE:
    internal_error ("line more used directly"); /* LCOV_EXCL_LINE */
    break;

  /* Implicit mode, determine by operation (e.g. 'sum' => groupby mode) */
  case MODE_INVALID:
    fop = get_field_operation (tok_peek (), &pm);
    if (fop==OP_INVALID)
      error (EXIT_FAILURE,0, _("invalid operation %s"), quote (tok_peek ()));
    dm->mode = pm;
    parse_operations (pm);
    break;

  default:
    internal_error ("invalid processing mode"); /* LCOV_EXCL_LINE */
    break;
  }

  if (have_more_tokens ())
    error (EXIT_FAILURE,0,_("extra operand %s"), quote (tok_peek ()));
}

struct datamash_ops*
datamash_ops_parse ( int argc, const char* argv[] )
{
  _alloc_ops ();
  set_tokens (argc, argv);
  parse_mode ();
  return dm;
}

struct datamash_ops*
datamash_ops_parse_premode ( enum processing_mode pm,
                             const char* grouping,
                             int argc, const char* argv[] )
{
  _alloc_ops ();
  assert (argc > 0);                             /* LCOV_EXCL_LINE */
  set_tokens (argc, argv);
  assert (pm == MODE_GROUPBY);                   /* LCOV_EXCL_LINE */
  dm->mode = pm;
  add_processing_mode_columns (grouping);
  parse_operations (pm);
  return dm;
}


/* LCOV_EXCL_START */
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
        fprintf (stderr,"  group-by named column '%s'\n",tmp->name);
      else
        fprintf (stderr,"  group-by numeric column %zu\n",tmp->num);
    }

  for (size_t i=0; i<p->num_ops; ++i)
    {
      struct fieldop *o = &p->ops[i];
      if (o->field_by_name)
        fprintf (stderr,"  operation '%s' on named column '%s'\n",
                        get_field_operation_name (o->op), o->field_name);
      else
        fprintf (stderr,"  operation '%s' on numeric column %zu\n",
                        get_field_operation_name (o->op), o->field);
    }
}
/* LCOV_EXCL_STOP */

void
datamash_ops_free ( struct datamash_ops* p )
{
  assert (p != NULL);                            /* LCOV_EXCL_LINE */
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

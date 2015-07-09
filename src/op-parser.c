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

#include "system.h"

#include "op-scanner.h"
#include "op-defs.h"
#include "op-parser.h"
#include "field-ops.h"

static struct datamash_ops *dm = NULL;

struct parser_field_t
{
  uintmax_t num;
  bool      by_name;
  char*     name;
  bool      range;
  bool      pair;
};

struct parser_param_t
{
  enum {
    PARAM_INT,
    PARAM_FLOAT
  } type;
  uintmax_t u;
  long double f;
};

/* The currently parsed operation */
static enum field_operation fop = OP_INVALID;

/* The currently parsed fields */
static struct parser_field_t *_fields;
static size_t _fields_alloc;
static size_t _fields_used;

/* The currently parsed operation's parameters */
static struct parser_param_t *_params;
static size_t _params_alloc;
static size_t _params_used;


static void
_alloc_ops ()
{
  dm = XZALLOC (struct datamash_ops);
  dm->mode = MODE_INVALID;

  _fields = NULL;
  _fields_alloc = 0;
  _fields_used = 0;

  _params = NULL;
  _params_alloc = 0;
  _params_used = 0;
}

static void
reset_parsed_operation ()
{
  fop = OP_INVALID;

  for (size_t i=0; i<_fields_used; ++i)
    free (_fields[i].name);

  _fields_used = 0;
  _params_used = 0;
}

static struct parser_field_t*
alloc_next_field ()
{
  if (_fields_used == _fields_alloc)
    _fields = x2nrealloc (_fields, &_fields_alloc,
                          sizeof (struct parser_field_t));
  struct parser_field_t *p = &_fields[_fields_used++];
  memset (p, 0, sizeof (*p));
  return p;
}

/* Evalutates to TRUE if operation X (=enum field_operation)
   requires a paired field parameters (e.g. 1:2) */
#define OP_NEED_PAIR_PARAMS(x) (((x)==OP_P_COVARIANCE)||\
                                ((x)==OP_S_COVARIANCE)||\
                                ((x)==OP_P_PEARSON_COR)||\
                                ((x)==OP_S_PEARSON_COR))

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

static struct fieldop *
add_op (enum field_operation op, const struct parser_field_t *f)
{
  if (dm->num_ops == dm->alloc_ops)
    dm->ops = x2nrealloc (dm->ops, &dm->alloc_ops, sizeof *dm->ops);
  struct fieldop *p = &dm->ops[dm->num_ops++];

  if (f->by_name)
    dm->header_required = true;

  #ifdef _STANDALONE_
  memset (p, 0, sizeof (struct fieldop));
  p->op = op;
  p->field = f->num;
  p->field_by_name = f->by_name;
  p->field_name = f->name;
  #else
  field_op_init (p, op, f->by_name, f->num, f->name);
  #endif
  return p;
}

static void
parse_simple_operation_column (struct parser_field_t /*OUTPUT*/ *p,
                               bool in_range, bool in_pair)
{
  assert (p);                                    /* LCOV_EXCL_LINE */
  enum TOKEN tok = scanner_get_token ();
  switch (tok)                                   /* LCOV_EXCL_BR */
    {
    case TOK_IDENTIFIER:
      p->by_name = true;
      p->name = xstrdup (scanner_identifier);
      break;

    case TOK_WHITESPACE:
      internal_error ("whitespace");            /* LCOV_EXCL_LINE */

    case TOK_COMMA:
      error (EXIT_FAILURE, 0, _("missing field for operation %s"),
          quote (get_field_operation_name (fop)));

    case TOK_END:
      /* informative error message depends on the context: */
      if (in_range)
        error (EXIT_FAILURE, 0, _("invalid field range for operation %s"),
            quote (get_field_operation_name (fop)));
      if (in_pair)
        error (EXIT_FAILURE, 0, _("invalid field pair for operation %s"),
            quote (get_field_operation_name (fop)));
      error (EXIT_FAILURE, 0, _("missing field for operation %s"),
          quote (get_field_operation_name (fop)));

    case TOK_DASH:
      error (EXIT_FAILURE, 0, _("invalid field range for operation %s"),
          quote (get_field_operation_name (fop)));

    case TOK_COLONS:
      error (EXIT_FAILURE, 0, _("invalid field pair for operation %s"),
          quote (get_field_operation_name (fop)));

    case TOK_INTEGER:
      /* Zero values will fall-thought to the error message below */
      if (scan_val_int>0)
        {
          p->by_name = false;
          p->num = scan_val_int;
          break;
        }

    case TOK_FLOAT:
    default:
      error (EXIT_FAILURE, 0, _("invalid field '%s' for operation %s"),
          scanner_identifier,
          quote (get_field_operation_name (fop)));
    }
}

static void
parse_operation_column ()
{
  struct parser_field_t *p = alloc_next_field ();
  parse_simple_operation_column (p, false, false);

  if (scanner_peek_token () == TOK_COLONS)
    {
      scanner_get_token ();

      /* mark the previous field as pair, this will be the other field */
      p->pair = true;

      struct parser_field_t *q = alloc_next_field ();
      parse_simple_operation_column (q, false, true);
    }

  if (scanner_peek_token () == TOK_DASH)
    {
      scanner_get_token ();

      /* mark the previous field as range, this will be the 'to' field */
      p->range = true;

      struct parser_field_t *q = alloc_next_field ();
      parse_simple_operation_column (q, true, false);

      if (p->by_name || q->by_name)
        error (EXIT_FAILURE, 0, _("field range for %s must be numeric"),
                                quote (get_field_operation_name (fop)));
      if (p->num >= q->num)
        error (EXIT_FAILURE, 0, _("invalid field range for operation %s"),
                                quote (get_field_operation_name (fop)));
    }
}

static void
parse_operation_column_list ()
{
  parse_operation_column ();

  enum TOKEN tok = scanner_peek_token ();
  while (tok == TOK_COMMA)
    {
      scanner_get_token ();
      parse_operation_column ();
      tok = scanner_peek_token ();
    }
}

static void
parse_operation_params ()
{
  /* currently, the only place we want to detect a whitespace, spearating
     between operation's parameters and field numbers.
     e.g.:
      'foo:10: 4' should not be treated as 'foo:10:4' (with two parameters).
      instead, it should produce an error (missing second parameter),
      and the '4' should be treated as the field number.
  */
  scanner_keep_whitespace = true;

  enum TOKEN tok = scanner_peek_token ();
  while (tok == TOK_COLONS)
    {
      scanner_get_token ();
      tok = scanner_get_token ();

      if (_params_used == _params_alloc)
        _params = x2nrealloc (_params, &_params_alloc,
                                        sizeof (struct parser_param_t));
      struct parser_param_t *p = &_params[_params_used++];

      switch (tok)
        {
        case TOK_INTEGER:
          p->type = PARAM_INT;
          p->u    = scan_val_int;
          break;

        case TOK_FLOAT:
          p->type = PARAM_FLOAT;
          p->f    = scan_val_float;
          break;

        case TOK_WHITESPACE:
        case TOK_END:
          error (EXIT_FAILURE, 0, _("missing parameter for operation %s"),
                                  quote (get_field_operation_name (fop)));

        case TOK_COMMA:
        case TOK_DASH:
        case TOK_IDENTIFIER:
        case TOK_COLONS:
        default:
          error (EXIT_FAILURE, 0, _("invalid parameter %s for operation %s"),
                                  scanner_identifier,
                                  quote (get_field_operation_name (fop)));

        }

      tok = scanner_peek_token ();
    }
  if (tok == TOK_WHITESPACE)
    scanner_get_token ();

  scanner_keep_whitespace = false;
}

static inline bool
compatible_operation_modes (enum processing_mode current,
                             enum processing_mode added)
{
  return ((current==MODE_CROSSTAB)&&(added==MODE_GROUPBY))||
         (current==added);
}

static void
create_field_ops ()
{
  for (size_t i=0; i<_fields_used; ++i)
    {
      const struct parser_field_t *f = &_fields[i];
      struct fieldop *op = add_op (fop, f);

      if (OP_NEED_PAIR_PARAMS (fop) && !f->pair)
        error (EXIT_FAILURE, 0, _("operation %s requires field pairs"),
                                quote (get_field_operation_name (fop)));
      if (!OP_NEED_PAIR_PARAMS (fop) && f->pair)
        error (EXIT_FAILURE, 0, _("operation %s cannot use pair of fields"),
                                quote (get_field_operation_name (fop)));

      if (f->range)
        {
          uintmax_t to   = _fields[++i].num;
          struct parser_field_t t = *f;
          while (t.num<to)
            {
              ++t.num;
              add_op (fop, &t);
            }
        }

      if (f->pair)
        {
          op->slave = true;

          const struct parser_field_t *other_f = &_fields[++i];
          op = add_op (fop, other_f);
          op->master = true;
          op->slave_idx = dm->num_ops-2; /* index of the prev op = slave op */
        }
    }
}

static void
parse_operation (enum processing_mode pm)
{
  reset_parsed_operation ();

  scanner_get_token ();
  enum processing_mode pm2;
  fop = get_field_operation (scanner_identifier, &pm2);
  if (fop==OP_INVALID)
    {
      pm2 = get_processing_mode (scanner_identifier);
      if (pm2 != MODE_INVALID)
        error (EXIT_FAILURE,0, _("conflicting operation %s"),
                               quote (scanner_identifier));

      error (EXIT_FAILURE,0, _("invalid operation %s"),
                             quote (scanner_identifier));
    }

  if (!compatible_operation_modes (pm,pm2))
    error (EXIT_FAILURE, 0, _("conflicting operation found: "\
           "expecting %s operations, but found %s operation %s"),
           get_processing_mode_name (pm),
           get_processing_mode_name (pm2),
           quote (scanner_identifier));

  parse_operation_params ();

  parse_operation_column_list ();

  create_field_ops ();
}

static void
parse_operations (enum processing_mode pm)
{
  enum TOKEN tok = scanner_peek_token ();
  while (tok != TOK_END)
    {
      parse_operation (pm);
      tok = scanner_peek_token ();
    }

  /* After adding all operations, see of there are master/slave ops
   * that need resolving - caching their pointer instead of index */
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
parse_mode_column (enum processing_mode pm)
{
  enum TOKEN tok = scanner_get_token ();
  switch (tok)                                   /* LCOV_EXCL_BR */
    {
    case TOK_IDENTIFIER:
      ADD_NAMED_GROUP (scanner_identifier);
      break;

    case TOK_WHITESPACE:
      internal_error ("whitespace");            /* LCOV_EXCL_LINE */
    case TOK_COMMA:
    case TOK_END:
      error (EXIT_FAILURE, 0, _("missing field for operation %s"),
          quote (get_processing_mode_name (pm)));

    case TOK_INTEGER:
      if (scan_val_int>0)
        {
          ADD_NUMERIC_GROUP (scan_val_int);
          break;
        }

    case TOK_DASH:
    case TOK_COLONS:
    case TOK_FLOAT:
    default:
      error (EXIT_FAILURE, 0, _("invalid field '%s' for operation %s"),
          scanner_identifier,
          quote (get_processing_mode_name (pm)));

    }
}

static void
parse_mode_column_list (enum processing_mode pm)
{
  parse_mode_column (pm);

  enum TOKEN tok = scanner_peek_token ();
  while (tok == TOK_COMMA)
   {
     scanner_get_token ();
     parse_mode_column (pm);
     tok = scanner_peek_token ();
   }
}

static void
parse_mode ()
{
  scanner_get_token ();
  enum processing_mode pm = get_processing_mode (scanner_identifier);
  dm->mode = pm;

  switch (pm)                                    /* LCOV_EXCL_BR_LINE */
  {
  case MODE_TRANSPOSE:
  case MODE_NOOP:
  case MODE_REVERSE:
  case MODE_TABULAR_CHECK:
    break;

  case MODE_REMOVE_DUPS:
    parse_mode_column_list (pm);
    break;

  case MODE_CROSSTAB:
    parse_mode_column_list (pm);
    if (dm->num_grps!=2)
      error (EXIT_FAILURE,0, _("crosstab requires exactly 2 fields, " \
	                             "found %zu"), dm->num_grps);

    /* if the user didn't specify an operation, print counts */
    parse_operations (pm);
    if (dm->num_ops==0)
      {
        const uintmax_t grp_col = dm->grps[0].num;
        struct parser_field_t dummy = {grp_col,false,NULL,false,false};
        add_op (OP_COUNT, &dummy);
      }
    break;

  case MODE_GROUPBY:
    parse_mode_column_list (pm);
    parse_operations (pm);
    if (dm->num_ops==0)
      error (EXIT_FAILURE,0, _("missing operation"));
    break;

  case MODE_PER_LINE:
    internal_error ("line mode used directly"); /* LCOV_EXCL_LINE */
    break;

  case MODE_INVALID:
  default:
    internal_error ("wrong opmode"); /* LCOV_EXCL_LINE */
    break;
  }

  if (scanner_peek_token ()!=TOK_END)
    error (EXIT_FAILURE,0,_("extra operand %s"), quote (scanner_identifier));
}

static void
parse_mode_or_op ()
{
  enum TOKEN tok = scanner_peek_token ();
  assert ( tok != TOK_END );                      /* LCOV_EXCL_LINE */

  enum processing_mode pm = get_processing_mode (scanner_identifier);
  if (pm != MODE_INVALID)
    {
      parse_mode ();
      return ;
    }

  enum field_operation fop = get_field_operation (scanner_identifier, &pm);
  if (fop!=OP_INVALID)
    {
      dm->mode = pm;
      parse_operations (pm);
      return ;
    }

  error (EXIT_FAILURE,0, _("invalid operation %s"),
		  quote (scanner_identifier));
}

struct datamash_ops*
datamash_ops_parse ( int argc, const char* argv[] )
{
  _alloc_ops ();
  scanner_set_input_from_argv (argc, argv);
  parse_mode_or_op ();
  scanner_free ();
  return dm;
}

struct datamash_ops*
datamash_ops_parse_premode ( enum processing_mode pm,
                             const char* grouping,
                             int argc, const char* argv[] )
{
  _alloc_ops ();
  assert (argc > 0);                             /* LCOV_EXCL_LINE */
  assert (pm == MODE_GROUPBY);                   /* LCOV_EXCL_LINE */
  dm->mode = pm;
  scanner_set_input_from_argv (1, &grouping);
  parse_mode_column_list (pm);
  scanner_free ();
  scanner_set_input_from_argv (argc, argv);
  parse_operations (pm);
  scanner_free ();
  return dm;
}

void
datamash_ops_free ( struct datamash_ops* p )
{
  assert (p != NULL);                            /* LCOV_EXCL_LINE */
  for (size_t i=0; i<p->num_grps; ++i)
    free (p->grps[i].name);
  free (p->grps);
  p->grps = NULL;

  #ifndef _STANDALONE_
  for (size_t i=0; i<p->num_ops; ++i)
    field_op_free (&p->ops[i]);
  #endif

  free (p->ops);
  p->ops = NULL;

  free (_fields);
  _fields = NULL;
  _fields_alloc = 0;
  _fields_used = 0;

  free (_params);
  _params_alloc = 0;
  _params_used = 0;

  free (p);
}

#ifdef PARSER_TEST_MAIN
/*
 Trivial parser tester.
 To compile:
    cc -D_STANDALONE_ -DPARSER_TEST_MAIN \
       -I. \
       -std=c99 -Wall -Wextra -Werror -g -O0 \
       -o dm-parser \
       op-parser.c op-scanner.c op-defs.c
 Test:
   ./dm-parser groupby 1,4 sum 4-5,foo
*/

void
datamash_ops_debug_print ( const struct datamash_ops* p )
{
  assert (p != NULL );
  printf ("datamash_ops =\n processing_mode = %s\n header_required = %d\n",
          get_processing_mode_name (p->mode), (int)p->header_required);

  if (p->num_grps==0)
      puts ("   no grouping specified");
  for (size_t i=0; i<p->num_grps; ++i)
    {
      const struct group_column_t *tmp = &p->grps[i];
      if (tmp->by_name)
        printf ("  group-by named column '%s'\n",tmp->name);
      else
        printf ("  group-by numeric column %zu\n",tmp->num);
    }

  for (size_t i=0; i<p->num_ops; ++i)
    {
      struct fieldop *o = &p->ops[i];
      if (o->field_by_name)
        printf ("  operation '%s' on named column '%s'",
                        get_field_operation_name (o->op), o->field_name);
      else
        printf ("  operation '%s' on numeric column %zu",
                        get_field_operation_name (o->op), o->field);
      if (o->master)
        printf (" (master)");
      if (o->slave)
        printf (" (slave)");
      printf ( "\n");
    }
}

#define TESTMAIN main
int TESTMAIN (int argc, const char* argv[])
{
  if (argc<2)
    error (EXIT_FAILURE, 0, _("missing script (as parameters"));

  struct datamash_ops *o = datamash_ops_parse (argc-1, argv+1);
  datamash_ops_debug_print ( o );
  datamash_ops_free (o);
  return 0;
}
#endif

/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=2: */
/* vim: set expandtab: */

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

#include "system.h"

#include "op-defs.h"

struct field_operation_definition
{
  const char* name;
  enum field_operation op;
  enum processing_mode mode; /* The default mode implied by this operation */
};

struct field_operation_definition field_operations[] =
{
  {"invalid", OP_INVALID, MODE_GROUPBY},
  {"count",   OP_COUNT,   MODE_GROUPBY},
  {"sum",     OP_SUM,     MODE_GROUPBY},
  {"min",     OP_MIN,     MODE_GROUPBY},
  {"max",     OP_MAX,     MODE_GROUPBY},
  {"absmin",  OP_ABSMIN,  MODE_GROUPBY},
  {"absmax",  OP_ABSMAX,  MODE_GROUPBY},
  {"range",   OP_RANGE,   MODE_GROUPBY},
  {"first",   OP_FIRST,   MODE_GROUPBY},
  {"last",    OP_LAST,    MODE_GROUPBY},
  {"rand",    OP_RAND,    MODE_GROUPBY},
  {"mean",    OP_MEAN,    MODE_GROUPBY},
  {"median",  OP_MEDIAN,  MODE_GROUPBY},
  {"q1",      OP_QUARTILE_1, MODE_GROUPBY},
  {"q3",      OP_QUARTILE_3, MODE_GROUPBY},
  {"iqr",     OP_IQR,     MODE_GROUPBY},
  {"perc",    OP_PERCENTILE, MODE_GROUPBY},
  {"pstdev",  OP_PSTDEV,  MODE_GROUPBY},
  {"sstdev",  OP_SSTDEV,  MODE_GROUPBY},
  {"pvar",    OP_PVARIANCE,    MODE_GROUPBY},
  {"svar",    OP_SVARIANCE,    MODE_GROUPBY},
  {"mad",     OP_MAD,     MODE_GROUPBY},
  {"madraw",  OP_MADRAW,  MODE_GROUPBY},
  {"sskew",   OP_S_SKEWNESS,   MODE_GROUPBY},
  {"pskew",   OP_P_SKEWNESS,   MODE_GROUPBY},
  {"skurt",   OP_S_EXCESS_KURTOSIS,   MODE_GROUPBY},
  {"pkurt",   OP_P_EXCESS_KURTOSIS,   MODE_GROUPBY},
  {"jarque",  OP_JARQUE_BERA,  MODE_GROUPBY},
  {"dpo",     OP_DP_OMNIBUS,     MODE_GROUPBY},
  {"mode",    OP_MODE,    MODE_GROUPBY},
  {"antimode",OP_ANTIMODE,MODE_GROUPBY},
  {"unique",  OP_UNIQUE,  MODE_GROUPBY},
  {"collapse",OP_COLLAPSE,MODE_GROUPBY},
  {"countunique", OP_COUNT_UNIQUE, MODE_GROUPBY},
  {"base64",  OP_BASE64,  MODE_PER_LINE},
  {"debase64",OP_DEBASE64,MODE_PER_LINE},
  {"md5",     OP_MD5,     MODE_PER_LINE},
  {"sha1",    OP_SHA1,    MODE_PER_LINE},
  {"sha256",  OP_SHA256,  MODE_PER_LINE},
  {"sha512",  OP_SHA512,  MODE_PER_LINE},
  {"dirname", OP_DIRNAME, MODE_PER_LINE},
  {"basename",OP_BASENAME,MODE_PER_LINE},
  {"extname", OP_EXTNAME, MODE_PER_LINE},
  {"barename",OP_BARENAME,MODE_PER_LINE},
  {"pcov",    OP_P_COVARIANCE, MODE_GROUPBY},
  {"scov",    OP_S_COVARIANCE, MODE_GROUPBY},
  {"ppearson",OP_P_PEARSON_COR, MODE_GROUPBY},
  {"spearson",OP_S_PEARSON_COR, MODE_GROUPBY},
  {"bin",     OP_BIN_BUCKETS,     MODE_PER_LINE},
  {"strbin",  OP_STRBIN,     MODE_PER_LINE},
  {"floor",   OP_FLOOR,      MODE_PER_LINE},
  {"ceil",    OP_CEIL,       MODE_PER_LINE},
  {"round",   OP_ROUND,      MODE_PER_LINE},
  {"trunc",   OP_TRUNCATE,   MODE_PER_LINE},
  {"frac",    OP_FRACTION,   MODE_PER_LINE},
  {"trimmean",OP_TRIMMED_MEAN,   MODE_GROUPBY},
  {"getnum",  OP_GETNUM,   MODE_PER_LINE},
  {"cut",     OP_CUT,     MODE_PER_LINE},
  {NULL,      OP_INVALID, MODE_INVALID}
};

struct processing_mode_definition
{
  const char* name;
  enum processing_mode mode;
};

const struct processing_mode_definition processing_modes[] =
{
  {"invalid",  MODE_INVALID},
  {"groupby",  MODE_GROUPBY},
  {"grouping", MODE_GROUPBY},
  {"gb" ,      MODE_GROUPBY},
  {"transpose",MODE_TRANSPOSE},
  {"reverse",  MODE_REVERSE},
  {"line",     MODE_PER_LINE},
  {"dedup",    MODE_REMOVE_DUPS},
  {"rmdup",    MODE_REMOVE_DUPS},
  {"nop",      MODE_NOOP},
  {"noop",     MODE_NOOP},
  {"crosstab", MODE_CROSSTAB},
  {"ct",       MODE_CROSSTAB},
  {"check",    MODE_TABULAR_CHECK},
  {NULL,       MODE_INVALID},
};

enum field_operation
get_field_operation (const char* s, enum processing_mode* /*out*/ mode)
{
  const struct field_operation_definition* fod = field_operations;
  while (fod->name) {
    if (strcasecmp (fod->name, s)==0)
      {
        if (mode)
          *mode = fod->mode;
        return fod->op;
      }
    ++fod;
  }
  return OP_INVALID;
}

const char* _GL_ATTRIBUTE_PURE
get_field_operation_name (enum field_operation op)
{
  const struct field_operation_definition* fod = field_operations;
  while (fod->name)                              /* LCOV_EXCL_BR_LINE */
    {
      if (fod->op == op)
        return fod->name;
      ++fod;
    }
  internal_error ("invalid op value");            /* LCOV_EXCL_LINE */
  return NULL;                                    /* LCOV_EXCL_LINE */
}

enum processing_mode _GL_ATTRIBUTE_PURE
get_processing_mode (const char* s)
{
  const struct processing_mode_definition* pmd = processing_modes;
  while (pmd->name)
    {
      if (strcasecmp (pmd->name, s)==0)
        return pmd->mode;
      ++pmd;
    }
  return MODE_INVALID;
}

const char* _GL_ATTRIBUTE_PURE
get_processing_mode_name (enum processing_mode m)
{
  const struct processing_mode_definition* pmd = processing_modes;
  while (pmd->name)                               /* LCOV_EXCL_BR_LINE */
    {
      if (pmd->mode == m)
        return pmd->name;
      ++pmd;
    }
  internal_error ("invalid mode value");          /* LCOV_EXCL_LINE */
  return NULL;                                    /* LCOV_EXCL_LINE */
}

/* vim: set cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1: */
/* vim: set shiftwidth=2: */
/* vim: set tabstop=8: */
/* vim: set expandtab: */

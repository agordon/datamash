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
#ifndef __OPERATION_DEFINITONS_H__
#define __OPERATION_DEFINITONS_H__

enum field_operation
{
  OP_INVALID = -1,
  OP_COUNT = 0,
  OP_SUM,
  OP_MIN,
  OP_MAX,
  OP_ABSMIN,
  OP_ABSMAX,
  OP_RANGE,
  OP_FIRST,
  OP_LAST,
  OP_RAND,
  OP_MEAN,
  OP_MEDIAN,
  OP_QUARTILE_1,
  OP_QUARTILE_3,
  OP_IQR,       /* Inter-quartile range */
  OP_PERCENTILE,
  OP_PSTDEV,    /* Population Standard Deviation */
  OP_SSTDEV,    /* Sample Standard Deviation */
  OP_PVARIANCE, /* Population Variance */
  OP_SVARIANCE, /* Sample Variance */
  OP_MAD,       /* MAD - Median Absolute Deviation, with adjustment constant of
                   1.4826 for normal distribution */
  OP_MADRAW,    /* MAD (same as above), with constant=1 */
  OP_S_SKEWNESS,/* Sample Skewness */
  OP_P_SKEWNESS,/* Population Skewness */
  OP_S_EXCESS_KURTOSIS, /* Sample Excess Kurtosis */
  OP_P_EXCESS_KURTOSIS, /* Population Excess Kurtosis */
  OP_JARQUE_BERA,   /* Jarque-Bera test of normality */
  OP_DP_OMNIBUS,    /* D'Agostino-Pearson omnibus test of normality */
  OP_MODE,
  OP_ANTIMODE,
  OP_UNIQUE,        /* Collapse Unique string into comma separated values */
  OP_COLLAPSE,      /* Collapse strings into comma separated values */
  OP_COUNT_UNIQUE,  /* count number of unique values */
  OP_BASE64,        /* Encode Field to Base64 */
  OP_DEBASE64,      /* Decode Base64 field */
  OP_MD5,           /* Calculate MD5 of a field */
  OP_SHA1,          /* Calculate SHA1 of a field */
  OP_SHA256,        /* Calculate SHA256 of a field */
  OP_SHA512,        /* Calculate SHA512 of a field */
  OP_P_COVARIANCE,  /* Population Covariance */
  OP_S_COVARIANCE,  /* Sample Covariance */
  OP_P_PEARSON_COR, /* Pearson Correlation Coefficient (population) */
  OP_S_PEARSON_COR, /* Pearson Correlation Coefficient (sample) */
  OP_BIN_BUCKETS,   /* numeric binning operation */
  OP_STRBIN,        /* String hash/binning */
  OP_FLOOR,         /* Floor */
  OP_CEIL,          /* Ceiling */
  OP_ROUND,         /* Round */
  OP_TRUNCATE,      /* Truncate */
  OP_FRACTION,      /* Fraction */
  OP_TRIMMED_MEAN,  /* Trimmed Mean */
  OP_DIRNAME,       /* like dirname (1) */
  OP_BASENAME,      /* like basename (1) */
  OP_EXTNAME,       /* guess extension of file name */
  OP_BARENAME,      /* like basename without the guessed extension  */
  OP_GETNUM,        /* Extract a number from a string */
  OP_CUT            /* like cut (1) */
};

enum processing_mode
{
  MODE_INVALID = -1,
  MODE_GROUPBY = 0,   /* Group By similar keys */
  MODE_TRANSPOSE,     /* transpose */
  MODE_REVERSE,       /* reverse fields in each line */
  MODE_PER_LINE,      /* Operations on each line, no grouping */
  MODE_REMOVE_DUPS,   /* Remove duplicated keys from a file */
  MODE_CROSSTAB,      /* Cross tabulation (aka pivot tables) */
  MODE_TABULAR_CHECK, /* Verif the file has tabular format */
  MODE_NOOP           /* Do nothing. Used for testing and profiling */
};

/* Given a text string, returns the matching operation, or OP_INVALID.
   if 'mode' is not NULL, stores the implied processing mode
   (e.g. sum=>MODE_GROUPBY,  md5=>MODE_PER_LINE). */
enum field_operation
get_field_operation (const char* s, enum processing_mode* /*out*/ mode);

const char*
get_field_operation_name (enum field_operation op);

/* Given a text string,
   returns the matching processing mode, or MODE_INVALID. */
enum processing_mode
get_processing_mode (const char* s);

const char*
get_processing_mode_name (enum processing_mode m);

#endif

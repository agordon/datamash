/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2014 Assaf Gordon.

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
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <strings.h>

#include "system.h"
#include "intprops.h"

#include "field-content-type.h"

/*
$ seq 0 9 |
    perl -lne 'BEGIN{ $lo=0; $hi=0; }
      foreach(split//) {
        $i = ord $_ ;
        $hi |= 1<<($i-64) if $i >=64;
        $lo |= 1<<($i) if $i<64;
    }
    END { printf "hi = 0x%016xL\nlo = 0x%016xL\n", $hi, $lo; }'
*/

/* 0-9 */
#define BITS_DIGITS_LO (0x3ff000000000000L)

/* A-Z */
#define BITS_ALPHA_UPPERCASE_HI (0x0000000007fffffeL)

/* a-z */
#define BITS_ALPHA_LOWERCASE_HI (0x07fffffe00000000L)

/* 0-9A-Fa-f - valid characters for hex values */
/* TODO: allow 0x prefix */
#define BITS_XDIGITS_LO (0x03ff000000000000L)
#define BITS_XDIGITS_HI (0x0000007e0000007eL)

/* Punctuation characters, according to ispunct:
       !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~
*/
#define BITS_PUNCT_LO (0xfc00fffe00000000L)
#define BITS_PUNCT_HI (0x78000001f8000001L)

/* Punctuation characters, EXCEPT the main six:
       period, comma, minus, underscore, colon, semicolon */
#define BITS_PUNCT_EXTRA_LO (BITS_PUNCT_LO & \
		            ~(CHAR_TO_BITS_LO('.')+\
		              CHAR_TO_BITS_LO(',')+\
		              CHAR_TO_BITS_LO('-')+\
		              CHAR_TO_BITS_LO(':')+\
		              CHAR_TO_BITS_LO(';')))
#define BITS_PUNCT_EXTRA_HI (BITS_PUNCT_HI & ~(CHAR_TO_BITS_HI('_')))

/* Control characters, \x00 to \x1F */
#define BITS_CNTRL_LO (0x00000000FFFFFFFFL)

#define CHAR_TO_BITS_LO(ch) ((uint64_t)1<<((uint8_t)ch))
#define CHAR_TO_BITS_HI(ch) ((uint64_t)1<<(((uint8_t)ch)-64))

/* Plus, ASCII 0x53 */
#define BITS_PLUS_LO CHAR_TO_BITS_LO('+')
/* Minus, ASCII 0x55 */
#define BITS_MINUS_LO CHAR_TO_BITS_LO('-')
/* Dot, ASCII 0x56 */
#define BITS_DOT_LO  CHAR_TO_BITS_LO('.')
/* E/e, ASCII 0x56 */
#define BITS_LETTER_E_HI  (CHAR_TO_BITS_HI('e')+CHAR_TO_BITS_HI('E'))
/* Space / tab */
#define BITS_WHITESPACE_LO (CHAR_TO_BITS_LO(' ')+CHAR_TO_BITS_LO('\t'))


#define MASK_XDIGITS_LO (~BITS_XDIGITS_LO)
#define MASK_XDIGITS_HI (~BITS_XDIGITS_HI)

#define BITS_INTEGER_LO (BITS_DIGITS_LO+\
			 BITS_PLUS_LO+\
			 BITS_MINUS_LO)

#define BITS_FLOAT_LO (BITS_DIGITS_LO+\
		       BITS_PLUS_LO+\
		       BITS_MINUS_LO+\
		       BITS_DOT_LO)
#define BITS_FLOAT_HI (BITS_LETTER_E_HI)

#if 1
#define CHECK_EXCLUSIVE_BITS(fct,bits_lo,bits_hi) (\
 ((fct->seen_bits_lo&(~bits_lo))==0)&&((fct->seen_bits_hi&(~bits_hi))==0)\
 &&(((fct->seen_bits_lo&(bits_lo))!=0)||((fct->seen_bits_hi&(bits_hi))!=0)))

#define CHECK_HAVE_BITS(fct,bits_lo,bits_hi) \
 (((fct->seen_bits_lo&(bits_lo))!=0)||((fct->seen_bits_hi&(bits_hi))!=0))

#else
static bool
CHECK_SEEN_BITS(const struct field_content_type_t* fct,
		const uint64_t bits_lo, const uint64_t bits_hi)
{
bool lo1 = ((fct->seen_bits_lo&(~bits_lo))==0);
bool lo2 = ((fct->seen_bits_lo&(bits_lo))!=0);
bool hi1 = ((fct->seen_bits_hi&(~bits_hi))==0);
bool hi2 = ((fct->seen_bits_hi&(bits_hi))!=0);

return lo1 && hi1 && ( lo2 || hi2 );
}
#endif


void
init_field_content_type (struct field_content_type_t* fct)
{
  memset (fct, 0, sizeof(*fct));
  fct->first = true;
}

void
update_field_content_type (struct field_content_type_t *fct,
		           const char* str, const size_t len)
{
  int64_t intval;
  long double floatval;
  char *endptr;

  if (len==0)
    return;

  if (fct->first)
    {
      fct->text_max_length = len;
      fct->text_min_length = len;
    }

  if (len > fct->text_max_length)
    fct->text_max_length = len;
  if (len < fct->text_min_length)
    fct->text_min_length = len;

  /* Update the array of seen character values */
  for (size_t i = 0; i < len; ++i)
    {
      const char ch = str[i];
      if (ch<64)
	fct->seen_bits_lo |= CHAR_TO_BITS_LO(ch);
      else /* TODO: if (ch<128) deal with 8bit ascii / utf8*/
	fct->seen_bits_hi |= CHAR_TO_BITS_HI(ch);
/*    else
	fct->high_ascii_bit = true;*/
    }

  /* Based on seen characters, check possible content */

  if (fct->fcf_integer != FCF_NOT_VALID
      && (CHECK_EXCLUSIVE_BITS(fct,BITS_INTEGER_LO,0)))
    {
      errno = 0;
      intval = strtol (str, &endptr, 10);
      if (errno != 0 || endptr == str || endptr == NULL || endptr != (str+len))
	{
	  fct->fcf_integer = FCF_NOT_VALID;
	}
      else
	{
          if (fct->fcf_integer == FCF_NOT_TESTED)
            fct->integer_min_value = fct->integer_max_value = intval;
          fct->fcf_integer = FCF_VALID;

	  if (intval > fct->integer_max_value)
            fct->integer_max_value = intval;
	  if (intval < fct->integer_min_value)
            fct->integer_min_value = intval;
	}
    }

  if (fct->fcf_float != FCF_NOT_VALID
      && (CHECK_EXCLUSIVE_BITS (fct, BITS_FLOAT_LO, BITS_FLOAT_HI)))
    {
      errno = 0;
      floatval = strtold (str, &endptr);
      if (errno != 0 || endptr == str || endptr == NULL || endptr != (str+len))
	{
	  fct->fcf_float = FCF_NOT_VALID;
	}
      else
	{
          if (fct->fcf_float == FCF_NOT_TESTED)
            {
              fct->float_min_value = floatval;
              fct->float_max_value = floatval;
	    }
          fct->fcf_float = FCF_VALID;

	  if (floatval > fct->float_max_value)
            fct->float_max_value = floatval;
	  if (floatval < fct->float_min_value)
            fct->float_min_value = floatval;
	}
    }

  fct->first = false;
}

void
report_field_content_type (struct field_content_type_t *fct,
		           char* /*OUTPUT*/ str, const size_t maxlen)
{
  //fprintf (stderr, "hi = 0x%016lx\n", fct->seen_bits_hi);
  //fprintf (stderr, "lo = 0x%016lx\n", fct->seen_bits_lo);
  if (fct->high_ascii_bit)
    {
      snprintf (str, maxlen, "mixed-characters");
      return ;
    }

  /* Report detected types, starting from the stricter possibilities */
  if (fct->fcf_integer==FCF_VALID)
    {
      snprintf (str,maxlen, "integer (%zd -> %zd)",
		   fct->integer_min_value, fct->integer_max_value);
      return;
    }

  if (fct->fcf_float==FCF_VALID)
    {
      snprintf (str,maxlen, "decimal value (%Lf -> %Lf)",
		   fct->float_min_value, fct->float_max_value);
      return;
    }

#define ADD_LENGTH() \
    do { \
	 snprintf (str+strlen(str), maxlen-strlen(str), \
	           " (length: %zu -> %zu characters)", \
		   fct->text_min_length, \
		   fct->text_max_length); \
    } while (0)


  /* If not a specific type, give description based on seen characters */
  if (CHECK_EXCLUSIVE_BITS(fct,BITS_XDIGITS_LO,BITS_XDIGITS_HI))
    {
      snprintf (str, maxlen, "hex value");
      ADD_LENGTH();
      return ;
    }

  if (CHECK_EXCLUSIVE_BITS(fct,0,BITS_ALPHA_UPPERCASE_HI))
    {
      snprintf (str, maxlen, "Upper-case alphabet");
      ADD_LENGTH();
      return ;
    }

  if (CHECK_EXCLUSIVE_BITS(fct,0,BITS_ALPHA_LOWERCASE_HI))
    {
      snprintf (str, maxlen, "Lower-case alphabet");
      ADD_LENGTH();
      return ;
    }

  if (CHECK_EXCLUSIVE_BITS(fct,0,BITS_ALPHA_LOWERCASE_HI|BITS_ALPHA_UPPERCASE_HI))
    {
      snprintf (str, maxlen, "alphabet");
      ADD_LENGTH();
      return ;
    }
  if (CHECK_EXCLUSIVE_BITS(fct,BITS_DIGITS_LO,
		BITS_ALPHA_LOWERCASE_HI|BITS_ALPHA_UPPERCASE_HI))
    {
      snprintf (str, maxlen, "alpha-numeric");
      return ;
    }

  /* Not a clear-cut - mix of different catagories.
     Try to find the optimal description. */
  const bool uppercase_alpha =
	  CHECK_HAVE_BITS(fct,0,BITS_ALPHA_UPPERCASE_HI);
  const bool lowercase_alpha =
	  CHECK_HAVE_BITS(fct,0,BITS_ALPHA_LOWERCASE_HI);
  const bool alpha = uppercase_alpha || lowercase_alpha;
  const bool digits =
	  CHECK_HAVE_BITS(fct,BITS_DIGITS_LO,0);
  const bool cntrl =
	  CHECK_HAVE_BITS(fct,BITS_CNTRL_LO,0);
  const bool whitespace =
	  CHECK_HAVE_BITS(fct,BITS_WHITESPACE_LO,0);

  const bool period =
	  CHECK_HAVE_BITS(fct,CHAR_TO_BITS_LO('.'),0);
  const bool comma =
	  CHECK_HAVE_BITS(fct,CHAR_TO_BITS_LO(','),0);
  const bool underscore =
	  CHECK_HAVE_BITS(fct,0,CHAR_TO_BITS_HI('_'));
  const bool minus =
	  CHECK_HAVE_BITS(fct,CHAR_TO_BITS_LO('-'),0);
  const bool colon =
	  CHECK_HAVE_BITS(fct,CHAR_TO_BITS_LO(':'),0);
  const bool semicolon =
	  CHECK_HAVE_BITS(fct,CHAR_TO_BITS_LO(';'),0);
  const bool punct_any =
	  CHECK_HAVE_BITS(fct,BITS_PUNCT_LO,BITS_PUNCT_HI);
  const bool punct_extra =
          CHECK_HAVE_BITS(fct,BITS_PUNCT_EXTRA_LO,BITS_PUNCT_EXTRA_HI);

  strcpy(str,"");
  /* "main" content */
  if (alpha && digits)
    strncat (str, "alpha-numeric", maxlen-1-strlen(str));
  else if (alpha)
    strncat (str, "alphabet", maxlen-1-strlen(str));
  else if (digits)
    strncat (str, "digits", maxlen-1-strlen(str));

#define CONDITIONAL_ADD(cond,name) \
   do { \
      if (cond) \
        { \
	  if (strlen(str)>0) \
	    strncat (str, ",", maxlen-1-strlen(str)); \
          strncat (str, name, maxlen-1-strlen(str)); \
	} \
  } while(0)


  /* Extra content */
  if (!punct_extra)
    {
      CONDITIONAL_ADD(comma,"comma");
      CONDITIONAL_ADD(period,"period");
      CONDITIONAL_ADD(minus,"minus");
      CONDITIONAL_ADD(underscore,"underscore");
      CONDITIONAL_ADD(colon,"colon");
      CONDITIONAL_ADD(semicolon,"semicolon");
    }
  else CONDITIONAL_ADD(punct_any,"punctuation");

  CONDITIONAL_ADD(whitespace,"whitespace");
  CONDITIONAL_ADD(cntrl,"control-characters");

  ADD_LENGTH();

//  snprintf (str, maxlen, "mixed-characters");
}

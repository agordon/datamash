/* GNU Datamash - perform simple calculation on input data

   Copyright (C) 2013,2014 Assaf Gordon <assafgordon@gmail.com>

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
#ifndef FIELD_CONTENT_TYPE
#define FIELD_CONTENT_TYPE

enum FIELD_CONTENT_FLAGS
{
  FCF_NOT_TESTED = 0,
  FCF_VALID,
  FCF_NOT_VALID
};

#define LOMASK_CHAR(ch) (((ch)<64)?((uint64_t)1<<((uint8_t)ch)):0)
#define HIMASK_CHAR(ch) (((ch)>=64&&((ch)<128)?((uint64_t)1<<(((uint8_t)ch)-64)):0)

struct field_content_type_t
{
  /* one bit for each octect seen in 8bit ASCII */
  uint64_t seen_bits_lo, seen_bits_hi;
  bool high_ascii_bit;
  bool first;

  size_t text_min_length;
  size_t text_max_length;

  /* Decimal Integer accepted by strtol */
  enum FIELD_CONTENT_FLAGS fcf_integer;
  /* floating-point value accepted by strtod */
  enum FIELD_CONTENT_FLAGS fcf_float;

  int64_t integer_min_value;
  int64_t integer_max_value;

  /* Hex-Decimal Value, without 0x prefix */
  enum FIELD_CONTENT_FLAGS fcf_hex_no_prefix;
  /* Hex-Decimal Value, with 0x prefix */
  enum FIELD_CONTENT_FLAGS fcf_hex_with_prefix;

  long double float_min_value;
  long double float_max_value;

  /* TODO, in the future:
  URL
  PATH
  IPV4_ADDR
  IPV4_ADDR_WITH_MASK
  IPV6_ADDR
  HOSTNAME
  EMAIL
  CIGAR_STRING
  LIST (commas/etc)
  COMMON_PREFIX
  HIGH_BIT_ON
  UTF8?
  */
};

void
init_field_content_type (struct field_content_type_t* fct);

void
update_field_content_type (struct field_content_type_t *fct,
		           const char* str, const size_t len);

void
report_field_content_type (struct field_content_type_t *fct,
		           char* /*OUTPUT*/ str, const size_t maxlen);

#endif

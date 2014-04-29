# Copyright (C) 2014 Assaf Gordon <assafgordon@gmail.com>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# Syntax-checks to skip:
#  sc_copyright_check: requires the word 'Free' in texi after year.
#  sc_prohibit_strings_without_use: using memset/memmove requirs <string.h>
local-checks-to-skip = \
  sc_copyright_check \
  sc_prohibit_strings_without_use

# Hash of the current/old NEWS file.
# checked by 'sc_immutable_NEWS' rule.
# updated by 'update-NEWS-hash' rule.
old_NEWS_hash = d41d8cd98f00b204e9800998ecf8427e

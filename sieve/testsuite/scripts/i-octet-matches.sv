# -*- sieve -*-
# This file is part of Mailutils testsuite.
# Copyright (C) 2002, Free Software Foundation.
# See file COPYING for distribution conditions.

require "comparator-i;octet";

if header :comparator "i;octet" :matches "subject" "$$$*$$$"
  {
    discard;
  }

# -*- sieve -*-
# This file is part of Mailutils testsuite.
# Copyright (C) 2002, Free Software Foundation.
# See file COPYING for distribution conditions.

require "comparator-i;ascii-numeric";

if header :comparator "i;ascii-numeric" :contains "X-Number" "15"
  {
    discard;
  }

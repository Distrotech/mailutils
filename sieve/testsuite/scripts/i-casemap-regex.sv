# -*- sieve -*-
# This file is part of Mailutils testsuite.
# Copyright (C) 2002, Free Software Foundation.
# See file COPYING for distribution conditions.

require "comparator-i;ascii-casemap";

if header :comparator "i;ascii-casemap" :regex "subject" ".*you.*"
  {
    discard;
  }

# -*- sieve -*-
# This file is part of Mailutils testsuite.
# Copyright (C) 2002, Free Software Foundation.
# See file COPYING for distribution conditions.

require "test-numaddr";

if numaddr [ "to", "cc" ] :over 5
  {
    discard;
  }

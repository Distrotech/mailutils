# -*- sieve -*-
# This file is part of Mailutils testsuite.
# Copyright (C) 2002, Free Software Foundation.
# See file COPYING for distribution conditions.

if exists ["X-Caffeine", "X-Status"] {
        discard;
}


#  GNU Mailutils -- a suite of utilities for electronic mail
#  Copyright (C) 2009-2012 Free Software Foundation, Inc.
#
#  GNU Mailutils is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3, or (at your option)
#  any later version.
#
#  GNU Mailutils is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. 

import sys
from mailutils import stream
from mailutils import filter

if len (sys.argv) != 3:
    print "usage: %s from-code to-code" % sys.argv[0]
    sys.exit (0)

sti = stream.StdioStream (stream.MU_STDIN_FD)
cvt = filter.FilterIconvStream (sti, sys.argv[1], sys.argv[2])
out = stream.StdioStream (stream.MU_STDOUT_FD, 0)

total = 0
while True:
    buf = cvt.read ()
    out.write (buf)
    total += cvt.read_count
    if not cvt.read_count:
        break

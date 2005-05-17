# generr.awk -- Create error reporting sources for GNU Mailutils
# Copyright (C) 2005 Free Software Foundation, Inc.
#
# GNU Mailutils is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc. 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA

# This program creates error reporting source files (muerrno.c and errno.h)
# for GNU Mailutils.
#
# Usage:
#      awk -f generr.awk errors muerrno.cin > muerrno.c
#      awk -f generr.awk errors muerrno.hin > errno.h

BEGIN {
  defno = 0
}

ARGIND == 1 && /^ *#/ { next }
ARGIND == 1 && NF == 0 { next }
ARGIND == 1 {
  def[defno] = $1
  match(substr($0,length($1)), "[ \\t]+")
  text[defno] = substr($0,length($1)+RLENGTH+1)
  defno++
}

ARGIND == 2 && /\$AUTOWARN/ {
  match($0,"\\$AUTOWARN");
  print substr($0,1,RSTART-1) "This file is generated automatically. Please, do not edit." substr($0,RSTART+RLENGTH)
  next
}

ARGIND == 2 && $1 == "$MESSAGE_STRINGS" {
  match($0, "[ \\t]+")
  for (i = 0; i < defno; i++) {
    printf "%*.*scase %s:\n", RLENGTH, RLENGTH, "", def[i]
    printf "%*.*sreturn %s;\n\n", RLENGTH+2,RLENGTH+2,"", text[i] 
  }
  next
}

ARGIND == 2 && $1 == "$MESSAGE_CODES" {
  match($0, "[ \\t]+")
  for (i = 0; i < defno; i++) {
    printf "%*.*scase %s:\n", RLENGTH, RLENGTH, "", def[i]
    printf "%*.*sreturn \"%s\";\n\n", RLENGTH+2,RLENGTH+2,"", def[i] 
  }
  next
}

ARGIND == 2 && $1 == "$MESSAGE_DEFS" {
  for (i = 0; i < defno; i++) {
    print "#define " def[i] " (MU_ERR_BASE+" i ")"
  }
  next
}

ARGIND == 2 {
  print
}

# End of generr.awk

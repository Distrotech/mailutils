# This file is part of GNU Mailutils.
# Copyright (C) 2010-2012 Free Software Foundation, Inc.
#
# GNU Mailutils is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2, or (at
# your option) any later version.
#
# GNU Mailutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.

state == 0 && /MU Setup:/  {
  if (NF != 3) {
    print FILENAME ":" NR ":", "missing module name" > "/dev/stderr"
    exit(1)
  } else {  
    state = 1;
    module_name=$3;
    if (modules[module_name]) {
      print FILENAME ":" NR ":", "this module already declared" > "/dev/stderr"
      print modules[module_name] ":", "location of the prior declaration" > "/dev/stderr"
    } else
      modules[module_name] = (FILENAME":"NR);
  }
}
state == 1 && /mu-.*:/ {
  setup[module_name,substr($1,4,length($1)-4)] = $2;
  next
}
state == 1 && /End MU Setup/ { state = 0 }
END {
  print "/* -*- buffer-read-only: t -*- vi: set ro:"
  print "   THIS FILE IS GENERATED AUTOMATICALLY.  PLEASE DO NOT EDIT."
  print "*/"

  vartab[1] = "handler"
  vartab[2] = "docstring"
  n = asorti(modules,modname)
  for (i = 1; i <= n; i++) {
    name = modname[i]
    for (j in vartab) {
      if (!setup[name,vartab[j]]) {
	print modules[name] ":", "mu-" vartab[j], "not defined"
	err = 1
      }
    }
    if (err)
      continue
    if (mode == "h") {
      print "extern int", setup[name,"handler"], "(int argc, char **argv);"
      print "extern char " setup[name,"docstring"] "[];"
    } else {
      if (setup[name,"cond"])
	print "#ifdef", setup[name,"cond"]
      print "{", "\"" name "\",", setup[name,"handler"] ",", setup[name,"docstring"], "},"
      if (setup[name,"cond"])
	print "#endif"
    }
  }
}

  
  

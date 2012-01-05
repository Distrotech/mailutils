# This file is part of GNU Mailutils.
# Copyright (C) 2010-2012 Free Software Foundation, Inc.
#
# GNU Mailutils is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3, or (at
# your option) any later version.
#
# GNU Mailutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.

BEGIN {
	print "/* -*- buffer-read-only: t -*- vi: set ro:"
	print "   THIS FILE IS GENERATED AUTOMATICALLY.  PLEASE DO NOT EDIT."
	print "*/"
	print "#define MU_DEBCAT_ALL 0"
}
{ sub(/#.*/, "") }
NF == 0 { next }
{ print "#ifdef __MU_DEBCAT_C_ARRAY"
  print "{ \"" tolower($1) "\", },"
  print "#else"
  print "# define MU_DEBCAT_" toupper($1), ++category
  print "#endif" }


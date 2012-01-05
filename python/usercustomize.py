# Libtool library loader for Python.
# This file is part of GNU Mailutils.
# Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

# This file modifies the normal import routine by looking additionally
# into module.la file and trying to load corresponding so library (as
# determined by the dlname variable in it) from the .libs subdirectory.
#
# It is used to load Mailutils Python modules from the source tree.
#
# Example usage:
#
#  PYTHONPATH=/path/to/mailutils-3.0/python python test.py

import sys,os,imp

class laloader(object):
    def __init__(self, filename, libdir):
        self.filename = filename
        self.libdir = libdir

    def load_module(self, name):
        with open(self.filename) as f:
            for line in f:
                s = line.strip()
                if s.startswith('dlname='):
                    s = s[7:].strip("'\"")
                    libname = self.libdir + '/' + s
                    if os.path.exists(libname):
                        return imp.load_dynamic(name, libname)
        return None
    
class lafinder(object):
    def __init__(self):
        pass
    def find_module(self, fullname, path=None):
        ml = fullname.split(".")
        s = '/'.join(ml)+'.la'
        libs = '/'.join(ml[:-1]) + '/.libs'
        for dir in sys.path:
            name = dir + '/' + s
            libdir = dir + '/' + libs
            if os.path.exists(name) and os.path.exists(libdir):
                return laloader(name, libdir)
        return None

sys.meta_path.append(lafinder())

# End of file


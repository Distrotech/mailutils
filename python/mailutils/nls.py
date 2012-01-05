#  GNU Mailutils -- a suite of utilities for electronic mail
#  Copyright (C) 2009-2012 Free Software Foundation, Inc.
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 3 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General
#  Public License along with this library.  If not, see
#  <http://www.gnu.org/licenses/>.

from mailutils.c_api import nls

def init ():
    """Initialize Native Language Support."""
    nls.init ()

def set_locale (locale):
    """Set locale via LC_ALL."""
    nls.set_locale (locale)

def restore_locale ():
    nls.restore_locale ()

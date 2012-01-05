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

from mailutils.c_api import util

def get_user_email (name=None):
    if name == None:
        return util.get_user_email ()
    else:
        return util.get_user_email (name)

def set_user_email (email):
    util.set_user_email (email)

def get_user_email_domain ():
    status, domain = util.get_user_email_domain ()
    return domain

def set_user_email_domain (domain):
    util.set_user_email_domain (domain)

def tempname (tmpdir=None):
    return util.tempname (tmpdir)

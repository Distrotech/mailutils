#
#  GNU Mailutils -- a suite of utilities for electronic mail
#  Copyright (C) 2009 Free Software Foundation, Inc.
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
#  Public License along with this library; if not, write to the
#  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#  Boston, MA 02110-1301 USA
#

import types
from mailutils.c_api import attribute
from mailutils.error import *

MU_ATTRIBUTE_ANSWERED = 0x01
MU_ATTRIBUTE_FLAGGED  = 0x02
MU_ATTRIBUTE_DELETED  = 0x04
MU_ATTRIBUTE_DRAFT    = 0x08
MU_ATTRIBUTE_SEEN     = 0x10
MU_ATTRIBUTE_READ     = 0x20
MU_ATTRIBUTE_MODIFIED = 0x40

class Attribute:
    def __init__ (self, attr):
        self.attr = attr

    def __del__ (self):
        del self.attr

    def __str__ (self):
        return attribute.to_string (self.attr)

    def __getitem__ (self, flag):
        return self.is_flag (flag)

    def __setitem__ (self, flag, value):
        if value == True:
            self.set_flags (flag)
        elif value == False:
            self.unset_flags (flag)
        else:
            raise TypeError, value

    def is_modified (self):
        return attribute.is_modified (self.attr)

    def clear_modified (self):
        attribute.clear_modified (self.attr)

    def set_modified (self):
        attribute.set_modified (self.attr)

    def get_flags (self):
        status, flags = attribute.get_flags (self.attr)
        if status:
            raise Error (status)
        return flags

    def set_flags (self, flags):
        status = attribute.set_flags (self.attr, flags)
        if status:
            raise Error (status)

    def unset_flags (self, flags):
        status = attribute.unset_flags (self.attr, flags)
        if status:
            raise Error (status)

    def is_flag (self, flag):
        flags = self.get_flags ()
        if flags & flag:
            return True
        return False

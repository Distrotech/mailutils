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

from mailutils.c_api import error
from mailutils.c_api.errno import *

def strerror (status):
    return error.strerror (status)

class Error (Exception):
    def __init__ (self, status, str_error=None):
        self.status = status
        self.strerror = str_error or strerror (status)
    def __str__ (self):
        return "%d: %s" % (self.status, self.strerror)

class AddressError (Error): pass
class AuthError (Error): pass
class BodyError (Error): pass
class DebugError (Error): pass
class EnvelopeError (Error): pass
class FolderError (Error): pass
class HeaderError (Error): pass
class MailerError (Error): pass
class MailboxError (Error): pass
class MailcapError (Error): pass
class MessageError (Error): pass
class MimeError (Error): pass
class SecretError (Error): pass
class SieveMachineError (Error): pass
class StreamError (Error): pass
class UrlError (Error): pass


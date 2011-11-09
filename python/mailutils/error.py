#  GNU Mailutils -- a suite of utilities for electronic mail
#  Copyright (C) 2009, 2010, 2011 Free Software Foundation, Inc.
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

def strerror (status):
    return error.strerror (status)

class Error (Exception):
    def __init__ (self, status):
        self.status = status
        self.strerror = strerror (status)
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

MU_ERR_BASE = 0x1000

MU_ERR_FAILURE = (MU_ERR_BASE+0)
MU_ERR_CANCELED = (MU_ERR_BASE+1)
MU_ERR_EMPTY_VFN = (MU_ERR_BASE+2)
MU_ERR_OUT_PTR_NULL = (MU_ERR_BASE+3)
MU_ERR_MBX_REMOVED = (MU_ERR_BASE+4)
MU_ERR_NOT_OPEN = (MU_ERR_BASE+5)
MU_ERR_OPEN = (MU_ERR_BASE+6)
MU_ERR_INVALID_EMAIL = (MU_ERR_BASE+7)
MU_ERR_EMPTY_ADDRESS = (MU_ERR_BASE+8)
MU_ERR_LOCKER_NULL = (MU_ERR_BASE+9)
MU_ERR_LOCK_CONFLICT = (MU_ERR_BASE+10)
MU_ERR_LOCK_BAD_LOCK = (MU_ERR_BASE+11)
MU_ERR_LOCK_BAD_FILE = (MU_ERR_BASE+12)
MU_ERR_LOCK_NOT_HELD = (MU_ERR_BASE+13)
MU_ERR_LOCK_EXT_FAIL = (MU_ERR_BASE+14)
MU_ERR_LOCK_EXT_ERR = (MU_ERR_BASE+15)
MU_ERR_LOCK_EXT_KILLED = (MU_ERR_BASE+16)
MU_ERR_NO_SUCH_USER = (MU_ERR_BASE+17)
MU_ERR_GETHOSTBYNAME = (MU_ERR_BASE+18)
MU_ERR_MAILER_BAD_FROM = (MU_ERR_BASE+19)
MU_ERR_MAILER_BAD_TO = (MU_ERR_BASE+20)
MU_ERR_MAILER_NO_RCPT_TO = (MU_ERR_BASE+21)
MU_ERR_MAILER_BAD_URL = (MU_ERR_BASE+22)
MU_ERR_SMTP_RCPT_FAILED = (MU_ERR_BASE+23)
MU_ERR_TCP_NO_HOST = (MU_ERR_BASE+24)
MU_ERR_TCP_NO_PORT = (MU_ERR_BASE+25)
MU_ERR_BAD_2047_INPUT = (MU_ERR_BASE+26)
MU_ERR_BAD_2047_ENCODING = (MU_ERR_BASE+27)
MU_ERR_NOUSERNAME = (MU_ERR_BASE+28)
MU_ERR_NOPASSWORD = (MU_ERR_BASE+29)
MU_ERR_BADREPLY = (MU_ERR_BASE+30)
MU_ERR_SEQ = (MU_ERR_BASE+31)
MU_ERR_REPLY = (MU_ERR_BASE+32)
MU_ERR_BAD_AUTH_SCHEME = (MU_ERR_BASE+33)
MU_ERR_AUTH_FAILURE = (MU_ERR_BASE+34)
MU_ERR_PROCESS_NOEXEC = (MU_ERR_BASE+35)
MU_ERR_PROCESS_EXITED = (MU_ERR_BASE+36)
MU_ERR_PROCESS_SIGNALED = (MU_ERR_BASE+37)
MU_ERR_PROCESS_UNKNOWN_FAILURE = (MU_ERR_BASE+38)
MU_ERR_CONN_CLOSED = (MU_ERR_BASE+39)
MU_ERR_PARSE = (MU_ERR_BASE+40)
MU_ERR_NOENT = (MU_ERR_BASE+41)
MU_ERR_EXISTS = (MU_ERR_BASE+42)
MU_ERR_BUFSPACE = (MU_ERR_BASE+43)
MU_ERR_SQL = (MU_ERR_BASE+44)
MU_ERR_DB_ALREADY_CONNECTED = (MU_ERR_BASE+45)
MU_ERR_DB_NOT_CONNECTED = (MU_ERR_BASE+46)
MU_ERR_RESULT_NOT_RELEASED = (MU_ERR_BASE+47)
MU_ERR_NO_QUERY = (MU_ERR_BASE+48)
MU_ERR_BAD_COLUMN = (MU_ERR_BASE+49)
MU_ERR_NO_RESULT = (MU_ERR_BASE+50)
MU_ERR_NO_INTERFACE = (MU_ERR_BASE+51)
MU_ERR_BADOP = (MU_ERR_BASE+52)
MU_ERR_BAD_FILENAME = (MU_ERR_BASE+53)
MU_ERR_READ = (MU_ERR_BASE+54)
MU_ERR_NO_TRANSPORT = (MU_ERR_BASE+55)
MU_ERR_AUTH_NO_CRED = (MU_ERR_BASE+56)
MU_ERR_URL_MISS_PARTS = (MU_ERR_BASE+57)
MU_ERR_URL_EXTRA_PARTS = (MU_ERR_BASE+58)
MU_ERR_URL_INVALID_PARAMETER = (MU_ERR_BASE+59)
MU_ERR_INFO_UNAVAILABLE = (MU_ERR_BASE+60)
MU_ERR_NONAME = (MU_ERR_BASE+61)
MU_ERR_BADFLAGS = (MU_ERR_BASE+62)
MU_ERR_SOCKTYPE = (MU_ERR_BASE+63)
MU_ERR_FAMILY = (MU_ERR_BASE+64)
MU_ERR_SERVICE = (MU_ERR_BASE+65)
MU_ERR_PERM_OWNER_MISMATCH = (MU_ERR_BASE+66)
MU_ERR_PERM_GROUP_WRITABLE = (MU_ERR_BASE+67)
MU_ERR_PERM_WORLD_WRITABLE = (MU_ERR_BASE+68)
MU_ERR_PERM_GROUP_READABLE = (MU_ERR_BASE+69)
MU_ERR_PERM_WORLD_READABLE = (MU_ERR_BASE+70)
MU_ERR_PERM_LINKED_WRDIR = (MU_ERR_BASE+71)
MU_ERR_PERM_DIR_IWGRP = (MU_ERR_BASE+72)
MU_ERR_PERM_DIR_IWOTH = (MU_ERR_BASE+73)
MU_ERR_DISABLED = (MU_ERR_BASE+74)
MU_ERR_LAST = (MU_ERR_BASE+75)

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

from mailutils.c_api import mailbox
from mailutils import message
from mailutils import folder
from mailutils import url
from mailutils import debug
from mailutils.error import MailboxError

class MailboxBase:
    def open (self, flags = 0):
        """Open the connection."""
        status = mailbox.open (self.mbox, flags)
        if status:
            raise MailboxError (status)

    def close (self):
        """Close the connection."""
        status = mailbox.close (self.mbox)
        if status:
            raise MailboxError (status)

    def flush (self, expunge = False):
        """Flush the mailbox."""
        status = mailbox.flush (self.mbox, expunge)
        if status:
            raise MailboxError (status)

    def messages_count (self):
        """Return the number of messages in mailbox."""
        status, total = mailbox.messages_count (self.mbox)
        if status:
            raise MailboxError (status)
        return total

    def messages_recent (self):
        """Return the number of recent messages in mailbox."""
        status, recent = mailbox.messages_recent (self.mbox)
        if status:
            raise MailboxError (status)
        return recent

    def message_unseen (self):
        """Return the number of first unseen message in mailbox."""
        status, recent = mailbox.message_unseen (self.mbox)
        if status:
            raise MailboxError (status)
        return unseen

    def get_message (self, msgno):
        """Retrieve message number MSGNO."""
        status, c_msg = mailbox.get_message (self.mbox, msgno)
        if status:
            raise MailboxError (status)
        return message.Message (c_msg)

    def append_message (self, msg):
        """Append MSG to the mailbox."""
        status = mailbox.append_message (self.mbox, msg.msg)
        if status:
            raise MailboxError (status)

    def expunge (self):
        """Remove all messages marked for deletion."""
        status = mailbox.expunge (self.mbox)
        if status:
            raise MailboxError (status)

    def sync (self):
        """Synchronize the mailbox."""
        status = mailbox.sync (self.mbox)
        if status:
            raise MailboxError (status)

    def lock (self):
        """Lock the mailbox."""
        status = mailbox.lock (self.mbox)
        if status:
            raise MailboxError (status)

    def unlock (self):
        """Unlock the mailbox."""
        status = mailbox.unlock (self.mbox)
        if status:
            raise MailboxError (status)

    def get_size (self):
        """Return the mailbox size."""
        status, size = mailbox.get_size (self.mbox)
        if status:
            raise MailboxError (status)
        return size

    def get_folder (self):
        """Get the FOLDER."""
        status, fld = mailbox.get_folder (self.mbox)
        if status:
            raise MailboxError (status)
        return folder.Folder (fld)

    def get_debug (self):
        status, dbg = mailbox.get_debug (self.mbox)
        if status:
            raise MailboxError (status)
        return debug.Debug (dbg)

    def get_url (self):
        status, u = mailbox.get_url (self.mbox)
        if status:
            raise MailboxError (status)
        return url.Url (u)

    def next (self):
        if self.__count >= self.__len:
            self.__count = 0
            raise StopIteration
        else:
            self.__count += 1
            return self.get_message (self.__count)

    def __getitem__ (self, msgno):
        return self.get_message (msgno)

    def __iter__ (self):
        self.__count = 0
        self.__len = self.messages_count ()
        return self

    def __getattr__ (self, name):
        if name == 'size':
            return self.get_size ()
        elif name == 'folder':
            return self.get_folder ()
        elif name == 'url':
            return self.get_url ()
        elif name == 'debug':
            return self.get_debug ()
        else:
            raise AttributeError, name

    def __len__ (self):
        return self.messages_count ()


class Mailbox (MailboxBase):
    __owner = False
    def __init__ (self, name):
        if isinstance (name, mailbox.MailboxType):
            self.mbox = name
        else:
            self.mbox = mailbox.MailboxType ()
            self.__owner = True
            status = mailbox.create (self.mbox, name)
            if status:
                raise MailboxError (status)

    def __del__ (self):
        if self.__owner:
            mailbox.destroy (self.mbox)
        del self.mbox

class MailboxDefault (MailboxBase):
    def __init__ (self, name):
        self.mbox = mailbox.MailboxType ()
        status = mailbox.create_default (self.mbox, name)
        if status:
            raise MailboxError (status)

    def __del__ (self):
        mailbox.destroy (self.mbox)
        del self.mbox

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "pop3d.h"

/* EXPIRE see RFC2449:

  Implementation:
    When a message is downloaded by RETR, it is marked with
    "X-Expire-Timestamp: N", where N is the current value of
    UNIX timestamp.

    If pop3d was started with --delete-expired, the messages whose
    X-Expire-Timestamp is more than (time(NULL)-expire days) old
    are deleted.

    Otherwise, such messages remain in the mailbox and the system
    administrator is supposed to run a cron job that purges the mailboxes
    (easily done using GNU sieve timestamp extension).

*/

void
pop3d_mark_retr (attribute_t attr)
{
  attribute_set_userflag (attr, POP3_ATTRIBUTE_RETR);
}
 
int
pop3d_is_retr (attribute_t attr)
{
  return attribute_is_userflag (attr, POP3_ATTRIBUTE_RETR);
}
 
void
pop3d_unmark_retr (attribute_t attr)
{
  if (attribute_is_userflag (attr, POP3_ATTRIBUTE_RETR))
    attribute_unset_userflag (attr, POP3_ATTRIBUTE_RETR);
}

static int
header_is_expired (header_t hdr)
{
  time_t timestamp;
  char buf[64];
  char *p;
  
  if (!expire_on_exit)
    return 0;
  if (header_get_value (hdr, MU_HEADER_X_EXPIRE_TIMESTAMP,
			buf, sizeof buf, NULL))
    return 0;
  timestamp = strtoul (buf, &p, 0);
  while (*p && isspace (*p))
    p++;
  if (*p)
    return 0;
  return time (NULL) >= timestamp + expire * 86400;
}

/* If pop3d is started with --expire, add an expiration header to the message.
   Additionally, if --deltete-expired option was given, mark
   the message as deleted if its X-Expire-Timestamp is too old.
   Arguments:
      msg   - Message to operate upon
      value - Points to a character buffer where the value of
              X-Expire-Timestamp is to be stored. *value must be set to
	      NULL upon the first invocation of this function */
void
expire_mark_message (message_t msg, char **value)
{
  /* Mark the message with a timestamp. */
  if (expire != EXPIRE_NEVER)
    {
      header_t header = NULL;
      attribute_t attr = NULL;
      
      if (!*value)
	asprintf (value, "%lu", (unsigned long) time (NULL));
      
      message_get_header (msg, &header);
      message_get_attribute (msg, &attr);
	  
      if (pop3d_is_retr (attr))
	header_set_value (header, MU_HEADER_X_EXPIRE_TIMESTAMP, *value, 0);
      
      if (header_is_expired (header))
	attribute_set_deleted (attr);
    }
}

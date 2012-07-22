/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "pop3d.h"

/* From RFC 1939:
   
   In order to simplify parsing, all POP3 servers are
   required to use a certain format for scan listings.  A
   scan listing consists of the message-number of the
   message, followed by a single space and the exact size of
   the message in octets.  Methods for calculating the exact
   size of the message are described in the "Message Format"
   section below.  This memo makes no requirement on what
   follows the message size in the scan listing.  Minimal
   implementations should just end that line of the response
   with a CRLF pair.  More advanced implementations may
   include other information, as parsed from the message.
   <end of quote>
   
  GNU pop3d uses this allowance and includes in the scan
  listing the number of lines in message.  This optional
  feature is enabled by setting "scan-lines yes" in the
  configuration file.  When on, it is indicated by the
  XLINES capability.
*/
int
pop3d_list (char *arg, struct pop3d_session *sess)
{
  size_t mesgno;
  mu_message_t msg = NULL;
  mu_attribute_t attr = NULL;
  size_t size = 0;
  size_t lines = 0;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  if (strchr (arg, ' ') != NULL)
    return ERR_BAD_ARGS;

  if (strlen (arg) == 0)
    {
      size_t total = 0;
      pop3d_outf ("+OK\n");
      mu_mailbox_messages_count (mbox, &total);
      for (mesgno = 1; mesgno <= total; mesgno++)
	{
	  mu_mailbox_get_message (mbox, mesgno, &msg);
	  mu_message_get_attribute (msg, &attr);
	  if (!pop3d_is_deleted (attr))
	    {
	      mu_message_size (msg, &size);
	      mu_message_lines (msg, &lines);
	      pop3d_outf ("%s %s", 
                          mu_umaxtostr (0, mesgno), 
                          mu_umaxtostr (1, size + lines));
	      if (pop3d_xlines)
		pop3d_outf (" %s", mu_umaxtostr (2, lines));
	      pop3d_outf ("\n");
	    }
	}
      pop3d_outf (".\n");
    }
  else
    {
      mesgno = strtoul (arg, NULL, 10);
      if (mu_mailbox_get_message (mbox, mesgno, &msg) != 0)
	return ERR_NO_MESG;
      mu_message_get_attribute (msg, &attr);
      if (pop3d_is_deleted (attr))
	return ERR_MESG_DELE;
      mu_message_size (msg, &size);
      mu_message_lines (msg, &lines);
      pop3d_outf ("+OK %s %s", 
                  mu_umaxtostr (0, mesgno),
                  mu_umaxtostr (1, size + lines));
      if (pop3d_xlines)
	pop3d_outf (" %s", mu_umaxtostr (2, lines));
      pop3d_outf ("\n");
    }

  return OK;
}


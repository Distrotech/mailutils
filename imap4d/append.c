/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "imap4d.h"

/* APPEND mbox [(flags)] [date_time] message_literal */
int
imap4d_append (struct imap4d_command *command, char *arg)
{
  char *sp;
  char *mboxname;
  char *attr_str;
  int flags = 0;
  mailbox_t dest_mbox = NULL;
  int status;
  
  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");

  mboxname = util_getword (arg, &sp);
  if (!mboxname)
    return util_finish (command, RESP_BAD, "Too few arguments");

  if (*sp == '(' && util_parse_attributes (sp+1, &sp, &flags))
    return util_finish (command, RESP_BAD, "Missing closing parenthesis");

  mboxname = namespace_getfullpath (mboxname, "/");
  if (!mboxname)
    return util_finish (command, RESP_NO, "Couldn't open mailbox"); 

  status = mailbox_create_default (&dest_mbox, mboxname);
  if (status == 0)
    {
      /* It SHOULD NOT automatifcllly create the mailbox. */
      status = mailbox_open (dest_mbox, MU_STREAM_RDWR);
      if (status == 0)
	{
	  status = imap4d_append0 (dest_mbox, flags, sp);
	  mailbox_close (dest_mbox);
	}
      mailbox_destroy (&dest_mbox);
    }
  
  free (mboxname);
  if (status == 0)
    return util_finish (command, RESP_OK, "Completed");

  return util_finish (command, RESP_NO, "[TRYCREATE] failed");
}

int
imap4d_append0 (mailbox_t mbox, int flags, char *text)
{
  mailbox_t tmp;
  stream_t stream;
  int rc = 0;
  size_t len = 0;
  message_t msg;
  struct tm *tm;
  time_t t;
  char date[80];
  
  if (mailbox_create (&tmp, "/dev/null"))
    return 1;
  if (mailbox_open (tmp, MU_STREAM_READ) != 0)
    return 1;
  
  if (memory_stream_create (&stream))
    {
      mailbox_close (tmp);
      return 1;
    }

  /* If a date_time is specified, the internal date SHOULD be set in the
     resulting message; otherwise, the internal date of the resulting
     message is set to the current date and time by default. */
  if (util_parse_internal_date0 (text, &t, &text) == 0)
    {
      while (*text && isspace(*text))
	text++;
    }
  else
    {
      time(&t);
    }
  tm = gmtime(&t);
  strftime (date, sizeof (date),
	    "From GNU-imap4d %a %b %e %H:%M:%S %Y%n",
	    tm);
  
  stream_write (stream, date, strlen (date), 0, &len);
  stream_write (stream, text, strlen (text), len, &len);

  mailbox_destroy_folder (tmp);
  mailbox_set_stream (tmp, stream);
  mailbox_messages_count (tmp, &len);
  if (len == 1)
    {
      mailbox_get_message (tmp, 1, &msg);
      mailbox_append_message (mbox, msg);
      if (flags)
	{
	  size_t num = 0;
	  attribute_t attr = NULL;
	  mailbox_messages_count (mbox, &num);
	  mailbox_get_message (mbox, num, &msg);
	  message_get_attribute (msg, &attr);
	  attribute_set_flags (attr, flags);
	}
    }
  else
    rc = 1;
  
  mailbox_close (tmp);
  mailbox_destroy (&tmp);
  return rc;
}

  

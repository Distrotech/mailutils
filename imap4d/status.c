/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "imap4d.h"

/*
 *  
 */

typedef int (*status_funcp) (mu_mailbox_t);

static int status_messages    (mu_mailbox_t);
static int status_recent      (mu_mailbox_t);
static int status_uidnext     (mu_mailbox_t);
static int status_uidvalidity (mu_mailbox_t);
static int status_unseen      (mu_mailbox_t);

struct status_table {
  char *name;
  status_funcp fun;
} status_table[] = {
  {"MESSAGES", status_messages},
  {"RECENT", status_recent},
  {"UIDNEXT", status_uidnext},
  {"UIDVALIDITY", status_uidvalidity},
  {"UNSEEN", status_unseen},
  { NULL }
};

static status_funcp
status_get_handler (const char *name)
{
  struct status_table *p;

  for (p = status_table; p->name; p++)
    if (strcasecmp (p->name, name) == 0)
      return p->fun;
  return NULL;
}
  
int
imap4d_status (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *name;
  char *mailbox_name;
  const char *delim = "/";
  mu_mailbox_t smbox = NULL;
  int status;
  int count = 0;
  char *err_msg = NULL;
  
  name = util_getword (arg, &sp);
  util_unquote (&name);
  if (!name || *name == '\0' || !sp || *sp == '\0')
    return util_finish (command, RESP_BAD, "Too few args");

  mailbox_name = namespace_getfullpath (name, delim);

  if (!mailbox_name)
    return util_finish (command, RESP_NO, "Error opening mailbox");

  /* We may be opening the current mailbox, so make sure the attributes are
     preserved */
  mu_mailbox_save_attributes (mbox);
  
  status = mu_mailbox_create_default (&smbox, mailbox_name);
  if (status == 0)
    {
      status = mu_mailbox_open (smbox, MU_STREAM_READ);
      if (status == 0)
	{
	  char item[32];
	  item[0] = '\0';

	  if (*sp == '(')
	    sp++;
	  else
	    *sp = 0;
	  
	  /* Get the status item names.  */
	  while (*sp && *sp != ')')
	    {
	      status_funcp fun;
	      
	      util_token (item, sizeof (item), &sp);
	      fun = status_get_handler (item);
	      if (!fun)
		{
		  err_msg = "Invalid flag in list";
		  break;
		}
		  
	      if (count++ == 0)
		util_send ("* STATUS %s (", name);

	      if (!fun (smbox))
		util_send (" ");
	    }
	  if (count > 0)
	    util_send (")\r\n");
	  mu_mailbox_close (smbox);
	}
      mu_mailbox_destroy (&smbox);
    }
  free (mailbox_name);

  if (status == 0)
    {
      if (count == 0)
	return util_finish (command, RESP_BAD, "Too few args (empty list)");
      else if (err_msg)
	return util_finish (command, RESP_BAD, err_msg);
      return util_finish (command, RESP_OK, "Completed");
    }
  
  return util_finish (command, RESP_NO, "Error opening mailbox");
}

static int
status_messages (mu_mailbox_t smbox)
{
  size_t total = 0;
  mu_mailbox_messages_count (smbox, &total);
  util_send ("MESSAGES %u", total);
  return 0;
}

static int
status_recent (mu_mailbox_t smbox)
{
  size_t recent = 0;
  mu_mailbox_messages_recent (smbox, &recent);
  util_send ("RECENT %u", recent);
  return 0;
}

static int
status_uidnext (mu_mailbox_t smbox)
{
  size_t uidnext = 1;
  mu_mailbox_uidnext (smbox, &uidnext);
  util_send ("UIDNEXT %u", uidnext);
  return 0;
}

static int
status_uidvalidity (mu_mailbox_t smbox)
{
  unsigned long uidvalidity = 0;
  util_uidvalidity (smbox, &uidvalidity);
  util_send ("UIDVALIDITY %u", uidvalidity);
  return 0;
}

/* Note that unlike the unseen response code, which indicates the message
   number of the first unseen message, the unseeen item in the response the
   status command indicates the quantity of unseen messages.  */
static int
status_unseen (mu_mailbox_t smbox)
{
  size_t total = 0;
  size_t unseen = 0;
  size_t i;
  mu_mailbox_messages_count (smbox, &total);
  for (i = 1; i <= total; i++)
    {
      mu_message_t msg = NULL;
      mu_attribute_t attr = NULL;
      mu_mailbox_get_message (smbox, i, &msg);
      mu_message_get_attribute (msg, &attr);
      /* RFC 3501:
           \Seen
 	      Message has been read
      */
      if (!mu_attribute_is_read (attr))
	unseen++;
    }
  util_send ("UNSEEN %d", unseen);
  return 0;

}

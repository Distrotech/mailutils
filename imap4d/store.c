/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
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
 * Now you're messing with a sumbitch
 */

int
imap4d_store (struct imap4d_command *command, char *arg)
{
  int rc;
  char buffer[64];

  rc = imap4d_store0 (arg, 0, buffer, sizeof buffer);
  return util_finish (command, rc, "%s", buffer);
}

int
imap4d_store0 (char *arg, int isuid, char *resp, size_t resplen)
{
  char *msgset;
  char *data;
  char *sp = NULL;
  int status;
  int ack = 0;
  size_t i;
  int n = 0;
  size_t *set = NULL;
  enum value_type { STORE_SET, STORE_ADD, STORE_UNSET } how;

  msgset = util_getword (arg, &sp);
  data = util_getword (NULL, &sp);

  if (!msgset || !data || !sp || *sp == '\0')
    {
      snprintf (resp, resplen, "Too few args");
      return RESP_BAD;
    }

  /* The parsing of the data-item is a little slugish.  */
  if (strcasecmp (data, "FLAGS") == 0)
    {
      ack = 1;
      how = STORE_SET;
    }
  else if (strcasecmp (data, "+FLAGS") == 0)
    {
      ack = 1;
      how = STORE_ADD;
    }
  else if (strcasecmp (data, "-FLAGS") == 0)
    {
      ack = 1;
      how = STORE_UNSET;
    }
  else if (strcasecmp (data, "FLAGS.SILENT") == 0)
    {
      ack = 0;
      how = STORE_SET;
    }
  else if (strcasecmp (data, "+FLAGS.SILENT") == 0)
    {
      ack = 0;
      how = STORE_ADD;
    }
  else if (strcasecmp (data, "-FLAGS.SILENT") == 0)
    {
      ack = 0;
      how = STORE_UNSET;
    }
  else
    {
      snprintf (resp, resplen, "Bogus data item");
      return RESP_BAD;
    }

  /* Get the message numbers in set[].  */
  status = util_msgset (msgset, &set, &n, isuid);
  if (status != 0)
    {
      snprintf (resp, resplen, "Bogus number set");
      return RESP_BAD;
    }

  for (i = 0; i < n; i++)
    {
      mu_message_t msg = NULL;
      mu_attribute_t attr = NULL;
      char *items = strdup (sp); /* Don't use the orignal list.  */
      int first = 1;
      size_t msgno;
      char *p = items;

      msgno = (isuid) ? uid_to_msgno (set[i]) : set[i];
      if (msgno)
	{
	  mu_mailbox_get_message (mbox, msgno, &msg);
	  mu_message_get_attribute (msg, &attr);

	  /* Get the fetch command names.  */
	  while (*items && *items != ')')
	    {
	      int type = 0;
	      char item[64] = "";
	      util_token (item, sizeof (item), &items);
	      if (!util_attribute_to_type (item, &type))
		{
		  if (how == STORE_ADD )
		    mu_attribute_set_flags (attr, type);
		  else if (how == STORE_UNSET )
		    mu_attribute_unset_flags (attr, type);
		  else if (how == STORE_SET )
		    {
		      if (first)
			{
			  mu_attribute_set_flags (attr, 0);
			  first = 0;
			}
		      mu_attribute_set_flags (attr, type);
		    }
		  mu_attribute_set_flags (attr, MU_ATTRIBUTE_MODIFIED);
		}
	    }
	}
      if (ack)
	{
	  util_send ("* %d FETCH (", msgno);
	  fetch_flags0 ("FLAGS", msg, isuid);
	  util_send (")\r\n");
	}
      free (p);
      /* Update the flags of uid table.  */
      imap4d_sync_flags (set[i]);
    }
  free (set);
  snprintf (resp, resplen, "Completed");
  return RESP_OK;
}


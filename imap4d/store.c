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

/*
 * Now you're messing with a sumbitch
 */
static int get_attribute_type __P ((const char *, int *));

int
imap4d_store (struct imap4d_command *command, char *arg)
{
  char *msgset;
  char *data;
  char *sp = NULL;
  int status;
  int ack = 0;
  size_t i, n = 0;
  int *set = NULL;
  enum value_type { STORE_SET, STORE_ADD, STORE_UNSET } how;

  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");

  msgset = util_getword (arg, &sp);
  data = util_getword (NULL, &sp);

  if (!msgset || !data || !sp || *sp == '\0')
    return util_finish (command, RESP_BAD, "Too few args");

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
    return util_finish (command, RESP_BAD, "Bogus data item");

  /* Get the message numbers in set[].  */
  status = util_msgset (msgset, &set, &n, 0);
  if (status != 0)
    return util_finish (command, RESP_BAD, "Bogus number set");

  for (i = 0; i < n; i++)
    {
      message_t msg = NULL;
      attribute_t attr = NULL;
      char *items = strdup (sp); /* Don't use the orignal list.  */
      char *flags = strdup ("");
      int first = 1;

      mailbox_get_message (mbox, set[i], &msg);
      message_get_attribute (msg, &attr);

      /* Get the fetch command names.  */
      while (*items && *items != ')')
	{
	  int type = 0;
	  char item[64] = "";
	  util_token (item, sizeof (item), &items);
	  if (get_attribute_type (item, &type))
	    {
	      if (how == STORE_ADD )
		attribute_set_flags (attr, type);
	      else if (how == STORE_UNSET )
		attribute_unset_flags (attr, type);
	      else if (how == STORE_SET )
		{
		  if (first)
		    {
		      attribute_set_flags (attr, 0);
		      first = 0;
		    }
		  attribute_set_flags (attr, type);
		}
	      flags = realloc (flags, strlen (flags) + strlen (item) + 2);
	      if (*flags)
		strcat (flags, " ");
	      strcat (flags, item);
	    }
	}
      if (ack && *flags)
	util_out (RESP_NONE, "%d FETCH FLAGS (%s)", set[i], flags);
      free (items);
      free (flags);
    }
  free (set);
  return util_finish (command, RESP_OK, "Completed");
}

static int
get_attribute_type (const char *item, int *type)
{
  if (strcasecmp (item, "\\Answered") == 0)
    *type = MU_ATTRIBUTE_ANSWERED;
  else if (strcasecmp (item, "\\Deleted") == 0)
    *type = MU_ATTRIBUTE_DELETED;
  else if (strcasecmp (item, "\\Draft") == 0)
    *type = MU_ATTRIBUTE_DRAFT;
  else if (strcasecmp (item, "\\Flagged") == 0)
    *type = MU_ATTRIBUTE_FLAGGED;
  else if (strcasecmp (item, "\\Recent") == 0)
    *type = MU_ATTRIBUTE_RECENT;
  else if (strcasecmp (item, "\\Seen") == 0)
    *type = MU_ATTRIBUTE_SEEN;
  else
    return 0;
  return 1;
}

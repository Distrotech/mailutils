/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2002, 2005, 2007, 2010-2012, 2014-2016 Free
   Software Foundation, Inc.

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

#include "mail.h"

static char *prev_name;

/* Expand mail special characters:
 * #	    the previous file
 * &	    the current mbox
 * +file    the file named in the folder directory (set folder=foo)
 * %	    system mailbox
 * %user    system mailbox of the user
 */
char *
mail_expand_name (const char *name)
{
  int status = 0;
  char *exp = NULL;
    
  if (strcmp (name, "#") == 0)
    {
      if (!prev_name)
	{
	  mu_error (_("No previous file"));
	  return NULL;
	}
      else
	return mu_strdup (prev_name);
    }

  if (strcmp (name, "&") == 0)
    {
      name = getenv ("MBOX");
      if (!name)
	{
	  mu_error (_("MBOX environment variable not set"));
	  return NULL;
	}
      /* else fall through */
    }

  status = mu_mailbox_expand_name (name, &exp);

  if (status)
    mu_error (_("Failed to expand %s: %s"), name, mu_strerror (status));

  return (char*) exp;
}

/*
 * fi[le] [file]
 * fold[er] [file]
 */

int
mail_file (int argc, char **argv)
{
  if (argc == 1)
    {
      mail_summary (0, NULL);
    }
  else if (argc == 2)
    {
      /* switch folders */
      char *pname;
      mu_url_t url;
      mu_mailbox_t newbox = NULL;
      char *name = mail_expand_name (argv[1]);
      int status;

      if (!name)
	return 1;
      
      if ((status = mu_mailbox_create (&newbox, name)) != 0 
	  || (status = mu_mailbox_open (newbox, MU_STREAM_RDWR)) != 0)
	{
	  mu_mailbox_destroy (&newbox);
	  mu_error(_("Cannot open mailbox %s: %s"), name, mu_strerror (status));
	  free (name);
	  return 1;
	}

      free (name); /* won't need it any more */
      page_invalidate (1); /* Invalidate current page map */
      
      mu_mailbox_get_url (mbox, &url);
      pname = mu_strdup (mu_url_to_string (url));
      if (mail_mbox_close ())
	{
	  if (pname)
	    free (pname);
	  mu_mailbox_close (newbox);
	  mu_mailbox_destroy (&newbox);
	  return 1;
	}
      
      if (prev_name)
	free (prev_name);
      prev_name = pname;
      
      mbox = newbox;
      mu_mailbox_messages_count (mbox, &total);
      set_cursor (1);
      if (mailvar_get (NULL, "header", mailvar_type_boolean, 0) == 0)
	{
	  util_do_command ("summary");
	  util_do_command ("headers");
	}
      return 0;
    }
  else
    {
      mu_error (_("%s takes only one argument"), argv[0]);
    }
  return 1;
}


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mail.h"

static char *prev_name;

/* Expand mail special characters:
 * #	    the previous file
 * &	    the current mbox
 * +file    the file named in the folder directory (set folder=foo)
 * Note 1) The followig notations are left intact, since they are
 * handled by mailbox_create_default:
 * %	    system mailbox
 * %user    system mailbox of the user 
 * Note 2) Allocates memory
 */
char *
mail_expand_name (const char *name)
{
  switch (name[0])
    {
    case '#':
      if (!prev_name)
	util_error (_("No previous file"));
      else
	name = xstrdup (prev_name);
      break;
	  
    case '&':
      name = getenv ("MBOX");
      if (!name)
	util_error (_("MBOX environment variable not set"));
      else
	name = xstrdup (name);
      break;
	  
    case '+':
      name = util_folder_path (name);
      break;

    default:
      name = xstrdup (name);
      break;
    }
  return (char*) name;
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
      url_t url;
      mailbox_t newbox = NULL;
      char *name = mail_expand_name (argv[1]);

      if (!name)
	return 1;
      
      if (mailbox_create_default (&newbox, name) != 0 
	  || mailbox_open (newbox, MU_STREAM_RDWR) != 0)
	{
	  mailbox_destroy (&newbox);
	  util_error(_("can't open mailbox %s: %s"), name, mu_strerror (errno));
	  free (name);
	  return 1;
	}

      free (name); /* won't need it any more */

      mailbox_get_url (mbox, &url);
      pname = strdup (url_to_string (url));
      if (mail_mbox_close ())
	{
	  if (pname)
	    free (pname);
	  mailbox_close (newbox);
	  mailbox_destroy (&newbox);
	  return 1;
	}
      
      if (prev_name)
	free (prev_name);
      prev_name = pname;
      
      mbox = newbox;
      mailbox_messages_count (mbox, &total);
      cursor = 1;
      if (util_getenv (NULL, "header", Mail_env_boolean, 0) == 0)
	{
	  util_do_command ("summary");
	  util_do_command ("z.");
	}
      return 0;
    }
  else
    {
      util_error(_("%s takes only one arg"), argv[0]);
    }
  return 1;
}

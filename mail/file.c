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

#include "mail.h"

static char *prev_name;

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
      char *pname;
      url_t url;

      /* switch folders */
      /*
       * special characters:
       * %	system mailbox
       * %user	system mailbox of the user
       * #	the previous file
       * &	the current mbox
       * +file	the file named in the folder directory (set folder=foo)
       */
      mailbox_t newbox = NULL;
      struct mail_env_entry *env;
      char *name;
      
      switch (argv[1][0])
	{
	case '#':
	  if (!prev_name)
	    {
	      util_error("No previous file");
	      return 1;
	    }
	  name = prev_name;
	  break;
	  
	case '&':
	  name = getenv ("MBOX");
	  break;
	  
	case '+':
	  env = util_find_env ("folder");
	  if (env->set && env->value[0] != '/' && env->value[1] != '~')
	    {
	      char *home = mu_get_homedir ();
	      name = alloca (strlen (home) + 1 +
			     strlen (env->value) + 1 +
			     strlen (argv[1] + 1) + 1);
	      if (!name)
		{
		  util_error ("Not enough memory");
		  return 1;
		} 
	      sprintf (name, "%s/%s/%s", home, env->value, argv[1] + 1);
	    }
	  else
	    name = argv[1];
	  break;
	  
	default:
	  name = argv[1];
	}
      
      if (mailbox_create_default (&newbox, name) != 0 
	  || mailbox_open (newbox, MU_STREAM_READ) != 0)
	{
	  mailbox_destroy (&newbox);
	  util_error("can't open mailbox %s: %s",
		     name ? name : "%", strerror(errno));
	  return 1;
	}

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
      cursor = realcursor = 1;
      if ((util_find_env("header"))->set)
	{
	  util_do_command ("summary");
	  util_do_command ("z.");
	}
      return 0;
    }
  else
    {
      util_error("%s takes only one arg", argv[0]);
    }
  return 1;
}

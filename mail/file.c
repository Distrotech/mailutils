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

/*
 * fi[le] [file]
 * fold[er] [file]
 */

int
mail_file (int argc, char **argv)
{
  if (argc == 1)
    {
      url_t url;

      mailbox_get_url (mbox, &url);
      fprintf (ofile, "%s\n", url_to_string (url));
      
      /*FIXME: display current folder info */
    }
  else if (argc == 2)
    {
      /* switch folders */
      /*
       * special characters:
       * %	system mailbox
       * %user	system mailbox of user
       * #	the previous file
       * &	the current mbox
       * +file	the file named in the folder directory (set folder=foo)
       */
      mailbox_t newbox;
      struct mail_env_entry *env;
      char *name;
      
      switch (argv[1][0])
	{
	case '%':
	  if (argv[1][1] == 0)
	    name = NULL; /* our system mailbox */
	  else
	    {
	      util_error("%%user not supported");
	      return 1;
	    }
	  break;
	  
	case '#':
	  util_error("# notation not supported");
	  return 1;
	  
	case '&':
	  name = getenv ("MBOX");
	  break;
	  
	case '+':
	  env = util_find_env ("folder");
	  if (env->set)
	    name = env->value;
	  else
	    name = argv[1];
	  break;
	  
	default:
	  name = argv[1];
	}
      
      if (mailbox_create_default (&newbox, name) != 0)
	return 1;
      if (mailbox_open (newbox, MU_STREAM_READ) != 0)
	return 1;

      if (mail_mbox_close ())
	{
	  mailbox_close (newbox);
	  mailbox_destroy (&newbox); 
	  return 1;
	}
      
      mbox = newbox;
      mailbox_messages_count (mbox, &total);
      cursor = realcursor = 1;
      if ((util_find_env("header"))->set)
	util_do_command ("z.");
      return 0;
    }
  else
    {
      util_error("%s takes only one arg", argv[0]);
    }
  return 1;
}

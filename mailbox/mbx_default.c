/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <paths.h>
#include <errno.h>
#include <stdio.h>

#include <mailutils/mailbox.h>

#ifndef _PATH_MAILDIR
# define _PATH_MAILDIR "/var/spool/mail"
#endif

int
mailbox_create_default (mailbox_t *pmbox, const char *mail)
{
  const char *user = NULL;

  if (pmbox == NULL)
    return EINVAL;

  if (mail == NULL)
    mail = getenv ("MAIL");

  if (mail)
    {
      /* Is it a fullpath ?  */
      if (mail[0] != '/')
	{
	  /* Is it a URL ?  */
	  if (strchr (mail, ':') == NULL)
	    {
	      /* A user name.  */
	      user = mail;
	      mail = NULL;
	    }
	}
    }

  if (mail == NULL)
    {
      int status;
      char *mail0;
      if (user == NULL)
	{
	  user = (getenv ("LOGNAME")) ? getenv ("LOGNAME") : getenv ("USER");
	  if (user == NULL)
	    {
	      fprintf (stderr, "Who am I ?\n");
	      return EINVAL;
	    }
	}
      mail0 = malloc (strlen (user) + strlen (_PATH_MAILDIR) + 2);
      if (mail0 == NULL)
	return ENOMEM;
      sprintf (mail0, "%s/%s", _PATH_MAILDIR, user);
      status = mailbox_create (pmbox, mail0);
      free (mail0);
      return status;
    }
  return  mailbox_create (pmbox, mail);
}

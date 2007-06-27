/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2007 Free Software Foundation, Inc.

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
#include <unistd.h>

/*
 * Must create a new mailbox
 */

/* FIXME: How do we do this ??????:
   IF a new mailbox is created with the same name as a mailbox which was
   deleted, its unique identifiers MUST be greater than any unique identifiers
   used in the previous incarnation of the mailbox.  */
int
imap4d_create (struct imap4d_command *command, char *arg)
{
  char *name;
  char *sp = NULL;
  const char *delim = "/";
  int isdir = 0;
  int rc = RESP_OK;
  const char *msg = "Completed";

  name = util_getword (arg, &sp);
  if (!name)
    return util_finish (command, RESP_BAD, "Too few arguments");

  util_unquote (&name);

  if (*name == '\0')
    return util_finish (command, RESP_BAD, "Too few arguments");

  /* Creating, "Inbox" should always fail.  */
  if (strcasecmp (name, "INBOX") == 0)
    return util_finish (command, RESP_BAD, "Already exist");

  /* RFC 3501:
         If the mailbox name is suffixed with the server's hierarchy
	 separator character, this is a declaration that the client intends
	 to create mailbox names under this name in the hierarchy.

     The trailing delimiter will be removed by namespace normalizer, so
     test for it now.

     The RFC goes on:
         Server implementations that do not require this declaration MUST
	 ignore the declaration.

     That's it! If isdir is set, we do all checks but do not create anything.
  */
  if (name[strlen (name) - 1] == delim[0])
    isdir = 1;
  
  /* Allocates memory.  */
  name = namespace_getfullpath (name, delim);

  if (!name)
    return util_finish (command, RESP_NO, "Can not create mailbox");

  /* It will fail if the mailbox already exists.  */
  if (access (name, F_OK) != 0)
    {
      if (!isdir)
	{
	  char *dir;
	  char *d = name + strlen (delim); /* Pass the root delimeter.  */

	  /* If the server's hierarchy separtor character appears elsewhere in
	     name, the server SHOULD create any superior hierarchcal names
	     that are needed for the CREATE command to complete successfully.
	  */
	  if (chdir (delim) == 0) /* start on the root.  */
	    for (; (dir = strchr (d, delim[0])); d = dir)
	      {
		*dir++ = '\0';
		if (chdir (d) != 0)
		  {
		    if (mkdir (d, 0700) == 0)
		      {
			if (chdir (d) == 0)
			  continue;
			else
			  {
			    rc = RESP_NO;
			    msg = "Can not create mailbox";
			    break;
			  }
		      }
		  }
	      }

	  if (rc == RESP_OK && d && *d != '\0')
	    {
	      int fd = creat (d, 0600);
	      if (fd != -1)
		close (fd);
	      else
		{
		  rc = RESP_NO;
		  msg = "Can not create mailbox";
		}
	    }
	}
    }
  else
    {
      rc = RESP_NO;
      msg = "already exists";
    }
  chdir (homedir);
  free (name);
  return util_finish (command, rc, msg);
}

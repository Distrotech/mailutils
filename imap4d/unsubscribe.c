/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "imap4d.h"

/*
 *
 */

int
imap4d_unsubscribe (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *name;
  char *file;
  FILE *fp;

  name = util_getword (arg, &sp);
  util_unquote (&name);
  if (!name || *name == '\0')
    return util_finish (command, RESP_BAD, "Too few arguments");

  asprintf (&file, "%s/.mailboxlist", homedir);
  fp = fopen (file, "r");
  free (file);
  if (fp)
    {
      char buffer[124];
      int found = 0;
      while (fgets (buffer, sizeof (buffer), fp))
	{
	  size_t n = strlen (buffer);
	  if (n && buffer[n - 1] == '\n')
	    buffer[n - 1] = '\0';
	  if (strcmp (buffer, name) == 0)
	    {
	      found = 1;
	      break;
	    }
	}
      if (found)
	{
	  FILE *fp2;
	  asprintf (&file, "%s/.mailboxlist.%d", homedir, getpid ());
	  fp2 = fopen (file, "a");
	  if (fp2)
	    {
	      rewind (fp);
	      while (fgets (buffer, sizeof (buffer), fp))
		{
		  size_t n = strlen (buffer);
		  if (n && buffer[n - 1] == '\n')
		    buffer[n - 1] = '\0';
		  if (strcmp (buffer, name) == 0)
		    continue;
		  fputs (buffer, fp2);
		  fputs ("\n", fp2);
		}
	      fclose (fp2);
	      remove (file);
	    }
	  free (file);
	}
      else
	{
	  fclose (fp);
	  return util_finish (command, RESP_NO, "Can not unsubscribe");
	}
      fclose (fp);
    }
  else
    return util_finish (command, RESP_NO, "Can not unsubscribe");
  return util_finish (command, RESP_OK, "Completed");
}

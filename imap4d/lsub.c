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
 *
 */

int
imap4d_lsub (struct imap4d_command *command, char *arg)
{
  char *sp;
  char *ref;
  char *wcard;
  char *file = NULL;
  char *pattern = NULL;
  const char *delim = "/";
  FILE *fp;
  
  ref = util_getword (arg, &sp);
  wcard = util_getword (NULL, &sp);
  if (!ref || !wcard)
    return util_finish (command, RESP_BAD, "Too few arguments");

  /* Remove the double quotes.  */
  util_unquote (&ref);
  util_unquote (&wcard);

  asprintf (&pattern, "%s%s", ref, wcard);
  if (!pattern)
    return util_finish (command, RESP_NO, "Not enough memory");
  
  asprintf (&file, "%s/.mailboxlist", homedir);
  if (!file)
    {
      free (pattern);
      return util_finish (command, RESP_NO, "Not enough memory");
    }
  
  fp = fopen (file, "r");
  free (file);
  if (fp)
    {
      char *buf = NULL;
      size_t n = 0;
	
      while (getline(&buf, &n, fp) > 0)
	{
	  int len = strlen (buf);
	  if (buf[len - 1] == '\n')
	    buf[len - 1] = '\0';
	  if (util_wcard_match (buf, pattern, delim) != WCARD_NOMATCH)
	    util_out (RESP_NONE, "LIST () \"%s\" %s", delim, buf);
	}
      fclose (fp);
      free (buf);
      return util_finish (command, RESP_OK, "Completed");
    }
  else if (errno == ENOENT)
    return util_finish (command, RESP_OK, "Completed");
  return util_finish (command, RESP_NO, "Can not list subscriber");
}

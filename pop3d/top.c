/* GNU POP3 - a small, fast, and efficient POP3 daemon
   Copyright (C) 1999 Jakob 'sparky' Kaivo <jkaivo@nodomainname.net>

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

#include "pop3d.h"

/* Prints the header of a message plus a specified number of lines */

int
pop3_top (const char *arg)
{
#ifdef _HAVE_BLACK_MAGIC
  int i = 0, header = 1, done = 0;
  int mesg, lines;
  char *mesgc, *linesc;
  char *buf, buf2[80];

  if (strlen (arg) == 0)
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  mesgc = pop3_cmd (arg);
  linesc = pop3_args (arg);
  mesg = atoi (mesgc) - 1;
  lines = strlen (linesc) > 0 ? atoi (linesc) : -1;
  free (mesgc);
  free (linesc);

  if (lines < 0)
    return ERR_BAD_ARGS;

  if (pop3_mesg_exist (mesg) != OK)
    return ERR_NO_MESG;

  mbox = freopen (mailbox, "r", mbox);
  if (mbox == NULL)
    return ERR_FILE;
  fsetpos (mbox, &(messages[mesg].header));

  fprintf (ofile, "+OK\r\n");
  buf = malloc (sizeof (char) * 80);
  if (buf == NULL)
    pop3_abquit (ERR_NO_MEM);

  while (fgets (buf, 80, mbox) && !done)
    {
      while (strchr (buf, '\n') == NULL)
	{
	  buf = realloc (buf, sizeof (char) * (strlen (buf) + 81));
	  if (buf == NULL)
	    pop3_abquit (ERR_NO_MEM);
	  fgets (buf2, 80, mbox);
	  strncat (buf, buf2, 80);
	}
      if (!strncmp (buf, "From ", 5))
	done = 1;
      else
	{
	  buf[strlen (buf) - 1] = '\0';
	  if (header == 1)
	    {
	      if (buf[0] == '.')
		fprintf (ofile, ".%s\r\n", buf);
	      else
		fprintf (ofile, "%s\r\n", buf);
	      if ((buf[0] == '\r') || (buf[0] == '\n') || (buf[0]) == '\0')
		header = 0;
	    }
	  else
	    {
	      if (++i <= lines)
		{
		  if (buf[0] == '.')
		    fprintf (ofile, ".%s\r\n", buf);
		  else
		    fprintf (ofile, "%s\r\n", buf);
		}
	      else
		done = 1;
	    }
	}
      buf = realloc (buf, sizeof (char) * 80);
      if (buf == NULL)
	pop3_abquit (ERR_NO_MEM);
    }

  free (buf);
  fprintf (ofile, ".\r\n");
  return OK;
#else
  return ERR_NOT_IMPL;
#endif
}


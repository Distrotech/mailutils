/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

extern int ifile;
extern int ofile;

int
util_out (char *seq, int tag, char *f, ...)
{
  char *buf = NULL;
  int len = 0;
  va_list ap;
  va_start (ap, f);
  len = vasprintf (&buf, f, ap);

  if (tag == TAG_SEQ)
    {
      write (ofile, seq, strlen (seq));
      write (ofile, " ", 1);
    }
  else if (tag == TAG_NONE)
    write (ofile, "* ", 2);

  write (ofile, buf, len);
  write (ofile, "\r\n", 2);
  free (buf);

  return 0;
}

int
util_finish (int argc, char **argv, int resp, char *f, ...)
{
  char *buf = NULL, *buf2 = NULL, *code = NULL;
  int len = 0;
  va_list ap;
  va_start (ap, f);

  switch (resp)
    {
    case RESP_OK:
      code = strdup ("OK");
      break;
    case RESP_BAD:
      code = strdup ("BAD");
      break;
    case RESP_NO:
      code = strdup ("NO");
      break;
    default:
      code = strdup ("X-BUG");
    }
  
  vasprintf (&buf, f, ap);
  len = asprintf (&buf2, "%s %s %s %s\r\n", argv[0], code, argv[1], buf);

  write (ofile, buf2, len);

  free (buf);
  free (buf2);
  free (code);

  return resp;
}

char *
util_getline (void)
{
  fd_set rfds;
  struct timeval tv;
  char buf[1024], *ret = NULL;
  int fd = ifile, len = 0;

  FD_ZERO (&rfds);
  FD_SET (fd, &rfds);
  tv.tv_sec = 30;
  tv.tv_usec = 0;
  memset (buf, '\0', 1024);

  do
    {
      if (!select (fd + 1, &rfds, NULL, NULL, &tv))
	return NULL;
      else if (read (fd, buf, 1024) < 1)
	exit (1);		/* FIXME: dead socket */
      else if (ret == NULL)
	{
	  ret = malloc ((strlen (buf + 1) * sizeof (char)));
	  strcpy (ret, buf);
	}
      else
	{
	  ret = realloc (ret, (strlen (ret) + strlen (buf) + 1) *
			 sizeof (char));
	  strcat (ret, buf);
	}
    }
  while (strchr (buf, '\n') == NULL);

  for (len = strlen (ret); len > 0; len--)
    if (ret[len] == '\r' || ret[len] == '\n')
      ret[len] = '\0';

  return ret;
}

int
util_do_command (char *cmd)
{
  if (cmd == NULL)
    return 0;
  else
    {
      int argc = 0, status = 0, i = 0, len;
      char **argv = NULL;
      struct imap4d_command *command = NULL;

      if (argcv_get (cmd, &argc, &argv) != 0)
	{
	  argcv_free (argc, argv);
	  return 1;
	}


      util_start (argv[0]);

      if (argc < 2)
	{
	  util_finish (argc, argv, RESP_BAD,
		       "Client did not send space after tag");
	  argcv_free (argc, argv);
	  return 1;
	}

      len = strlen (argv[1]);
      for (i=0; i < len; i++)
	argv[1][i] = tolower (argv[1][i]);
      
      for (i=0; command == NULL && imap4d_command_table[i].name != 0; i++)
	{
	  if (strlen (imap4d_command_table[i].name) == len &&
	      !strcmp (imap4d_command_table[i].name, argv[1]))
	    command = &imap4d_command_table[i];
	}

      for (i=0; i < len; i++)
	argv[1][i] = toupper (argv[1][i]);

      if (command == NULL)
	{
	  util_finish (argc, argv, RESP_BAD, "Invalid command");
	  argcv_free (argc, argv);
	  return 1;
	}
      else if (! (command->states & util_getstate ()))
	{
	  util_finish (argc, argv, RESP_BAD, "Incorrect state");
	  argcv_free (argc, argv);
	  return 1;
	}
      else
	{
	  status = command->func (argc, argv);
	  argcv_free (argc, argv);
	  return status;
	}
    }
  return 1;
}

int
util_start (char *seq)
{
  return 0;
}

int
util_getstate (void)
{
  return STATE_NONAUTH;
}

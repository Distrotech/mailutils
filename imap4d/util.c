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

static const char *
rc2string (int rc)
{
  switch (rc)
    {
    case RESP_OK:
      return "OK ";

    case RESP_BAD:
      return "BAD ";

    case RESP_NO:
      return "NO ";

    case RESP_BYE:
      return "BYE ";
    }
  return "";
}

/* FIXME:  Some words are:
   between double quotes, consider like one word.
   between parenthesis, consider line one word.  */
char *
util_getword (char *s, char **save)
{
  static char *sp;
  return strtok_r (s, " \r\n", ((save) ? save : &sp));
}

int
util_out (int rc, const char *format, ...)
{
  char *buf = NULL;
  va_list ap;

  va_start (ap, format);
  vasprintf (&buf, format, ap);
  va_end (ap);

  fprintf (ofile, "* %s%s\r\n", rc2string (rc), buf);
  free (buf);
  return 0;
}

int
util_finish (struct imap4d_command *command, int rc, const char *format, ...)
{
  char *buf = NULL;
  const char *resp;
  va_list ap;

  va_start (ap, format);
  vasprintf (&buf, format, ap);
  va_end(ap);

  resp = rc2string (rc);
  fprintf (ofile, "%s %s%s %s\r\n", command->tag, resp, command->name, buf);
  free (buf);
  return 0;
}

char *
imap4d_readline (int fd)
{
  fd_set rfds;
  struct timeval tv;
  char buf[512], *ret = NULL;
  int nread;
  int available;

  FD_ZERO (&rfds);
  FD_SET (fd, &rfds);
  tv.tv_sec = timeout;
  tv.tv_usec = 0;

  do
    {
      if (timeout > 0)
	{
	  available = select (fd + 1, &rfds, NULL, NULL, &tv);
	  if (!available)
	    util_quit (1); /* FIXME: Timeout.  */
	}
      nread = read (fd, buf, sizeof (buf) - 1);
      if (nread < 1)
	util_quit (1);		/* FIXME: dead socket */

      buf[nread] = '\0';

      if (ret == NULL)
	{
	  ret = malloc ((nread + 1) * sizeof (char));
	  strcpy (ret, buf);
	}
      else
	{
	  ret = realloc (ret, (strlen (ret) + nread + 1) * sizeof (char));
	  strcat (ret, buf);
	}
      /* FIXME: handle literal strings here.  */
    }
  while (strchr (buf, '\n') == NULL);

  return ret;
}

int
util_do_command (char *prompt)
{
  char *sp = NULL, *tag, *cmd, *arg;
  struct imap4d_command *command;
  static struct imap4d_command nullcommand;

  tag = util_getword (prompt, &sp);
  cmd = util_getword (NULL, &sp);
  if (!tag)
    {
      nullcommand.name = "";
      nullcommand.tag = (char *)"*";
      return util_finish (&nullcommand, RESP_BAD, "Null command");
    }
  else if (!cmd)
    {
      nullcommand.name = "";
      nullcommand.tag = tag;
      return util_finish (&nullcommand, RESP_BAD, "Missing arguments");
    }

  util_start (tag);

  command = util_getcommand (cmd);
  if (command == NULL)
    {
      nullcommand.name = "";
      nullcommand.tag = tag;
      return util_finish (&nullcommand, RESP_BAD,  "Invalid command");
    }

  command->tag = tag;
  return command->func (command, sp);
}

/* FIXME:  What is this for?  */
int
util_start (char *tag)
{
  (void)tag;
  return 0;
}

/* FIXME: Incomplete send errmsg to syslog, see pop3d:pop3_abquit().  */
void
util_quit (int err)
{
  exit (err);
}

/* FIXME:  What is this for?  */
int
util_getstate (void)
{
  return STATE_NONAUTH;
}

struct imap4d_command *
util_getcommand (char *cmd)
{
  size_t i, len = strlen (cmd);

  for (i = 0; imap4d_command_table[i].name != 0; i++)
    {
      if (strlen (imap4d_command_table[i].name) == len &&
	  !strcasecmp (imap4d_command_table[i].name, cmd))
	return &imap4d_command_table[i];
    }
  return NULL;
}

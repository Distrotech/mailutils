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
#include <ctype.h>

static int add2set (int **set, int *n, unsigned long val, size_t max);
static const char * sc2string (int rc);

/* FIXME:  Some words are:
   between double quotes, between parenthesis.  */
char *
util_getword (char *s, char **save)
{
  return strtok_r (s, " \r\n", save);
}

int
util_token (char *buf, size_t len, char **ptr)
{
  char *start = *ptr;
  size_t i;
  /* Skip leading space.  */
  while (**ptr && **ptr == ' ')
    (*ptr)++;
  for (i = 1; **ptr && i < len; (*ptr)++, buf++, i++)
    {
      if (**ptr == ' ' || **ptr == '.'
          || **ptr == '(' || **ptr == ')'
          || **ptr == '[' || **ptr == ']'
          || **ptr == '<' || **ptr  == '>')
        {
          /* Advance.  */
          if (start == (*ptr))
            (*ptr)++;
          break;
        }
      *buf = **ptr;
  }
  *buf = '\0';
  /* Skip trailing space.  */
  while (**ptr && **ptr == ' ')
    (*ptr)++;
  return  *ptr - start;;
}

/* Return in set an allocated array contain (n) numbers, for imap messsage set

   set ::= sequence_num / (sequence_num ":" sequence_num) / (set "," set)
   sequence_num    ::= nz_number / "*"
   ;; * is the largest number in use.  For message
   ;; sequence numbers, it is the number of messages
   ;; in the mailbox.  For unique identifiers, it is
   ;; the unique identifier of the last message in
   ;; the mailbox.
   nz_number       ::= digit_nz *digit

   FIXME: The algo below is to relaxe, things like <,,,> or <:12> or <20:10>
   will not generate an error.  */
int
util_msgset (char *s, int **set, int *n, int isuid)
{
  unsigned long val = 0;
  unsigned long low = 0;
  int done = 0;
  int status = 0;
  size_t max = 0;

  status = mailbox_messages_count (mbox, &max);
  if (status != 0)
    return status;
  if (isuid)
    {
      message_t msg = NULL;
      mailbox_get_message (mbox, max, &msg);
      message_get_uid (msg, &max);
    }

  *n = 0;
  *set = NULL;
  while (*s)
    {
      switch (*s)
	{
	  /* isdigit */
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  {
	    errno = 0;
	    val = strtoul (s, &s, 10);
	    if (val == ULONG_MAX && errno == ERANGE)
	      {
		if (*set)
		  free (*set);
		*n = 0;
		return EINVAL;
	      }
	    if (low)
	      {
		for (;low && low <= val; low++)
		  {
		    status = add2set (set, n, low, max);
		    if (status != 0)
		      return status;
		  }
		low = 0;
	      }
	    else
	      {
		status = add2set(set, n, val, max);
		if (status != 0)
		  return status;
	      }
	    break;
	  }

	case ':':
	  low = val + 1;
	  s++;
	  break;

	case '*':
	  {
	    if (status != 0)
	      {
		if (*set)
		  free (*set);
		*n = 0;
		return status;
	      }
	    val = max;
	    s++;
	    break;
	  }

	case ',':
	  s++;
	  break;

	default:
	  done = 1;
	  if (*set)
	    free (*set);
	  *n = 0;
	  return EINVAL;

	} /* switch */

      if (done)
	break;
    } /* while */

  if (low)
    {
      for (;low && low <= val; low++)
	{
	  status = add2set (set, n, low, max);
	  if (status != 0)
	    return status;
	}
    }
  return 0;
}

int
util_send (const char *format, ...)
{
  int status;
  va_list ap;
  va_start (ap, format);
  status = vfprintf (ofile, format, ap);
  va_end (ap);
  return status;
}

int
util_out (int rc, const char *format, ...)
{
  char *buf = NULL;
  va_list ap;

  va_start (ap, format);
  vasprintf (&buf, format, ap);
  va_end (ap);

  fprintf (ofile, "* %s%s\r\n", sc2string (rc), buf);
  free (buf);
  return 0;
}

int
util_finish (struct imap4d_command *command, int rc, const char *format, ...)
{
  char *buf = NULL;
  const char *resp;
  int new_state;
  va_list ap;

  va_start (ap, format);
  vasprintf (&buf, format, ap);
  va_end(ap);

  resp = sc2string (rc);
  fprintf (ofile, "%s %s%s %s\r\n", command->tag, resp, command->name, buf);
  free (buf);
  new_state = (rc == RESP_OK) ? command->success : command->failure;
  if (new_state != STATE_NONE)
    state = new_state;
  return 0;
}

char *
imap4d_readline (int fd)
{
  fd_set rfds;
  struct timeval tv;
  char buf[512], *ret = NULL;
  int nread;
  int total = 0;
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
	    util_quit (1); /* FIXME: Timeout, send a "* BYE".  */
	}
      nread = read (fd, buf, sizeof (buf) - 1);
      if (nread < 1)
	util_quit (1);	/* FIXME: dead socket, need to do something?  */

      buf[nread] = '\0';

      ret = realloc (ret, (total + nread + 1) * sizeof (char));
      if (ret == NULL)
	util_quit (1); /* FIXME: ENOMEM, send a "* BYE" to the client.  */
      memcpy (ret + total, buf, nread + 1);
      total += nread;

      /* FIXME: handle literal strings here.  */

    }
  while (memchr (buf, '\n', nread) == NULL);

  for (nread = total; nread > 0; nread--)
    if (ret[nread] == '\r' || ret[nread] == '\n')
      ret[nread] = '\0';
  return ret;
}

int
util_do_command (char *prompt)
{
  char *sp = NULL, *tag, *cmd;
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

  command = util_getcommand (cmd, imap4d_command_table);
  if (command == NULL)
    {
      nullcommand.name = "";
      nullcommand.tag = tag;
      return util_finish (&nullcommand, RESP_BAD,  "Invalid command");
    }

  command->tag = tag;
  return command->func (command, sp);
}

int
util_upper (char *s)
{
  if (!s)
    return 0;
  for (; *s; s++)
    *s = toupper ((unsigned)*s);
  return 0;
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
util_getcommand (char *cmd, struct imap4d_command command_table[])
{
  size_t i, len = strlen (cmd);

  for (i = 0; command_table[i].name != 0; i++)
    {
      if (strlen (command_table[i].name) == len &&
	  !strcasecmp (command_table[i].name, cmd))
	return &command_table[i];
    }
  return NULL;
}

/* Status Code to String.  */
static const char *
sc2string (int rc)
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

static int
add2set (int **set, int *n, unsigned long val, size_t max)
{
  int *tmp;
  if (val == 0 || val > max
      || (tmp = realloc (*set, (*n + 1) * sizeof (**set))) == NULL)
    {
      if (*set)
	free (*set);
      *n = 0;
      return ENOMEM;
    }
  *set = tmp;
  (*set)[*n] = val;
  (*n)++;
  return 0;
}

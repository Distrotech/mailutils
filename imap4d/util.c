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

static int add2set __P ((int **, int *, unsigned long, size_t));
static const char *sc2string __P ((int));

/* Get the next space/CR/NL separated word, some words are between double
   quotes, strtok() can not handle it.  */
char *
util_getword (char *s, char **save)
{
  /* Take care of words between double quotes.  */
  {
    char *p;
    if ((p = s) || (p = *save))
      {
	while (isspace ((unsigned)*p))
	  p++;
	if (*p == '"')
	  {
	    s = p;
	    p++;
	    while (*p && *p != '"')
	      p++;
	    if (*p == '"')
		p++;
	    *save = (*p) ? p + 1 : p;
	    *p = '\0';
	    return s;
	  }
      }
  }
  return strtok_r (s, " \r\n", save);
}

/* Stop a the first char that IMAP4 represent as a special characters.  */
int
util_token (char *buf, size_t len, char **ptr)
{
  char *start = *ptr;
  size_t i;
  /* Skip leading space.  */
  while (**ptr && **ptr == ' ')
    (*ptr)++;

  /* Break the string by token, i.e when we reconize IMAP special
     atoms we stop and send it.  */
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
  return  *ptr - start;
}

/* Remove the surrounding double quotes.  */
void
util_unquote (char **ptr)
{
  char *s = *ptr;
  if (s && *s == '"')
    {
      char *p = ++s;
      while (*p && *p != '"')
        p++;
      if (*p == '"')
        *p = '\0';
    }
  *ptr = s;
}

/* NOTE: Allocates Memory.  */
/* Expand: ~ --> /home/user and to ~guest --> /home/guest.  */
char *
util_tilde_expansion (const char *ref, const char *delim)
{
  char *p = strdup (ref);
  if (*p == '~')
    {
      p++;
      if (*p == delim[0] || *p == '\0')
        {
          char *s = calloc (strlen (homedir) + strlen (p) + 1, 1);
          strcpy (s, homedir);
          strcat (s, p);
          free (--p);
          p = s;
        }
      else
        {
          struct passwd *pw;
          char *s = p;
          char *name;
          while (*s && *s != delim[0])
            s++;
          name = calloc (s - p + 1, 1);
          memcpy (name, p, s - p);
          name [s - p] = '\0';
          pw = getpwnam (name);
          free (name);
          if (pw)
            {
              char *buf = calloc (strlen (pw->pw_dir) + strlen (s) + 1, 1);
              strcpy (buf, pw->pw_dir);
              strcat (buf, s);
              free (--p);
              p = buf;
            }
          else
            p--;
        }
    }
  return p;
}

/* Get the absolute path.  */
/* NOTE: Path is allocated and must be free()d by the caller.  */
char *
util_getfullpath (char *name, const char *delim)
{
  char *p = util_tilde_expansion (name, delim);
  if (*p != delim[0])
    {
      char *s = calloc (strlen (homedir) + strlen (delim) + strlen (p) + 1, 1);
      sprintf (s, "%s%s%s", homedir, delim, p);
      free (p);
      p = s;
    }
  return p;
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
  /* If it is a uid sequence, override max with the UID.  */
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

	  /* A pair of numbers separated by a ':' character indicates a
	     contiguous set of mesages ranging from the first number to the
	     second:
	     3:5  --> 3 4 5
	  */
	case ':':
	  low = val + 1;
	  s++;
	  break;

	  /* As a convenience. '*' is provided to refer to the highest message
	     number int the mailbox:
	     5:*  --> 5 6 7 8
	  */
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

	  /* IMAP also allows a set of noncontiguous numbers to be specified
	     with the ',' character:
	     1,3,5,7  --> 1 3 5 7
	  */
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

/* Use vfprintf for the dirty work.  */
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

/* Send an unsolicited response.  */
int
util_out (int rc, const char *format, ...)
{
  char *buf = NULL;
  int status;
  va_list ap;
  asprintf (&buf, "* %s%s\r\n", sc2string (rc), format);
  va_start (ap, format);
  status = vfprintf (ofile, buf, ap);
  va_end (ap);
  free (buf);
  return status;
}

/* Send the tag response and reset the state.  */
int
util_finish (struct imap4d_command *command, int rc, const char *format, ...)
{
  char *buf = NULL;
  int new_state;
  int status;
  va_list ap;

  asprintf (&buf, "%s %s%s %s\r\n", command->tag, sc2string (rc),
	    command->name, format);

  va_start (ap, format);
  status = vfprintf (ofile, buf, ap);
  va_end(ap);
  free (buf);
  /* Reset the state.  */
  new_state = (rc == RESP_OK) ? command->success : command->failure;
  if (new_state != STATE_NONE)
    state = new_state;
  return status;
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
      if (timeout)
	{
	  available = select (fd + 1, &rfds, NULL, NULL, &tv);
	  if (!available)
	    util_quit (1);
	}
      nread = read (fd, buf, sizeof (buf) - 1);
      if (nread < 1)
	util_quit (1);

      buf[nread] = '\0';

      ret = realloc (ret, (total + nread + 1) * sizeof (char));
      if (ret == NULL)
	util_quit (1);
      memcpy (ret + total, buf, nread + 1);
      total += nread;
    }
  while (memchr (buf, '\n', nread) == NULL);

  /* Nuke CR'\r'  */
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
  if (err)
    util_out (RESP_BYE, "Server terminating");
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

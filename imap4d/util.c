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

static int add2set __P ((size_t **, int *, unsigned long));
static const char *sc2string __P ((int));

/* Get the next space/CR/NL separated word, some words are between double
   quotes, strtok() can not handle it.  */
char *
util_getword (char *s, char **save)
{
  return util_getitem (s, " \r\n", save);
}

/* Take care of words between double quotes.  */
char *
util_getitem (char *s, const char *delim, char **save)
{
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
  return strtok_r (s, delim, save);
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
          || **ptr == '<' || **ptr  == '>'
	  || **ptr == '\r' || **ptr == '\n')
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
/* FIXME:  Check if the quote was escaped and ignore it.  */
void
util_unquote (char **ptr)
{
  char *s = *ptr;
  size_t len;
  if (s && *s == '"' && (len = strlen (s)) > 1 && s[len - 1] == '"')
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
util_msgset (char *s, size_t **set, int *n, int isuid)
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
		/* Reverse it. */
		if (low > val)
		  {
		    long tmp = low;
		    tmp -= 2;
		    if (tmp <= 0 || val == 0)
		      {
			free (*set);
			*n = 0;
			return EINVAL;
		      }
		    low = val;
		    val = tmp;
		  }
		for (;low && low <= val; low++)
		  {
		    status = add2set (set, n, low);
		    if (status != 0)
		      return status;
		  }
		low = 0;
	      }
	    else
	      {
		status = add2set(set, n, val);
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
      /* Reverse it. */
      if (low > val)
	{
	  long tmp = low;
	  tmp -= 2;
	  if (tmp <= 0 || val == 0)
	    {
	      free (set);
	      *n = 0;
	      return EINVAL;
	    }
	  low = val;
	  val = tmp;
	}
      for (;low && low <= val; low++)
	{
	  status = add2set (set, n, low);
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

/* Send NIL if empty string, change the quoted string to a literal if the
   string contains: double quotes, CR, LF, and '/'.  CR, LF will be change
   to spaces.  */
int
util_send_qstring (const char *buffer)
{
  if (buffer == NULL || *buffer == '\0')
    return util_send ("NIL");
  if (strchr (buffer, '"') || strchr (buffer, '\r') || strchr (buffer, '\n')
      || strchr (buffer, '\\'))
    {
      char *s;
      int ret;
      char *b = strdup (buffer);
      while ((s = strchr (b, '\n')) || (s = strchr (b, '\r')))
	*s = ' ';
      ret = util_send_literal (b);
      free (b);
      return ret;
    }
  return util_send ("\"%s\"", buffer);
}

int
util_send_literal (const char *buffer)
{
  return util_send ("{%u}\r\n%s", strlen (buffer), buffer);
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

/* Clients are allowed to send literal string to the servers.  this
   mean that it can me everywhere where a string is allowed.
   A literal is a sequence of zero or more octets (including CR and LF)
   prefix-quoted with an octet count in the form of an open brace ("{"),
   the number of octets, close brace ("}"), and CRLF.
 */
char *
imap4d_readline (FILE *fp)
{
  char buffer[512];
  size_t len;
  long number = 0;
  size_t total = 0;
  char *line = malloc (1);

  if (!line)
    imap4d_bye (ERR_NO_MEM);

  line[0] = '\0'; /* start with a empty string.  */
  do
    {
      alarm (timeout);
      if (fgets (buffer, sizeof (buffer), fp) == NULL)
	{
	  imap4d_bye (ERR_NO_OFILE); /* Logout.  */
	}
      alarm (0);

      len = strlen (buffer);
      /* If we were in a litteral substract. We have to do it here since the CR
	 is part of the count in a literal.  */
      if (number)
        number -= len;

      /* Remove CR.  */
      if (len > 1 && buffer[len - 1] == '\n')
	{
	  if (buffer[len - 2] == '\r')
	    {
	      buffer[len - 2] = '\n';
	      buffer[len - 1] = '\0';
	    }
	}
      line = realloc (line, total + len + 1);
      if (!line)
	imap4d_bye (ERR_NO_MEM);
      strcat (line, buffer);

      total = strlen (line);

      /* I observe some client requesting long FETCH operations since, I did
	 not see any limit in the command length in the RFC, catch things
	 here.  */
      /* Check that we do have a terminated NL line and we are not retrieving
	 a literal. If we don't continue the read.  */
      if (number <= 0 && total && line[total - 1] != '\n')
	continue;

      /* Check if the client try to send a literal and make sure we are not
	 already retrieving a literal.  */
      if (number <= 0 && len > 2)
        {
          size_t n = total - 1; /* C arrays are 0-based.  */
	  /* A literal is this "{number}\n".  The CR is already strip.  */
          if (line[n] == '\n' && line[n - 1] == '}')
            {
	      /* Search for the matching bracket.  */
              while (n && line[n] != '{') n--;
              if (line [n] == '{')
                {
                  char *sp = NULL;
		  /* Truncate where the literal number was.  */
                  line[n] = '\0';
                  number = strtoul (line + n + 1, &sp, 10);
		  /* Client can ask for non synchronise literal,
		   if a '+' is append to the octet count. */
                  if (*sp != '+')
                    util_send ("+ GO AHEAD\r\n");
                }
            }
        }
    }
  while (number > 0);
  /* syslog (LOG_INFO, "readline: %s", line); */
  return line;
}

int
util_do_command (char *prompt)
{
  char *sp = NULL, *tag, *cmd;
  struct imap4d_command *command;
  static struct imap4d_command nullcommand;
  size_t len;

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
  len = strlen (sp);
  if (len  && sp[len - 1] == '\n')
    sp[len - 1] = '\0';
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
add2set (size_t **set, int *n, unsigned long val)
{
  int *tmp;
  tmp = realloc (*set, (*n + 1) * sizeof (**set));
  if (tmp == NULL)
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

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2004, 
   2005, 2006, 2007 Free Software Foundation, Inc.

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

static mu_stream_t istream;
static mu_stream_t ostream;

static int add2set (size_t **, int *, unsigned long);
static const char *sc2string (int);

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
	while (isspace ((unsigned) *p))
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

/* Stop at the first char that represents an IMAP4 special character. */
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
	  || **ptr == '<' || **ptr == '>' || **ptr == '\r' || **ptr == '\n')
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
  return *ptr - start;
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
  return mu_tilde_expansion (ref, delim, homedir);
}

/* Get the absolute path.  */
/* NOTE: Path is allocated and must be free()d by the caller.  */
char *
util_getfullpath (char *name, const char *delim)
{
  char *p = util_tilde_expansion (name, delim);
  if (*p != delim[0])
    {
      char *s =
	calloc (strlen (homedir) + strlen (delim) + strlen (p) + 1, 1);
      sprintf (s, "%s%s%s", homedir, delim, p);
      free (p);
      p = s;
    }
  return mu_normalize_path (p, delim);
}

static int
comp_int (const void *a, const void *b)
{
  return *(int *) a - *(int *) b;
}

/* Parse the message set specification from S. Store message numbers
   in SET, store number of element in the SET into the memory pointed to
   by N.

   A message set is defined as:

   set ::= sequence_num / (sequence_num ":" sequence_num) / (set "," set)
   sequence_num    ::= nz_number / "*"
   ;; * is the largest number in use.  For message
   ;; sequence numbers, it is the number of messages
   ;; in the mailbox.  For unique identifiers, it is
   ;; the unique identifier of the last message in
   ;; the mailbox.
   nz_number       ::= digit_nz *digit

   FIXME: The message sets like <,,,> or <:12> or <20:10> are not considered
   an error */
int
util_msgset (char *s, size_t ** set, int *n, int isuid)
{
  unsigned long val = 0;
  unsigned long low = 0;
  int done = 0;
  int status = 0;
  size_t max = 0;
  size_t *tmp;
  int i, j;
  unsigned long invalid_uid = 0; /* For UID mode only: have we 
				    encountered an uid > max uid? */
  
  status = mu_mailbox_messages_count (mbox, &max);
  if (status != 0)
    return status;
  /* If it is a uid sequence, override max with the UID.  */
  if (isuid)
    {
      mu_message_t msg = NULL;
      mu_mailbox_get_message (mbox, max, &msg);
      mu_message_get_uid (msg, &max);
    }

  *n = 0;
  *set = NULL;
  while (*s)
    {
      switch (*s)
	{
	  /* isdigit */
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
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
	    else if (val > max)
	      {
		if (isuid)
		  {
		    invalid_uid = 1;
		    continue;
		  }
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
		    if (tmp < 0 || val == 0)
		      {
			free (*set);
			*n = 0;
			return EINVAL;
		      }
		    low = val;
		    val = tmp;
		  }
		for (; low && low <= val; low++)
		  {
		    status = add2set (set, n, low);
		    if (status != 0)
		      return status;
		  }
		low = 0;
	      }
	    else
	      {
		status = add2set (set, n, val);
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
	    val = max;
	    s++;
	    status = add2set (set, n, val);
	    if (status != 0)
	      return status;
	  }
	  break;

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

	}			/* switch */

      if (done)
	break;
    }				/* while */

  /* For message sets in form X:Y where Y is a not-existing UID greater
     than max UID, replace Y with the max UID in the mailbox */
  if (*n == 1 && invalid_uid)
    {
      val = max;
      status = add2set (set, n, val);
      if (status != 0)
	return status;
    }
  
  if (low)
    {
      /* Reverse it. */
      if (low > val)
	{
	  long tmp = low;
	  tmp -= 2;
	  if (tmp < 0 || val == 0)
	    {
	      free (*set);
	      *n = 0;
	      return EINVAL;
	    }
	  low = val;
	  val = tmp;
	}
      for (; low && low <= val; low++)
	{
	  status = add2set (set, n, low);
	  if (status != 0)
	    return status;
	}
    }

  /* Sort the resulting message set */
  qsort (*set, *n, sizeof (**set), comp_int);

  /* Remove duplicates. tmp serves to avoid extra dereferences */
  tmp = *set;
  for (i = 0, j = 1; i < *n; i++)
    if (tmp[j - 1] != tmp[i])
      tmp[j++] = tmp[i];
  *n = j;
  return 0;
}

int
util_send (const char *format, ...)
{
  char *buf = NULL;
  int status = 0;
  va_list ap;

  va_start (ap, format);
  vasprintf (&buf, format, ap);
  va_end (ap);
  if (!buf)
      imap4d_bye (ERR_NO_MEM);

  if (daemon_param.transcript)
    syslog (LOG_DEBUG, "sent: %s", buf);

  status = mu_stream_sequential_write (ostream, buf, strlen (buf));
  free (buf);

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
  char *tempbuf = NULL;
  char *buf = NULL;
  int status = 0;
  va_list ap;

  asprintf (&tempbuf, "* %s%s\r\n", sc2string (rc), format);
  va_start (ap, format);
  vasprintf (&buf, tempbuf, ap);
  va_end (ap);
  if (!buf)
    imap4d_bye (ERR_NO_MEM);

  if (daemon_param.transcript)
    syslog (LOG_DEBUG, "sent: %s", buf);

  status = mu_stream_sequential_write (ostream, buf, strlen (buf));
  free (buf);
  free (tempbuf);
  return status;
}

/* Send the tag response and reset the state.  */
int
util_finish (struct imap4d_command *command, int rc, const char *format, ...)
{
  size_t size;
  char *buf = NULL;
  char *tempbuf = NULL;
  int new_state;
  int status = 0;
  va_list ap;
  const char *sc = sc2string (rc);
  
  va_start (ap, format);
  vasprintf (&tempbuf, format, ap);
  va_end (ap);
  if (!tempbuf)
    imap4d_bye (ERR_NO_MEM);
  
  size = strlen (command->tag) + 1 +
         strlen (sc) + strlen (command->name) + 1 +
         strlen (tempbuf) + 1;
  buf = malloc (size);
  if (!buf)
    imap4d_bye (ERR_NO_MEM);
  strcpy (buf, command->tag);
  strcat (buf, " ");
  strcat (buf, sc);
  strcat (buf, command->name);
  strcat (buf, " ");
  strcat (buf, tempbuf);
  free (tempbuf);

  if (daemon_param.transcript)
    syslog (LOG_DEBUG, "sent: %s\r\n", buf);

  mu_stream_sequential_write (ostream, buf, strlen (buf));
  free (buf);
  mu_stream_sequential_write (ostream, "\r\n", 2);

  /* Reset the state.  */
  if (rc == RESP_OK)
    new_state = command->success;
  else if (command->failure <= state)
    new_state = command->failure;
  else
    new_state = STATE_NONE;

  if (new_state != STATE_NONE)
    {
      util_run_events (state, new_state);
      state = new_state;
    }
  
  return status;
}

/* Clients are allowed to send literal string to the servers.  this
   means that it can occur everywhere where a string is allowed.
   A literal is a sequence of zero or more octets (including CR and LF)
   prefix-quoted with an octet count in the form of an open brace ("{"),
   the number of octets, close brace ("}"), and CRLF.
 */
char *
imap4d_readline (void)
{
  char buffer[512];
  size_t len;
  long number = 0;
  size_t total = 0;
  char *line = malloc (1);

  if (!line)
    imap4d_bye (ERR_NO_MEM);

  line[0] = '\0';		/* start with a empty string.  */
  do
    {
      size_t sz;
      int rc;
      
      alarm (daemon_param.timeout);
      rc = mu_stream_sequential_readline (istream, buffer, sizeof (buffer), &sz);
      if (sz == 0)
	{
	  syslog (LOG_INFO, _("Unexpected eof on input"));
	  imap4d_bye (ERR_NO_OFILE);
	}
      else if (rc)
	{
	  const char *p;
	  if (mu_stream_strerror (istream, &p))
	    p = strerror (errno);

	  syslog (LOG_INFO, _("Error reading from input file: %s"), p);
	  imap4d_bye (ERR_NO_OFILE);
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
	  size_t n = total - 1;	/* C arrays are 0-based.  */
	  /* A literal is this "{number}\n".  The CR is already strip.  */
	  if (line[n] == '\n' && line[n - 1] == '}')
	    {
	      /* Search for the matching bracket.  */
	      while (n && line[n] != '{')
		n--;
	      if (line[n] == '{')
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
  while (number > 0 || (total && line[total - 1] != '\n'));
  if (daemon_param.transcript)
    syslog (LOG_DEBUG, "recv: %s", line);
  return line;
}

char *
imap4d_readline_ex (void)
{
  int len;
  char *s = imap4d_readline ();

  if (s && (len = strlen (s)) > 0 && s[len - 1] == '\n')
    s[len - 1] = 0;
  return s;
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
      nullcommand.tag = (char *) "*";
      return util_finish (&nullcommand, RESP_BAD, "Null command");
    }
  else if (!cmd)
    {
      nullcommand.name = "";
      nullcommand.tag = tag;
      return util_finish (&nullcommand, RESP_BAD, "Missing arguments");
    }

  command = util_getcommand (cmd, imap4d_command_table);
  if (command == NULL)
    {
      nullcommand.name = "";
      nullcommand.tag = tag;
      return util_finish (&nullcommand, RESP_BAD, "Invalid command");
    }

  command->tag = tag;

  if (command->states && (command->states & state) == 0)
    return util_finish (command, RESP_BAD, "Wrong state");

  len = strlen (sp);
  if (len && sp[len - 1] == '\n')
    sp[len - 1] = '\0';
  return command->func (command, sp);
}

int
util_upper (char *s)
{
  if (!s)
    return 0;
  for (; *s; s++)
    *s = toupper ((unsigned) *s);
  return 0;
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
add2set (size_t ** set, int *n, unsigned long val)
{
  size_t *tmp;
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

int
util_parse_internal_date0 (char *date, time_t * timep, char **endp)
{
  struct tm tm;
  mu_timezone tz;
  time_t time;
  char **datep = &date;

  if (mu_parse_imap_date_time ((const char **) datep, &tm, &tz))
    return 1;

  time = mu_tm2time (&tm, &tz);
  if (time == (time_t) - 1)
    return 2;

  *timep = time;
  if (endp)
    *endp = *datep;
  return 0;
}

int
util_parse_internal_date (char *date, time_t * timep)
{
  return util_parse_internal_date0 (date, timep, NULL);
}


int
util_parse_822_date (const char *date, time_t * timep)
{
  struct tm tm;
  mu_timezone tz;
  const char *p = date;

  if (mu_parse822_date_time (&p, date + strlen (date), &tm, &tz) == 0)
    {
      *timep = mu_tm2time (&tm, &tz);
      return 0;
    }
  return 1;
}

int
util_parse_ctime_date (const char *date, time_t * timep)
{
  struct tm tm;
  mu_timezone tz;

  if (mu_parse_ctime_date_time (&date, &tm, &tz) == 0)
    {
      *timep = mu_tm2time (&tm, &tz);
      return 0;
    }
  return 1;
}

/* Return the first ocurrence of NEEDLE in HAYSTACK. Case insensitive
   comparison */
char *
util_strcasestr (const char *haystack, const char *needle)
{
  return mu_strcasestr (haystack, needle);
}

struct
{
  char *name;
  int flag;
}
_imap4d_attrlist[] =
{
  { "\\Answered", MU_ATTRIBUTE_ANSWERED },
  { "\\Flagged", MU_ATTRIBUTE_FLAGGED },
  { "\\Deleted", MU_ATTRIBUTE_DELETED },
  { "\\Draft", MU_ATTRIBUTE_DRAFT },
  { "\\Seen", MU_ATTRIBUTE_READ },
  { "\\Recent", MU_ATTRIBUTE_RECENT },
};

#define NATTR sizeof(_imap4d_attrlist)/sizeof(_imap4d_attrlist[0])

int _imap4d_nattr = NATTR;

int
util_attribute_to_type (const char *item, int *type)
{
  int i;
  for (i = 0; i < _imap4d_nattr; i++)
    if (strcasecmp (item, _imap4d_attrlist[i].name) == 0)
      {
	*type = _imap4d_attrlist[i].flag;
	return 0;
      }
  return 1;
}

/* Note: currently unused. Not needed, possibly? */
int
util_type_to_attribute (int type, char **attr_str)
{
  char *attr_list[NATTR];
  int nattr = 0;
  int i;
  size_t len = 0;

  if (MU_ATTRIBUTE_IS_UNSEEN (type))
    *attr_str = strdup ("\\Recent");
  else
    *attr_str = NULL;

  for (i = 0; i < _imap4d_nattr; i++)
    if (type & _imap4d_attrlist[i].flag)
      {
	attr_list[nattr++] = _imap4d_attrlist[i].name;
	len += 1 + strlen (_imap4d_attrlist[i].name);
      }

  *attr_str = malloc (len + 1);
  (*attr_str)[0] = 0;
  if (*attr_str)
    {
      for (i = 0; i < nattr; i++)
	{
	  strcat (*attr_str, attr_list[i]);
	  if (i != nattr - 1)
	    strcat (*attr_str, " ");
	}
    }

  if (!*attr_str)
    imap4d_bye (ERR_NO_MEM);
  return 0;
}

void
util_print_flags (mu_attribute_t attr)
{
  int i;
  int flags = 0;
  int space = 0;

  mu_attribute_get_flags (attr, &flags);
  for (i = 0; i < _imap4d_nattr; i++)
    if (flags & _imap4d_attrlist[i].flag)
      {
	if (space)
	  util_send (" ");
	else
	  space = 1;
	util_send (_imap4d_attrlist[i].name);
      }

  if (MU_ATTRIBUTE_IS_UNSEEN (flags))
    {
      if (space)
	util_send (" ");
      util_send ("\\Recent");
    }
}

int
util_attribute_matches_flag (mu_attribute_t attr, const char *item)
{
  int flags = 0, mask = 0;

  mu_attribute_get_flags (attr, &flags);
  util_attribute_to_type (item, &mask);
  if (mask == MU_ATTRIBUTE_RECENT)
    return MU_ATTRIBUTE_IS_UNSEEN (flags);

  return flags & mask;
}


int
util_parse_attributes (char *items, char **save, int *flags)
{
  int rc = 0;

  *flags = 0;
  while (*items)
    {
      int type = 0;
      char item[64] = "";

      util_token (item, sizeof (item), &items);
      if (!util_attribute_to_type (item, &type))
	*flags |= type;
      /*FIXME: else? */

      if (*items == ')')
	{
	  items++;
	  rc = 0;
	  break;
	}
    }

  *save = items;
  return rc;
}

int
util_base64_encode (const unsigned char *input, size_t input_len,
		    unsigned char **output, size_t * output_len)
{
  static char b64tab[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t olen = 4 * (input_len + 2) / 3;
  unsigned char *out = malloc (olen);

  if (!out)
    return 1;
  *output = out;
  while (input_len >= 3)
    {
      *out++ = b64tab[input[0] >> 2];
      *out++ = b64tab[((input[0] << 4) & 0x30) | (input[1] >> 4)];
      *out++ = b64tab[((input[1] << 2) & 0x3c) | (input[2] >> 6)];
      *out++ = b64tab[input[2] & 0x3f];
      olen -= 4;
      input_len -= 3;
      input += 3;
    }

  if (input_len > 0)
    {
      unsigned char c = (input[0] << 4) & 0x30;
      *out++ = b64tab[input[0] >> 2];
      if (input_len > 1)
	c |= input[1] >> 4;
      *out++ = b64tab[c];
      *out++ = (input_len < 2) ? '=' : b64tab[(input[1] << 2) & 0x3c];
      *out++ = '=';
    }
  *output_len = out - *output;
  return 0;
}

int
util_base64_decode (const unsigned char *input, size_t input_len,
		    unsigned char **output, size_t * output_len)
{
  static int b64val[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
  };
  int olen = input_len;
  unsigned char *out = malloc (olen);

  if (!out)
    return 1;
  *output = out;
  do
    {
      if (input[0] > 127 || b64val[input[0]] == -1
	  || input[1] > 127 || b64val[input[1]] == -1
	  || input[2] > 127 || ((input[2] != '=') && (b64val[input[2]] == -1))
	  || input[3] > 127 || ((input[3] != '=')
				&& (b64val[input[3]] == -1)))
	return -1;
      *out++ = (b64val[input[0]] << 2) | (b64val[input[1]] >> 4);
      if (input[2] != '=')
	{
	  *out++ = ((b64val[input[1]] << 4) & 0xf0) | (b64val[input[2]] >> 2);
	  if (input[3] != '=')
	    *out++ = ((b64val[input[2]] << 6) & 0xc0) | b64val[input[3]];
	}
      input += 4;
      input_len -= 4;
    }
  while (input_len > 0);
  *output_len = out - *output;
  return 0;
}

char *
util_localname ()
{
  static char *localname;

  if (!localname)
    {
      char *name;
      int name_len = 256;
      int status = 1;
      struct hostent *hp;

      name = malloc (name_len);
      while (name
	     && (status = gethostname (name, name_len)) == 0
	     && !memchr (name, 0, name_len))
	{
	  name_len *= 2;
	  name = realloc (name, name_len);
	}
      if (status)
	{
	  syslog (LOG_CRIT, _("Cannot find out my own hostname"));
	  exit (1);
	}

      hp = gethostbyname (name);
      if (hp)
	{
	  struct in_addr inaddr;
	  inaddr.s_addr = *(unsigned int *) hp->h_addr;
	  hp = gethostbyaddr ((const char *) &inaddr,
			      sizeof (struct in_addr), AF_INET);
	  if (hp)
	    {
	      free (name);
	      name = strdup ((char *) hp->h_name);
	    }
	}
      localname = name;
    }
  return localname;
}

/* Match STRING against the IMAP4 wildard pattern PATTERN */

int
util_wcard_match (const char *string, const char *pattern, const char *delim)
{
  const char *p = pattern, *n = string;
  char c;

  for (; (c = *p++) != '\0' && *n; n++)
    {
      switch (c)
	{
	case '%':
	  if (*p == '\0')
	    {
	      /* Matches everything except '/' */
	      for (; *n && *n != delim[0]; n++)
		;
	      return (*n == delim[0]) ? WCARD_RECURSE_MATCH : WCARD_MATCH;
	    }
	  else
	    for (; *n != '\0'; ++n)
	      if (util_wcard_match (n, p, delim) == WCARD_MATCH)
		return WCARD_MATCH;
	  break;

	case '*':
	  if (*p == '\0')
	    return WCARD_RECURSE_MATCH;
	  for (; *n != '\0'; ++n)
	    {
	      int status = util_wcard_match (n, p, delim);
	      if (status == WCARD_MATCH || status == WCARD_RECURSE_MATCH)
		return status;
	    }
	  break;

	default:
	  if (c != *n)
	    return WCARD_NOMATCH;
	}
    }

  if (!c && !*n)
    return WCARD_MATCH;

  return WCARD_NOMATCH;
}

/* Return the uindvalidity of a mailbox.
   When a mailbox is selected, whose first message does not keep X-UIDVALIDITY
   value, the uidvalidity is computed basing on the return of time(). Now,
   if we call "EXAMINE mailbox" or "STATUS mailbox (UIDVALIDITY)" the same
   mailbox is opened second time and the uidvalidity recalculated. Thus each
   subsequent call to EXAMINE or STATUS upon an already selected mailbox
   will return a different uidvalidity value. To avoid this, util_uidvalidity()
   first sees if it is asked to operate upon an already opened mailbox
   and if so, returns the previously computed value. */
int
util_uidvalidity (mu_mailbox_t smbox, unsigned long *uidvp)
{
  mu_url_t mbox_url = NULL;
  mu_url_t smbox_url = NULL;

  mu_mailbox_get_url (mbox, &mbox_url);
  mu_mailbox_get_url (smbox, &smbox_url);
  if (strcmp (mu_url_to_string (mbox_url), mu_url_to_string (smbox_url)) == 0)
    smbox = mbox;
  return mu_mailbox_uidvalidity (smbox, uidvp);
}


void
util_setio (FILE *in, FILE *out)
{
  if (!out || !in)
    imap4d_bye (ERR_NO_OFILE);

  setvbuf (in, NULL, _IOLBF, 0);
  setvbuf (out, NULL, _IOLBF, 0);
  if (mu_stdio_stream_create (&istream, in, MU_STREAM_NO_CLOSE)
      || mu_stdio_stream_create (&ostream, out, MU_STREAM_NO_CLOSE))
    imap4d_bye (ERR_NO_OFILE);
}

void
util_get_input (mu_stream_t *pstr)
{
  *pstr = istream;
}

void
util_get_output (mu_stream_t *pstr)
{
  *pstr = ostream;
}

void
util_set_input (mu_stream_t str)
{
  istream = str;
}

void
util_set_output (mu_stream_t str)
{
  ostream = str;
}

/* Wait TIMEOUT seconds for data on the input stream.
   Returns 0   if no data available
           1   if some data is available
	   -1  an error occurred */
int
util_wait_input (int timeout)
{
  int wflags = MU_STREAM_READY_RD;
  struct timeval tv;
  int status;
  
  tv.tv_sec = timeout;
  tv.tv_usec = 0;
  status = mu_stream_wait (istream, &wflags, &tv);
  if (status)
    {
      syslog (LOG_ERR, _("Cannot poll input stream: %s"),
	      mu_strerror(status));
      return -1;
    }
  return wflags & MU_STREAM_READY_RD;
}

void
util_flush_output ()
{
  mu_stream_flush (ostream);
}

int
util_is_master ()
{
  return ostream == NULL;
}

#ifdef WITH_TLS
int
imap4d_init_tls_server ()
{
  mu_stream_t stream;
  int rc;
 
  rc = mu_tls_stream_create (&stream, istream, ostream, 0);
  if (rc)
    return 0;

  if (mu_stream_open (stream))
    {
      const char *p;
      mu_stream_strerror (stream, &p);
      syslog (LOG_ERR, _("Cannot open TLS stream: %s"), p);
      return 0;
    }

  istream = ostream = stream;
  return 1;
}
#endif /* WITH_TLS */

static mu_list_t atexit_list;

void
util_atexit (void (*fp) (void))
{
  if (!atexit_list)
    mu_list_create (&atexit_list);
  mu_list_append (atexit_list, (void*)fp);
}

static int
atexit_run (void *item, void *data)
{
  ((void (*) (void)) item) ();
  return 0;
}

void
util_bye ()
{
  int rc = istream != ostream;
  
  mu_stream_close (istream);
  mu_stream_destroy (&istream, mu_stream_get_owner (istream));

  if (rc)
    {
      mu_stream_close (ostream);
      mu_stream_destroy (&ostream, mu_stream_get_owner (ostream));
    }
      
  mu_list_do (atexit_list, atexit_run, 0);
}

struct state_event {
  int old_state;
  int new_state;
  mu_list_action_t *action;
  void *data;
};

static mu_list_t event_list;

void
util_register_event (int old_state, int new_state,
		     mu_list_action_t *action, void *data)
{
  struct state_event *evp = malloc (sizeof (*evp));
  if (!evp)
    imap4d_bye (ERR_NO_MEM);
  evp->old_state = old_state;
  evp->new_state = new_state;
  evp->action = action;
  evp->data = data;
  if (!event_list)
    mu_list_create (&event_list);
  mu_list_append (event_list, (void*)evp);
}

void
util_event_remove (void *id)
{
  mu_list_remove (event_list, id);
}

static int
event_exec (void *item, void *data)
{
  struct state_event *ev = data, *elem = item;

  if (ev->old_state == elem->old_state && ev->new_state == elem->new_state)
    return elem->action (item, elem->data);
  return 0;
}

void
util_run_events (int old_state, int new_state)
{
  if (event_list)
    {
      struct state_event ev;
      mu_iterator_t itr;
      ev.old_state = old_state;
      ev.new_state = new_state;

      mu_list_get_iterator (event_list, &itr);
      for (mu_iterator_first (itr);
	   !mu_iterator_is_done (itr); mu_iterator_next (itr))
	{
	  struct state_event *p;
	  mu_iterator_current (itr, (void **)&p);
	  if (event_exec (p, &ev))
	    break;
	}
      mu_iterator_destroy (&itr);
    }
}
  
void
util_chdir (const char *homedir)
{
  int rc = chdir (homedir);
  if (rc)
    mu_error ("Cannot change to home directory `%s': %s",
	      homedir, mu_strerror (errno));
}

int
is_atom (const char *s)
{
  if (strpbrk (s, "(){ \t%*\"\\"))
    return 0;
  for (; *s; s++)
    {
      if (*(const unsigned char *)s > 127 || iscntrl (*s))
	return 0;
    }
  return 1;
}
     

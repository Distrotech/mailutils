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

/* This will suck, too.
   Alain: Yes it does.  */

/*  Taken from RFC2060
    fetch           ::= "FETCH" SPACE set SPACE ("ALL" / "FULL" /
    "FAST" / fetch_att / "(" 1#fetch_att ")")

    fetch_att       ::= "ENVELOPE" / "FLAGS" / "INTERNALDATE" /
    "RFC822" [".HEADER" / ".SIZE" / ".TEXT"] /
    "BODY" ["STRUCTURE"] / "UID" /
    "BODY" [".PEEK"] section
    ["<" number "." nz_number ">"]
*/

static int fetch_all           __P ((struct imap4d_command *, char*));
static int fetch_full          __P ((struct imap4d_command *, char*));
static int fetch_fast          __P ((struct imap4d_command *, char*));
static int fetch_envelope      __P ((struct imap4d_command *, char*));
static int fetch_flags         __P ((struct imap4d_command *, char*));
static int fetch_internaldate  __P ((struct imap4d_command *, char*));
static int fetch_rfc822_header __P ((struct imap4d_command *, char*));
static int fetch_rfc822_size   __P ((struct imap4d_command *, char*));
static int fetch_rfc822_text   __P ((struct imap4d_command *, char*));
static int fetch_rfc822        __P ((struct imap4d_command *, char*));
static int fetch_bodystructure __P ((struct imap4d_command *, char*));
static int fetch_body_peek     __P ((struct imap4d_command *, char*));
static int fetch_body          __P ((struct imap4d_command *, char*));
static int fetch_uid           __P ((struct imap4d_command *, char*));

static int fetch_operation     __P ((size_t, char *, int));
static int fetch_message       __P ((message_t, unsigned long, unsigned long));
static int fetch_header        __P ((message_t, unsigned long, unsigned long));
static int fetch_content       __P ((message_t, unsigned long, unsigned long));
static int fetch_io            __P ((stream_t, unsigned long, unsigned long));
static int fetch_header_fields __P ((message_t, char *, unsigned long,
				     unsigned long));
static int fetch_header_fields_not __P ((message_t, char *, unsigned long,
					 unsigned long));
static int fetch_send_address  __P ((char *));

struct imap4d_command fetch_command_table [] =
{
#define F_ALL 0
  {"ALL", fetch_all, 0, 0, 0, NULL},
#define F_FULL 1
  {"FULL", fetch_full, 0, 0, 0, NULL},
#define F_FAST 2
  {"FAST", fetch_fast, 0, 0, 0, NULL},
#define F_ENVELOPE 3
  {"ENVELOPE", fetch_envelope, 0, 0, 0, NULL},
#define F_FLAGS 4
  {"FLAGS", fetch_flags, 0, 0, 0, NULL},
#define F_INTERNALDATE 5
  {"INTERNALDATE", fetch_internaldate, 0, 0, 0, NULL},
#define F_RFC822_HEADER 6
  {"RFC822.HEADER", fetch_rfc822_header, 0, 0, 0, NULL},
#define F_RFC822_SIZE 7
  {"RFC822.SIZE", fetch_rfc822_size, 0, 0, 0, NULL},
#define F_RFC822_TEXT 8
  {"RFC822.TEXT", fetch_rfc822_text, 0, 0, 0, NULL},
#define F_RFC822 9
  {"RFC822", fetch_rfc822, 0, 0, 0, NULL},
#define F_BODYSTRUCTURE 10
  {"BODYSTRUCTURE", fetch_bodystructure, 0, 0, 0, NULL},
#define F_BODY_PEEK 11
  {"BODY.PEEK", fetch_body_peek, 0, 0, 0, NULL},
#define F_BODY 12
  {"BODY", fetch_body, 0, 0, 0, NULL},
#define F_UID 13
  {"UID", fetch_uid, 0, 0, 0, NULL},
  { NULL, 0, 0, 0, 0, NULL}
};

/* NOTE: the state field in the command structure is use as a place
   holder for the message number. This save us from redifining another
   data structure.  */
int
imap4d_fetch (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *msgset;
  int *set = NULL;
  int i, n = 0;
  int rc = RESP_OK;
  int status;
  const char *errmsg = "Completed";
  struct imap4d_command *fcmd;

  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");

  msgset = util_getword (arg, &sp);
  if (!msgset || !sp || *sp == '\0')
    return util_finish (command, RESP_BAD, "Too few args");

  /* Get the message numbers in set[].  */
  status = util_msgset (msgset, &set, &n, 0);
  if (status != 0)
    return util_finish (command, RESP_BAD, "Bogus number set");

  for (i = 0; i < n; i++)
    {
      char item[32];
      char *items = strdup (sp);
      char *p = items;
      util_send ("* FETCH %d (", set[i]);
      item[0] = '\0';
      /* Get the fetch command names.  */
      while (*items && *items != ')')
	{
	  util_token (item, sizeof (item), &items);
	  /* Search in the table.  */
	  fcmd = util_getcommand (item, fetch_command_table);
	  if (fcmd)
	    {
	      /* We use the states field to hold the msgno/uid.  */
	      fcmd->states = set[i];
	      fcmd->func (fcmd, items);
	      util_send (" ");
	    }
	}
      free (p);
      util_send (")\r\n");
    }
  free (set);
  return util_finish (command, rc, errmsg);
}

/* Combination of (FAST ENVELOPE).  */
static int
fetch_all (struct imap4d_command *command, char *arg)
{
  struct imap4d_command c_env = fetch_command_table[F_ENVELOPE];
  fetch_fast (command, arg);
  util_send (" ");
  c_env.states = command->states;
  fetch_envelope (&c_env, arg);
  return 0;
}

/* Combination of (ALL BODY).  */
static int
fetch_full (struct imap4d_command *command, char *arg)
{
  struct imap4d_command c_body = fetch_command_table[F_BODY];
  fetch_all (command, arg);
  util_send (" ");
  c_body.states = command->states;
  fetch_body (&c_body, arg);
  return 0;
}

/* Combination of (FLAGS INTERNALDATE RFC822.SIZE).  */
static int
fetch_fast (struct imap4d_command *command, char *arg)
{
  struct imap4d_command c_idate = fetch_command_table[F_INTERNALDATE];
  struct imap4d_command c_rfc = fetch_command_table[F_RFC822_SIZE];
  struct imap4d_command c_flags = fetch_command_table[F_FLAGS];
  c_flags.states = command->states;
  fetch_flags (&c_flags, arg);
  util_send (" ");
  c_idate.states = command->states;
  fetch_internaldate (&c_idate, arg);
  util_send (" ");
  c_rfc.states = command->states;
  fetch_rfc822_size (&c_rfc, arg);
  return 0;
}

/* Header: Date, Subject, From, Sender, Reply-To, To, Cc, Bcc, In-Reply-To,
   and Message-Id.  */
/* FIXME: - strings change to literals when detecting '"'  */
static int
fetch_envelope (struct imap4d_command *command, char *arg)
{
  char *buffer;
  char *from;
  header_t header = NULL;
  message_t msg = NULL;

  mailbox_get_message (mbox, command->states, &msg);
  message_get_header (msg, &header);
  util_send ("%s(", command->name);

  /* FIXME: Incorrect Date.  */
  header_aget_value (header, "Date", &buffer);
  if (*buffer == '\0')
    util_send ("NIL");
  else
    util_send ("\"%s\"", buffer);
  free (buffer);
  util_send (" ");

  /* Subject.  */
  header_aget_value (header, "Subject", &buffer);
  if (*buffer == '\0')
    util_send ("NIL");
  else
    util_send ("\"%s\"", buffer);
  free (buffer);
  util_send (" ");

  /* From.  */
  header_aget_value (header, "From", &from);
  fetch_send_address (from);
  util_send (" ");

  /* Note that the server MUST default the reply-to and sender fields from
     the From field; a client is not expected to know to do this. */
  header_aget_value (header, "Sender", &buffer);
  fetch_send_address ((*buffer == '\0') ? from : buffer);
  free (buffer);
  util_send (" ");

  header_aget_value (header, "Reply-to", &buffer);
  fetch_send_address ((*buffer == '\0') ? from : buffer);
  free (buffer);
  util_send (" ");

  header_aget_value (header, "To", &buffer);
  fetch_send_address (buffer);
  free (buffer);
  util_send (" ");

  header_aget_value (header, "Cc", &buffer);
  fetch_send_address (buffer);
  free (buffer);
  util_send (" ");

  header_aget_value (header, "Bcc", &buffer);
  fetch_send_address (buffer);
  free (buffer);
  util_send (" ");

  header_aget_value (header, "In-Reply-To", &buffer);
  fetch_send_address (buffer);
  free (buffer);
  util_send (" ");

  header_aget_value (header, "Message-ID", &buffer);
  if (*buffer == '\0')
    util_send ("NIL");
  else
    util_send ("\"%s\"", buffer);

  free (buffer);
  free (from);
  util_send (")");
  return 0;
}

/* The flags that are set for this message.  */
/* FIXME: User flags not done.  */
static int
fetch_flags (struct imap4d_command *command, char *arg)
{
  attribute_t attr = NULL;
  message_t msg = NULL;
  mailbox_get_message (mbox, command->states, &msg);
  message_get_attribute (msg, &attr);
  util_send ("%s (", command->name);
  if (attribute_is_deleted (attr))
    util_send (" \\Deleted");
  if (attribute_is_answered (attr))
    util_send (" \\Answered");
  if (attribute_is_flagged (attr))
    util_send (" \\Flagged");
  if (attribute_is_seen (attr))
    util_send (" \\Seen");
  if (attribute_is_draft (attr))
    util_send (" \\Draft");
  util_send (" )");
  return 0;
}

/* The internal date of the message.  */
/* FIXME: Wrong format?  */
static int
fetch_internaldate (struct imap4d_command *command, char *arg)
{
  char date[512];
  envelope_t env = NULL;
  message_t msg = NULL;
  mailbox_get_message (mbox, command->states, &msg);
  message_get_envelope (msg, &env);
  date[0] = '\0';
  envelope_date (env, date, sizeof (date), NULL);
  util_send ("%s", command->name);
  if (date[strlen (date) - 1] == '\n')
    date[strlen (date) - 1] = '\0';
  util_send (" \"%s\"", date);
  return 0;
}

/* Equivalent to BODY.PEEK[HEADER].  */
static int
fetch_rfc822_header (struct imap4d_command *command, char *arg)
{
  char buffer[16];
  (void)arg;
  util_send ("%s ", command->name);
  strcpy (buffer, "[HEADER]");
  fetch_operation (command->states, buffer, 1);
  return 0;
}

/* Equivalent to BODY[TEXT].  */
/* FIXME: send a Fetch flag if the mail was not set seen ?  */
static int
fetch_rfc822_text (struct imap4d_command *command, char *arg)
{
  char buffer[16];
  attribute_t attr = NULL;
  message_t msg = NULL;
  mailbox_get_message (mbox, command->states, &msg);
  message_get_attribute (msg, &attr);
  attribute_set_read (attr);
  util_send ("%s ", command->name);
  strcpy (buffer, "[TEXT]");
  fetch_operation (command->states, buffer, 1);
  return 0;
}

/* The [RFC-822] size of the message.  */
static int
fetch_rfc822_size (struct imap4d_command *command, char *arg)
{
  size_t size = 0;
  size_t lines = 0;
  message_t msg = NULL;
  (void)arg;
  mailbox_get_message (mbox, command->states, &msg);
  message_size (msg, &size);
  message_lines (msg, &lines);
  util_send ("%s %u", command->name, size + lines);
  return 0;
}

/* Equivalent to BODY[].  */
/* FIXME: send a Fetch flag if the mail was not set seen ?  */
static int
fetch_rfc822 (struct imap4d_command *command, char *arg)
{
  if (*arg == '.')
    {
      if (strncasecmp (arg, ".SIZE", 5) == 0)
	{
	  struct imap4d_command c_rfc= fetch_command_table[F_RFC822_SIZE];
	  c_rfc.states = command->states;
	  fetch_rfc822_size (&c_rfc, arg);
	}
      else if (strncasecmp (arg, ".TEXT", 5) == 0)
	{
	  struct imap4d_command c_rfc = fetch_command_table[F_RFC822_TEXT];
	  c_rfc.states = command->states;
	  fetch_rfc822_text (&c_rfc, arg);
	}
      else if (strncasecmp (arg, ".HEADER", 7) == 0)
	{
	  struct imap4d_command c_rfc = fetch_command_table[F_RFC822_HEADER];
	  c_rfc.states = command->states;
	  fetch_rfc822_header (&c_rfc, arg);
	}
    }
  else
    {
      char buffer[16];
      attribute_t attr = NULL;
      message_t msg = NULL;
      mailbox_get_message (mbox, command->states, &msg);
      message_get_attribute (msg, &attr);
      attribute_set_read (attr);
      util_send ("%s ", command->name);
      strcpy (buffer, "[]");
      fetch_operation (command->states, buffer, 1);
    }
  return 0;
}

/* The unique identifier for the message.  */
static int
fetch_uid (struct imap4d_command *command, char *arg)
{
  size_t uid = 0;
  message_t msg = NULL;
  mailbox_get_message (mbox, command->states, &msg);
  message_get_uid (msg, &uid);
  util_send ("%s %d", command->name, uid);
  return 0;
}

/* FIXME: not implemeted.  */
static int
fetch_bodystructure (struct imap4d_command *command, char *arg)
{
  util_send ("%s ()", command->name);
  return 0;
}

/* An alternate form of BODY that does not implicitly set the \Seen flag.  */
/* FIXME: send notificaton if seen attribute is set?  */
static int
fetch_body (struct imap4d_command *command, char *arg)
{
  struct imap4d_command c_body_p = fetch_command_table[F_BODY_PEEK];
  c_body_p.states = command->states;
  /* It's BODY set the message as seen  */
  if (*arg == '[')
    {
      message_t msg = NULL;
      attribute_t attr = NULL;
      mailbox_get_message (mbox, command->states, &msg);
      message_get_attribute (msg, &attr);
      attribute_set_seen (attr);
    }
  else if (strncasecmp (arg,".PEEK", 5) == 0)
    {
      /* Move pass the .peek  */
      arg += strlen (".PEEK");
      while (isspace ((unsigned)*arg))
	arg++;
    }
  else if (*arg != '[' && *arg != '.')
    {
      struct imap4d_command c_bs = fetch_command_table[F_BODYSTRUCTURE];
      c_bs.states = command->states;
      /* FIXME: Call body structure without the extension.  */
      /* return fetch_bodystructure (&c_bs, arg); */
      util_send (" ()");
      return 0;
    }
  return fetch_body_peek (&c_body_p, arg);
}

static int
fetch_body_peek (struct imap4d_command *command, char *arg)
{
  util_send ("%s ", command->name);
  fetch_operation (command->states, arg, 0);
  return 0;
}

static int
fetch_operation (size_t msgno, char *arg, int silent)
{
  unsigned long start = ULONG_MAX;
  unsigned long end = ULONG_MAX; /* No limit. */
  message_t msg = NULL;
  char *partial = strchr (arg, '<');

  /* Check for section specific offset.  */
  if (partial)
    {
      /* FIXME: This should be move in imap4d_fetch() and have a more
	 draconian check.  */
      *partial = '\0';
      partial++;
      start = strtoul (partial, &partial, 10);
      if (*partial == '.')
	{
	  partial++;
	  end = strtoul (partial, NULL, 10);
	}
    }

  mailbox_get_message (mbox, msgno, &msg);

  /* Retreive the sub message.  */
  for (arg++; isdigit ((unsigned)*arg) || *arg == '.'; arg++)
    {
      unsigned long j = strtoul (arg, &arg, 10);
      int status;
      /* Technicaly I should check errno too.  */
      if (j == 0 || j == ULONG_MAX)
	break;
      status = message_get_part (msg, j, &msg);
      if (status != 0)
	{
	  util_send ("\"\"");
	  return 0;
	}
    }

  /* Choose the right fetch attribute.  */
  if (*arg == ']')
    {
      if (!silent)
	util_send ("[]");
      fetch_message (msg, start, end);
    }
  else if (strncasecmp (arg, "HEADER]", 7) == 0
	   || strncasecmp (arg, "MIME]", 5) == 0)
    {
      if (!silent)
	{
	  util_upper (arg);
	  util_send ("[%s", arg);
	}
      fetch_header (msg, start, end);
    }
  else if (strncasecmp (arg, "TEXT]", 5) == 0)
    {
      if (!silent)
	util_send ("[TEXT]");
      return fetch_content (msg, start, end);
    }
  else if (strncasecmp (arg, "HEADER.FIELDS.NOT", 17) == 0)
    {
      arg += 17;
      fetch_header_fields_not (msg, arg, start, end);
    }
  else if (strncasecmp (arg, "HEADER.FIELDS", 13) == 0)
    {
      arg += 13;
      fetch_header_fields (msg, arg, start, end);
    }
  else
    util_send ("\"\"");
  return 0;
}

static int
fetch_message (message_t msg, unsigned long start, unsigned long end)
{
  stream_t stream = NULL;
  size_t size = 0, lines = 0;
  message_get_stream (msg, &stream);
  message_size (msg, &size);
  message_lines (msg, &lines);
  if ((size + lines) < end || end == ULONG_MAX)
    end = size + lines;
  return fetch_io (stream, start, end);
}

static int
fetch_header (message_t msg, unsigned long start, unsigned long end)
{
  header_t header = NULL;
  stream_t stream = NULL;
  size_t size = 0, lines = 0;
  message_get_header (msg, &header);
  header_size (header, &size);
  header_lines (header, &lines);
  if ((size + lines) < end || end == ULONG_MAX)
    end = size + lines;
  header_get_stream (header, &stream);
  return fetch_io (stream, start, end);
}

static int
fetch_content (message_t msg, unsigned long start, unsigned long end)
{
  body_t body = NULL;
  stream_t stream = NULL;
  size_t size = 0, lines = 0;
  message_get_body (msg, &body);
  body_size (body, &size);
  body_lines (body, &lines);
  if ((size + lines) < end || end == ULONG_MAX)
    end = size + lines;
  body_get_stream (body, &stream);
  return fetch_io (stream, start, end);
}

static int
fetch_io (stream_t stream, unsigned long start, unsigned long end)
{
  stream_t rfc = NULL;
  char buffer[1024];
  size_t n = 0;
  size_t total = 0;

  filter_create (&rfc, stream, "rfc822", MU_FILTER_ENCODE, MU_STREAM_READ);
  if (start == ULONG_MAX)
    {
      start = 0;
      util_send ("{%u}\r\n", end);
    }
  else
      util_send ("<%lu> {%u}\r\n", start , end);

  while (start < end &&
	 stream_read (rfc, buffer, sizeof (buffer), start, &n) == 0
	 && n > 0)
    {
      start += n;
      total += n;
      if (total > end)
	{
	  size_t diff = n - (total - end);
	  buffer[diff] = '\0';
	}
      util_send ("%s", buffer);
    }
  return 0;
}

static int
fetch_header_fields (message_t msg, char *arg, unsigned long start,
		     unsigned long end)
{
  char *buffer = NULL;
  char **array = NULL;
  size_t array_len = 0;
  size_t off = 0;
  size_t lines = 0;
  stream_t stream = NULL;
  int status;

  status = memory_stream_create (&stream);
  if (status != 0)
    util_quit (1); /* FIXME: ENOMEM, send a "* BYE" to the client. */

  /* Save the fields in an array.  */
  {
    char *field;
    char *sp = NULL;
    for (;(field = strtok_r (arg, " ()]\r\n", &sp));
	 arg = NULL, array_len++)
      {
	array = realloc (array, (array_len + 1) * sizeof (*array));
	if (!array)
	  util_quit (1); /* FIXME: ENOMEM, send a "* BYE" to the client. */
	array[array_len] = field;
      }
  }

  /* Get the header values.  */
  {
    size_t j;
    header_t header = NULL;
    message_get_header (msg, &header);
    for (j = 0; j < array_len; j++)
      {
	char *value;
	size_t n = 0;
	header_aget_value (header, array[j], &value);
	if (*value == '\0')
	  continue;
	n = asprintf (&buffer, "%s: %s\n", array[j], value);
	status = stream_write (stream, buffer, n, off, &n);
	off += n;
	lines++;
	free (value);
	free (buffer);
	buffer = NULL;
	if (status != 0)
	  {
	    free (array);
	    util_quit (1); /* FIXME: send a "* BYE" to the client.  */
	  }
      }
  }
  /* Headers are always sent with the NL separator.  */
  stream_write (stream, "\n", 1, off, NULL);
  off++;
  lines++;

  /* Send the command back.  */
  util_send ("[HEADER.FIELDS");
  {
    size_t j;
    for (j = 0; j < array_len; j++)
      {
	util_upper (array[j]);
	util_send (" \"%s\"", array[j]);
      }
    util_send ("]");
  }

  if ((off + lines) < end || end == ULONG_MAX)
    end = off + lines;
  fetch_io (stream, start, end);
  if (array)
    free (array);
  return 0;
}

static int
fetch_header_fields_not (message_t msg, char *arg, unsigned long start,
			 unsigned long end)
{
  char **array = NULL;
  size_t array_len = 0;
  char *buffer = NULL;
  size_t off = 0;
  size_t lines = 0;
  stream_t stream = NULL;
  int status;

  status = memory_stream_create (&stream);
  if (status)
    util_quit (1); /* FIXME: ENOMEM, send a "* BYE" to the client. */

  /* Save the field we want to ignore.  */
  {
    char *field;
    char *sp = NULL;
    for (;(field = strtok_r (arg, " ()]\r\n", &sp));
	 arg = NULL, array_len++)
      {
	array = realloc (array, (array_len + 1) * sizeof (*array));
	if (!array)
	  util_quit (1); /* FIXME: ENOMEM, send a "* BYE" to the client. */
	array[array_len] = field;
      }
  }

  /* Build the memory buffer.  */
  {
    size_t i;
    header_t header = NULL;
    size_t count = 0;
    message_get_header (msg, &header);
    header_get_field_count (header, &count);
    for (i = 1; i <= count; i++)
      {
	char *name;
	char *value ;
	size_t n = 0;
	size_t ignore = 0;

	/* Get the field name.  */
	status = header_aget_field_name (header, i, &name);
	if (status != 0 || *name == '\0')
	  continue;

	/* Should we ignore the field?  */
	{
	  size_t j;
	  for (j = 0; j < array_len; j++)
	    {
	      if (strcasecmp (array[j], name) == 0)
		{
		  ignore = 1;
		  break;
		}
	    }
	  if (ignore)
	    {
	      free (name);
	      continue;
	    }
	}

	header_aget_field_value (header, i, &value);
	/* Save the field.  */
	n = asprintf (&buffer, "%s: %s\n", name, value);
	status = stream_write (stream, buffer, n, off, &n);
        off += n;
	lines++;
        free (value);
        free (name);
        free (buffer);
        buffer = NULL;
        if (status != 0)
	  {
	    free (array);
	    util_quit (1); /* FIXME: send a "* BYE" to the client.  */
	  }
      }
  }
  /* Headers are always sent with a NL separator.  */
  stream_write (stream, "\n", 1, off, NULL);
  off++;
  lines++;

  util_send ("[HEADER.FIELDS.NOT");
  {
    size_t j;
    for (j = 0; j < array_len; j++)
      {
	util_upper (array[j]);
	util_send (" \"%s\"", array[j]);
      }
    util_send ("]");
  }

  if ((off + lines) < end || end == ULONG_MAX)
    end = off + lines;
  fetch_io (stream, start, end);
  if (array)
    free (array);
  return 0;
}

/* FIXME: The address is limit by a buffer of 128, no good.  We should
   allocate the buffer.  */
static int
fetch_send_address (char *addr)
{
  address_t address;
  size_t i, count = 0;

  if (*addr == '\0')
    {
      util_send ("NIL");
      return 0;
    }

  address_create (&address, addr);
  address_get_count (address, &count);

  util_send ("(", count);
  for (i = 1; i <= count; i++)
    {
      char buf[128];

      util_send ("(");

      *buf = '\0';
      address_get_personal (address, i, buf, sizeof (buf), NULL);
      if (*buf == '\0')
	util_send ("NIL");
      else
	util_send ("\"%s\"", buf);

      util_send (" ");

      *buf = '\0';
      address_get_route (address, i, buf, sizeof (buf), NULL);
      if (*buf == '\0')
	util_send ("NIL");
      else
	util_send ("\"%s\"", buf);

      util_send (" ");

      *buf = '\0';
      {
	int is_group = 0;

	address_is_group(address, i, &is_group);
	if(is_group)
	  address_get_personal (address, i, buf, sizeof (buf), NULL);
	else
	  address_get_local_part (address, i, buf, sizeof (buf), NULL);
      }
      if (*buf == '\0')
	util_send ("NIL");
      else
	util_send ("\"%s\"", buf);

      util_send (" ");

      *buf = '\0';
      address_get_domain (address, i, buf, sizeof (buf), NULL);
      if (*buf == '\0')
	util_send ("NIL");
      else
	util_send ("\"%s\"", buf);

      util_send (")");
    }
  util_send (")");
  return 0;
}

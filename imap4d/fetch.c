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

static int fetch_all (struct imap4d_command *, char*);
static int fetch_full (struct imap4d_command *, char*);
static int fetch_fast (struct imap4d_command *, char*);
static int fetch_envelope (struct imap4d_command *, char*);
static int fetch_flags (struct imap4d_command *, char*);
static int fetch_internaldate (struct imap4d_command *, char*);
static int fetch_rfc822_header (struct imap4d_command *, char*);
static int fetch_rfc822_size (struct imap4d_command *, char*);
static int fetch_rfc822_text (struct imap4d_command *, char*);
static int fetch_rfc822 (struct imap4d_command *, char*);
static int fetch_bodystructure (struct imap4d_command *, char*);
static int fetch_body_peek (struct imap4d_command *, char*);
static int fetch_body (struct imap4d_command *, char*);
static int fetch_uid (struct imap4d_command *, char*);

static int fetch_send_address (char *addr);
static int fetch_operation (size_t, char *, int);

struct imap4d_command fetch_command_table [] =
{
#define F_ALL 0
  {"ALL", fetch_all},
#define F_FULL 1
  {"FULL", fetch_full},
#define F_FAST 2
  {"FAST", fetch_fast},
#define F_ENVELOPE 3
  {"ENVELOPE", fetch_envelope},
#define F_FLAGS 4
  {"FLAGS", fetch_flags},
#define F_INTERNALDATE 5
  {"INTERNALDATE", fetch_internaldate},
#define F_RFC822_HEADER 6
  {"RFC822.HEADER", fetch_rfc822_header},
#define F_RFC822_SIZE 7
  {"RFC822.SIZE", fetch_rfc822_size},
#define F_RFC822_TEXT 8
  {"RFC822.TEXT", fetch_rfc822_text},
#define F_RFC822 9
  {"RFC822", fetch_rfc822},
#define F_BODYSTRUCTURE 10
  {"BODYSTRUCTURE", fetch_bodystructure},
#define F_BODY_PEEK 11
  {"BODY.PEEK", fetch_body_peek},
#define F_BODY 12
  {"BODY", fetch_body},
#define F_UID 13
  {"UID", fetch_uid},
  { 0, 0},
};

/* NOTE: the states field in the command structure is use to as a place
   holder for the message number.  */
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
  char item[32];

  msgset = util_getword (arg, &sp);
  if (!msgset)
    return util_finish (command, RESP_BAD, "Too few args");

  /* Get the command name.  */
  item[0] = '\0';
  util_token (item, sizeof (item), &sp);

  /* Search in the table.  */
  fcmd = util_getcommand (item, fetch_command_table);
  if (!fcmd)
    return util_finish (command, RESP_BAD, "Command unknown");

  /* Get the message numbers in set[].  */
  status = util_msgset (msgset, &set, &n, 0);
  if (status != 0)
    return util_finish (command, RESP_BAD, "Bogus number set");

  for (i = 0; i < n; i++)
    {
      /* We use the states field to hold the msgno/uid.  */
      fcmd->states = set[i];
      util_send ("* FETCH %d (", set[i]);
      fcmd->func (fcmd, sp);
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
      char *at;
      size_t len = 0;

      util_send ("(");

      *buf = '\0';
      address_get_personal (address, 1, buf, sizeof (buf), NULL);
      if (*buf == '\0')
	util_send ("NIL ");
      else
	util_send ("\"%s\" ", buf);

      /* FIXME: Source route.  */
      util_send ("NIL ", buf);

      *buf = '\0';
      address_get_email (address, 1, buf, sizeof (buf), &len);
      if (*buf == '\0')
	strcpy (buf, "NIL");
      at = memchr (buf, '@', len);
      if (at)
	{
	  *at = '\0';
	  at++;
	  util_send ("\"%s\" \"%s\"", buf, at);
	}
      else
	util_send ("\"%s\" NIL", buf);

      util_send (")");
    }
  util_send (")");
  return 0;
}
/* Header: Date, Subject, From, Sender, Reply-To, To, Cc, Bcc, In-Reply-To,
   and Message-Id.  */
/* FIXME, FIXME:
   - incorrect DATE
   - address not the correct format
   - strings change to literals when detecting '"'
*/
static int
fetch_envelope (struct imap4d_command *command, char *arg)
{
  char buffer[512];
  char from[128];
  header_t header = NULL;
  message_t msg = NULL;

  mailbox_get_message (mbox, command->states, &msg);
  message_get_header (msg, &header);
  util_send ("%s(", command->name);

  /* FIXME: Incorrect Date.  */
  *buffer = '\0';
  header_get_value (header, "Date", buffer, sizeof (buffer), NULL);
  if (*buffer == '\0')
    util_send ("NIL");
  else
    util_send ("\"%s\"", buffer);
  util_send (" ");

  /* Subject.  */
  *buffer = '\0';
  header_get_value (header, "Subject", buffer, sizeof (buffer), NULL);
  if (*buffer == '\0')
    util_send ("NIL");
  else
    util_send ("\"%s\"", buffer);
  util_send (" ");

  /* From.  */
  *from = '\0';
  header_get_value (header, "From", from, sizeof (from), NULL);
  fetch_send_address (from);
  util_send (" ");

  /* Note that the server MUST default the reply-to and sender fields from
     the from field; a client is not expected to know to do this. */
  *buffer = '\0';
  header_get_value (header, "Sender", buffer, sizeof (buffer), NULL);
  fetch_send_address ((*buffer == '\0') ? from : buffer);
  util_send (" ");

  *buffer = '\0';
  header_get_value (header, "Reply-to", buffer, sizeof (buffer), NULL);
  fetch_send_address ((*buffer == '\0') ? from : buffer);
  util_send (" ");

  *buffer = '\0';
  header_get_value (header, "To", buffer, sizeof (buffer), NULL);
  fetch_send_address (buffer);
  util_send (" ");

  *buffer = '\0';
  header_get_value (header, "Cc", buffer, sizeof (buffer), NULL);
  fetch_send_address (buffer);
  util_send (" ");

  *buffer = '\0';
  header_get_value (header, "Bcc", buffer, sizeof (buffer), NULL);
  fetch_send_address (buffer);
  util_send (" ");

  *buffer = '\0';
  header_get_value (header, "In-Reply-To", buffer, sizeof (buffer), NULL);
  fetch_send_address (buffer);
  util_send (" ");

  *buffer = '\0';
  header_get_value (header, "Message-ID", buffer, sizeof (buffer), NULL);
  if (*buffer == '\0')
    util_send ("NIL");
  else
    util_send ("\"%s\"", buffer);

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
  if (attribute_is_read (attr))
    util_send (" \\Read");
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
      if (strcasecmp (arg, ".SIZE") == 0)
	{
	  struct imap4d_command c_rfc= fetch_command_table[F_RFC822_SIZE];
	  c_rfc.states = command->states;
	  fetch_rfc822_size (&c_rfc, arg);
	}
      else if (strcasecmp (arg, ".TEXT") == 0)
	{
	  struct imap4d_command c_rfc = fetch_command_table[F_RFC822_TEXT];
	  c_rfc.states = command->states;
	  fetch_rfc822_text (&c_rfc, arg);
	}
      else if (strcasecmp (arg, ".HEADER") == 0)
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
static int
fetch_body_peek (struct imap4d_command *command, char *arg)
{
  struct imap4d_command c_body = fetch_command_table[F_BODY];
  c_body.states = command->states;
  return fetch_body (&c_body, arg);
}

/* FIXME: send notificaton if seen attribute is set?  */
/* FIXME: partial offset not done.  */
/* FIXME: HEADER.FIELDS not done.  */
/* FIXME: HEADER.FIELDS.NOT not done.  */
static int
fetch_body (struct imap4d_command *command, char *arg)
{
  struct imap4d_command c_body_p = fetch_command_table[F_BODY_PEEK];

  util_send ("%s ", command->name);

  if (strncasecmp (arg, ".PEEK", 5) == 0)
    {
      arg += strlen (".PEEK");
    }
  else if (*arg == '[')
    {
      attribute_t attr = NULL;
      message_t msg = NULL;
      mailbox_get_message (mbox, command->states, &msg);
      message_get_attribute (msg, &attr);
      attribute_set_seen (attr);
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

  fetch_operation (command->states, arg, 0);
  return 0;
}

/* FIXME: "\n" -->"\r\n" is not handle right still it will not be as messy.  */
/* FIXME:  Code bloat move some stuff in routines, to be reuse.  */
/* FIXME: partial fetch <x.y> still not handle.  */
static int
fetch_operation (size_t msgno, char *arg, int silent)
{
  off_t offset = 0;
  size_t limit = -1; /* No limit. */
  message_t msg = NULL;
  char *partial = strchr (arg, '<');

  /* Check for section specific offset.  */
  if (partial)
    {
      /* FIXME: This shoud be move in imap4d_fetch() and have more
	 draconian check.  */
      *partial = '\0';
      partial++;
      offset = strtoul (partial, &partial, 10);
      if (*partial == '.')
	{
	  partial++;
	  limit = strtoul (partial, NULL, 10);
	}
      partial = alloca (64);
      snprintf (partial, 64, "<%lu>", (unsigned long)offset);
    }
  else
    partial = (char *)"";

  mailbox_get_message (mbox, msgno, &msg);

  if (strncasecmp (arg, "[]", 2) == 0)
    {
      stream_t stream = NULL;
      char buffer[512];
      size_t size = 0, lines = 0, n = 0;
      off_t off = 0;
      message_get_stream (msg, &stream);
      message_size (msg, &size);
      message_size (msg, &lines);
      if (!silent)
	util_send ("%s", arg);
      util_send ("%s {%u}\r\n", partial, size + lines);
      while (stream_readline (stream, buffer, sizeof (buffer), off, &n) == 0
	     && n > 0)
	{
	  /* Nuke the trainline newline.  */
	  if (buffer[n - 1] == '\n')
	    buffer[n - 1] = '\0';
	  util_send ("%s\r\n", buffer);
	  off += n;
	}
    }
  else if (strncasecmp (arg, "[HEADER]", 8) == 0
	   || strncasecmp (arg, "[MIME]", 6) == 0)
    {
      header_t header = NULL;
      stream_t stream = NULL;
      char buffer[512];
      size_t size = 0, lines = 0, n = 0;
      off_t off = 0;
      message_get_header (msg, &header);
      header_size (header, &size);
      header_lines (header, &lines);
      if (!silent)
	{
	  util_upper (arg);
	  util_send ("%s", arg);
	}
      util_send ("%s {%u}\r\n", partial, size + lines);
      header_get_stream (header, &stream);
      while (stream_readline (stream, buffer, sizeof (buffer), off, &n) == 0
	     && n > 0)
	{
	  /* Nuke the trainline newline.  */
	  if (buffer[n - 1] == '\n')
	    buffer[n - 1] = '\0';
	  util_send ("%s\r\n", buffer);
	  off += n;
	}
    }
  else if (strncasecmp (arg, "[HEADER.FIELDS.NOT", 18) == 0)
    {
      char **array = NULL;
      size_t array_len = 0;
      char *buffer = NULL;
      size_t total = 0;

      arg += 18;

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

      /* Build the buffer.  */
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
	    size_t ototal;
	    size_t name_len = 0;
	    size_t value_len = 0;
	    size_t ignore = 0;

	    /* Get the field name.  */
	    header_get_field_name (header, i, NULL, 0, &name_len);
	    if (name_len == 0)
	      continue;
	    name = calloc (name_len + 1, sizeof (*name));
	    if (!name)
	      util_quit (1); /* FIXME: ENOMEM, send a "* BYE" to the client. */
	    header_get_field_name (header, i, name, name_len + 1, NULL);

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

	    /* Fet the field value.  */
	    header_get_field_value (header, i, NULL, 0, &value_len);
	    value = calloc (value_len + 1, sizeof (*value));
	    if (!value)
	      util_quit (1); /* FIXME: ENOMEM, send a "* BYE" to the client. */
	    header_get_field_value (header, i, value, value_len + 1, NULL);

	    /* Save the field.  */
	    ototal = name_len + value_len + 4; /* 4 for ": \r\n"  */
	    buffer = realloc (buffer, (ototal + total + 1) * sizeof (*buffer));
	    if (!buffer)
	      util_quit (1); /* FIXME: ENOMEM, send a "* BYE" to the client. */
	    snprintf (buffer + total, ototal + 1, "%s: %s\r\n", name, value);
	    free (value);
	    free (name);
	    total += ototal;
	  }
      }

      util_send ("[HEADER.FIELDS.NOT");
      {
	size_t j;
	for (j = 0; j < array_len; j++)
	  {
	    util_upper (array[j]);
	    util_send (" \"%s\"", array[j]);
	  }
      }
      util_send ("]%s {%u}\r\n", partial, total + 2);
      util_send ("%s\r\n", (buffer) ? buffer : "");
      if (buffer)
	free (buffer);
      if (array)
	free (array);
    }
  else if (strncasecmp (arg, "[HEADER.FIELDS", 14) == 0)
    {
      char *buffer = NULL;
      char **array = NULL;
      size_t array_len = 0;
      size_t total = 0;

      arg += 14;

      /* Save the field.  */
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

      {
	size_t j;
	header_t header = NULL;

	message_get_header (msg, &header);

	for (j = 0; j < array_len; j++)
	  {
	    size_t ototal;
	    char *value;
	    size_t val_len = 0;

	    header_get_value (header, array[j], NULL, 0, &val_len);
	    if (val_len == 0)
	      continue;
	    value = calloc (val_len + 1, sizeof (*value));
	    if (!value)
	      util_quit (1); /* FIXME: send a "* BYE" to the client.  */
	    header_get_value (header, array[j], value, val_len + 1, NULL);

	    ototal = strlen (array[j]) + val_len + 4; /* 4 for ": \r\n"  */
	    buffer = realloc (buffer, (ototal + total + 1) * sizeof (*buffer));
	    if (!buffer)
	      util_quit (1); /* FIXME: send a "* BYE" to the client.  */
	    snprintf (buffer + total, ototal + 1, "%s: %s\r\n",
		      array[j], value);
	    free (value);
	    total += ototal;
	  }
      }

      util_send ("[HEADER.FIELDS");
      {
	size_t j;
	for (j = 0; j < array_len; j++)
	  {
	    util_upper (array[j]);
	    util_send (" \"%s\"", array[j]);
	  }
      }
      util_send ("]%s {%u}\r\n", partial, total + 2);
      if (total)
	util_send ("%s", buffer);
      util_send ("\r\n");
      if (buffer)
	free (buffer);
      if (array)
	free (array);
    }
  else if (strncasecmp (arg, "[TEXT]", 6) == 0)
    {
      body_t body = NULL;
      stream_t stream = NULL;
      char buffer[512];
      size_t size = 0, lines = 0, n = 0;
      off_t off = 0;
      message_get_body (msg, &body);
      body_size (body, &size);
      body_lines (body, &lines);
      if (!silent)
	util_send ("[TEXT]");
      util_send ("%s {%u}\r\n", partial, size + lines);
      body_get_stream (body, &stream);
      while (stream_readline (stream, buffer, sizeof (buffer), off, &n) == 0
	     && n > 0)
	{
	  /* Nuke the trainline newline.  */
	  if (buffer[n - 1] == '\n')
	    buffer[n - 1] = '\0';
	  util_send ("%s\r\n", buffer);
	  off += n;
	}
    }
  /* else util_send (" Not supported"); */
  return 0;
}

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
   Alain: Yest it does.  */

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
#define F_BODY_PEEK 12
  {"BODY.PEEK", fetch_body_peek},
#define F_BODY 13
  {"BODY", fetch_body},
#define F_UID 14
  {"UID", fetch_uid},
  { 0, 0},
};

int
imap4d_fetch (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *msgset;
  int *set = NULL;
  char *item;
  int i, n = 0;
  int rc = RESP_OK;
  int status;
  const char *errmsg = "Completed";
  struct imap4d_command *fcmd;

  msgset = util_getword (arg, &sp);
  if (!msgset)
    return util_finish (command, RESP_BAD, "Too few args");

  item = strchr (sp, '[');
  if (item)
    {
      char *s = alloca (item - sp + 1);
      memcpy (s, sp, item - sp);
      s[item - sp] = '\0';
      arg = item;
      item = s;
    }
  else
      arg = item = sp;

  fcmd = util_getcommand (item, fetch_command_table);
  if (!fcmd)
    return util_finish (command, RESP_BAD, "Command unknown");

  status = util_msgset (msgset, &set, &n, 0);
  if (status != 0)
    return util_finish (command, RESP_BAD, "Bogus number set");

  /* We use the states to hold the msgno/uid, the success to say if we're
     The first.  */
  for (i = 0; i < n; i++)
    {
      fcmd->states = set[i];
      util_send ("* FETCH %d (%s", set[i], command->name);
      fcmd->func (fcmd, sp);
      util_send (")\r\n");
    }
  free (set);
  return util_finish (command, rc, errmsg);
}

/* --------------- Fetch commands definition  ----- */

static int
fetch_all (struct imap4d_command *command, char *arg)
{
  struct imap4d_command c_env = fetch_command_table[F_ENVELOPE];
  fetch_fast (command, arg);
  c_env.states = command->states;
  fetch_envelope (&c_env, arg);
  return 0;
}

static int
fetch_full (struct imap4d_command *command, char *arg)
{
  struct imap4d_command c_body = fetch_command_table[F_BODY];
  fetch_all (command, arg);
  c_body.states = command->states;
  fetch_body (&c_body, arg);
  return 0;
}

static int
fetch_fast (struct imap4d_command *command, char *arg)
{
  struct imap4d_command c_idate = fetch_command_table[F_INTERNALDATE];
  struct imap4d_command c_rfc = fetch_command_table[F_RFC822_SIZE];
  struct imap4d_command c_flags = fetch_command_table[F_FLAGS];
  c_flags.states = command->states;
  fetch_flags (&c_flags, arg);
  c_idate.states = command->states;
  fetch_internaldate (&c_idate, arg);
  c_rfc.states = command->states;
  fetch_rfc822_size (&c_rfc, arg);
  return 0;
}

/* FIXME, FIXME:
   - incorrect DATE
   - address not the correct format
   - strings change to literals when detecting '"'
*/
static int
fetch_envelope (struct imap4d_command *command, char *arg)
{
  char buffer[512];
  header_t header = NULL;
  message_t msg = NULL;
  int status;
  mailbox_get_message (mbox, command->states, &msg);
  message_get_header (msg, &header);
  util_send (" %s", command->name);
  status = header_get_value (header, "Date", buffer, sizeof (buffer), NULL);
  util_send (" \"%s\"", buffer);
  status = header_get_value (header, "Subject", buffer, sizeof (buffer), NULL);
  util_send (" \"%s\"", buffer);
  status = header_get_value (header, "From", buffer, sizeof (buffer), NULL);
  util_send (" ((NIL NIL NIL \"%s\"))", buffer);
  status = header_get_value (header, "Sender", buffer, sizeof (buffer), NULL);
  util_send (" ((NIL NIL NIL \"%s\"))", buffer);
  status = header_get_value (header, "Reply-to", buffer, sizeof (buffer), NULL);
  util_send (" ((NIL NIL NIL \"%s\"))", buffer);
  status = header_get_value (header, "To", buffer, sizeof (buffer), NULL);
  util_send (" ((NIL NIL NIL \"%s\"))", buffer);
  status = header_get_value (header, "Cc", buffer, sizeof (buffer), NULL);
  util_send (" ((NIL NIL NIL \"%s\"))", buffer);
  status = header_get_value (header, "Bcc", buffer, sizeof (buffer), NULL);
  util_send (" ((NIL NIL NIL \"%s\"))", buffer);
  status = header_get_value (header, "In-Reply-To", buffer, sizeof (buffer), NULL);
  util_send (" \"%s\"", buffer);
  status = header_get_value (header, "Message-ID", buffer, sizeof (buffer), NULL);
  util_send (" \"%s\"", buffer);
  return 0;
}

static int
fetch_flags (struct imap4d_command *command, char *arg)
{
  attribute_t attr = NULL;
  message_t msg = NULL;
  mailbox_get_message (mbox, command->states, &msg);
  message_get_attribute (msg, &attr);
  util_send (" %s", command->name);
  util_send (" (");
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
  util_send (" %s", command->name);
  if (date[strlen (date) - 1] == '\n')
    date[strlen (date) - 1] = '\0';
  util_send (" \"%s\"", date);
  return 0;
}

static int
fetch_rfc822_header (struct imap4d_command *command, char *arg)
{
  char buffer[64];
  struct imap4d_command c_body_p = fetch_command_table[F_BODY_PEEK];
  c_body_p.states = command->states;
  strcpy (buffer, "[HEADER]");
  fetch_body_peek (&c_body_p, buffer);
  return 0;
}

static int
fetch_rfc822_size (struct imap4d_command *command, char *arg)
{
  size_t size = 0;
  size_t lines = 0;
  message_t msg = NULL;
  mailbox_get_message (mbox, command->states, &msg);
  message_size (msg, &size);
  message_lines (msg, &lines);
  util_send (" %s", command->name);
  util_send (" %u", size + lines);
  return 0;
}

static int
fetch_rfc822_text (struct imap4d_command *command, char *arg)
{
  char buffer[64];
  struct imap4d_command c_body = fetch_command_table[F_BODY];
  c_body.states = command->states;
  strcpy (buffer, "[TEXT]");
  fetch_body (&c_body, buffer);
  return 0;
}


static int
fetch_rfc822 (struct imap4d_command *command, char *arg)
{
  char buffer[64];
  struct imap4d_command c_body = fetch_command_table[F_BODY];
  c_body.states = command->states;
  strcpy (buffer, "[]");
  fetch_body (&c_body, buffer);
  return 0;
}

/* FIXME: not implemeted.  */
static int
fetch_bodystructure (struct imap4d_command *command, char *arg)
{
  return 0;
}

static int
fetch_body (struct imap4d_command *command, char *arg)
{
  attribute_t attr = NULL;
  message_t msg = NULL;
  struct imap4d_command c_body_p = fetch_command_table[F_BODY_PEEK];
  mailbox_get_message (mbox, command->states, &msg);
  message_get_attribute (msg, &attr);
  c_body_p.states = command->states;
  fetch_body_peek (&c_body_p, arg);
  attribute_set_seen (attr);
  return 0;
}

static int
fetch_uid (struct imap4d_command *command, char *arg)
{
  size_t uid = 0;
  message_t msg = NULL;
  mailbox_get_message (mbox, command->states, &msg);
  message_get_uid (msg, &uid);
  util_send (" %s %d", command->name, uid);
  return 0;
}

static int
fetch_body_peek (struct imap4d_command *command, char *arg)
{
  message_t msg = NULL;
  char *partial = strchr (arg, '<');

  mailbox_get_message (mbox, command->states, &msg);

  util_send (" %s", command->name);

  if (strncasecmp (arg, "[]", 2) == 0)
    {
      stream_t stream = NULL;
      char buffer[512];
      size_t size = 0, lines = 0, n = 0;
      off_t off = 0;
      message_get_stream (msg, &stream);
      message_size (msg, &size);
      message_size (msg, &lines);
      util_send (" BODY[] {%u}\r\n", size + lines);
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
  else if (strncasecmp (arg, "[HEADER]", 8) == 0)
    {
      header_t header = NULL;
      stream_t stream = NULL;
      char buffer[512];
      size_t size = 0, lines = 0, n = 0;
      off_t off = 0;
      message_get_header (msg, &header);
      header_size (header, &size);
      header_lines (header, &lines);
      util_send (" BODY[HEADER] {%u}\r\n", size + lines);
      header_get_stream (msg, &stream);
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
      util_send (" BODY[TEXT] {%u}\r\n", size + lines);
      body_get_stream (msg, &stream);
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
  else
    util_send (" Not supported");
  return 0;
}

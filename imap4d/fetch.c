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

struct fetch_command;

static int fetch_all               __P ((struct fetch_command *, char*));
static int fetch_full              __P ((struct fetch_command *, char*));
static int fetch_fast              __P ((struct fetch_command *, char*));
static int fetch_envelope          __P ((struct fetch_command *, char*));
static int fetch_envelope0         __P ((message_t));
static int fetch_flags             __P ((struct fetch_command *, char*));
static int fetch_internaldate      __P ((struct fetch_command *, char*));
static int fetch_rfc822_header     __P ((struct fetch_command *, char*));
static int fetch_rfc822_size       __P ((struct fetch_command *, char*));
static int fetch_rfc822_text       __P ((struct fetch_command *, char*));
static int fetch_rfc822            __P ((struct fetch_command *, char*));
static int fetch_bodystructure     __P ((struct fetch_command *, char*));
static int fetch_bodystructure0    __P ((message_t, int));
static int bodystructure           __P ((message_t, int));
static int fetch_body              __P ((struct fetch_command *, char*));
static int fetch_uid               __P ((struct fetch_command *, char*));

static int fetch_operation         __P ((message_t, char *, int));
static int fetch_message           __P ((message_t, unsigned long, unsigned long));
static int fetch_header            __P ((message_t, unsigned long, unsigned long));
static int fetch_content           __P ((message_t, unsigned long, unsigned long));
static int fetch_io                __P ((stream_t, unsigned long, unsigned long));
static int fetch_header_fields     __P ((message_t, char *, unsigned long, unsigned long));
static int fetch_header_fields_not __P ((message_t, char *, unsigned long, unsigned long));
static int fetch_send_address      __P ((char *));

static struct fetch_command* fetch_getcommand __P ((char *, struct fetch_command[]));

struct fetch_command
{
  const char *name;
  int (*func) __P ((struct fetch_command *, char *));
  message_t msg;
} fetch_command_table [] =
{
#define F_ALL 0
  {"ALL", fetch_all, 0},
#define F_FULL 1
  {"FULL", fetch_full, 0},
#define F_FAST 2
  {"FAST", fetch_fast, 0},
#define F_ENVELOPE 3
  {"ENVELOPE", fetch_envelope, 0},
#define F_FLAGS 4
  {"FLAGS", fetch_flags, 0},
#define F_INTERNALDATE 5
  {"INTERNALDATE", fetch_internaldate, 0},
#define F_RFC822_HEADER 6
   {"RFC822.HEADER", fetch_rfc822_header, 0},
#define F_RFC822_SIZE 7
  {"RFC822.SIZE", fetch_rfc822_size, 0},
#define F_RFC822_TEXT 8
  {"RFC822.TEXT", fetch_rfc822_text, 0},
#define F_RFC822 9
  {"RFC822", fetch_rfc822, 0},
#define F_BODYSTRUCTURE 10
  {"BODYSTRUCTURE", fetch_bodystructure, 0},
#define F_BODY 11
  {"BODY", fetch_body, 0},
#define F_UID 12
  {"UID", fetch_uid, 0},
  { NULL, 0, 0}
};

/* Go through the fetch array sub command and returns the the structure.  */

static struct fetch_command *
fetch_getcommand (char *cmd, struct fetch_command command_table[])
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

/* The FETCH command retrieves data associated with a message in the
   mailbox.  The data items to be fetched can be either a single atom
   or a parenthesized list.  */
int
imap4d_fetch (struct imap4d_command *command, char *arg)
{
  int rc;
  char buffer[64];

  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");

  rc = imap4d_fetch0 (arg, 0, buffer, sizeof buffer);
  return util_finish (command, rc, buffer);
}

/* Where the real implementation is.  It is here since UID command also
   calls FETCH.  */
int
imap4d_fetch0 (char *arg, int isuid, char *resp, size_t resplen)
{
  struct fetch_command *fcmd = NULL;
  int rc = RESP_NO;
  char *sp = NULL;
  char *msgset;
  size_t *set = NULL;
  int n = 0;
  int i;
  int status;

  msgset = util_getword (arg, &sp);
  if (!msgset || !sp || *sp == '\0')
    {
      snprintf (resp, resplen, "Too few args");
      return RESP_BAD;
    }

  /* Get the message numbers in set[].  */
  status = util_msgset (msgset, &set, &n, isuid);
  if (status != 0)
    {
      snprintf (resp, resplen, "Bogus number set");
      return RESP_BAD;
    }

  for (i = 0; i < n; i++)
    {
      char item[32];
      char *items = strdup (sp);
      char *p = items;
      size_t msgno;
      int space = 0;
      message_t msg = NULL;

      msgno = (isuid) ? uid_to_msgno (set[i]) : set[i];
      if (msgno && mailbox_get_message (mbox, msgno, &msg) == 0)
	{
	  fcmd = NULL;
	  util_send ("* %d FETCH (", msgno);
	  item[0] = '\0';
	  /* Server implementations MUST implicitly
	     include the UID message data item as part of any FETCH response
	     caused by a UID command, regardless of whether a UID was specified
	     as a message data item to the FETCH. */
	  if (isuid)
	    {
	      fcmd = &fetch_command_table[F_UID];
	      fcmd->msg = msg;
	      rc = fetch_uid (fcmd, p);
	    }
	  /* Get the fetch command names.  */
	  while (*items && *items != ')')
	    {
	      util_token (item, sizeof (item), &items);
	      /* Do not send the UID again.  */
	      if (isuid && strcasecmp (item, "UID") == 0)
		  continue;
	      if (fcmd)
		space = 1;
	      /* Search in the table.  */
	      fcmd = fetch_getcommand (item, fetch_command_table);
	      if (fcmd)
		{
		  if (space)
		    {
		      util_send (" ");
		      space = 0;
		    }
		  fcmd->msg = msg;
		  rc = fcmd->func (fcmd, items);
		}
	    }
	  util_send (")\r\n");
	}
      free (p);
    }
  free (set);
  snprintf (resp, resplen, "Completed");
  return rc;
}

/* The Fetch comand retrieves data associated with a message in the
   mailbox,  The data items to be fetched can be either a single  atom
   or a parenthesized list.  */

/* Macro equivalent to: (FLAGS INTERNALDATE RFC822.SIZE ENVELOPE)
   or (FAST ENVELOPE) */
static int
fetch_all (struct fetch_command *command, char *arg)
{
  struct fetch_command c_env = fetch_command_table[F_ENVELOPE];
  fetch_fast (command, arg);
  util_send (" ");
  c_env.msg = command->msg;
  fetch_envelope (&c_env, arg);
  return RESP_OK;
}

/* Combination of (ALL BODY).  */
static int
fetch_full (struct fetch_command *command, char *arg)
{
  struct fetch_command c_body = fetch_command_table[F_BODY];
  fetch_all (command, arg);
  util_send (" ");
  c_body.msg = command->msg;
  fetch_body (&c_body, arg);
  return RESP_OK;
}

/* Combination of (FLAGS INTERNALDATE RFC822.SIZE).  */
static int
fetch_fast (struct fetch_command *command, char *arg)
{
  struct fetch_command c_idate = fetch_command_table[F_INTERNALDATE];
  struct fetch_command c_rfc = fetch_command_table[F_RFC822_SIZE];
  struct fetch_command c_flags = fetch_command_table[F_FLAGS];
  c_flags.msg = command->msg;
  fetch_flags (&c_flags, arg);
  util_send (" ");
  c_idate.msg = command->msg;
  fetch_internaldate (&c_idate, arg);
  util_send (" ");
  c_rfc.msg = command->msg;
  fetch_rfc822_size (&c_rfc, arg);
  return RESP_OK;
}

/* Header: Date, Subject, From, Sender, Reply-To, To, Cc, Bcc, In-Reply-To,
   and Message-Id.  */
static int
fetch_envelope (struct fetch_command *command, char *arg)
{
  int status;
  (void)arg;
  util_send ("%s (", command->name);
  status = fetch_envelope0 (command->msg);
  util_send (")");
  return status;
}

static int
fetch_envelope0 (message_t msg)
{
  char *buffer = NULL;
  char *from = NULL;
  header_t header = NULL;

  message_get_header (msg, &header);

  /* Date.  */
  header_aget_value (header, "Date", &buffer);
  util_send_string (buffer);
  free (buffer);
  util_send (" ");

  /* Subject.  */
  header_aget_value (header, "Subject", &buffer);
  util_send_string (buffer);
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
  util_send_string (buffer);

  free (buffer);
  free (from);
  return RESP_OK;
}

/* The flags that are set for this message.  */
/* FIXME: User flags not done.  */
static int
fetch_flags (struct fetch_command *command, char *arg)
{
  attribute_t attr = NULL;
  int space = 0;
  (void)arg;
  message_get_attribute (command->msg, &attr);
  util_send ("%s (", command->name);
  if (attribute_is_deleted (attr))
    {
      util_send ("\\Deleted");
      space = 1;
    }
  if (attribute_is_answered (attr))
    {
      if (space)
	util_send (" ");
      util_send ("\\Answered");
      space = 1;
    }
  if (attribute_is_flagged (attr))
    {
      if (space)
	util_send (" ");
      util_send ("\\Flagged");
      space = 1;
    }
  if (attribute_is_seen (attr) && attribute_is_read (attr))
    {
      if (space)
	util_send (" ");
      util_send ("\\Seen");
      space = 1;
    }
  if (attribute_is_draft (attr))
    {
      if (space)
	util_send (" ");
      util_send (" \\Draft");
    }
  util_send (")");
  return RESP_OK;
}

/* The internal date of the message.  */
static const char *MONTHS[] =
{
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *
get_timezone (const struct tm *tptr)
{
  char tz[5];
  tz[0] = '\0';
  strftime (tz, sizeof (tz), "%Z", tptr);
  if (strcasecmp (tz, "UT") == 0)
    return "+0000";
  else if (strcasecmp (tz, "GMT") == 0)
    return "+0000";
  else if (strcasecmp (tz, "EDT") == 0)
    return "-0400";
  else if (strcasecmp (tz, "EST") == 0)
    return "-0500";
  else if (strcasecmp (tz, "CDT") == 0)
    return "-0500";
  else if (strcasecmp (tz, "CST") == 0)
    return "-0600";
  else if (strcasecmp (tz, "MDT") == 0)
    return "-0600";
  else if (strcasecmp (tz, "MST") == 0)
    return "-0700";
  else if (strcasecmp (tz, "PDT") == 0)
    return "-0700";
  else if (strcasecmp (tz, "PST") == 0)
    return "-0800";
  return "-0000"; /* Oops.  */
}

/* FIXME: Wrong format?  */
static int
fetch_internaldate (struct fetch_command *command, char *arg)
{
  char date[512];
  envelope_t env = NULL;
  struct tm tm;
  struct tm *tptr;
  (void)arg;
  message_get_envelope (command->msg, &env);
  date[0] = '\0';
  envelope_date (env, date, sizeof (date), NULL);
  {
    int year, mon, day, hour, min, sec;
    int offt;
    int i;
    time_t now;
    char month[5];
    char wday[5];

    month[0] = '\0';
    wday[0] = '\0';
    day = mon = year = hour = min = sec = offt = 0;

    sscanf (date, "%3s %3s %2d %2d:%2d:%2d %d\n", wday, month, &day,
	    &hour, &min, &sec, &year);
    tm.tm_sec = sec;
    tm.tm_min = min;
    tm.tm_hour = hour;
    for (i = 0; i < 12; i++)
      {
	if (strncasecmp(month, MONTHS[i], 3) == 0)
	  {
	    mon = i;
	    break;
	  }
      }
    tm.tm_mday = day;
    tm.tm_mon = mon;
    tm.tm_year = (year > 1900) ? year - 1900 : year;
    tm.tm_yday = 0; /* unknown. */
    tm.tm_wday = 0; /* unknown. */
    tm.tm_isdst = -1; /* unknown. */
    /* What to do the timezone?  */
    now = mktime (&tm);
    if (now == (time_t)-1)
      {
	/* Fall back to localtime.  */
	now = time (NULL);
	tptr = localtime (&now);
      }
    else
      tptr = &tm;
  }
  strftime (date, sizeof (date), "%d-%b-%Y %X", &tm);
  util_send ("%s", command->name);
  util_send (" \"%s %s\"", date, get_timezone (&tm));
  return RESP_OK;
}

/* Equivalent to BODY.PEEK[HEADER].  */
static int
fetch_rfc822_header (struct fetch_command *command, char *arg)
{
  char buffer[16];
  (void)arg;
  util_send ("%s ", command->name);
  strcpy (buffer, "[HEADER]");
  fetch_operation (command->msg, buffer, 1);
  return RESP_OK;
}

/* Equivalent to BODY[TEXT].  */
static int
fetch_rfc822_text (struct fetch_command *command, char *arg)
{
  char buffer[16];
  attribute_t attr = NULL;
  (void)arg;
  message_get_attribute (command->msg, &attr);
  if (!attribute_is_seen (attr) && !attribute_is_read (attr))
    {
      util_send ("FLAGS (\\Seen) ");
      attribute_set_seen (attr);
      attribute_set_read (attr);
    }
  util_send ("%s ", command->name);
  strcpy (buffer, "[TEXT]");
  fetch_operation (command->msg, buffer, 1);
  return RESP_OK;
}

/* The [RFC-822] size of the message.  */
static int
fetch_rfc822_size (struct fetch_command *command, char *arg)
{
  size_t size = 0;
  size_t lines = 0;
  (void)arg;
  message_size (command->msg, &size);
  message_lines (command->msg, &lines);
  util_send ("%s %u", command->name, size + lines);
  return RESP_OK;
}

/* Equivalent to BODY[].  */
static int
fetch_rfc822 (struct fetch_command *command, char *arg)
{
  if (*arg == '.')
    {
      if (strncasecmp (arg, ".SIZE", 5) == 0)
	{
	  struct fetch_command c_rfc= fetch_command_table[F_RFC822_SIZE];
	  c_rfc.msg = command->msg;
	  fetch_rfc822_size (&c_rfc, arg);
	}
      else if (strncasecmp (arg, ".TEXT", 5) == 0)
	{
	  struct fetch_command c_rfc = fetch_command_table[F_RFC822_TEXT];
	  c_rfc.msg = command->msg;
	  fetch_rfc822_text (&c_rfc, arg);
	}
      else if (strncasecmp (arg, ".HEADER", 7) == 0)
	{
	  struct fetch_command c_rfc = fetch_command_table[F_RFC822_HEADER];
	  c_rfc.msg = command->msg;
	  fetch_rfc822_header (&c_rfc, arg);
	}
    }
  else
    {
      char buffer[16];
      attribute_t attr = NULL;
      message_get_attribute (command->msg, &attr);
      if (!attribute_is_seen (attr) && !attribute_is_read (attr))
	{
	  util_send ("FLAGS (\\Seen) ");
	  attribute_set_seen (attr);
	  attribute_set_read (attr);
	}
      util_send ("%s ", command->name);
      strcpy (buffer, "[]");
      fetch_operation (command->msg, buffer, 1);
    }
  return RESP_OK;
}

/* The unique identifier for the message.  */
static int
fetch_uid (struct fetch_command *command, char *arg)
{
  size_t uid = 0;
  (void)arg;
  message_get_uid (command->msg, &uid);
  util_send ("%s %d", command->name, uid);
  return RESP_OK;
}

static int
fetch_bodystructure (struct fetch_command *command, char *arg)
{
  (void)arg;
  util_send ("%s (", command->name);
  fetch_bodystructure0 (command->msg, 1);
  util_send (")");
  return RESP_OK;
}

static int
fetch_bodystructure0 (message_t message, int extension)
{
  size_t nparts = 1;
  size_t i;
  int is_multipart = 0;
  message_is_multipart (message, &is_multipart);
  if (is_multipart)
    {
      message_get_num_parts (message, &nparts);
      for (i = 1; i <= nparts; i++)
	{
	  message_t msg = NULL;
	  message_get_part (message, i, &msg);
	  util_send ("(");
	  fetch_bodystructure0 (msg, extension);
	  util_send (")");
	} /* for () */
      /* The extension data for multipart. */
      if (extension)
	{
	  header_t header = NULL;
	  char *buffer = NULL;
	  char *sp = NULL;
	  char *s;
	  /* The subtype.  */
	  message_get_header (message, &header);
	  header_aget_value (header, MU_HEADER_CONTENT_TYPE, &buffer);
	  s = strtok_r (buffer, " \t\r\n;", &sp);
	  s = strchr (buffer, '/');
	  if (s)
	    {
	      s++;
	      util_send (" \"%s\"", s);
	    }
	  else
	    util_send (" NIL");
	  /* Content-type parameter list. */
	  util_send (" (");
	  {
	    int space = 0;
	    while ((s = strtok_r (NULL, " \t\r\n;", &sp)))
	      {
		char *p = strchr (s, '=');
		if (p)
		  *p++ = '\0';
		if (space)
		  util_send (" ");
		util_send ("\"%s\"", s);
		if (p)
		  util_send (" \"%s\"", p);
		space = 1;
	      }
	  }
	  util_send (")");
	  free (buffer);
	  /* Content-Disposition.  */
	  header_aget_value (header, MU_HEADER_CONTENT_DISPOSITION, &buffer);
	  if (*buffer)
	    {
	      char *t = buffer;
	      util_send (" (");
	      while ((s = strtok_r (t, " \t\r\n;", &sp)))
		{
		  char *p = strchr (s, '=');
		  if (p)
		    *p++ = '\0';
		  if (t == NULL)
		    util_send (" ");
		  util_send ("\"%s\"", s);
		  if (p)
		    util_send (" \"%s\"", (p) ? p : "NIL");
		  t = NULL;
		}
	      util_send (")");
	    }
	  else
	    util_send (" NIL");
	  free (buffer);
	  /* Content-Language.  */
	  header_aget_value (header, MU_HEADER_CONTENT_LANGUAGE, &buffer);
	  util_send (" ");
	  util_send_string (buffer);
	  free (buffer);
	} /* extension */
    }
  else
    bodystructure (message, extension);
  return RESP_OK;
}

static int
bodystructure (message_t msg, int extension)
{
  header_t header = NULL;
  char *sp = NULL;
  char *buffer = NULL;
  char *s;
  size_t blines = 0;
  int message_rfc822 = 0;
  int text_plain = 0;

  message_get_header (msg, &header);

  /* MIME:  */
  header_aget_value (header, MU_HEADER_CONTENT_TYPE, &buffer);
  s = strtok_r (buffer, " \t\r\n;", &sp);
  /* MIME media type and subtype  */
  if (s)
    {
      char *p = strchr (s, '/');
      if (strcasecmp (s, "MESSAGE/RFC822") == 0)
	message_rfc822 = 1;
      if (strcasecmp (s, "TEXT/PLAIN") == 0)
	text_plain = 1;
      if (p)
	*p++ = '\0';
      util_send ("\"%s\"", s);
      util_send (" \"%s\"", (p) ? p : "NIL");
    }
  else
    {
      /* Default?  */
      util_send ("TEXT");
      util_send (" PLAIN");
    }
  /* Content-type parameter list. */
  util_send (" (");
    {
      int have_charset = 0;
      int space = 0;
      while ((s = strtok_r (NULL, " \t\r\n;", &sp)))
	{
	  char *p = strchr (s, '=');
	  if (p)
	    *p++ = '\0';
	  if (space)
	    util_send (" ");
	  util_send ("\"%s\"", s);
	  util_send (" \"%s\"", (p) ? p : "NIL");
	  if (strcasecmp (s, "charset") == 0)
	    have_charset = 1;
	}
      /* Default.  */
      if (!have_charset)
	{
	  util_send ("\"CHARSET\"");
	  util_send (" \"US-ASCII\"");
	}
    }
  util_send (")");
  free (buffer);

  /* Content-ID. */
  header_aget_value (header, MU_HEADER_CONTENT_ID, &buffer);
  util_send (" ");
  util_send_string (buffer);
  free (buffer);

  /* Content-Description. */
  header_aget_value (header, MU_HEADER_CONTENT_DESCRIPTION, &buffer);
  util_send (" ");
  util_send_string (buffer);
  free (buffer);

  /* Content-Transfer-Encoding. */
  header_aget_value (header, MU_HEADER_CONTENT_TRANSFER_ENCODING, &buffer);
  util_send (" ");
  util_send_string ((*buffer) ? buffer : "7BIT");
  free (buffer);

  /* Body size RFC822 format.  */
  {
    size_t size = 0;
    body_t body = NULL;
    message_get_body (msg, &body);
    body_size (body, &size);
    body_lines (body, &blines);
    util_send (" %d", size + blines);
  }

  /* If the mime type was text.  */
  if (text_plain)
    {
      /* Add the line number of the body.  */
      util_send (" %d", blines);
    }
  else if (message_rfc822)
    {
      size_t lines = 0;
      /* Add envelope structure */
      util_send ("(");
      fetch_envelope0 (msg);
      util_send (")");
      /* Add body structure */
      util_send ("(");
      bodystructure (msg, 1);
      util_send (")");
      /* size in text lines of the encapsulated message.  */
      message_lines (msg, &lines);
      util_send (" %d", lines);
    }

  if (extension)
    {
      /* Content-MD5.  */
      header_aget_value (header, MU_HEADER_CONTENT_MD5, &buffer);
      util_send (" ");
      util_send_string (buffer);
      free (buffer);
      /* Content-Disposition.  */
      header_aget_value (header, MU_HEADER_CONTENT_DISPOSITION, &buffer);
      if (*buffer)
	{
	  char *t = buffer;
	  util_send (" (");
	  while ((s = strtok_r (t, " \t\r\n;", &sp)))
	    {
	      char *p = strchr (s, '=');
	      if (p)
		*p++ = '\0';
	      if (t == NULL)
		util_send (" ");
	      util_send ("\"%s\"", s);
	      util_send (" \"%s\"", (p) ? p : "NIL");
	      t = NULL;
	    }
	  util_send (")");
	}
      else
	util_send (" NIL");
      free (buffer);
      /* Content-Language.  */
      header_aget_value (header, MU_HEADER_CONTENT_LANGUAGE, &buffer);
      util_send (" ");
      util_send_string (buffer);
      free (buffer);
    }
  return RESP_OK;
}

/* An alternate form of BODY that does not implicitly set the \Seen flag.  */
static int
fetch_body (struct fetch_command *command, char *arg)
{
  /* It's BODY set the message as seen  */
  if (*arg == '[')
    {
      attribute_t attr = NULL;
      message_get_attribute (command->msg, &attr);
      if (!attribute_is_seen (attr) && !attribute_is_read (attr))
	{
	  util_send ("FLAGS (\\Seen) ");
	  attribute_set_seen (attr);
	  attribute_set_read (attr);
	}
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
      /* Call body structure without the extension.  */
      util_send ("%s (", command->name);
      fetch_bodystructure0 (command->msg, 0);
      util_send (")");
      return RESP_OK;
    }
  util_send ("%s", command->name);
  return fetch_operation (command->msg, arg, 0);
}

static int
fetch_operation (message_t msg, char *arg, int silent)
{
  unsigned long start = ULONG_MAX;
  unsigned long end = ULONG_MAX; /* No limit. */
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
	  return RESP_OK;
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
  return RESP_OK;
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
  char *buffer, *p;
  size_t n = 0;
  size_t total = 0;
  off_t offset;

  offset = (start == ULONG_MAX) ? 0 : start;

  filter_create (&rfc, stream, "rfc822", MU_FILTER_ENCODE, MU_STREAM_READ);

  p = buffer = calloc (end + 2, 1);
  while (end > 0 && stream_read (rfc, buffer, end + 1, offset, &n) == 0 && n > 0)
    {
      offset += n;
      total += n;
      end -= n;
      buffer += n;
    }
  /* Make sure we null terminate.  */
  *buffer = '\0';

  if (start != ULONG_MAX)
    util_send ("<%lu>", start);

  if (total)
    {
      util_send (" {%u}\r\n", total);
      util_send ("%s", p);
    }
  else
    util_send (" \"\"");
  free (p);
  return RESP_OK;
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
    imap4d_bye (ERR_NO_MEM);

  /* Save the fields in an array.  */
  {
    char *field;
    char *sp = NULL;
    for (;(field = strtok_r (arg, " ()]\r\n", &sp));
	 arg = NULL, array_len++)
      {
	array = realloc (array, (array_len + 1) * sizeof (*array));
	if (!array)
	  imap4d_bye (ERR_NO_MEM);
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
	    imap4d_bye (ERR_NO_MEM);
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
  return RESP_OK;
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
    imap4d_bye (ERR_NO_MEM);

  /* Save the field we want to ignore.  */
  {
    char *field;
    char *sp = NULL;
    for (;(field = strtok_r (arg, " ()]\r\n", &sp));
	 arg = NULL, array_len++)
      {
	array = realloc (array, (array_len + 1) * sizeof (*array));
	if (!array)
	  imap4d_bye (ERR_NO_MEM);
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
	    imap4d_bye (ERR_NO_MEM);
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
  return RESP_OK;
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
      return RESP_OK;
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
  return RESP_OK;
}

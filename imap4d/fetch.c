/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2006, 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
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
#include <ctype.h>
#include <mailutils/argcv.h>

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

static int fetch_all               (struct fetch_command *, char**);
static int fetch_full              (struct fetch_command *, char**);
static int fetch_fast              (struct fetch_command *, char**);
static int fetch_envelope          (struct fetch_command *, char**);
static int fetch_flags             (struct fetch_command *, char**);
static int fetch_internaldate      (struct fetch_command *, char**);
static int fetch_rfc822_header     (struct fetch_command *, char**);
static int fetch_rfc822_size       (struct fetch_command *, char**);
static int fetch_rfc822_text       (struct fetch_command *, char**);
static int fetch_rfc822            (struct fetch_command *, char**);
static int fetch_bodystructure     (struct fetch_command *, char**);
static int fetch_body              (struct fetch_command *, char**);
static int fetch_uid               (struct fetch_command *, char**);

/* Helper functions.  */
static int fetch_envelope0         (mu_message_t);
static int fetch_bodystructure0    (mu_message_t, int);
static int bodystructure           (mu_message_t, int);
static void send_parameter_list    (const char *);
static int fetch_operation         (mu_message_t, char **, int);
static int fetch_message           (mu_message_t, unsigned long, unsigned long);
static int fetch_header            (mu_message_t, unsigned long, unsigned long);
static int fetch_body_content      (mu_message_t, unsigned long, unsigned long);
static int fetch_io                (mu_stream_t, unsigned long, unsigned long, unsigned long);
static int fetch_header_fields     (mu_message_t, char **, unsigned long, unsigned long);
static int fetch_header_fields_not (mu_message_t, char **, unsigned long, unsigned long);
static int fetch_send_address      (const char *);

static struct fetch_command* fetch_getcommand (char *, struct fetch_command*);

struct fetch_command
{
  const char *name;
  int (*func) (struct fetch_command *, char **);
  mu_message_t msg;
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
fetch_getcommand (char *cmd, struct fetch_command *command_table)
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

  rc = imap4d_fetch0 (arg, 0, buffer, sizeof buffer);
  return util_finish (command, rc, "%s", buffer);
}

/* Where the real implementation is.  It is here since UID command also
   calls FETCH.  */
int
imap4d_fetch0 (char *arg, int isuid, char *resp, size_t resplen)
{
  struct fetch_command *fcmd = NULL;
  int rc = RESP_OK;
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

  /* Prepare status code. It will be replaced if an error occurs in the
     loop below */
  snprintf (resp, resplen, "Completed");
  
  for (i = 0; i < n && rc == RESP_OK; i++)
    {
      size_t msgno = (isuid) ? uid_to_msgno (set[i]) : set[i];
      mu_message_t msg = NULL;

      if (msgno && mu_mailbox_get_message (mbox, msgno, &msg) == 0)
	{
	  char item[32];
	  char *items = strdup (sp);
	  char *p = items;
	  int space = 0;

	  fcmd = NULL;
	  util_send ("* %s FETCH (", mu_umaxtostr (0, msgno));
	  item[0] = '\0';
	  /* Server implementations MUST implicitly
	     include the UID message data item as part of any FETCH
	     response caused by a UID command, regardless of whether
	     a UID was specified as a message data item to the FETCH. */
	  if (isuid)
	    {
	      fcmd = &fetch_command_table[F_UID];
	      fcmd->msg = msg;
	      rc = fetch_uid (fcmd, &items);
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
		  rc = fcmd->func (fcmd, &items);
		}
	    }
	  util_send (")\r\n");
	  free (p);
	}
      else if (!isuid)
	/* According to RFC 3501, "A non-existent unique identifier is
	   ignored without any error message generated." */
	{
	  snprintf (resp, resplen,
		    "Bogus message set: message number out of range");
	  rc = RESP_BAD;
	  break;
	}
    }
  free (set);
  return rc;
}

/* ALL:
   Macro equivalent to: (FLAGS INTERNALDATE RFC822.SIZE ENVELOPE)
   Combination of FAST and ENVELOPE.  */
static int
fetch_all (struct fetch_command *command, char **arg)
{
  struct fetch_command c_env = fetch_command_table[F_ENVELOPE];
  fetch_fast (command, arg);
  util_send (" ");
  c_env.msg = command->msg;
  fetch_envelope (&c_env, arg);
  return RESP_OK;
}

/* FULL:
   Macro equivalent to: (FLAGS INTERNALDATE
   RFC822.SIZE ENVELOPE BODY).
   Combination of (ALL BODY).  */
static int
fetch_full (struct fetch_command *command, char **arg)
{
  struct fetch_command c_body = fetch_command_table[F_BODY];
  fetch_all (command, arg);
  util_send (" ");
  c_body.msg = command->msg;
  return fetch_body (&c_body, arg);
}

/* FAST:
   Macro equivalent to: (FLAGS INTERNALDATE RFC822.SIZE)
   Combination of (FLAGS INTERNALDATE RFC822.SIZE).  */
static int
fetch_fast (struct fetch_command *command, char **arg)
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

/* ENVELOPE:
   Header: Date, Subject, From, Sender, Reply-To, To, Cc, Bcc, In-Reply-To,
   and Message-Id.  */
static int
fetch_envelope (struct fetch_command *command, char **arg ARG_UNUSED)
{
  int status;
  util_send ("%s (", command->name);
  status = fetch_envelope0 (command->msg);
  util_send (")");
  return status;
}

/* FLAGS: The flags that are set for this message.  */
/* FIXME: User flags not done. If enable change the PERMANENTFLAGS in SELECT */
void
fetch_flags0 (const char *prefix, mu_message_t msg, int isuid)
{
  mu_attribute_t attr = NULL;

  mu_message_get_attribute (msg, &attr);
  if (isuid)
    {
      struct fetch_command *fcmd = &fetch_command_table[F_UID];
      fcmd->msg = msg;
      fetch_uid (fcmd, NULL);
      util_send (" ");
    }
  util_send ("%s (", prefix);
  util_print_flags(attr);
  util_send (")");
}

static int
fetch_flags (struct fetch_command *command, char **arg)
{
  fetch_flags0 (command->name, command->msg, 0);
  return RESP_OK;
}


/* INTERNALDATE   The internal date of the message.
   Format:

   date_time       ::= <"> date_day_fixed "-" date_month "-" date_year
   SPACE time SPACE zone <">

   date_day        ::= 1*2digit
   ;; Day of month

   date_day_fixed  ::= (SPACE digit) / 2digit
   ;; Fixed-format version of date_day

   date_month      ::= "Jan" / "Feb" / "Mar" / "Apr" / "May" / "Jun" /
   "Jul" / "Aug" / "Sep" / "Oct" / "Nov" / "Dec"

   date_text       ::= date_day "-" date_month "-" date_year

   date_year       ::= 4digit

   time            ::= 2digit ":" 2digit ":" 2digit
   ;; Hours minutes seconds

   zone            ::= ("+" / "-") 4digit
   ;; Signed four-digit value of hhmm representing
   ;; hours and minutes west of Greenwich (that is,
   ;; (the amount that the given time differs from
   ;; Universal Time).  Subtracting the timezone
   ;; from the given time will give the UT form.
   ;; The Universal Time zone is "+0000".  */
static int
fetch_internaldate (struct fetch_command *command, char **arg ARG_UNUSED)
{
  char date[128];
  mu_envelope_t env = NULL;
  struct tm tm, *tmp = NULL;
  mu_timezone tz;

  mu_message_get_envelope (command->msg, &env);
  date[0] = '\0';
  if (mu_envelope_date (env, date, sizeof (date), NULL) == 0)
    {
      char *p = date;
      if (mu_parse_ctime_date_time ((const char **) &p, &tm, &tz) == 0)
	tmp = &tm;
    }
  if (!tmp)
    {
      time_t t = time(NULL);
      tmp = localtime(&t);
    }
  mu_strftime (date, sizeof (date), "%d-%b-%Y %H:%M:%S", tmp);
  util_send ("%s", command->name);
  util_send (" \"%s +0000\"", date);
  return RESP_OK;
}

/*
  RFC822.HEADER:
  Functionally equivalent to BODY.PEEK[HEADER], differing in the syntax of
  the resulting untagged FETCH data (RFC822.HEADER is returned). */
static int
fetch_rfc822_header (struct fetch_command *command, char **arg ARG_UNUSED)
{
  char buffer[32];
  char *p = buffer;

  strcpy (buffer, ".PEEK[HEADER]");
  return fetch_body (command, &p);
}

/* RFC822.TEXT:
   Functionally equivalent to BODY[TEXT], differing in the syntax of the
   resulting untagged FETCH data (RFC822.TEXT is returned). */
static int
fetch_rfc822_text (struct fetch_command *command, char **arg ARG_UNUSED)
{
  char buffer[16];
  char *p = buffer;

  strcpy (buffer, "[TEXT]");
  return fetch_body (command, &p);
}

/* The [RFC-822] size of the message.  */
static int
fetch_rfc822_size (struct fetch_command *command, char **arg ARG_UNUSED)
{
  size_t size = 0;
  size_t lines = 0;
  
  mu_message_size (command->msg, &size);
  mu_message_lines (command->msg, &lines);
  util_send ("%s %u", command->name, size + lines);
  return RESP_OK;
}

/* RFC822:
   Functionally equivalent to BODY[], differing in the syntax of the
   resulting untagged FETCH data (RFC822 is returned). */
static int
fetch_rfc822 (struct fetch_command *command, char **arg)
{
  if (**arg == '.')
    {
      /* We have to catch the other RFC822.XXX commands here.  This is because
	 util_token() in imap4d_fetch0 will return the RFC822 token only.  */
      if (strncasecmp (*arg, ".SIZE", 5) == 0)
	{
	  struct fetch_command c_rfc= fetch_command_table[F_RFC822_SIZE];
	  c_rfc.msg = command->msg;
	  (*arg) += 5;
	  fetch_rfc822_size (&c_rfc, arg);
	}
      else if (strncasecmp (*arg, ".TEXT", 5) == 0)
	{
	  struct fetch_command c_rfc = fetch_command_table[F_RFC822_TEXT];
	  c_rfc.msg = command->msg;
	  (*arg) += 5;
	  fetch_rfc822_text (&c_rfc, arg);
	}
      else if (strncasecmp (*arg, ".HEADER", 7) == 0)
	{
	  struct fetch_command c_rfc = fetch_command_table[F_RFC822_HEADER];
	  c_rfc.msg = command->msg;
	  (*arg) += 7;
	  fetch_rfc822_header (&c_rfc, arg);
	}
    }
  else
    {
      char buffer[16];
      char *p = buffer;
      strcpy (buffer, "[]");
      fetch_body (command, &p);
    }
  return RESP_OK;
}

/* UID: The unique identifier for the message.  */
static int
fetch_uid (struct fetch_command *command, char **arg ARG_UNUSED)
{
  size_t uid = 0;

  mu_message_get_uid (command->msg, &uid);
  util_send ("%s %s", command->name, mu_umaxtostr (0, uid));
  return RESP_OK;
}

/* BODYSTRUCTURE:
   The [MIME-IMB] body structure of the message.  This is computed by the
   server by parsing the [MIME-IMB] header fields in the [RFC-822] header and
   [MIME-IMB] headers.  */
static int
fetch_bodystructure (struct fetch_command *command, char **arg ARG_UNUSED)
{
  util_send ("%s (", command->name);
  fetch_bodystructure0 (command->msg, 1); /* 1 means with extension data.  */
  util_send (")");
  return RESP_OK;
}

/* BODY:          Non-extensible form of BODYSTRUCTURE.
   BODY[<section>]<<partial>> :
   The text of a particular body section.  The section specification is a set
   of zero or more part specifiers delimited by periods.  A part specifier
   is either a part number or one of the following: HEADER, HEADER.FIELDS,
   HEADER.FIELDS.NOT, MIME, and TEXT.  An empty section specification refers
   to the entire message, including the header.

   Note: for body section, the \Seen flag is implicitly set;
         if this causes the flags to change they SHOULD be
	 included as part of the FETCH responses. */
static int
fetch_body (struct fetch_command *command, char **arg)
{
  /* It's body section, set the message as seen  */
  if (**arg == '[')
    {
      mu_attribute_t attr = NULL;
      mu_message_get_attribute (command->msg, &attr);
      if (!mu_attribute_is_read (attr))
	{
	  util_send ("FLAGS (\\Seen) ");
	  mu_attribute_set_read (attr);
	}
    }
  else if (strncasecmp (*arg,".PEEK", 5) == 0)
    {
      /* Move pass the .peek  */
      (*arg) += 5;
      while (isspace ((unsigned)**arg))
	(*arg)++;
    }
  else if (**arg != '[' && **arg != '.')
    {
      /* Call body structure without the extension.  */
      util_send ("%s (", command->name);
      fetch_bodystructure0 (command->msg, 0);
      util_send (")");
      return RESP_OK;
    }
  util_send ("%s", command->name);
  return fetch_operation (command->msg, arg,
			  strcasecmp (command->name, "BODY"));
}

/* Helper Functions: Where the Beef is.  */

static void
fetch_send_header_value (mu_header_t header, const char *name,
			 const char *defval, int space)
{
  char *buffer;
  
  if (space)
    util_send (" ");
  if (mu_header_aget_value (header, name, &buffer) == 0)
    {
      util_send_qstring (buffer);
      free (buffer);
    }
  else if (defval)
    util_send_qstring (defval);
  else
    util_send ("NIL");
}

static void
fetch_send_header_list (mu_header_t header, const char *name,
			const char *defval, int space)
{
  char *buffer;
  
  if (space)
    util_send (" ");
  if (mu_header_aget_value (header, name, &buffer) == 0)
    {
      send_parameter_list (buffer);
      free (buffer);
    }
  else if (defval)
    send_parameter_list (defval);
  else
    util_send ("NIL");
}

static void
fetch_send_header_address (mu_header_t header, const char *name,
			   const char *defval, int space)
{
  char *buffer;
  
  if (space)
    util_send (" ");
  if (mu_header_aget_value (header, name, &buffer) == 0)
    {
      fetch_send_address (buffer);
      free (buffer);
    }
  else
    fetch_send_address (defval);
}

/* ENVELOPE:
   The envelope structure of the message.  This is computed by the server by
   parsing the [RFC-822] header into the component parts, defaulting various
   fields as necessary.  The fields are presented in the order:
   Date, Subject, From, Sender, Reply-To, To, Cc, Bcc, In-Reply-To, Message-ID.
   Any field of an envelope or address structure that is not applicable is
   presented as NIL.  Note that the server MUST default the reply-to and sender
   fields from the from field.  The date, subject, in-reply-to, and message-id
   fields are strings.  The from, sender, reply-to, to, cc, and bcc fields
   are parenthesized lists of address structures.  */
static int
fetch_envelope0 (mu_message_t msg)
{
  char *from = NULL;
  mu_header_t header = NULL;

  mu_message_get_header (msg, &header);

  fetch_send_header_value (header, "Date", NULL, 0);
  fetch_send_header_value (header, "Subject", NULL, 1);

  /* From:  */
  mu_header_aget_value (header, "From", &from);
  util_send (" ");
  fetch_send_address (from);

  fetch_send_header_address (header, "Sender", from, 1);
  fetch_send_header_address (header, "Reply-to", from, 1);
  fetch_send_header_address (header, "To", NULL, 1);
  fetch_send_header_address (header, "Cc", NULL, 1);
  fetch_send_header_address (header, "Bcc", NULL, 1);
  fetch_send_header_value (header, "In-Reply-To", NULL, 1);
  fetch_send_header_value (header, "Message-ID", NULL, 1);

  free (from);
  return RESP_OK;
}

/* The beef BODYSTRUCTURE.
   A parenthesized list that describes the [MIME-IMB] body structure of a
   message. Multiple parts are indicated by parenthesis nesting.  Instead of
   a body type as the first element of the parenthesized list there is a nested
   body.  The second element of the parenthesized list is the multipart
   subtype (mixed, digest, parallel, alternative, etc.).

   The extension data of a multipart body part are in the following order:
   body parameter parenthesized list:
   A parenthesized list of attribute/value pairs [e.g. ("foo" "bar" "baz"
   "rag") where "bar" is the value of "foo" and "rag" is the value of
   "baz"] as defined in [MIME-IMB].

   body disposition:
   A parenthesized list, consisting of a disposition type string followed by a
   parenthesized list of disposition attribute/value pairs.  The disposition
   type and attribute names will be defined in a future standards-track
   revision to [DISPOSITION].

   body language:
   A string or parenthesized list giving the body language value as defined
   in [LANGUAGE-TAGS].  */
static int
fetch_bodystructure0 (mu_message_t message, int extension)
{
  size_t nparts = 1;
  size_t i;
  int is_multipart = 0;

  mu_message_is_multipart (message, &is_multipart);
  if (is_multipart)
    {
      char *buffer = NULL;
      mu_header_t header = NULL;

      mu_message_get_num_parts (message, &nparts);

      /* Get all the sub messages.  */
      for (i = 1; i <= nparts; i++)
        {
          mu_message_t msg = NULL;
          mu_message_get_part (message, i, &msg);
          util_send ("(");
          fetch_bodystructure0 (msg, extension);
          util_send (")");
        } /* for () */

      mu_message_get_header (message, &header);


      /* The subtype.  */
      if (mu_header_aget_value (header, MU_HEADER_CONTENT_TYPE, &buffer) == 0)
	{
	  int argc = 0;
	  char **argv;
	  char *s;
	  
	  mu_argcv_get (buffer, " \t\r\n;=", NULL, &argc, &argv);

	  s = strchr (argv[0], '/');
	  if (s)
	    s++;
	  util_send (" ");
	  util_send_qstring (s);

	  /* The extension data for multipart. */
	  if (extension)
	    {
	      int space = 0;
	      char *lvalue = NULL;
	      
	      util_send (" (");
	      for (i = 1; i < argc; i++)
		{
		  /* body parameter parenthesized list:
		     Content-type parameter list. */
		  if (lvalue)
		    {
		      if (space)
			util_send (" ");
		      util_send_qstring (lvalue);
		      lvalue = NULL;
		      space = 1;
		    }

		  switch (argv[i][0])
		    {
		    case ';':
		      continue;
		      
		    case '=':
		      if (++i < argc)
			{
			  char *p = argv[i];
			  util_send (" ");
			  util_unquote (&p);
			  util_send_qstring (p);
			}
		      break;
		      
		    default:
		      lvalue = argv[i];
		    }
		}
	      if (lvalue)
		{
		  if (space)
		    util_send (" ");
		  util_send_qstring (lvalue);
		}
	      util_send (")");
	    }
	  else
	    util_send (" NIL");
	  mu_argcv_free (argc, argv);
          free (buffer);
	}
      else
	/* No content-type header */
	util_send (" NIL");

      /* body disposition: Content-Disposition.  */
      fetch_send_header_list (header, MU_HEADER_CONTENT_DISPOSITION,
			      NULL, 1);
      /* body language: Content-Language.  */
      fetch_send_header_list (header, MU_HEADER_CONTENT_LANGUAGE,
			      NULL, 1);
    }
  else
    bodystructure (message, extension);
  return RESP_OK;
}

/* The basic fields of a non-multipart body part are in the following order:
   body type:
   A string giving the content media type name as defined in [MIME-IMB].

   body subtype:
   A string giving the content subtype name as defined in [MIME-IMB].

   body parameter parenthesized list:
   A parenthesized list of attribute/value pairs [e.g. ("foo" "bar" "baz"
   "rag") where "bar" is the value of "foo" and "rag" is the value of "baz"]
   as defined in [MIME-IMB].

   body id:
   A string giving the content id as defined in [MIME-IMB].

   body description:
   A string giving the content description as defined in [MIME-IMB].

   body encoding:
   A string giving the content transfer encoding as defined in [MIME-IMB].

   body size:
   A number giving the size of the body in octets. Note that this size is the
   size in its transfer encoding and not the resulting size after any decoding.

   A body type of type TEXT contains, immediately after the basic fields, the
   size of the body in text lines.

   A body type of type MESSAGE and subtype RFC822 contains, immediately after
   the basic fields, the envelope structure, body structure, and size in text
   lines of the encapsulated message.

   The extension data of a non-multipart body part are in the following order:
   body MD5:
   A string giving the body MD5 value as defined in [MD5].

   body disposition:
   A parenthesized list with the same content and function as the body
   disposition for a multipart body part.

   body language:\
   A string or parenthesized list giving the body language value as defined
   in [LANGUAGE-TAGS].
 */
static int
bodystructure (mu_message_t msg, int extension)
{
  mu_header_t header = NULL;
  char *buffer = NULL;
  size_t blines = 0;
  int message_rfc822 = 0;
  int text_plain = 0;

  mu_message_get_header (msg, &header);

  if (mu_header_aget_value (header, MU_HEADER_CONTENT_TYPE, &buffer) == 0)
    {
      int argc = 0;
      char **argv;
      char *s, *p;
	  
      mu_argcv_get (buffer, " \t\r\n;=", NULL, &argc, &argv);

      if (strcasecmp (argv[0], "MESSAGE/RFC822") == 0)
        message_rfc822 = 1;
      else if (strcasecmp (argv[0], "TEXT/PLAIN") == 0)
        text_plain = 1;

      s = strchr (argv[0], '/');
      if (s)
	*s++ = 0;
      p = argv[0];
      util_send_qstring (p);
      util_send (" ");
      util_send_qstring (s);

      /* body parameter parenthesized list: Content-type attributes */
      if (argc > 1 || text_plain)
	{
	  int space = 0;
	  char *lvalue = NULL;
	  int have_charset = 0;
	  int i;
	  
	  util_send (" (");
	  for (i = 1; i < argc; i++)
	    {
	      /* body parameter parenthesized list:
		 Content-type parameter list. */
	      if (lvalue)
		{
		  if (space)
		    util_send (" ");
		  util_send_qstring (lvalue);
		  lvalue = NULL;
		  space = 1;
		}
	      
	      switch (argv[i][0])
		{
		case ';':
		  continue;
		  
		case '=':
		  if (++i < argc)
		    {
		      char *p = argv[i];
		      util_send (" ");
		      util_unquote (&p);
		      util_send_qstring (p);
		    }
		  break;
		  
		default:
		  lvalue = argv[i];
		  if (strcasecmp (lvalue, "charset") == 0)
		    have_charset = 1;

		}
	    }
	  
	  if (lvalue)
	    {
	      if (space)
		util_send (" ");
	      util_send_qstring (lvalue);
	    }
	  
	  if (!have_charset && text_plain)
	    {
	      if (space)
		util_send (" ");
	      util_send ("\"CHARSET\" \"US-ASCII\"");
	    }
	  util_send (")");
	}
      else
	util_send (" NIL");
      mu_argcv_free (argc, argv);
      free (buffer);
    }
  else
    {
      /* Default? If Content-Type is not present consider as text/plain.  */
      util_send ("\"TEXT\" \"PLAIN\" (\"CHARSET\" \"US-ASCII\")");
      text_plain = 1;
    }
  
  /* body id: Content-ID. */
  fetch_send_header_value (header, MU_HEADER_CONTENT_ID, NULL, 1);
  /* body description: Content-Description. */
  fetch_send_header_value (header, MU_HEADER_CONTENT_DESCRIPTION, NULL, 1);

  /* body encoding: Content-Transfer-Encoding. */
  fetch_send_header_value (header, MU_HEADER_CONTENT_TRANSFER_ENCODING,
			   "7BIT", 1);

  /* body size RFC822 format.  */
  {
    size_t size = 0;
    mu_body_t body = NULL;
    mu_message_get_body (msg, &body);
    mu_body_size (body, &size);
    mu_body_lines (body, &blines);
    util_send (" %s", mu_umaxtostr (0, size + blines));
  }

  /* If the mime type was text.  */
  if (text_plain)
    {
      /* Add the line number of the body.  */
      util_send (" %s", mu_umaxtostr (0, blines));
    }
  else if (message_rfc822)
    {
      size_t lines = 0;
      mu_message_t emsg = NULL;
      mu_message_unencapsulate  (msg, &emsg, NULL);
      /* Add envelope structure of the encapsulated message.  */
      util_send (" (");
      fetch_envelope0 (emsg);
      util_send (")");
      /* Add body structure of the encapsulated message.  */
      util_send ("(");
      bodystructure (emsg, extension);
      util_send (")");
      /* Size in text lines of the encapsulated message.  */
      mu_message_lines (emsg, &lines);
      util_send (" %s", mu_umaxtostr (0, lines));
      mu_message_destroy (&emsg, NULL);
    }

  if (extension)
    {
      /* body MD5: Content-MD5.  */
      fetch_send_header_value (header, MU_HEADER_CONTENT_MD5, NULL, 1);

      /* body disposition: Content-Disposition.  */
      fetch_send_header_list (header, MU_HEADER_CONTENT_DISPOSITION, NULL, 1);

      /* body language: Content-Language.  */
      fetch_send_header_value (header, MU_HEADER_CONTENT_LANGUAGE, NULL, 1);
    }
  return RESP_OK;
}

static int
fetch_operation (mu_message_t msg, char **arg, int silent)
{
  unsigned long start = ULONG_MAX; /* No starting offset.  */
  unsigned long end = ULONG_MAX; /* No limit. */
  char *section; /* Hold the section number string.  */
  char *partial = strchr (*arg, '<');
  int rc;
  
  /* Check for section specific offset.  */
  if (partial)
    {
      /* NOTE: should this should be move in imap4d_fetch() and have a more
	 draconian check?  */
      *partial = '\0';
      partial++;
      start = strtoul (partial, &partial, 10);
      if (*partial == '.')
	{
	  partial++;
	  end = strtoul (partial, NULL, 10);
	}
    }

  /* Pass the first bracket '['  */
  (*arg)++;
  section = *arg;

  /* Retreive the section message.  */
  while (isdigit ((unsigned)**arg))
    {
      unsigned long j = strtoul (*arg, arg, 10);
      int status;

      /* Wrong section message number bail out.  */
      if (j == 0 || j == ULONG_MAX) /* Technical: I should check errno too.  */
	break;

      /* If the section message did not exist bail out here.  */
      status = mu_message_get_part (msg, j, &msg);
      if (status != 0)
	{
	  util_send (" \"\"");
	  return RESP_OK;
	}
      if (**arg == '.')
	(*arg)++;
      else
	break;
    }

  /* Did we have a section message?  */
  if (((*arg) - section) > 0)
    {
      char *p = section;
      section = calloc ((*arg) - p + 1, 1);
      if (section)
	memcpy (section, p, (*arg) - p);
    }
  else
    section = calloc (1, 1);

  rc = RESP_OK;
  
  /* Choose the right fetch attribute.  */
  if (*section == '\0' && **arg == ']')
    {
      if (!silent)
	util_send ("[]");
      (*arg)++;
      rc = fetch_message (msg, start, end);
    }
  else if (strncasecmp (*arg, "HEADER]", 7) == 0)
    {
      if (!silent)
	{
	  /* NOTE: We violate the RFC here: Header can not take a prefix for
	     section messages it only referes to the RFC822 header .. ok
	     see it as an extension. But according to IMAP4 we should
	     have send an empty string: util_send (" \"\"");
	  */
	  util_send ("[%sHEADER]", section);
	}
      (*arg) += 7;
      rc = fetch_header (msg, start, end);
    }
  else if (strncasecmp (*arg, "MIME]", 5) == 0)
    {
      if (!silent)
	{
	  if (*section)
	    util_send ("[%sMIME]", section);
	  else
	    util_send ("[%s", *arg);
	}
      (*arg) += 5;
      rc = fetch_header (msg, start, end);
    }
  else if (strncasecmp (*arg, "HEADER.FIELDS.NOT", 17) == 0)
    {
      /* NOTE: we should flag an error if section is not empty: accept
	 as an extension for now.  */
      if (*section)
	util_send ("[%s", section);
      else
	util_send ("[");
      (*arg) += 17;
      rc = fetch_header_fields_not (msg, arg, start, end);
    }
  else if (strncasecmp (*arg, "HEADER.FIELDS", 13) == 0)
    {
      /* NOTE: we should flag an error if section is not empty: accept
	 as an extension for now.  */
      if (*section)
	util_send ("[%s", section);
      else
	util_send ("[");
      (*arg) += 13;
      rc = fetch_header_fields (msg, arg, start, end);
    }

  else if (strncasecmp (*arg, "TEXT]", 5) == 0)
    {
      if (!silent)
	{
	  if (*section)
	    util_send ("[%sTEXT]", section);
	  else
	    util_send ("[TEXT]");
	}
      (*arg) += 5;
      rc = fetch_body_content (msg, start, end);
    }
  else if (**arg == ']')
    {
      if (!silent)
	util_send ("[%s]", section);
      (*arg)++;
      rc = fetch_body_content (msg, start, end);
    }
  else
    {
      util_send (" \"\"");/*FIXME: ERROR Message!*/
      rc = RESP_BAD;
    }
  free (section);
  return rc;
}

static int
fetch_message (mu_message_t msg, unsigned long start, unsigned long end)
{
  mu_stream_t stream = NULL;
  size_t size = 0, lines = 0;
  mu_message_get_stream (msg, &stream);
  mu_message_size (msg, &size);
  mu_message_lines (msg, &lines);
  return fetch_io (stream, start, end, size + lines);
}

static int
fetch_header (mu_message_t msg, unsigned long start, unsigned long end)
{
  mu_header_t header = NULL;
  mu_stream_t stream = NULL;
  size_t size = 0, lines = 0;
  mu_message_get_header (msg, &header);
  mu_header_size (header, &size);
  mu_header_lines (header, &lines);
  mu_header_get_stream (header, &stream);
  return fetch_io (stream, start, end, size + lines);
}

static int
fetch_body_content (mu_message_t msg, unsigned long start, unsigned long end)
{
  mu_body_t body = NULL;
  mu_stream_t stream = NULL;
  size_t size = 0, lines = 0;
  mu_message_get_body (msg, &body);
  mu_body_size (body, &size);
  mu_body_lines (body, &lines);
  mu_body_get_stream (body, &stream);
  return fetch_io (stream, start, end, size + lines);
}

static int
fetch_io (mu_stream_t stream, unsigned long start, unsigned long end,
	  unsigned long max)
{
  mu_stream_t rfc = NULL;
  size_t n = 0;
  mu_off_t offset;

  mu_filter_create (&rfc, stream, "rfc822", MU_FILTER_ENCODE, MU_STREAM_READ);

  if (start == ULONG_MAX || end == ULONG_MAX)
    {
      char buffer[512];
      offset = 0;
      if (max)
	{
	  util_send (" {%u}\r\n", max);
	  while (mu_stream_read (rfc, buffer, sizeof (buffer) - 1, offset,
			      &n) == 0 && n > 0)
	    {
	      buffer[n] = '\0';
	      util_send ("%s", buffer);
	      offset += n;
	    }
	}
      else
	util_send (" \"\"");
    }
  else if (end + 2 < end) /* Check for integer overflow */
    {
      return RESP_BAD;
    }
  else
    {
      char *buffer, *p;
      size_t total = 0;
      offset = start;
      p = buffer = calloc (end + 2, 1);
      while (end > 0
	     && mu_stream_read (rfc, buffer, end + 1, offset, &n) == 0 && n > 0)
	{
	  offset += n;
	  total += n;
	  end -= n;
	  buffer += n;
	}
      /* Make sure we null terminate.  */
      *buffer = '\0';
      util_send ("<%lu>", start);
      if (total)
	{
	  util_send (" {%s}\r\n", mu_umaxtostr (0, total));
	  util_send ("%s", p);
	}
      else
	util_send (" \"\"");
      free (p);
    }
  return RESP_OK;
}

static int
fetch_header_fields (mu_message_t msg, char **arg, unsigned long start,
		     unsigned long end)
{
  char *buffer = NULL;
  char **array = NULL;
  size_t array_len = 0;
  size_t off = 0;
  size_t lines = 0;
  mu_stream_t stream = NULL;
  int status;

  status = mu_memory_stream_create (&stream, 0, 0);
  if (status != 0)
    imap4d_bye (ERR_NO_MEM);

  /* Save the fields in an array.  */
  {
    char *field;
    char *sp = NULL;
    char *f = *arg;
    /* Find the end of the header.field tag.  */
    field = strchr (f, ']');
    if (field)
      {
	*field++ = '\0';
	*arg = field;
      }
    else
      *arg += strlen (*arg);

    for (;(field = util_getitem (f, " ()]\r\n", &sp)); f = NULL, array_len++)
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
    mu_header_t header = NULL;
    mu_message_get_header (msg, &header);
    for (j = 0; j < array_len; j++)
      {
	char *value = NULL;
	size_t n = 0;
	if (mu_header_aget_value (header, array[j], &value))
	    continue;

	n = asprintf (&buffer, "%s: %s\n", array[j], value);
	status = mu_stream_write (stream, buffer, n, off, &n);
	off += n;
	/* count the lines.  */
	{
	  char *nl = buffer;
	  for (;(nl = strchr (nl, '\n')); nl++)
	    lines++;
	}
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
  mu_stream_write (stream, "\n", 1, off, NULL);
  off++;
  lines++;

  /* Send the command back. The first braket was already sent.  */
  util_send ("HEADER.FIELDS");
  {
    size_t j;
    util_send (" (");
    for (j = 0; j < array_len; j++)
      {
	util_upper (array[j]);
	if (j)
	  util_send (" ");
	util_send_qstring (array[j]);
      }
    util_send (")");
    util_send ("]");
  }

  fetch_io (stream, start, end, off + lines);
  if (array)
    free (array);
  return RESP_OK;
}

static int
fetch_header_fields_not (mu_message_t msg, char **arg, unsigned long start,
			 unsigned long end)
{
  char **array = NULL;
  size_t array_len = 0;
  char *buffer = NULL;
  size_t off = 0;
  size_t lines = 0;
  mu_stream_t stream = NULL;
  int status;

  status = mu_memory_stream_create (&stream, 0, 0);
  if (status)
    imap4d_bye (ERR_NO_MEM);

  /* Save the field we want to ignore.  */
  {
    char *field;
    char *sp = NULL;
    char *f = *arg;
    /* Find the end of the header.field.no tag.  */
    field = strchr (f, ']');
    if (field)
      {
	*field++ = '\0';
	*arg = field;
      }
    else
      *arg += strlen (*arg);

    for (;(field = strtok_r (f, " ()\r\n", &sp)); f = NULL, array_len++)
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
    mu_header_t header = NULL;
    size_t count = 0;
    mu_message_get_header (msg, &header);
    mu_header_get_field_count (header, &count);
    for (i = 1; i <= count; i++)
      {
	char *name = NULL;
	char *value ;
	size_t n = 0;
	size_t ignore = 0;

	/* Get the field name.  */
	status = mu_header_aget_field_name (header, i, &name);
	if (*name == '\0')
	  {
	    free (name);
	    continue;
	  }

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

	if (mu_header_aget_field_value (header, i, &value) == 0)
	  {
	    char *nl;
	    
	    /* Save the field.  */
	    n = asprintf (&buffer, "%s: %s\n", name, value);
	    status = mu_stream_write (stream, buffer, n, off, &n);
	    off += n;
	    /* count the lines.  */
	    for (nl = buffer;(nl = strchr (nl, '\n')); nl++)
	      lines++;

	    free (value);
	  }
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
  mu_stream_write (stream, "\n", 1, off, NULL);
  off++;
  lines++;

  util_send ("HEADER.FIELDS.NOT");
  {
    size_t j;
    util_send (" (");
    for (j = 0; j < array_len; j++)
      {
	util_upper (array[j]);
	if (j)
	  util_send (" ");
	util_send_qstring (array[j]);
      }
    util_send (")");
    util_send ("]");
  }

  fetch_io (stream, start, end, off + lines);
  if (array)
    free (array);
  return RESP_OK;
}

static int
fetch_send_address (const char *addr)
{
  mu_address_t address;
  size_t i, count = 0;

  /* Short circuit.  */
  if (addr == NULL || *addr == '\0')
    {
      util_send ("NIL");
      return RESP_OK;
    }

  mu_address_create (&address, addr);
  mu_address_get_count (address, &count);

  /* We failed: can't parse.  */
  if (count == 0)
    {
      util_send ("NIL");
      return RESP_OK;
    }

  util_send ("(", count);
  for (i = 1; i <= count; i++)
    {
      const char *str;
      int is_group = 0;

      util_send ("(");

      mu_address_sget_personal (address, i, &str);
      util_send_qstring (str);
      util_send (" ");

      mu_address_sget_route (address, i, &str);
      util_send_qstring (str);

      util_send (" ");

      mu_address_is_group (address, i, &is_group);
      str = NULL;
      if (is_group)
	mu_address_sget_personal (address, i, &str);
      else
	mu_address_sget_local_part (address, i, &str);

      util_send_qstring (str);

      util_send (" ");

      mu_address_sget_domain (address, i, &str);
      util_send_qstring (str);

      util_send (")");
    }
  util_send (")");
  return RESP_OK;
}

/* Send parameter list for the bodystructure.  */
static void
send_parameter_list (const char *buffer)
{
  int argc = 0;
  char **argv;
  
  if (!buffer)
    {
      util_send ("NIL");
      return;
    }

  mu_argcv_get (buffer, " \t\r\n;=", NULL, &argc, &argv);
  
  if (argc == 0)
    util_send ("NIL");
  else
    {
      char *p;
      
      util_send ("(");
        
      p = argv[0];
      util_send_qstring (p);

      if (argc > 1)
	{
	  int i, space = 0;
	  char *lvalue = NULL;

	  util_send ("(");
	  for (i = 1; i < argc; i++)
	    {
	      if (lvalue)
		{
		  if (space)
		    util_send (" ");
		  util_send_qstring (lvalue);
		  lvalue = NULL;
		  space = 1;
		}
	      
	      switch (argv[i][0])
		{
		case ';':
		  continue;
		  
		case '=':
		  if (++i < argc)
		    {
		      char *p = argv[i];
		      util_send (" ");
		      util_unquote (&p);
		      util_send_qstring (p);
		    }
		  break;
		  
		default:
		  lvalue = argv[i];
		}
	    }
	  if (lvalue)
	    {
	      if (space)
		util_send (" ");
	      util_send_qstring (lvalue);
	    }
	  util_send (")");
	}
      else
	util_send (" NIL");
      util_send (")");
    }
  mu_argcv_free (argc, argv);
}
	  


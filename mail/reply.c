/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mail.h"

#define DATEBUFSIZE 128

static char *
concat (const char *s1, const char *s2)
{
  int len = (s1 ? strlen (s1) : 0) + (s2 ? strlen (s2) : 0) + 2;
  char *s = malloc (len);
  if (s)
    {
      char *p = s;
      
      if (s1)
	{
	  strcpy (p, s1);
	  p += strlen (s1);
	  *p++ = ' ';
	}
      if (s2)
	strcpy (p, s2);
    }
  return s;
}

void
make_in_reply_to (compose_env_t *env, message_t msg)
{
  char *value, *s1 = NULL, *s2 = NULL;
  header_t hdr;

  if (message_get_header (msg, &hdr))
    return;
  
  if (header_aget_value (hdr, MU_HEADER_DATE, &value))
    {
      envelope_t envelope = NULL;
      value = malloc (DATEBUFSIZE);
      if (value)
	{
	  message_get_envelope (msg, &envelope);
	  envelope_date (envelope, value, DATEBUFSIZE, NULL);
	}
    }

  if (value)
    asprintf (&s1, _("Your message of %s"), value);
  
  if (header_aget_value (hdr, MU_HEADER_MESSAGE_ID, &value) == 0)
    {
      asprintf (&s2, "\n\t%s", value);
      free (value);
    }

  if (s1 || s2)
    {
      value = concat (s1, s2);
      free (s1);
      free (s2);
      compose_header_set (env, MU_HEADER_IN_REPLY_TO, value,
			  COMPOSE_REPLACE);
      free (value);
    }      
}

int
strip_message_id (char *msgid, char **val)
{
  address_t addr;
  char *p = strchr (msgid, '<');
  if (!p)
    p = msgid;
  if (address_create (&addr, p) == 0)
    {
      size_t count = 0;
      char *p;
      
      address_get_count (addr, &count);
      if (count != 1)
	{
	  address_destroy (&addr);
	  return 1;
	}
      if (address_aget_email (addr, 1, &p))
	return -1;
      address_destroy (&addr);
      *val = malloc (strlen (p) + 3);
      if (!*val)
	{
	  free (p);
	  return -1;
	}
      sprintf (*val, "<%s>", p);
      free (p);
      return 0;
    }
  return 1;
}

int
get_msgid_header (header_t hdr, const char *name, char **val)
{
  char *p;
  int status = header_aget_value (hdr, name, &p);
  if (status)
    return status;
  status = strip_message_id (p, val);
  free (p);
  return status;
}

/* rfc2822:
   
   The "References:" field will contain the contents of the parent's
   "References:" field (if any) followed by the contents of the parent's
   "Message-ID:" field (if any).  If the parent message does not contain
   a "References:" field but does have an "In-Reply-To:" field
   containing a single message identifier, then the "References:" field
   will contain the contents of the parent's "In-Reply-To:" field
   followed by the contents of the parent's "Message-ID:" field (if
   any).  If the parent has none of the "References:", "In-Reply-To:",
   or "Message-ID:" fields, then the new message will have no
   References:" field. */

void
make_references (compose_env_t *env, message_t msg)
{
  char *ref = NULL, *msgid = NULL;
  header_t hdr;

  if (message_get_header (msg, &hdr))
    return;

  get_msgid_header (hdr, MU_HEADER_MESSAGE_ID, &msgid);
  if (get_msgid_header (hdr, MU_HEADER_REFERENCES, &ref))
    get_msgid_header (hdr, MU_HEADER_IN_REPLY_TO, &ref);

  if (ref || msgid)
    {
      char *s = concat (ref, msgid);
      free (ref);
      free (msgid);
      compose_header_set (env, MU_HEADER_REFERENCES, s, COMPOSE_REPLACE);
    }
}
  
/*
 * r[eply] [msglist] -- GNU extension
 * r[espond] [msglist] -- GNU extension
 * R[eply] [msglist]
 * R[espond] [msglist]
 */

int
mail_reply (int argc, char **argv)
{
  if (argc > 1)
    return util_msglist_command (mail_reply, argc, argv, 1);
  else
    {
      message_t msg;
      header_t hdr;
      compose_env_t env;
      int status;
      char *str;

      compose_init (&env);

      if (util_get_message (mbox, cursor, &msg, MSG_NODELETED))
	return 1;

      message_get_header (msg, &hdr);

      compose_header_set (&env, MU_HEADER_TO, util_get_sender (cursor, 0),
			  COMPOSE_SINGLE_LINE);

      if (islower (argv[0][0]))
	{
	  /* Add all recepients of the originate letter */

	  address_t addr = NULL;
	  size_t i, count = 0;
	  char buf[512];

	  if (header_aget_value (hdr, MU_HEADER_TO, &str) == 0)
	    {
	      address_create (&addr, str);
	      free (str);
	      address_get_count (addr, &count);
	    }

	  /* Make sure we do not include our alternate names */
	  for (i = 1; i <= count; i++)
	    {
	      address_get_email (addr, i, buf, sizeof (buf), NULL);
	      if (!mail_is_my_name (buf))
		compose_header_set (&env, MU_HEADER_TO,
				    buf,
				    COMPOSE_SINGLE_LINE);
	    }

	  /* Finally, add any Ccs */
	  if (header_aget_value (hdr, MU_HEADER_CC, &str) == 0)
	    compose_header_set (&env, MU_HEADER_TO, str, COMPOSE_SINGLE_LINE);
	}

      if (header_aget_value (hdr, MU_HEADER_SUBJECT, &str) == 0)
	{
	  char *p = NULL;

	  if (strncasecmp (str, "Re:", 3))
	    util_strcat (&p, "Re: ");
	  util_strcat (&p, str);
	  free (str);
	  compose_header_set (&env, MU_HEADER_SUBJECT, p, COMPOSE_REPLACE);
	  free (p);
	}
      else
	compose_header_set (&env, MU_HEADER_SUBJECT, "", COMPOSE_REPLACE);

      fprintf (ofile, _("To: %s\n"),
	       compose_header_get (&env, MU_HEADER_TO, ""));
      str = compose_header_get (&env, MU_HEADER_CC, NULL);
      if (str)
	fprintf (ofile, _("Cc: %s\n"), str);
      fprintf (ofile, _("Subject: %s\n\n"),
	       compose_header_get (&env, MU_HEADER_SUBJECT, ""));

      make_in_reply_to (&env, msg);
      make_references (&env, msg);
      status = mail_send0 (&env, 0);
      compose_destroy (&env);
      return status;
    }
  return 1;
}

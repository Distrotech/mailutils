/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/cctype.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/datetime.h>
#include <mailutils/util.h>
#include <mailutils/io.h>
#include <mailutils/envelope.h>
#include <mailutils/errno.h>
#include <mailutils/argcv.h>

#define ST_INIT  0
#define ST_MSGID 1

static int
strip_message_id (const char *msgid, char **pval)
{
  const char *p;
  char *q;
  int state;
  
  *pval = malloc (strlen (msgid) + 1);
  if (!*pval)
    return ENOMEM;
  state = ST_INIT;
  for (p = msgid, q = *pval; *p; p++)
    {
      switch (state)
	{
	case ST_INIT:
	  if (*p == '<')
	    {
	      *q++ = *p;
	      state = ST_MSGID;
	    }
	  else if (mu_isspace (*p))
	    *q++ = *p;
	  break;

	case ST_MSGID:
	  *q++ = *p;
	  if (*p == '>')
	    state = ST_INIT;
	  break;
	}
    }
  *q = 0;
  return 0;
}

static int
get_msgid_header (mu_header_t hdr, const char *name, char **val)
{
  const char *p;
  int status = mu_header_sget_value (hdr, name, &p);
  if (status)
    return status;
  return strip_message_id (p, val);
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

int
mu_rfc2822_references (mu_message_t msg, char **pstr)
{
  char *argv[3] = { NULL, NULL, NULL };
  mu_header_t hdr;
  int rc;
  
  rc = mu_message_get_header (msg, &hdr);
  if (rc)
    return rc;
  get_msgid_header (hdr, MU_HEADER_MESSAGE_ID, &argv[1]);
  if (get_msgid_header (hdr, MU_HEADER_REFERENCES, &argv[0]))
    get_msgid_header (hdr, MU_HEADER_IN_REPLY_TO, &argv[0]);

  if (argv[0] && argv[1])
    {
      rc = mu_argcv_join (2, argv, " ", mu_argcv_escape_no, pstr);
      free (argv[0]);
      free (argv[1]);
    }
  else if (argv[0])
    *pstr = argv[0];
  else if (argv[1])
    *pstr = argv[1];
  else
    rc = MU_ERR_FAILURE;
  return rc;
}

int
mu_rfc2822_msg_id (int subpart, char **pval)
{
  char date[4+2+2+2+2+2+1];
  time_t t = time (NULL);
  struct tm *tm = localtime (&t);
  char *host;
  char *p;
	  
  mu_strftime (date, sizeof date, "%Y%m%d%H%M%S", tm);
  mu_get_host_name (&host);

  if (subpart)
    {
      struct timeval tv;
      gettimeofday (&tv, NULL);
      mu_asprintf (&p, "<%s.%lu.%d@%s>",
		   date,
		   (unsigned long) getpid (),
		   subpart,
		   host);
    }
  else
    mu_asprintf (&p, "<%s.%lu@%s>", date, (unsigned long) getpid (), host);
  free (host);
  *pval = p;
  return 0;
}

#define COMMENT "Your message of "

/*
   The "In-Reply-To:" field will contain the contents of the "Message-
   ID:" field of the message to which this one is a reply (the "parent
   message").  If there is more than one parent message, then the "In-
   Reply-To:" field will contain the contents of all of the parents'
   "Message-ID:" fields.  If there is no "Message-ID:" field in any of
   the parent messages, then the new message will have no "In-Reply-To:"
   field.
*/
int
mu_rfc2822_in_reply_to (mu_message_t msg, char **pstr)
{
  const char *argv[] = { NULL, NULL, NULL, NULL, NULL };
  mu_header_t hdr;
  int rc;
  int idx = 0;
  
  rc = mu_message_get_header (msg, &hdr);
  if (rc)
    return rc;
  
  if (mu_header_sget_value (hdr, MU_HEADER_DATE, &argv[idx + 1]))
    {
      mu_envelope_t envelope = NULL;
      mu_message_get_envelope (msg, &envelope);
      mu_envelope_sget_date (envelope, &argv[idx + 1]);
    }

  if (argv[idx + 1])
    {
      argv[idx] = COMMENT;
      idx = 2;
    }
    
  if (mu_header_sget_value (hdr, MU_HEADER_MESSAGE_ID, &argv[idx]) == 0)
    {
      if (idx > 1)
	{
	  argv[idx + 1] = argv[idx];
	  argv[idx] = "\n\t";
	  idx++;
	}
      idx++;
    }

  if (idx > 1)
    rc = mu_argcv_join (idx, (char**) argv, "", mu_argcv_escape_no, pstr);
  else
    rc = MU_ERR_FAILURE;
  return rc;
}

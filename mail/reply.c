/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mail.h"

void
make_in_reply_to (compose_env_t *env, message_t msg)
{
  char *value = NULL;

  mu_rfc2822_in_reply_to (msg, &value);
  compose_header_set (env, MU_HEADER_IN_REPLY_TO, value,
		      COMPOSE_REPLACE);
  free (value);
}

void
make_references (compose_env_t *env, message_t msg)
{
  char *value = NULL;

  mu_rfc2822_references (msg, &value);
  compose_header_set (env, MU_HEADER_REFERENCES, value, COMPOSE_REPLACE);
  free (value);
}
  
/*
 * r[eply] [msglist] -- GNU extension
 * r[espond] [msglist] -- GNU extension
 * R[eply] [msglist]
 * R[espond] [msglist]
 */

int
reply0 (msgset_t *mspec, message_t msg, void *data)
{
  header_t hdr;
  compose_env_t env;
  int status;
  char *str;

  cursor = mspec->msg_part[0];
  
  compose_init (&env);

  message_get_header (msg, &hdr);

  compose_header_set (&env, MU_HEADER_TO,
		      util_get_sender (mspec->msg_part[0], 0),
		      COMPOSE_SINGLE_LINE);

  if (*(int*) data) /* reply starts with a lowercase */
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
	  if ((util_getenv (NULL, "metoo", Mail_env_boolean, 0) == 0)
	      || !mail_is_my_name (buf))
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
      
      if (munre_subject (str, NULL))
	util_strcat (&p, util_reply_prefix ());
      util_strcat (&p, str);
      free (str);
      compose_header_set (&env, MU_HEADER_SUBJECT, p, COMPOSE_REPLACE);
      free (p);
    }
  else
    compose_header_set (&env, MU_HEADER_SUBJECT, "", COMPOSE_REPLACE);

  fprintf (ofile, "To: %s\n",
	   compose_header_get (&env, MU_HEADER_TO, ""));
  str = compose_header_get (&env, MU_HEADER_CC, NULL);
  if (str)
    fprintf (ofile, "Cc: %s\n", str);
  fprintf (ofile, "Subject: %s\n\n",
	   compose_header_get (&env, MU_HEADER_SUBJECT, ""));
  
  make_in_reply_to (&env, msg);
  make_references (&env, msg);
  status = mail_send0 (&env, 0);
  compose_destroy (&env);

  return status;
}

int
mail_reply (int argc, char **argv)
{
  int lower = islower (argv[0][0]);
  if (util_getenv (NULL, "flipr", Mail_env_boolean, 0) == 0)
    lower = !lower;
  return util_foreach_msg (argc, argv, MSG_NODELETED, reply0, &lower);
}


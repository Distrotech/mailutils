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

#include "mail.h"

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

      if (mailbox_get_message (mbox, cursor, &msg))
	{
	  util_error ("%d: can't get message", cursor);
	  return 1;
	}

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
	  header_aget_value (hdr, MU_HEADER_CC, &str);
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

      fprintf (ofile, "To: %s\n",
	       compose_header_get (&env, MU_HEADER_TO, ""));
      str = compose_header_get (&env, MU_HEADER_CC, NULL);
      if (str)
	fprintf (ofile, "Cc: %s\n", str);
      fprintf (ofile, "Subject: %s\n\n",
	       compose_header_get (&env, MU_HEADER_SUBJECT, ""));

      status = mail_send0 (&env, 0);
      compose_destroy (&env);
      return status;
    }
  return 1;
}

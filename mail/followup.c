/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003 Free Software Foundation, Inc.

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

/*
 * fo[llowup] [msglist] -- GNU extension
 * F[ollowup] [msglist]
 */

int
mail_followup (int argc, char **argv)
{
  message_t msg;
  header_t hdr;
  char *str;
  msgset_t *msglist, *mp;
  compose_env_t env;
  int status;

  compose_init (&env);
  if (msgset_parse (argc, argv, MSG_NODELETED, &msglist))
    return 1;

  if (util_get_message (mbox, cursor, &msg))
    {
      msgset_free (msglist);
      return 1;
    }

  /* Create subject value */
  message_get_header (msg, &hdr);
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

  /* Generate "to" list */
  compose_header_set (&env, MU_HEADER_TO, util_get_sender (cursor, 0),
		      COMPOSE_SINGLE_LINE);

  /* Add authors of the subsequent messages to the to list
     (or should it be cc?) */
  for (mp = msglist; mp; mp = mp->next)
    compose_header_set (&env, MU_HEADER_TO,
			util_get_sender (mp->msg_part[0], 0),
			COMPOSE_SINGLE_LINE);

  msgset_free (msglist);

  fprintf (ofile, "To: %s\n", compose_header_get (&env, MU_HEADER_TO, ""));
  fprintf (ofile, "Subject: %s\n\n",
	   compose_header_get (&env, MU_HEADER_SUBJECT, ""));

  status = mail_send0 (&env, isupper (argv[0][0]));
  compose_destroy (&env);
  return status;
}

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
 * fo[llowup] [msglist] -- GNU extension
 * F[ollowup] [msglist]
 */

int
mail_followup (int argc, char **argv)
{
  message_t msg;
  header_t hdr;
  char *str;
  int i, num, *msglist;
  struct send_environ env;
  int status;

  env.to = env.cc = env.bcc = env.subj = NULL;
  env.outfiles = NULL; env.nfiles = 0;
  num = util_expand_msglist (argc, argv, &msglist);

  if (mailbox_get_message(mbox, cursor, &msg))
    {
      util_error("%d: can't get message", cursor);
      free(msglist);
      return 1;
    }

  /* Create subject value */
  message_get_header(msg, &hdr);
  header_aget_value(hdr, MU_HEADER_SUBJECT, &str);
  util_strcat(&env.subj, "Re: ");
  util_strcat(&env.subj, str);
  free(str);

  /* Generate "to" list */
  env.to = util_get_sender(cursor, 0);

  /* Add authors of the subsequent messages to the to list
     (or should it be cc?)*/
  for (i = 1; i < num; i++)
    {
      util_strcat(&env.to, ",");
      util_strcat(&env.to, util_get_sender(msglist[i], 0));
    }

  free(msglist);

  fprintf(ofile, "To: %s\n", env.to);
  fprintf(ofile, "Subject: %s\n\n", env.subj);

  status = mail_send0(&env, isupper(argv[0][0]));
  free_env_headers (&env);
  return status;
}

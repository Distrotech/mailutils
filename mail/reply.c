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
mail_reply(int argc, char **argv)
{
  if (argc > 1)
    return util_msglist_command(mail_reply, argc, argv, 1);
  else
    {
      message_t msg;
      header_t hdr;
      char *to = NULL, *cc = NULL, *subj = NULL;
      char *str;
      
      if (mailbox_get_message(mbox, cursor, &msg))
	{
	  fprintf(ofile, "%d: can't get message\n", cursor);
	  return 1;
	}

      message_get_header(msg, &hdr);

      to = util_get_sender(cursor, 0);
      if (islower(argv[0][0]))
	{
	  /* Add all recepients of the originate letter */

	  address_t addr = NULL;
	  size_t i, count = 0;
	  char buf[512];
	  
	  header_aget_value(hdr, MU_HEADER_TO, &str);

	  address_create(&addr, str);
	  free(str);
	  address_get_count(addr, &count);

	  /* Make sure we do not include our alternate names */
	  for (i = 1; i <= count; i++)
	    {
	      address_get_email(addr, i, buf, sizeof(buf), NULL);
	      if (!mail_is_alt_name(buf))
		{
		  util_strcat(&to, " ");
		  util_strcat(&to, buf);
		}
	    }

	  /* Finally, add any Ccs */
	  header_aget_value(hdr, MU_HEADER_CC, &cc);
	}

      header_aget_value(hdr, MU_HEADER_SUBJECT, &str);
      util_strcat(&subj, "Re: ");
      util_strcat(&subj, str);
      free(str);

      fprintf(ofile, "To: %s\n", to);
      if (cc)
	fprintf(ofile, "Cc: %s\n", cc);
      fprintf(ofile, "Subject: %s\n\n", subj);
      
      return mail_send0(to, cc, NULL, subj, 0);
    }
  return 1;
}

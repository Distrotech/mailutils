/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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
 * f[rom] [msglist]
 */

int
mail_from (int argc, char **argv)
{
  if (argc > 1)
    {
      return util_msglist_command (mail_from, argc, argv);
    }
  else 
    {
      message_t msg;
      header_t hdr;
      char *from, *subj;
      int froml, subjl;
      int columns = 74;
      char format[64];
      char *col = getenv("COLUMNS");

      if (col)
	columns = strtol (col, NULL, 10) - 5;
      
      if (mailbox_get_message (mbox, cursor, &msg) != 0 ||
	  message_get_header (msg, &hdr) != 0)
	return 1;

      froml = columns / 3;
      subjl = columns * 2 / 3;
      if (froml + subjl > columns)
	subjl--;

      from = malloc (froml * sizeof (char));
      subj = malloc (subjl * sizeof (char));

      if (from == NULL || subj == NULL)
	{
	  free (from);
	  free (subj);
	  return 1;
	}

      header_get_value (hdr, MU_HEADER_FROM, from, froml, NULL);
      header_get_value (hdr, MU_HEADER_SUBJECT, subj, subjl, NULL);

      snprintf (format, 64, "%%c%%2d %%-%ds%%-%ds\n", froml, subjl);
      printf (format, cursor == realcursor ? '>' : ' ', cursor, from, subj);

      free (from);
      free (subj);

      return 0;
    }
  return 1;
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
 * f[rom] [msglist]
 */

int
mail_from (int argc, char **argv)
{
  if (argc > 1)
    return util_msglist_command (mail_from, argc, argv, 0);
  else
    return mail_from0 (cursor, 1);
  return 1;
}

int
mail_from0 (int msgno, int verbose)
{
  message_t msg;
  header_t hdr = NULL;
  envelope_t env;
  attribute_t attr;
  char *from = NULL, *subj = NULL, *fromp, *subjp;
  int froml, subjl;
  char date[80], st[10];
  int cols = util_getcols () - 6;
  int cflag;
  size_t m_size = 0, m_lines = 0;
  const char *p;
  struct tm tm;
  mu_timezone tz;

  if (util_get_message (mbox, msgno, &msg,
			MSG_NODELETED|(verbose ? 0 : MSG_SILENT)))
    return 1;

  message_get_header (msg, &hdr);
  if (header_aget_value (hdr, MU_HEADER_FROM, &from) == 0)
    {
      address_t address = NULL;
      if (address_create (&address, from) == 0)
	{
	  char name[128];
	  size_t len = strlen (from);
	  *name = '\0';
	  address_get_personal (address, 1, name, sizeof name, NULL);
	  if (*name && len)
	    {
	      strncpy (from, name, len - 1);
	      from[len - 1] = '\0';
	    }
	  else
	    address_get_email (address, 1, from, strlen (from), NULL);
	  address_destroy (&address);
	}
    }
  header_aget_value (hdr, MU_HEADER_SUBJECT, &subj);
  
  message_get_attribute (msg, &attr);
  
  if (attribute_is_userflag(attr, MAIL_ATTRIBUTE_MBOXED))
    cflag = 'M';
  else if (attribute_is_userflag(attr, MAIL_ATTRIBUTE_SAVED))
    cflag = '*';
  else if (attribute_is_userflag(attr, MAIL_ATTRIBUTE_TAGGED))
    cflag = 'T';
  else if (attribute_is_read (attr))
    cflag = 'R';
  else if (attribute_is_seen (attr))
    cflag = 'U';
  else if (attribute_is_recent(attr))
    cflag = 'N';
  else
    cflag = ' ';
  
  message_get_envelope (msg, &env);
  envelope_date (env, date, sizeof (date), NULL);
  p = date;
  if (mu_parse_ctime_date_time(&p, &tm, &tz) == 0)
    strftime (date, sizeof(date), "%a %b %e %H:%M", &tm);
  
  message_size (msg, &m_size);
  message_lines (msg, &m_lines);
  
  snprintf (st, sizeof(st), "%3d/%-5d", m_lines, m_size);
  
  /* The "From" field will take a third of the screen.
     Subject will take the rest.
     FIXME: This is not quite correct that we use fixed sizes
     18, 16 for the other fields.
  */
  froml = cols / 3;
  subjl = cols - froml - strlen (st) - strlen (date);
  
  fromp = from ? from : "";
  subjp = subj ? subj : fromp;
  fprintf (ofile, "%c%c%4d %-18.18s %-16.16s %s %.*s\n",
	   msgno == realcursor ? '>' : ' ', cflag, msgno,
	   fromp, date, st, (subjl < 0) ? 0 : subjl, subjp);
  
  free (from);
  free (subj);
  
  return 0;
}

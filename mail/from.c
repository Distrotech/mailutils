/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 
   2005 Free Software Foundation, Inc.

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

/*
 * f[rom] [msglist]
 */

int
mail_from0 (msgset_t *mspec, message_t msg, void *data)
{
  header_t hdr = NULL;
  envelope_t env;
  attribute_t attr;
  char *from = NULL, *subj = NULL, *fromp, *subjp;
  int froml, subjl;
  char date[80], st[10];
  int cols = util_getcols () - 6;
  int cflag;
  size_t m_size = 0, m_lines = 0;

  message_get_header (msg, &hdr);
  if (header_aget_value_unfold (hdr, MU_HEADER_FROM, &from) == 0)
    {
      address_t address = NULL;
      if (address_create (&address, from) == 0)
	{
	  char name[128];
	  size_t len;
	  char *email;
	  
	  if (address_aget_email (address, 1, &email) == 0)
	    {
	      if (util_getenv (NULL, "showto", Mail_env_boolean, 0) == 0
		  && mail_is_my_name (email))
		{
		  char *tmp;

		  if (header_aget_value_unfold (hdr, MU_HEADER_TO, &tmp) == 0)
		    {
		      address_t addr_to;
		      if (address_create (&addr_to, tmp) == 0)
			{
			  address_destroy (&address);
			  address = addr_to;
			}
		      free (tmp);
		    }
		}
	      free (email);
	    }
	      
	  len = strlen (from) + 1;
	  *name = '\0';
	  address_get_personal (address, 1, name, sizeof name, NULL);
	  if (*name && len)
	    {
	      strncpy (from, name, len - 1);
	      from[len - 1] = '\0';
	    }
	  else
	    address_get_email (address, 1, from, len, NULL);
	  address_destroy (&address);
	}
    }
  util_rfc2047_decode (&from);

  header_aget_value_unfold (hdr, MU_HEADER_SUBJECT, &subj);
  util_rfc2047_decode (&subj);
  
  message_get_attribute (msg, &attr);
  
  if (attribute_is_userflag (attr, MAIL_ATTRIBUTE_MBOXED))
    cflag = 'M';
  else if (attribute_is_userflag (attr, MAIL_ATTRIBUTE_SAVED))
    cflag = '*';
  else if (attribute_is_userflag (attr, MAIL_ATTRIBUTE_TAGGED))
    cflag = 'T';
  else if (attribute_is_read (attr))
    cflag = 'R';
  else if (attribute_is_seen (attr))
    cflag = 'U';
  else if (attribute_is_recent (attr))
    cflag = 'N';
  else
    cflag = ' ';

  date[0] = 0;
  if (util_getenv (NULL, "datefield", Mail_env_boolean, 0) == 0
      && header_get_value (hdr, MU_HEADER_DATE, date, sizeof (date), NULL) == 0)
    {
      time_t t;
      if (mu_parse_date (date, &t, NULL) == 0)
	{
	  strftime (date, sizeof(date), "%a %b %e %H:%M", localtime (&t));
	}
      else
	date[0] = 0;
    }

  if (date[0] == 0)
    {
      const char *p;
      struct tm tm;
      mu_timezone tz;

      message_get_envelope (msg, &env);
      envelope_date (env, date, sizeof (date), NULL);
      p = date;
      if (mu_parse_ctime_date_time (&p, &tm, &tz) == 0)
	strftime (date, sizeof(date), "%a %b %e %H:%M", &tm);
    }
  
  message_size (msg, &m_size);
  message_lines (msg, &m_lines);
  
  snprintf (st, sizeof (st), "%3d/%-5d", m_lines, m_size);
  
  /* The "From" field will take a third of the screen.
     Subject will take the rest.
     FIXME: This is not quite correct that we use fixed sizes
     18, 16 for the other fields.
  */
  froml = cols / 3;
  subjl = cols - froml - strlen (st) - 16;
  
  fromp = from ? from : "";
  subjp = subj ? subj : fromp;
  fprintf (ofile, "%c%c%4d %-18.18s %-16.16s %s %.*s\n",
	   mspec->msg_part[0] == cursor ? '>' : ' ', cflag,
	   mspec->msg_part[0],
	   fromp, date, st, (subjl < 0) ? 0 : subjl, subjp);
  
  free (from);
  free (subj);

  return 0;
}

int
mail_from (int argc, char **argv)
{
  return util_foreach_msg (argc, argv, MSG_NODELETED|MSG_SILENT, mail_from0, NULL);
}


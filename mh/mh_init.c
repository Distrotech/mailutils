/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

/* Initialize MH applications. */

#include <mh.h>
#include <pwd.h>

void
mh_init ()
{
  list_t bookie;
  registrar_get_list (&bookie);
  list_append (bookie, mh_record);
  list_append (bookie, mbox_record);
  list_append (bookie, path_record);
  list_append (bookie, pop_record);
  list_append (bookie, imap_record);
  /* Possible supported mailers.  */
  list_append (bookie, sendmail_record);
  list_append (bookie, smtp_record);
}
     

static char *my_name;
static char *my_email;

/* FIXME: this lacks domain name part! */
void
mh_get_my_name (char *name)
{
  char hostname[256];

  if (!name)
    {
      struct passwd *pw = getpwuid (getuid ());
      if (!pw)
	{
	  mh_error ("can't determine my username");
	  return;
	}
      name = pw->pw_name;
    }

  my_name = strdup (name);
  gethostname(hostname, sizeof(hostname));
  hostname[sizeof(hostname)-1] = 0;
  my_email = xmalloc (strlen (name) + strlen (hostname) + 2);
  sprintf (my_email, "%s@%s", name, hostname);
}


int
mh_is_my_name (char *name)
{
  if (!my_email)
    mh_get_my_name (NULL);
  return strcasecmp (name, my_email) == 0;
}


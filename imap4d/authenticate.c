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

#include "imap4d.h"

extern int auth_gssapi __P((struct imap4d_command *, char **username));

struct imap_auth {
  char *name;
  int (*handler) __P((struct imap4d_command *, char **));
} imap_auth_tab[] = {
#ifdef WITH_GSSAPI
  { "GSSAPI", auth_gssapi },
#endif  
  { NULL, NULL }
};

int
imap4d_authenticate (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *auth_type;
  struct imap_auth *ap;
  char *username = NULL;
  
  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");

  auth_type = util_getword (arg, &sp);
  util_unquote (&auth_type);
  if (!auth_type)
    return util_finish (command, RESP_BAD, "Too few arguments");

  for (ap = imap_auth_tab; ap->name; ap++)
    if (strcmp (auth_type, ap->name) == 0)
      {
	if (ap->handler (command, &username))
	  return 1;
      }

  if (username)
    {
        struct passwd *pw = mu_getpwnam (username);
	if (pw == NULL)
	  return util_finish (command, RESP_NO,
			      "User name or passwd rejected");

	if (pw->pw_uid > 0 && !mu_virtual_domain)
	  setuid (pw->pw_uid);

	homedir = mu_normalize_path (strdup (pw->pw_dir), "/");
	/* FIXME: Check for errors.  */
	chdir (homedir);
	namespace_init(pw->pw_dir);
	syslog (LOG_INFO, "User '%s' logged in", username);
	return 0;
    }
      
  return util_finish (command, RESP_NO,
		      "Authentication mechanism not supported");
}


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

/*
 * FIXME: this should support PAM, shadow, and normal password
 */

int
imap4d_login (struct imap4d_command *command, char *arg)
{
  struct passwd *pw;
  char *sp = NULL, *username, *pass;

  username = util_getword (arg, &sp);
  pass = util_getword (NULL, &sp);

  if (username == NULL || pass == NULL)
    return util_finish (command, RESP_NO, "Too few args");
  else if (util_getword (NULL, &sp))
    return util_finish (command, RESP_NO, "Too many args");

  pw = getpwnam (arg);
  if (pw == NULL || pw->pw_uid < 1)
    return util_finish (command, RESP_NO, "User name or passwd rejected");
  if (strcmp (pw->pw_passwd, crypt (pass, pw->pw_passwd)))
    {
#ifdef HAVE_SHADOW_H
      struct spwd *spw;
      spw = getspnam (arg);
      if (spw == NULL || strcmp (spw->sp_pwdp, crypt (pass, spw->sp_pwdp)))
#endif /* HAVE_SHADOW_H */
	return util_finish (command, RESP_NO, "User name or passwd rejected");
    }

  if (pw->pw_uid > 1)
    setuid (pw->pw_uid);
  return util_finish (command, RESP_OK, "Completed");
}

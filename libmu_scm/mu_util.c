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

#include "mu_scm.h"
#include <pwd.h>

SCM_DEFINE (mu_scm_getpwuid, "mu_getpwuid", 0, 1, 0,
            (SCM USER),
	    "Look up an entry in the user database. USER can be an integer,\n"
	    "a string, or omitted, giving the behaviour of mu_getpwuid, mu_getpwnam\n"
	    "or (system) getpwent respectively.\n"
	    "Returns a vector with fields corresponding to those of the passwd\n"
	    "entry in question. If no matching entry was found, returns #f.\n")
#define FUNC_NAME s_mu_scm_getpwuid
{
  SCM result;
  struct passwd *entry;
  SCM *ve;

  result = scm_make_vector (SCM_MAKINUM (7), SCM_UNSPECIFIED);
  ve = SCM_VELTS (result);
  if (SCM_UNBNDP (USER) || SCM_FALSEP (USER))
    {
      entry = getpwent ();
    }
  else if (SCM_INUMP (USER))
    {
      entry = mu_getpwuid (SCM_INUM (USER));
    }
  else
    {
      SCM_VALIDATE_ROSTRING (1, USER);
      if (SCM_SUBSTRP (USER))
	USER = scm_makfromstr (SCM_ROCHARS (USER), SCM_ROLENGTH (USER), 0);
      entry = mu_getpwnam (SCM_ROCHARS (USER));
    }
  if (!entry)
    return SCM_BOOL_F;

  ve[0] = scm_makfrom0str (entry->pw_name);
  ve[1] = scm_makfrom0str (entry->pw_passwd);
  ve[2] = scm_ulong2num ((unsigned long) entry->pw_uid);
  ve[3] = scm_ulong2num ((unsigned long) entry->pw_gid);
  ve[4] = scm_makfrom0str (entry->pw_gecos);
  if (!entry->pw_dir)
    ve[5] = scm_makfrom0str ("");
  else
    ve[5] = scm_makfrom0str (entry->pw_dir);
  if (!entry->pw_shell)
    ve[6] = scm_makfrom0str ("");
  else
    ve[6] = scm_makfrom0str (entry->pw_shell);
  return result;
}
#undef FUNC_NAME

void
mu_scm_mutil_init ()
{
#include "mu_util.x"
}

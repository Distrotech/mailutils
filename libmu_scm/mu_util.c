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

#include "mu_scm.h"

SCM_DEFINE (mu_scm_getpwuid, "mu-getpwuid", 1, 0, 0,
            (SCM USER),
	    "Look up an entry in the user database. USER can be an integer,\n"
	    "or a string, giving the behaviour of mu_get_auth_by_uid or mu_get_auth_by_name\n"
	    "respectively.\n"
	    "Returns a vector with fields corresponding to those of the mu_auth_data\n"
	    "entry in question. If no matching entry was found, returns #f.\n")
#define FUNC_NAME s_mu_scm_getpwuid
{
  SCM result;
  struct mu_auth_data *entry;
  SCM *ve;

  result = scm_make_vector (SCM_MAKINUM (8), SCM_UNSPECIFIED);
  ve = SCM_VELTS (result);
  if (SCM_INUMP (USER))
    {
      entry = mu_get_auth_by_uid (SCM_INUM (USER));
    }
  else
    {
      SCM_VALIDATE_ROSTRING (1, USER);
      if (SCM_SUBSTRP (USER))
	USER = scm_makfromstr (SCM_ROCHARS (USER), SCM_ROLENGTH (USER), 0);
      entry = mu_get_auth_by_name (SCM_ROCHARS (USER));
    }
  if (!entry)
    return SCM_BOOL_F;

  ve[0] = scm_makfrom0str (entry->name);
  ve[1] = scm_makfrom0str (entry->passwd);
  ve[2] = scm_ulong2num ((unsigned long) entry->uid);
  ve[3] = scm_ulong2num ((unsigned long) entry->gid);
  ve[4] = scm_makfrom0str (entry->gecos);
  if (!entry->dir)
    ve[5] = scm_makfrom0str ("");
  else
    ve[5] = scm_makfrom0str (entry->dir);
  if (!entry->shell)
    ve[6] = scm_makfrom0str ("");
  else
    ve[6] = scm_makfrom0str (entry->shell);
  ve[7] = scm_makfrom0str (entry->mailbox);
  mu_auth_data_free (entry);
  return result;
}
#undef FUNC_NAME

void
mu_scm_mutil_init ()
{
#include "mu_util.x"
}

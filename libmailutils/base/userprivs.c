/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2008, 2010-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <mailutils/assoc.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>

/* Switch to the given UID/GID */
int
mu_set_user_privileges (uid_t uid, gid_t *gidv, size_t gidc)
{
  int rc = 0;
  gid_t gid;

  if (getuid ())
    return EACCES;
  
  if (uid == 0)
    return 0;

  /* Reset group permissions */
  if (gidv && gidc)
    {
      if (geteuid () == 0 && setgroups (gidc, gidv))
	{
	  mu_error(_("setgroups(1, %lu) failed: %s"),
		   (unsigned long) gidv[0], mu_strerror (errno));
	  return errno;
	}
      gid = gidv[0];
    }
  else
    {
      struct passwd *pwd = getpwuid (uid);
      if (pwd)
	gid = pwd->pw_gid;
      else
	gid = getegid ();
    }

  /* Switch to the user's gid. On some OSes the effective gid must
     be reset first */

#if defined(HAVE_SETEGID)
  if (setegid (gid) < 0)
    {
      rc = errno;
      mu_error (_("setegid(%lu) failed: %s"),
		(unsigned long) gid, mu_strerror (rc));
    }
#elif defined(HAVE_SETREGID)
  if (setregid (gid, gid) < 0)
    {
      rc = errno;
      mu_error (_("setregid(%lu,%lu) failed: %s"),
		(unsigned long) gid, (unsigned long) gid,
		mu_strerror (rc));
    }
#elif defined(HAVE_SETRESGID)
  if (setresgid (gid, gid, gid) < 0)
    {
      rc = errno;
      mu_error (_("setresgid(%lu,%lu,%lu) failed: %s"),
		(unsigned long) gid,
		(unsigned long) gid,
		(unsigned long) gid,
		mu_strerror (rc));
    }
#endif

  if (rc == 0 && gid != 0)
    {
      if (setgid (gid) < 0)
	{
	  rc = errno;
	  mu_error (_("setgid(%lu) failed: %s"),
		    (unsigned long) gid, mu_strerror (rc));
	}
      else if (getegid () != gid)
	{
	  rc = MU_ERR_FAILURE;
	  mu_error (_("setgid(%lu) failed: %s"),
		    (unsigned long) gid, mu_strerror (rc));
	}
      
      if (rc == 0 && getegid () != gid)
	{
	  mu_error (_("Cannot set effective gid to %lu"),
		    (unsigned long) gid);
	  rc = MU_ERR_FAILURE;
	}
    }

  /* Now reset uid */
  if (rc == 0 && uid != 0)
    {
      uid_t euid;

      if (setuid (uid) || geteuid () != uid
	  || (getuid () != uid && (geteuid () == 0 || getuid () == 0)))
	{
#if defined(HAVE_SETREUID)
	  if (geteuid () != uid)
	    {
	      if (setreuid (uid, -1) < 0)
		{
		  rc = errno;
		  mu_error (_("setreuid(%lu,-1) failed: %s"),
			    (unsigned long) uid,
			    mu_strerror (rc));
		}
	      if (setuid (uid) < 0)
		{
		  rc = errno;
		  mu_error (_("second setuid(%lu) failed: %s"),
			    (unsigned long) uid, mu_strerror (rc));
		}
	    } else
#endif
	        {
		  rc = errno;
		  mu_error (_("setuid(%lu) failed: %s"),
			    (unsigned long) uid,
			    mu_strerror (rc));
		}
	}
	
      euid = geteuid ();
      if (uid != 0 && setuid (0) == 0)
	{
	  mu_error (_("seteuid(0) succeeded when it should not"));
	  rc = MU_ERR_FAILURE;
	}
      else if (uid != euid && setuid (euid) == 0)
	{
	  mu_error (_("Cannot drop non-root setuid privileges"));
	  rc = MU_ERR_FAILURE;
	}
    }
  return rc;
}

int
mu_switch_to_privs (uid_t uid, gid_t gid, mu_list_t retain_groups)
{
  int rc = 0;
  gid_t *emptygidset;
  size_t size = 1, j = 1;
  mu_iterator_t itr;

  if (uid == 0)
    return 0;

  /* Create a list of supplementary groups */
  mu_list_count (retain_groups, &size);
  size++;
  emptygidset = malloc (size * sizeof emptygidset[0]);
  if (!emptygidset)
    return ENOMEM;
  emptygidset[0] = gid ? gid : getegid ();

  if (mu_list_get_iterator (retain_groups, &itr) == 0)
    {
      for (mu_iterator_first (itr);
	   !mu_iterator_is_done (itr); mu_iterator_next (itr)) 
	mu_iterator_current (itr,
			     (void **)(emptygidset + j++));
      mu_iterator_destroy (&itr);
    }
  rc = mu_set_user_privileges (uid, emptygidset, j);
  free (emptygidset);
  return rc;
}


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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mu_scm.h"

typedef int (*address_get_fp) __P((address_t, size_t, char *, size_t, size_t *));

static SCM
_get_address_part (const char *func_name, address_get_fp fun,
		   SCM ADDRESS, SCM NUM)
{
  address_t addr;
  int length;
  char *str;
  SCM ret;
  int num;
  
  SCM_ASSERT (SCM_NIMP (ADDRESS) && SCM_STRINGP (ADDRESS),
	      ADDRESS, SCM_ARG1, func_name);

  if (!SCM_UNBNDP (NUM))
    {
      SCM_ASSERT (SCM_IMP (NUM) && SCM_INUMP (NUM),
		  NUM, SCM_ARG1, func_name);
      num = SCM_INUM (NUM);
    }
  else
    num = 1;

  str = SCM_STRING_CHARS (ADDRESS);
  length = strlen (str);
  if (length == 0)
    return scm_makfrom0str("");
  
  if (address_create (&addr, SCM_STRING_CHARS (ADDRESS)))
    return SCM_BOOL_F;

  str = malloc (length + 1);
  if (!str)
    {
      address_destroy (&addr);
      return SCM_BOOL_F;
    }

  if ((*fun) (addr, num, str, length + 1, NULL) == 0)
    ret = scm_makfrom0str (str);
  else
    ret = SCM_BOOL_F;
  address_destroy (&addr);
  free (str);
  return ret;
}

SCM_DEFINE (mu_address_get_personal, "mu-address-get-personal", 1, 1, 0,
	    (SCM ADDRESS, SCM NUM),
	    "Return personal part of an email address.\n")
#define FUNC_NAME s_mu_address_get_personal
{
  return _get_address_part (FUNC_NAME, 
			    address_get_personal, ADDRESS, NUM);
}
#undef FUNC_NAME

SCM_DEFINE (mu_address_get_comments, "mu-address-get-comments", 1, 1, 0,
	    (SCM ADDRESS, SCM NUM),
	    "Return comment part of an email address.\n")
#define FUNC_NAME s_mu_address_get_comments
{
  return _get_address_part (FUNC_NAME, 
			    address_get_comments, ADDRESS, NUM);
}
#undef FUNC_NAME

SCM_DEFINE (mu_address_get_email, "mu-address-get-email", 1, 1, 0,
	    (SCM ADDRESS, SCM NUM),
	    "Return email part of an email address.\n")
#define FUNC_NAME s_mu_address_get_email
{
  return _get_address_part (FUNC_NAME, 
			    address_get_email, ADDRESS, NUM);
}
#undef FUNC_NAME

SCM_DEFINE (mu_address_get_domain, "mu-address-get-domain", 1, 1, 0,
	    (SCM ADDRESS, SCM NUM),
	    "Return domain part of an email address.\n")
#define FUNC_NAME s_mu_address_get_domain
{
  return _get_address_part (FUNC_NAME, 
			    address_get_domain, ADDRESS, NUM);
}
#undef FUNC_NAME

SCM_DEFINE (mu_address_get_local, "mu-address-get-local", 1, 1, 0,
	    (SCM ADDRESS, SCM NUM),
	    "Return local part of an email address.\n")
#define FUNC_NAME s_mu_address_get_local
{
  return _get_address_part (FUNC_NAME, 
			    address_get_local_part, ADDRESS, NUM);
}
#undef FUNC_NAME

SCM_DEFINE (mu_address_get_count, "mu-address-get-count", 1, 0, 0,
	    (SCM ADDRESS),
	    "Return number of parts in email address.\n")
#define FUNC_NAME s_mu_address_get_count
{
  address_t addr;
  size_t count = 0;
  
  SCM_ASSERT (SCM_NIMP (ADDRESS) && SCM_STRINGP (ADDRESS),
	      ADDRESS, SCM_ARG1, FUNC_NAME);

  if (address_create (&addr, SCM_STRING_CHARS (ADDRESS)))
    return SCM_MAKINUM(0);

  address_get_count (addr, &count);
  address_destroy (&addr);
  return scm_makenum (count);
}
#undef FUNC_NAME

SCM_DEFINE (mu_username_to_email, "mu-username->email", 0, 1, 0,
	    (SCM NAME),
	    "Deduce the email from the username. If NAME is omitted, current username\n"
	    "is assumed\n")
#define FUNC_NAME s_mu_username_to_email
{
  char *name;
  char *email;
  SCM ret;
  
  if (SCM_UNBNDP (NAME))
    name = NULL;
  else {
    SCM_ASSERT (SCM_NIMP (NAME) && SCM_STRINGP (NAME),
		NAME, SCM_ARG1, FUNC_NAME);
    name = SCM_STRING_CHARS (NAME);
  }

  email = mu_get_user_email (name);
  if (!email)
    return SCM_BOOL_F;
  ret = scm_makfrom0str (email);
  free (email);
  return ret;
}
#undef FUNC_NAME

void
mu_scm_address_init ()
{
#include <mu_address.x>
}

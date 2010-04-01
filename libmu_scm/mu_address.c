/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005, 2006, 2007, 2010 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#include "mu_scm.h"

typedef int (*address_get_fp) (mu_address_t, size_t, char *, size_t, size_t *);

static SCM
_get_address_part (const char *func_name, address_get_fp fun,
		   SCM ADDRESS, SCM NUM)
{
  mu_address_t addr;
  int length;
  char *str;
  SCM ret;
  int num;
  int status;
  
  SCM_ASSERT (scm_is_string (ADDRESS), ADDRESS, SCM_ARG1, func_name);

  if (!SCM_UNBNDP (NUM))
    {
      SCM_ASSERT (scm_is_integer (NUM), NUM, SCM_ARG1, func_name);
      num = scm_to_int (NUM);
    }
  else
    num = 1;

  str = scm_to_locale_string (ADDRESS);
  if (!str[0])
    {
      free (str);
      mu_scm_error (func_name, 0, "Empty address", SCM_BOOL_F);
    }
  
  status = mu_address_create (&addr, str);
  free (str);
  if (status)
    mu_scm_error (func_name, status, "Cannot create address", SCM_BOOL_F);

  str = malloc (length + 1);
  if (!str)
    {
      mu_address_destroy (&addr);
      mu_scm_error (func_name, ENOMEM,
		    "Cannot allocate memory", SCM_BOOL_F);
    }

  status = (*fun) (addr, num, str, length + 1, NULL);
  mu_address_destroy (&addr);

  if (status == 0)
    ret = scm_from_locale_string (str);
  else
    {
      free (str);
      mu_scm_error (func_name, status,
		    "Underlying function failed", SCM_BOOL_F);
    }
  
  free (str);
  return ret;
}

SCM_DEFINE (scm_mu_address_get_personal, "mu-address-get-personal", 1, 1, 0,
	    (SCM ADDRESS, SCM NUM),
	    "Return personal part of the NUMth email address from ADDRESS.\n")
#define FUNC_NAME s_scm_mu_address_get_personal
{
  return _get_address_part (FUNC_NAME, 
			    mu_address_get_personal, ADDRESS, NUM);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_address_get_comments, "mu-address-get-comments", 1, 1, 0,
	    (SCM ADDRESS, SCM NUM),
	    "Return comment part of the NUMth email address from ADDRESS.\n")
#define FUNC_NAME s_scm_mu_address_get_comments
{
  return _get_address_part (FUNC_NAME, 
			    mu_address_get_comments, ADDRESS, NUM);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_address_get_email, "mu-address-get-email", 1, 1, 0,
	    (SCM ADDRESS, SCM NUM),
	    "Return email part of the NUMth email address from ADDRESS.\n")
#define FUNC_NAME s_scm_mu_address_get_email
{
  return _get_address_part (FUNC_NAME, 
			    mu_address_get_email, ADDRESS, NUM);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_address_get_domain, "mu-address-get-domain", 1, 1, 0,
	    (SCM ADDRESS, SCM NUM),
	    "Return domain part of the NUMth email address from ADDRESS.\n")
#define FUNC_NAME s_scm_mu_address_get_domain
{
  return _get_address_part (FUNC_NAME, 
			    mu_address_get_domain, ADDRESS, NUM);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_address_get_local, "mu-address-get-local", 1, 1, 0,
	    (SCM ADDRESS, SCM NUM),
	    "Return local part of the NUMth email address from ADDRESS.\n")
#define FUNC_NAME s_scm_mu_address_get_local
{
  return _get_address_part (FUNC_NAME, 
			    mu_address_get_local_part, ADDRESS, NUM);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_address_get_count, "mu-address-get-count", 1, 0, 0,
	    (SCM ADDRESS),
	    "Return number of parts in email address ADDRESS.\n")
#define FUNC_NAME s_scm_mu_address_get_count
{
  mu_address_t addr;
  size_t count = 0;
  int status;
  char *str;
  
  SCM_ASSERT (scm_is_string (ADDRESS), ADDRESS, SCM_ARG1, FUNC_NAME);

  str = scm_to_locale_string (ADDRESS);
  status = mu_address_create (&addr, str);
  free (str);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot create address for ~A",
		  scm_list_1 (ADDRESS));

  mu_address_get_count (addr, &count);
  mu_address_destroy (&addr);
  return scm_from_size_t (count);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_username_to_email, "mu-username->email", 0, 1, 0,
	    (SCM NAME),
"Deduce user's email address from his username. If NAME is omitted, \n"
"current username is assumed\n")
#define FUNC_NAME s_scm_mu_username_to_email
{
  char *name;
  char *email;
  SCM ret;
  
  if (SCM_UNBNDP (NAME))
    name = NULL;
  else
    {
      SCM_ASSERT (scm_is_string (NAME), NAME, SCM_ARG1, FUNC_NAME);
      name = scm_to_locale_string (NAME);
    }

  email = mu_get_user_email (name);
  free (name);
  if (!email)
    mu_scm_error (FUNC_NAME, 0,
		  "Cannot get user email for ~A",
		  scm_list_1 (NAME));

  ret = scm_from_locale_string (email);
  free (email);
  return ret;
}
#undef FUNC_NAME

void
mu_scm_address_init ()
{
#include <mu_address.x>
}

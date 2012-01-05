/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/util.h>
#include <mailutils/smtp.h>
#include <mailutils/sys/smtp.h>

static int
_mech_comp (const void *item, const void *data)
{
  return mu_c_strcasecmp ((const char*)item, (const char*)data);
}

int
mu_smtp_add_auth_mech (mu_smtp_t smtp, const char *mech)
{
  char *p;
  
  if (!smtp)
    return EINVAL;
  if (!smtp->authmech)
    {
      int rc = mu_list_create (&smtp->authmech);
      if (rc)
	return rc;
      mu_list_set_destroy_item (smtp->authmech, mu_list_free_item);
      mu_list_set_comparator (smtp->authmech, _mech_comp);
    }
  p = strdup (mech);
  if (!p)
    return ENOMEM;
  mu_strupper (p);
  return mu_list_append (smtp->authmech, p);
}

int
mu_smtp_clear_auth_mech (mu_smtp_t smtp)
{
  if (!smtp)
    return EINVAL;
  mu_list_clear (smtp->authmech);
  return 0;
}

static int
_mech_append (void *item, void *data)
{
  mu_smtp_t smtp = data;
  const char *mech = item;
  return mu_smtp_add_auth_mech (smtp, mech);
}

int
mu_smtp_add_auth_mech_list (mu_smtp_t smtp, mu_list_t list)
{
  if (!smtp)
    return EINVAL;
  return mu_list_foreach (list, _mech_append, smtp);
}

/* Set a list of implemented authentication mechanisms */
int
_mu_smtp_mech_impl (mu_smtp_t smtp, mu_list_t list)
{
  mu_list_destroy (&smtp->authimpl);
  smtp->authimpl = list;
  mu_list_set_comparator (smtp->authimpl, _mech_comp);
  return 0;
}


static int
_mech_copy (void *item, void *data)
{
  const char *mech = item;
  mu_list_t list = data;
  return mu_list_append (list, (void *)mech);
}

/* Select authentication mechanism to use */
int
mu_smtp_mech_select (mu_smtp_t smtp, const char **pmech)
{
  int rc;
  const char *authstr;
  mu_list_t alist;
  mu_iterator_t itr;
  
  if (!smtp)
    return EINVAL;

  /* Obtain the list of mechanisms supported by the server */
  rc = mu_smtp_capa_test (smtp, "AUTH", &authstr);
  if (rc)
    return rc;
  
  /* Create an intersection of implemented and allowed mechanisms */
  if (!smtp->authimpl)
    return MU_ERR_NOENT; /* obvious case */

  if (!smtp->authmech)
    {
      rc = mu_list_create (&alist);
      if (rc == 0)
	rc = mu_list_foreach (smtp->authimpl, _mech_copy, alist);
    }
  else
    {
      rc = mu_list_intersect_dup (&alist, smtp->authmech, smtp->authimpl,
				  NULL, NULL);
    }
  if (rc)
    return rc;

  /* Select first element from the intersection that occurs also in the
     list of methods supported by the server */
  rc = mu_list_get_iterator (alist, &itr);
  if (rc == 0)
    {
      const char *p;
      int res = 1;

      rc = MU_ERR_NOENT;
      authstr += 5; /* "AUTH */
      for (mu_iterator_first (itr); rc && !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  const char *mech;
	  mu_iterator_current (itr, (void**) &mech);
	  for (p = authstr; *p; )
	    {
	      char *end;
	      
	      p = mu_str_stripws ((char *)p);
	      end = mu_str_skip_class_comp (p, MU_CTYPE_SPACE);

	      res = mu_c_strncasecmp (mech, p, end - p);
	      if (res == 0)
		{
		  *pmech = mech;
		  rc = 0;
		  break;
		}
	      p = end;
	    }
	}
    }

  /* cleanup and return */
  mu_list_destroy (&alist);
  return rc;
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
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
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <mailutils/types.h>
#include <mailutils/address.h>
#include <mailutils/mu_auth.h>
#include <mailutils/util.h>
#include <mailutils/parse822.h>

/*
 * Functions used to convert unix mailbox/user names into RFC822 addr-specs.
 */

static char *mu_user_email = 0;

int
mu_set_user_email (const char *candidate)
{
  int err = 0;
  mu_address_t addr = NULL;
  size_t emailno = 0;
  char *email = NULL;
  const char *domain = NULL;
  
  if ((err = mu_address_create (&addr, candidate)) != 0)
    return err;

  if ((err = mu_address_get_email_count (addr, &emailno)) != 0)
    goto cleanup;

  if (emailno != 1)
    {
      errno = EINVAL;
      goto cleanup;
    }

  if ((err = mu_address_aget_email (addr, 1, &email)) != 0)
    goto cleanup;

  if (mu_user_email)
    free (mu_user_email);

  mu_user_email = email;

  if ((err = mu_address_sget_domain (addr, 1, &domain)) == 0)
    mu_set_user_email_domain (domain);
  
cleanup:
  mu_address_destroy (&addr);

  return err;
}

static char *mu_user_email_domain = 0;

int
mu_set_user_email_domain (const char *domain)
{
  char *d = NULL;
  
  if (!domain)
    return EINVAL;
  
  d = strdup (domain);

  if (!d)
    return ENOMEM;

  if (mu_user_email_domain)
    free (mu_user_email_domain);

  mu_user_email_domain = d;

  return 0;
}

/* FIXME: must be called _sget_ */
int
mu_get_user_email_domain (const char **domain)
{
  int err = 0;

  if (!mu_user_email_domain)
    {
      if ((err = mu_get_host_name (&mu_user_email_domain)))
	return err;
    }

  *domain = mu_user_email_domain;

  return 0;
}

int
mu_aget_user_email_domain (char **pdomain)
{
  const char *domain;
  int status = mu_get_user_email_domain (&domain);
  if (status)
    return status;
  if (domain == NULL)
    *pdomain = NULL;
  else
    {
      *pdomain = strdup (domain);
      if (*pdomain == NULL)
	return ENOMEM;
    }
  return 0;
}

/* Note: allocates memory */
char *
mu_get_user_email (const char *name)
{
  int status = 0;
  char *localpart = NULL;
  const char *domainpart = NULL;
  char *email = NULL;
  char *tmpname = NULL;

  if (!name && mu_user_email)
    {
      email = strdup (mu_user_email);
      if (!email)
	errno = ENOMEM;
      return email;
    }

  if (!name)
    {
      struct mu_auth_data *auth = mu_get_auth_by_uid (getuid ());
      if (!auth)
	{
	  errno = EINVAL;
	  return NULL;
	}
      name = tmpname = strdup(auth->name);
      if (auth)
	mu_auth_data_free (auth);
      if (!name)
        {
          errno = ENOMEM;
          return NULL;
        }
    }

  status = mu_get_user_email_domain (&domainpart);

  if (status)
    {
      free(tmpname);
      errno = status;
      return NULL;
    }

  if ((status = mu_parse822_quote_local_part (&localpart, name)))
    {
      free(tmpname);
      errno = status;
      return NULL;
    }


  email = malloc (strlen (localpart) + 1
		  + strlen (domainpart) + 1);
  if (!email)
    errno = ENOMEM;
  else
    sprintf (email, "%s@%s", localpart, domainpart);

  free(tmpname);
  free (localpart);

  return email;
}

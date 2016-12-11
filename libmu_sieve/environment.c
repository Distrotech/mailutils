/* The Sieve "environment" extension for GNU Mailutils
   Copyright (C) 2016 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with GNU Mailutils.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  
#include <sieve-priv.h>

int
retrieve_env (void *item, void *data, size_t idx, char **pval)
{
  mu_sieve_machine_t mach;

  if (idx)
    return MU_ERR_NOENT;
  mach = data;
  return mu_sieve_get_environ (mach, item, pval);
}

static int
sieve_test_environment (mu_sieve_machine_t mach)
{
  mu_sieve_value_t *name, *key_list;
  
  name = mu_sieve_get_arg_untyped (mach, 0);
  key_list = mu_sieve_get_arg_untyped (mach, 1);

  return mu_sieve_vlist_compare (mach, name, key_list, retrieve_env, NULL,
				 mach);
}

static mu_sieve_data_type environ_args[] = {
  SVT_STRING,
  SVT_STRING_LIST,
  SVT_VOID
};

static mu_sieve_tag_group_t environ_tag_groups[] = {
  { mu_sieve_match_part_tags, mu_sieve_match_part_checker },
  { NULL }
};

int
mu_sieve_require_environment (mu_sieve_machine_t mach)
{
  mu_sieve_register_test (mach, "environment", sieve_test_environment,
			  environ_args, environ_tag_groups, 1);
  return 0;
}

static char *
std_name_get (mu_sieve_machine_t mach)
{
  return strdup (PACKAGE_NAME);
}

static char *
std_version_get (mu_sieve_machine_t mach)
{
  return strdup (PACKAGE_VERSION);
}

/*  "host"    => The fully-qualified domain name of the host where
                 the Sieve script is executing.
*/
static char *
std_host_get (mu_sieve_machine_t mach)
{
  char *host;
  int rc;
  
  rc = mu_get_host_name (&host);
  if (rc == 0)
    return host;
  return NULL;
}


/*  "domain"  => The primary DNS domain associated with the Sieve
                 execution context, usually but not always a proper
                 suffix of the host name.
*/
static char *
std_domain_get (mu_sieve_machine_t mach)
{
  char *host;
  int rc;
  
  rc = mu_get_host_name (&host);
  if (rc == 0)
    {
      char *p = strchr (host, '.');
      if (p)
	{
	  p = strdup (p + 1);
	  free (host);
	  return p;
	}
      return host;
    }
  return NULL;
}

/* FIXME: do we need set? If so, mu_set_host_name is also needed */

struct stdenviron
{
  char *name;
  char *(*get) (mu_sieve_machine_t);
  int (*set) (mu_sieve_machine_t, char const *, char const *value);
};

static struct stdenviron stdenv[] =
{
  { "domain", std_domain_get, NULL },
  { "host", std_host_get, NULL },
  { "name", std_name_get, NULL },
  { "version", std_version_get, NULL },
  { NULL }
};

static struct stdenviron const *
stdenv_find (char const *name)
{
  struct stdenviron const *p;

  for (p = stdenv; p->name; p++)
    if (strcmp (p->name, name) == 0)
      return p;
  return NULL;
}

static char *
stdenv_get (mu_sieve_machine_t mach, char const *name)
{
  struct stdenviron const *p = stdenv_find (name);
  if (!p)
    return NULL;
  return p->get (mach);
}

static int
stdenv_set (mu_sieve_machine_t mach, char const *name, char const *value)
{
  struct stdenviron const *p = stdenv_find (name);
  if (!p)
    return MU_ERR_NOENT;
  if (!p->set)
    return EACCES;
  return p->set (mach, name, value);
}

int
mu_sieve_get_environ (mu_sieve_machine_t mach, char const *name, char **retval)
{
  char *p;

  p = stdenv_get (mach, name);
  if (p)
    {
      *retval = p;
      return 0;
    }

  if (!mach->exenv)
    return MU_ERR_NOENT;

  p = mu_assoc_ref (mach->exenv, name);
  if (p)
    {
      *retval = strdup (*(char**)p);
      if (!*retval)
	return errno;
    }
  else
    return MU_ERR_NOENT;
  return 0;
}

int
mu_sieve_set_environ (mu_sieve_machine_t mach, char const *name,
		      char const *value)
{
  int rc;

  rc = stdenv_set (mach, name, value);
  if (rc == MU_ERR_NOENT)
    {
      char **pptr;
      
      if (!mach->exenv)
	{
          int rc = mu_assoc_create (&mach->exenv, sizeof (char *), 0);
	  if (rc)
	    return rc;
	}
      rc = mu_assoc_ref_install (mach->exenv, name, (void **) &pptr);
      if (rc == 0 || rc == MU_ERR_EXISTS)
	{
	  char *copy = strdup (value);
	  if (!copy)
	    rc = errno;
	  else
	    {
	      *pptr = copy;
	      rc = 0;
	    }
	}
    }
  return rc;
}
    

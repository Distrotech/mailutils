/* This file is part of GNU Mailutils
   Copyright (C) 2007-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   GNU Mailutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "mailutils/libcfg.h"
#include "mailutils/acl.h"
#include "mailutils/argcv.h"
#include "mailutils/cidr.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ISSPACE(c) ((c)==' '||(c)=='\t')

#define SKIPWS(p) while (*(p) && ISSPACE (*(p))) (p)++;

static const char *
getword (mu_config_value_t *val, int *pn)
{
  int n = (*pn)++;
  mu_config_value_t *v;

  if (n >= val->v.arg.c)
    {
      mu_error (_("not enough arguments"));
      return NULL;
    }
  v = &val->v.arg.v[n];
  if (mu_cfg_assert_value_type (v, MU_CFG_STRING))
    return NULL;
  return v->v.string;
}

static int
parsearg (mu_config_value_t *val, struct mu_cidr *cidr, char **prest) 
{
  const char *w;
  int n = 0;
  int rc;
  
  if (mu_cfg_assert_value_type (val, MU_CFG_ARRAY))
    return 1;
  
  w = getword (val, &n);
  if (!w)
    return 1;
  if (strcmp (w, "from") == 0) {
    w = getword (val, &n);
    if (!w)
      return 1;
  }

  if (strcmp (w, "any") == 0)
    cidr->len = 0;
  else
    {
      rc = mu_cidr_from_string (cidr, w);
      if (rc)
	{
	  mu_error (_("invalid source CIDR: %s"), mu_strerror (rc));
	  return 1;
	}
    }
  
  if (prest)
    {
      if (n == val->v.arg.c)
	*prest = NULL;
      else
	{
	  size_t size = 0;
	  int i;
	  char *buf;
	  
	  for (i = n; i < val->v.arg.c; i++)
	    {
	      if (mu_cfg_assert_value_type (&val->v.arg.v[i], MU_CFG_STRING))
		return 1;
	      size += strlen (val->v.arg.v[i].v.string) + 1;
	    }

	  buf = malloc (size);
	  if (!buf)
	    {
	      mu_error ("%s", mu_strerror (errno));
	      return 1;
	    }	    

	  *prest = buf;
	  for (i = n; i < val->v.arg.c; i++)
	    {
	      if (i > n)
		*buf++ = ' ';
	      strcpy (buf, val->v.arg.v[i].v.string);
	      buf += strlen (buf);
	    }
	  *buf = 0;
	}
    }
  else if (n != val->v.arg.c)
    {
      mu_error (_("junk after IP address"));
      return 1;
    }
  return 0;
}

static int
cb_allow (void *data, mu_config_value_t *val)
{
  int rc;
  mu_acl_t acl = *(mu_acl_t*)data;
  struct mu_cidr cidr;
  
  if (parsearg (val, &cidr, NULL))
    return 1;
  rc = mu_acl_append (acl, mu_acl_accept, NULL, &cidr);
  if (rc)
    mu_error (_("cannot append acl entry: %s"), mu_strerror (rc));
  return rc;
}

static int
cb_deny (void *data, mu_config_value_t *val)
{
  int rc;
  mu_acl_t acl = *(mu_acl_t*)data;
  struct mu_cidr cidr;
  
  if (parsearg (val, &cidr, NULL))
    return 1;
  rc = mu_acl_append (acl, mu_acl_deny, NULL, &cidr);
  if (rc)
    mu_error (_("cannot append acl entry: %s"), mu_strerror (rc));
  return rc;
}

static int
cb_log (void *data, mu_config_value_t *val)
{
  int rc;
  mu_acl_t acl = *(mu_acl_t*)data;
  struct mu_cidr cidr;
  char *rest;
  
  if (parsearg (val, &cidr, &rest))
    return 1;
  rc = mu_acl_append (acl, mu_acl_log, rest, &cidr);
  if (rc)
    mu_error (_("cannot append acl entry: %s"), mu_strerror (rc));
  return rc;
}

static int
cb_exec (void *data, mu_config_value_t *val)
{
  int rc;
  mu_acl_t acl = *(mu_acl_t*)data;
  struct mu_cidr cidr;
  char *rest;
  
  if (parsearg (val, &cidr, &rest))
    return 1;
  rc = mu_acl_append (acl, mu_acl_exec, rest, &cidr);
  if (rc)
    mu_error (_("cannot append acl entry: %s"), mu_strerror (rc));
  return rc;
}

static int
cb_ifexec (void *data, mu_config_value_t *val)
{
  int rc;
  mu_acl_t acl = *(mu_acl_t*)data;
  struct mu_cidr cidr;
  char *rest;
  
  if (parsearg (val, &cidr, &rest))
    return 1;
  rc = mu_acl_append (acl, mu_acl_ifexec, rest, &cidr);
  if (rc)
    mu_error (_("cannot append acl entry: %s"), mu_strerror (rc));
  return rc;
}

static struct mu_cfg_param acl_param[] = {
  { "allow", mu_cfg_callback, NULL, 0, cb_allow,
    N_("Allow connections from this IP address. Optional word `from' is "
       "allowed between it and its argument. The same holds true for other "
       "actions below."),
    N_("addr: IP") },
  { "deny", mu_cfg_callback, NULL, 0, cb_deny,
    N_("Deny connections from this IP address."),
    N_("addr: IP") },
  { "log", mu_cfg_callback, NULL, 0, cb_log,
    N_("Log connections from this IP address."),
    N_("addr: IP") },
  { "exec", mu_cfg_callback, NULL, 0, cb_exec,
    N_("Execute supplied program if a connection from this IP address is "
       "requested. Arguments are:\n"
       "  <addr: IP> <program: string>\n"
       "Following macros are expanded in <program> before executing:\n"
       "  address  -  Source IP address\n"
       "  port     -  Source port number\n") },
  { "ifexec", mu_cfg_callback, NULL, 0, cb_ifexec,
    N_("If a connection from this IP address is requested, execute supplied "
       "program and allow or deny the connection depending on its exit code. "
       "See `exec' for a description of its arguments.") },
  { NULL }
};

static int
acl_section_parser (enum mu_cfg_section_stage stage,
		    const mu_cfg_node_t *node,
		    const char *section_label, void **section_data,
		    void *call_data,
		    mu_cfg_tree_t *tree)
{
  switch (stage)
    {
    case mu_cfg_section_start:
      {
	void *data = *section_data;
	mu_acl_create ((mu_acl_t*)data);
      }
      break;
      
    case mu_cfg_section_end:
      break;
    }
  return 0;
}

void
mu_acl_cfg_init ()
{
  struct mu_cfg_section *section;
  if (mu_create_canned_section ("acl", &section) == 0)
    {
      section->parser = acl_section_parser;
      mu_cfg_section_add_params (section, acl_param);
    }
}

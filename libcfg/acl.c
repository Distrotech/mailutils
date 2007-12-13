/* This file is part of GNU Mailutils
   Copyright (C) 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "mailutils/libcfg.h"
#include "mailutils/acl.h"
#include "mailutils/argcv.h"
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

static char *
getword (char **parg)
{
  char *arg = *parg;
  char *word;

  SKIPWS (arg);
  word = arg;
  if (*arg)
    {
      while (*arg && !ISSPACE (*arg))
	arg++;
      *arg++ = 0;
    }
  *parg = arg;
  return word;
}

struct netdef
{
  struct sockaddr *sa;
  int len;
  unsigned long netmask;
};

#ifndef INADDR_ANY
# define INADDR_ANY 0
#endif

int
parse_address (mu_debug_t err, char *str, struct netdef *nd)
{
  struct sockaddr_in in;
  
  in.sin_family = AF_INET;
  if (strcmp (str, "any") == 0)
    {
      in.sin_addr.s_addr = INADDR_ANY;
      nd->netmask = 0;
    }
  else if (inet_aton (str, &in.sin_addr) == 0)
    {
      mu_cfg_format_error (err, MU_DEBUG_ERROR, _("Invalid IPv4: %s"), str);
      return 1;
    }
  in.sin_port = 0;
  nd->len = sizeof (in);
  nd->sa = malloc (nd->len);
  if (!nd->sa)
    {
      mu_cfg_format_error (err, MU_DEBUG_ERROR, "%s", mu_strerror (errno));
      return 1;
    }
  memcpy (nd->sa, &in, sizeof (in));
  return 0;
}

static int
parsearg (mu_debug_t err, char *arg, struct netdef *pnd, char **prest)
{
  char *w, *p;
  unsigned long netmask;
  
  w = getword (&arg);
  if (strcmp (w, "from") == 0)
    w = getword (&arg);

  p = strchr (w, '/');
  if (p)
    {
      char *q;
      unsigned netlen;
	  
      *p++ = 0;
      netlen = strtoul (p, &q, 10);
      if (*q == 0)
	{
	  if (netlen == 0)
	    netmask = 0;
	  else
	    {
	      netmask = 0xfffffffful >> (32 - netlen);
	      netmask <<= (32 - netlen);
	      netmask = htonl (netmask);
	    }
	}
      else if (*q == '.')
	{
	  struct in_addr addr;
	      
	  if (inet_aton (p, &addr) == 0)
	    {
	      mu_cfg_format_error (err, MU_DEBUG_ERROR, _("invalid netmask"));
	      return 1;
	    }
	  netmask = addr.s_addr;
	}
      else
	{
	  mu_cfg_format_error (err, MU_DEBUG_ERROR, _("invalid netmask"));
	  return 1;
	}
    }
  else
    netmask = 0xfffffffful;

  pnd->netmask = netmask;
  if (parse_address (err, w, pnd))
    return 1;

  SKIPWS (arg);
  if (prest)
    {
      if (*arg == 0)
	*prest = NULL;
      else
	{
	  int len = strlen (arg);
	  *prest = malloc (len);
	  if (!prest)
	    {
	      mu_cfg_format_error (err, MU_DEBUG_ERROR,
				   "%s", mu_strerror (errno));
	      return 1;
	    }
	  if (arg[0] == '"' || arg[0] == '\'')
	    mu_argcv_unquote_copy (*prest, arg, len);
	  else
	    strcpy (*prest, arg);
	}
    }
  else if (*arg != 0)
    {
      mu_cfg_format_error (err, MU_DEBUG_ERROR, _("junk after IP address"));
      return 1;
    }
  return 0;
}

static int
cb_allow (mu_debug_t err, void *data, char *arg)
{
  int rc;
  mu_acl_t acl = *(mu_acl_t*)data;
  struct netdef ndef;
  
  if (parsearg (err, arg, &ndef, NULL))
    return 1;
  rc = mu_acl_append (acl, mu_acl_accept, NULL, ndef.sa, ndef.len,
		      ndef.netmask);
  if (rc)
    mu_cfg_format_error (err, MU_DEBUG_ERROR,
			 _("cannot append acl entry: %s"), 
			 mu_strerror (rc));
  free (ndef.sa);
  return rc;
}

static int
cb_deny (mu_debug_t err, void *data, char *arg)
{
  int rc;
  mu_acl_t acl = *(mu_acl_t*)data;
  struct netdef ndef;
  
  if (parsearg (err, arg, &ndef, NULL))
    return 1;
  rc = mu_acl_append (acl, mu_acl_deny, NULL, ndef.sa, ndef.len,
		      ndef.netmask);
  if (rc)
    mu_cfg_format_error (err, MU_DEBUG_ERROR,
			 _("cannot append acl entry: %s"), 
			 mu_strerror (rc));
  free (ndef.sa);
  return rc;
}

static int
cb_log (mu_debug_t err, void *data, char *arg)
{
  int rc;
  mu_acl_t acl = *(mu_acl_t*)data;
  struct netdef ndef;
  char *rest;
  
  if (parsearg (err, arg, &ndef, &rest))
    return 1;
  rc = mu_acl_append (acl, mu_acl_log, rest, ndef.sa, ndef.len,
		      ndef.netmask);
  if (rc)
    mu_cfg_format_error (err, MU_DEBUG_ERROR,
			 _("cannot append acl entry: %s"), 
			 mu_strerror (rc));
  free (ndef.sa);
  return rc;
}

static int
cb_exec (mu_debug_t err, void *data, char *arg)
{
  int rc;
  mu_acl_t acl = *(mu_acl_t*)data;
  struct netdef ndef;
  char *rest;
  
  if (parsearg (err, arg, &ndef, &rest))
    return 1;
  rc = mu_acl_append (acl, mu_acl_exec, rest, ndef.sa, ndef.len,
		      ndef.netmask);
  if (rc)
    mu_cfg_format_error (err, MU_DEBUG_ERROR,
			 _("cannot append acl entry: %s"), 
			 mu_strerror (rc));
  free (ndef.sa);
  return rc;
}

static int
cb_ifexec (mu_debug_t err, void *data, char *arg)
{
  int rc;
  mu_acl_t acl = *(mu_acl_t*)data;
  struct netdef ndef;
  char *rest;
  
  if (parsearg (err, arg, &ndef, &rest))
    return 1;
  rc = mu_acl_append (acl, mu_acl_ifexec, rest, ndef.sa, ndef.len,
		      ndef.netmask);
  if (rc)
    mu_cfg_format_error (err, MU_DEBUG_ERROR,
			 _("cannot append acl entry: %s"), 
			 mu_strerror (rc));
  free (ndef.sa);
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

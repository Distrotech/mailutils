/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2012 Free Software Foundation, Inc.

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
#include <syslog.h>
#include <string.h>
#include <mailutils/debug.h>
#include <mailutils/nls.h>
#include <mailutils/syslog.h>
#include <mailutils/cfg.h>
#include <mailutils/diag.h>
#include <mailutils/error.h>
#include <mailutils/server.h>
#include "tcpwrap.h"

int mu_tcp_wrapper_enable = 1;
const char *mu_tcp_wrapper_daemon;

#ifdef WITH_LIBWRAP
# include <tcpd.h>

int
mu_tcpwrapper_access (int fd)
{
  struct request_info req;

  if (!mu_tcp_wrapper_enable)
    return 1;
  request_init (&req,
		RQ_DAEMON,
		mu_tcp_wrapper_daemon ?
		     mu_tcp_wrapper_daemon : mu_program_name,
		RQ_FILE, fd, NULL);
  fromhost (&req);
  return hosts_access (&req);
}

struct mu_cfg_param tcpwrapper_param[] = {
  { "enable", mu_cfg_bool, &mu_tcp_wrapper_enable, 0, NULL,	      
    N_("Enable TCP wrapper access control.  Default is \"yes\".") },	      
  { "daemon", mu_cfg_string, &mu_tcp_wrapper_daemon, 0, NULL,     
    N_("Set daemon name for TCP wrapper lookups.  Default is program name."), 
    N_("name") },							      
  { "allow-table", mu_cfg_string, &hosts_allow_table,
    0, NULL,
    N_("Use file for positive client address access control "		      
       "(default: /etc/hosts.allow)."),					      
    N_("file") },							      
  { "deny-table", mu_cfg_string, &hosts_deny_table,
    0, NULL,                                             
    N_("Use file for negative client address access control "		      
       "(default: /etc/hosts.deny)."),					      
    N_("file") },							      
  { NULL }
};

void
mu_tcpwrapper_cfg_init ()
{
  struct mu_cfg_section *section;
  mu_create_canned_section ("tcp-wrappers", &section);
  mu_cfg_section_add_params (section, tcpwrapper_param);
}

#else

void
mu_tcpwrapper_cfg_init ()
{
}

int
mu_tcpwrapper_access (int fd)
{
  return 1;
}

#endif

int
mu_tcp_wrapper_prefork (int fd, struct sockaddr *sa, int salen,
			struct mu_srv_config *pconf,
			void *data)
{
  if (mu_tcp_wrapper_enable
      && sa->sa_family == AF_INET
      && !mu_tcpwrapper_access (fd))
    {
      char *p = mu_sockaddr_to_astr (sa, salen);
      mu_error (_("access from %s blocked by TCP wrappers"), p);
      free (p);
      return 1;
    }
  return 0;
}
     

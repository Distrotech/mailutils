/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2004, 
   2005, 2006, 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include <mailutils/types.h>

extern int mu_tcp_wrapper_enable;
const char *mu_tcp_wrapper_daemon;
extern int mu_tcp_wrapper_cb_hosts_allow (mu_debug_t debug, void *data,
					  char *arg);
extern int mu_tcp_wrapper_cb_hosts_deny (mu_debug_t debug, void *data,
					 char *arg);
extern int mu_tcp_wrapper_cb_hosts_allow_syslog (mu_debug_t debug, void *data,
						 char *arg);
extern int mu_tcp_wrapper_cb_hosts_deny_syslog (mu_debug_t debug, void *data,
						char *arg);
extern int mu_tcpwrapper_access (int fd);

#ifdef WITH_LIBWRAP
# define TCP_WRAPPERS_CONFIG						      \
  { "tcp-wrapper-enable", mu_cfg_bool, &mu_tcp_wrapper_enable, 0, NULL,	      \
    N_("Enable TCP wrapper access control.  Default is \"yes\".") },	      \
  { "tcp-wrapper-daemon", mu_cfg_string, &mu_tcp_wrapper_daemon, 0, NULL,     \
    N_("Set daemon name for TCP wrapper lookups.  Default is program name."), \
    N_("name") },							      \
  { "hosts-allow-table", mu_cfg_callback, NULL, 0,                            \
    mu_tcp_wrapper_cb_hosts_allow,                                            \
    N_("Use file for positive client address access control "		      \
       "(default: /etc/hosts.allow)."),					      \
    N_("file") },							      \
  { "hosts-deny-table", mu_cfg_callback, NULL, 0,                             \
    mu_tcp_wrapper_cb_hosts_deny,                                             \
    N_("Use file for negative client address access control "		      \
       "(default: /etc/hosts.deny)."),					      \
    N_("file") },							      \
  { "hosts-allow-syslog-level", mu_cfg_callback, NULL, 0,	       	      \
    mu_tcp_wrapper_cb_hosts_allow_syslog,				      \
    N_("Log host allows at this syslog level.  See logging { facility } for " \
       "a description of argument syntax."),				      \
    N_("level") },							      \
  { "hosts-allow-deny-level", mu_cfg_callback, NULL, 0,			      \
    mu_tcp_wrapper_cb_hosts_deny_syslog,				      \
    N_("Log host denies at this syslog level.  See logging { facility } for " \
       "a description of argument syntax."),				      \
    N_("level") },
#else
# define TCP_WRAPPERS_CONFIG
#endif

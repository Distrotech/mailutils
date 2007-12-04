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
#include <stdlib.h>
#include <string.h>
#include "mailutils/libcfg.h"
#include <mailutils/daemon.h>
#include <mailutils/debug.h>
#include <mailutils/syslog.h>
#include <mu_umaxtostr.h>

static struct mu_gocs_daemon daemon_settings;
static struct mu_gocs_locking locking_settings;
static struct mu_gocs_logging logging_settings;
static struct mu_gocs_mailbox mailbox_settings;
static struct mu_gocs_source_email address_settings;
static struct mu_gocs_mailer mailer_settings;
static struct mu_gocs_debug debug_settings;


/* ************************************************************************* */
/* Mailbox                                                                   */
/* ************************************************************************* */

static struct mu_cfg_param mu_mailbox_param[] = {
  { "mail-spool", mu_cfg_string, &mailbox_settings.mail_spool, NULL,
    N_("Use specified URL as a mailspool directory."),
    N_("url") },
  { "mailbox-type", mu_cfg_string, &mailbox_settings.mailbox_type, NULL,
    N_("Default mailbox type."), N_("protocol") },
  { NULL }
};

DCL_CFG_CAPA (mailbox);


/* ************************************************************************* */
/* Locking                                                                   */
/* ************************************************************************* */

static struct mu_cfg_param mu_locking_param[] = {
  /* FIXME: Flags are superfluous. */
  { "flags", mu_cfg_string, &locking_settings.lock_flags, NULL,
    N_("Default locker flags (E=external, R=retry, T=time, P=pid).") },
  { "retry-timeout", mu_cfg_ulong, &locking_settings.lock_retry_timeout, NULL,
    N_("Set timeout for acquiring the lock.") },
  { "retry-count", mu_cfg_ulong, &locking_settings.lock_retry_count, NULL,
    N_("Set the maximum number of times to retry acquiring the lock.") },
  { "expire-timeout", mu_cfg_ulong, &locking_settings.lock_expire_timeout,
    NULL,
    N_("Expire locks older than this amount of time.") },
  { "external-locker", mu_cfg_string, &locking_settings.external_locker, NULL,
    N_("Use external locker program."),
    N_("prog") },
  { NULL, }
};

DCL_CFG_CAPA (locking);


/* ************************************************************************* */
/* Address                                                                   */
/* ************************************************************************* */
     
static struct mu_cfg_param mu_address_param[] = {
  { "email-addr", mu_cfg_string, &address_settings.address, NULL,
    N_("Set the current user email address (default is "
       "loginname@defaultdomain)."),
    N_("email") },
  { "email-domain", mu_cfg_string, &address_settings.domain, NULL,
    N_("Set e-mail domain for unqualified user names (default is this host)"),
    N_("domain") },
  { NULL }
};

DCL_CFG_CAPA (address);

     
/* ************************************************************************* */
/* Mailer                                                                    */
/* ************************************************************************* */
     
static struct mu_cfg_param mu_mailer_param[] = {
  { "url", mu_cfg_string, &mailer_settings.mailer, NULL,
    N_("Use this URL as the default mailer"),
    N_("url") },
  { NULL }
};

DCL_CFG_CAPA (mailer);


/* ************************************************************************* */
/* Logging                                                                   */
/* ************************************************************************* */

int
cb_facility (mu_debug_t debug, void *data, char *arg)
{
  if (mu_string_to_syslog_facility (arg, &logging_settings.facility))
    {
      mu_cfg_format_error (debug, MU_DEBUG_ERROR, 
                           _("Unknown syslog facility `%s'"), 
			   arg);
      return 1;
    }
   return 0;
}

static struct mu_cfg_param mu_logging_param[] = {
  { "facility", mu_cfg_callback, NULL, cb_facility,
    N_("Set syslog facility. Arg is one of the following: user, daemon, "
       "auth, authpriv, mail, cron, local0 through local7 (case-insensitive), "
       "or a facility number.") },
  { NULL }
};

DCL_CFG_CAPA (logging);


/* ************************************************************************* */
/* Daemon                                                                    */
/* ************************************************************************* */

static int
_cb_daemon_mode (mu_debug_t debug, void *data, char *arg)
{
  if (strcmp (arg, "inetd") == 0
      || strcmp (arg, "interactive") == 0)
    daemon_settings.mode = MODE_INTERACTIVE;
  else if (strcmp (arg, "daemon") == 0)
    daemon_settings.mode = MODE_DAEMON;
  else
    {
      mu_cfg_format_error (debug, MU_DEBUG_ERROR, _("unknown daemon mode"));
      return 1;
    }
  return 0;
}
  
static struct mu_cfg_param mu_daemon_param[] = {
  { "max-children", mu_cfg_ulong, &daemon_settings.maxchildren, NULL,
    N_("Maximum number of children processes to run simultaneously.") },
  { "mode", mu_cfg_callback, NULL, _cb_daemon_mode,
    N_("Set daemon mode (either inetd (or interactive) or daemon)."),
    N_("mode") },
  { "transcript", mu_cfg_bool, &daemon_settings.transcript, NULL,
    N_("Log the session transcript.") },
  { "pidfile", mu_cfg_string, &daemon_settings.pidfile, NULL,
    N_("Store PID of the master process in this file."),
    N_("file") },
  { "port", mu_cfg_ushort, &daemon_settings.port, NULL,
    N_("Listen on the specified port number.") },
  { "timeout", mu_cfg_ulong, &daemon_settings.timeout, NULL,
    N_("Set idle timeout.") },
  { NULL }
};

int									      
mu_daemon_section_parser
   (enum mu_cfg_section_stage stage, const mu_cfg_node_t *node,	      
    void *section_data, void *call_data, mu_cfg_tree_t *tree)
{									      
  switch (stage)							      
    {									      
    case mu_cfg_section_start:
      daemon_settings = mu_gocs_daemon;
      break;								      
      									      
    case mu_cfg_section_end:
      mu_gocs_daemon = daemon_settings;
      mu_gocs_store ("daemon", &daemon_settings);	      
    }									      
  return 0;								      
}

struct mu_cfg_capa mu_daemon_cfg_capa = {                
  "daemon",  mu_daemon_param, mu_daemon_section_parser
};


/* ************************************************************************* */
/* Debug                                                                     */
/* ************************************************************************* */

static int
cb_debug_level (mu_debug_t debug, void *data, char *arg)
{
  char buf[UINTMAX_STRSIZE_BOUND];
  char *p;
  size_t size;
  char *pfx;
  struct mu_debug_locus locus;
  
  debug_settings.string = arg;
  if (mu_debug_get_locus (debug, &locus) == 0)
    {
      p = umaxtostr (locus.line, buf);
      size = strlen (locus.file) + 1 + strlen (p) + 1;
      pfx = malloc (size);
      if (!pfx)
	{
	  mu_cfg_format_error (debug, MU_DEBUG_ERROR,
			       "%s", mu_strerror (errno));
	  return 1;
	}
      strcpy (pfx, locus.file);
      strcat (pfx, ":");
      strcat (pfx, p);
    }
  else
    pfx = strdup ("command line");/*FIXME*/
  debug_settings.errpfx = pfx;
  return 0;
}

static struct mu_cfg_param mu_debug_param[] = {
  { "level", mu_cfg_callback, NULL, &cb_debug_level,
    N_("Set Mailutils debugging level.  Argument is a colon-separated list "
       "of debugging specifications in the form:\n"
       "   <object: string>[[:]=<level: number>].") },
  { "line-info", mu_cfg_bool, &debug_settings.line_info, NULL,
    N_("Prefix debug messages with Mailutils source locations.") },
  { NULL }
};

DCL_CFG_CAPA (debug);

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
#include <stdlib.h>
#include <string.h>
#include "mailutils/libcfg.h"
#include <mailutils/debug.h>
#include <mailutils/syslog.h>
#include <mailutils/mailbox.h>
#include <mailutils/io.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>

static struct mu_gocs_locking locking_settings;
static struct mu_gocs_mailbox mailbox_settings;
static struct mu_gocs_source_email address_settings;
static struct mu_gocs_mailer mailer_settings;
static struct mu_gocs_debug debug_settings;


/* ************************************************************************* */
/* Mailbox                                                                   */
/* ************************************************************************* */

static int
_cb_folder (void *data, mu_config_value_t *val)
{
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  mu_set_folder_directory (val->v.string);
  return 0;
}

static struct mu_cfg_param mu_mailbox_param[] = {
  { "mail-spool", mu_cfg_string, &mailbox_settings.mail_spool, 0, NULL,
    N_("Use specified URL as a mailspool directory."),
    N_("url") },
  { "mailbox-pattern", mu_cfg_string, &mailbox_settings.mailbox_pattern,
    0, NULL,
    N_("Create mailbox URL using <pattern>."),
    N_("pattern") },
  { "mailbox-type", mu_cfg_string, &mailbox_settings.mailbox_type, 0, NULL,
    N_("Default mailbox type."), N_("protocol") },
  { "folder", mu_cfg_callback, NULL, 0, _cb_folder,
    N_("Default user mail folder"),
    N_("dir") },
  { NULL }
};

DCL_CFG_CAPA (mailbox);


/* ************************************************************************* */
/* Locking                                                                   */
/* ************************************************************************* */

static struct mu_cfg_param mu_locking_param[] = {
  /* FIXME: Flags are superfluous. */
  { "flags", mu_cfg_string, &locking_settings.lock_flags, 0, NULL,
    N_("Default locker flags (E=external, R=retry, T=time, P=pid).") },
  { "retry-timeout", mu_cfg_ulong, &locking_settings.lock_retry_timeout,
    0, NULL,
    N_("Set timeout for acquiring the lock.") },
  { "retry-count", mu_cfg_ulong, &locking_settings.lock_retry_count, 0, NULL,
    N_("Set the maximum number of times to retry acquiring the lock.") },
  { "expire-timeout", mu_cfg_ulong, &locking_settings.lock_expire_timeout,
    0, NULL,
    N_("Expire locks older than this amount of time.") },
  { "external-locker", mu_cfg_string, &locking_settings.external_locker, 
    0, NULL,
    N_("Use external locker program."),
    N_("prog") },
  { NULL, }
};

DCL_CFG_CAPA (locking);


/* ************************************************************************* */
/* Address                                                                   */
/* ************************************************************************* */
     
static struct mu_cfg_param mu_address_param[] = {
  { "email-addr", mu_cfg_string, &address_settings.address, 0, NULL,
    N_("Set the current user email address (default is "
       "loginname@defaultdomain)."),
    N_("email") },
  { "email-domain", mu_cfg_string, &address_settings.domain, 0, NULL,
    N_("Set e-mail domain for unqualified user names (default is this host)"),
    N_("domain") },
  { NULL }
};

DCL_CFG_CAPA (address);

     
/* ************************************************************************* */
/* Mailer                                                                    */
/* ************************************************************************* */
     
static struct mu_cfg_param mu_mailer_param[] = {
  { "url", mu_cfg_string, &mailer_settings.mailer, 0, NULL,
    N_("Use this URL as the default mailer"),
    N_("url") },
  { NULL }
};

DCL_CFG_CAPA (mailer);


/* ************************************************************************* */
/* Logging                                                                   */
/* ************************************************************************* */

static int
cb_facility (void *data, mu_config_value_t *val)
{
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  
  if (mu_string_to_syslog_facility (val->v.string, &mu_log_facility))
    {
      mu_error (_("unknown syslog facility `%s'"), val->v.string);
      return 1;
    }
   return 0;
}

static int
cb_severity (void *data, mu_config_value_t *val)
{
  unsigned n;
  
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  if (mu_severity_from_string (val->v.string, &n))
    {
      mu_error (_("unknown severity `%s'"), val->v.string);
      return 1;
    }
  mu_log_severity_threshold = n;
  return 0;
}      

static struct mu_cfg_param mu_logging_param[] = {
  { "syslog", mu_cfg_bool, &mu_log_syslog, 0, NULL,
    N_("Send diagnostics to syslog.") },
  { "print-severity", mu_cfg_bool, &mu_log_print_severity, 0, NULL,
    N_("Print message severity levels.") },
  { "severity", mu_cfg_callback, NULL, 0, cb_severity,
    N_("Output only messages with a severity equal to or greater than "
       "this one.") },
  { "facility", mu_cfg_callback, NULL, 0, cb_facility,
    N_("Set syslog facility. Arg is one of the following: user, daemon, "
       "auth, authpriv, mail, cron, local0 through local7 (case-insensitive), "
       "or a facility number.") },
  { "session-id", mu_cfg_bool, &mu_log_session_id, 0, NULL,
    N_("Log session ID") },
  { "tag", mu_cfg_string, &mu_log_tag, 0, NULL,
    N_("Tag syslog messages with this string.") },
  { NULL }
};

static int logging_settings; /* Dummy variable */
DCL_CFG_CAPA (logging);


/* ************************************************************************* */
/* Debug                                                                     */
/* ************************************************************************* */

static int
_cb2_debug_level (const char *arg, void *data MU_ARG_UNUSED)
{
  mu_debug_parse_spec (arg);
  return 0;
}

static int
cb_debug_level (void *data, mu_config_value_t *val)
{
  return mu_cfg_string_value_cb (val, _cb2_debug_level, NULL);
}

static struct mu_cfg_param mu_debug_param[] = {
  { "level", mu_cfg_callback, NULL, 0, &cb_debug_level,
    N_("Set Mailutils debugging level.  Argument is a colon-separated list "
       "of debugging specifications in the form:\n"
       "   <object: string>[[:]=<level: number>].") },
  { "line-info", mu_cfg_bool, &debug_settings.line_info, 0, NULL,
    N_("Prefix debug messages with Mailutils source locations.") },
  { NULL }
};

DCL_CFG_CAPA (debug);

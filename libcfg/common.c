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
  { "mail-spool", mu_cfg_string, &mailbox_settings.mail_spool },
  { "mailbox-type", mu_cfg_string, &mailbox_settings.mailbox_type },
  { NULL }
};

DCL_CFG_CAPA (mailbox);


/* ************************************************************************* */
/* Locking                                                                   */
/* ************************************************************************* */

static struct mu_cfg_param mu_locking_param[] = {
  { "flags", mu_cfg_string, &locking_settings.lock_flags },
  { "retry-timeout", mu_cfg_ulong, &locking_settings.lock_retry_timeout },
  { "retry-count", mu_cfg_ulong, &locking_settings.lock_retry_count },
  { "expire-timeout", mu_cfg_ulong, &locking_settings.lock_expire_timeout },
  { "external-locker", mu_cfg_string, &locking_settings.external_locker },
  { NULL, }
};

DCL_CFG_CAPA (locking);


/* ************************************************************************* */
/* Address                                                                   */
/* ************************************************************************* */
     
static struct mu_cfg_param mu_address_param[] = {
  { "email-addr", mu_cfg_string, &address_settings.address },
  { "email-domain", mu_cfg_string, &address_settings.domain },
  { NULL }
};

DCL_CFG_CAPA (address);

     
/* ************************************************************************* */
/* Mailer                                                                    */
/* ************************************************************************* */
     
static struct mu_cfg_param mu_mailer_param[] = {
  { "url", mu_cfg_string, &mailer_settings.mailer },
  { NULL }
};

DCL_CFG_CAPA (mailer);


/* ************************************************************************* */
/* Logging                                                                   */
/* ************************************************************************* */

int
cb_facility (mu_cfg_locus_t *locus, void *data, char *arg)
{
  if (mu_string_to_syslog_facility (arg, &logging_settings.facility))
    {
      mu_error (_("%s:%d: Unknown syslog facility `%s'"), 
                locus->file, locus->line, arg);
      return 1;
    }
   return 0;
}

static struct mu_cfg_param mu_logging_param[] = {
  { "facility", mu_cfg_callback, NULL, cb_facility },
  { NULL }
};

DCL_CFG_CAPA (logging);


/* ************************************************************************* */
/* Daemon                                                                    */
/* ************************************************************************* */

static int
_cb_daemon_mode (mu_cfg_locus_t *locus, void *data, char *arg)
{
  if (strcmp (arg, "inetd") == 0
      || strcmp (arg, "interactive") == 0)
    daemon_settings.mode = MODE_INTERACTIVE;
  else if (strcmp (arg, "daemon") == 0)
    daemon_settings.mode = MODE_DAEMON;
  else
    {
      mu_error ("%s:%d: unknown daemon mode", locus->file, locus->line);
      return 1;
    }
  return 0;
}
  
static struct mu_cfg_param mu_daemon_param[] = {
  { "max-children", mu_cfg_ulong, &daemon_settings.maxchildren },
  { "mode", mu_cfg_callback, NULL, _cb_daemon_mode },
  { "transcript", mu_cfg_bool, &daemon_settings.transcript },
  { "pidfile", mu_cfg_string, &daemon_settings.pidfile },
  { "port", mu_cfg_ushort, &daemon_settings.port },
  { "timeout", mu_cfg_ulong, &daemon_settings.timeout },
  { NULL }
};

int									      
mu_daemon_section_parser
   (enum mu_cfg_section_stage stage, const mu_cfg_node_t *node,	      
    void *section_data, void *call_data)				      
{									      
  switch (stage)							      
    {									      
    case mu_cfg_section_start:
      daemon_settings = mu_gocs_daemon;
      break;								      
      									      
    case mu_cfg_section_end:						      
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
cb_debug_level (mu_cfg_locus_t *locus, void *data, char *arg)
{
  char buf[UINTMAX_STRSIZE_BOUND];
  char *p;
  size_t size;
  char *pfx;
  
  debug_settings.string = arg;
  p = umaxtostr (locus->line, buf);
  size = strlen (locus->file) + 1 + strlen (p) + 1;
  pfx = malloc (size);
  if (!pfx)
    {
      mu_error ("%s", mu_strerror (errno));
      return 1;
    }
  strcpy (pfx, locus->file);
  strcat (pfx, ":");
  strcat (pfx, p);
  debug_settings.errpfx = pfx;
  return 0;
}

static struct mu_cfg_param mu_debug_param[] = {
  { "level", mu_cfg_callback, NULL, &cb_debug_level },
  { "line-info", mu_cfg_bool, &debug_settings.line_info },
  { NULL }
};

DCL_CFG_CAPA (debug);

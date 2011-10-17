/* This file is part of GNU Mailutils
   Copyright (C) 2007, 2008, 2010, 2011 Free Software Foundation, Inc.

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
#include "mailutils/libcfg.h"
#include <mailutils/tls.h>
#include <mailutils/util.h>
#include <mailutils/kwd.h>

static struct mu_tls_module_config tls_settings = {
    1,                   /* enabled by default */

    NULL,                /* Certificate file */
    MU_FILE_SAFETY_GROUP_WRITABLE |
     MU_FILE_SAFETY_GROUP_WRITABLE |
     MU_FILE_SAFETY_LINKED_WRDIR,

    NULL,               /* Key file */ 
    MU_FILE_SAFETY_ALL, /* Stringent safety checks for keys */

    NULL,               /* CA file */
    MU_FILE_SAFETY_GROUP_WRITABLE |
     MU_FILE_SAFETY_GROUP_WRITABLE |
     MU_FILE_SAFETY_LINKED_WRDIR
};


static int
cb2_safety_checks (const char *name, void *data)
{
  int negate = 0;
  int val;
  int *res = data;
  
  if (strcmp (name, "none") == 0)
    {
      *res = MU_FILE_SAFETY_NONE;
      return 0;
    }

  if (*name == '-')
    {
      negate = 1;
      name++;
    }
  else if (*name == '+')
    name++;
    
  if (mu_file_safety_name_to_code (name, &val))
    mu_error (_("unknown keyword: %s"), name);
  else
    {
      if (negate)
	*res &= ~val;
      else
	*res |= val;
    }
  return 0;
}

static int
cb_safety_checks (void *data, mu_config_value_t *arg)
{
  return mu_cfg_string_value_cb (arg, cb2_safety_checks, data);
}

static struct mu_cfg_param mu_tls_param[] = {
  { "enable", mu_cfg_bool, &tls_settings.enable, 0, NULL,
    N_("Enable client TLS encryption.") },
  { "ssl-cert", mu_cfg_string, &tls_settings.ssl_cert, 0, NULL,
    N_("Specify SSL certificate file."),
    N_("file") },
  { "ssl-key", mu_cfg_string, &tls_settings.ssl_key, 0, NULL,
    N_("Specify SSL certificate key file."),
    N_("file") },
  { "ssl-cafile", mu_cfg_string, &tls_settings.ssl_cafile, 0, NULL,
    N_("Specify trusted CAs file."),
    N_("file") },
  { "key-file-safety-checks", mu_cfg_callback,
    &tls_settings.ssl_key_safety_checks, 0,
    cb_safety_checks,
    N_("Configure safety checks for SSL key file.  Argument is a list or "
       "sequence of check names optionally prefixed with '+' to enable or "
       "'-' to disable the corresponding check.  Valid check names are:\n"
       "\n"
       "  none          disable all checks\n"
       "  all           enable all checks\n"
       "  gwrfil        forbid group writable files\n"
       "  awrfil        forbid world writable files\n"
       "  grdfil        forbid group readable files\n"
       "  ardfil        forbid world writable files\n"
       "  linkwrdir     forbid symbolic links in group or world writable directories\n"
       "  gwrdir        forbid files in group writable directories\n"
       "  awrdir        forbid files in world writable directories\n"),
    N_("arg: list") },  
  { "cert-file-safety-checks", mu_cfg_callback,
    &tls_settings.ssl_cert_safety_checks, 0,
    cb_safety_checks,
    N_("Configure safety checks for SSL certificate.  See above for a description of <arg>."),
    N_("arg: list") },  
  { "ca-file-safety-checks", mu_cfg_callback,
    &tls_settings.ssl_cafile_safety_checks, 0,
    cb_safety_checks,
    N_("Configure safety checks for SSL certificate authority file.  See above for a description of <arg>."),
    N_("arg: list") },  
  { NULL }
}; 

DCL_CFG_CAPA (tls);


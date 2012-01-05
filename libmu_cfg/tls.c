/* This file is part of GNU Mailutils
   Copyright (C) 2007-2008, 2010-2012 Free Software Foundation, Inc.

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

#define SSL_CERT_FILE_CHECKS  (MU_FILE_SAFETY_GROUP_WRITABLE |		\
			       MU_FILE_SAFETY_GROUP_WRITABLE |		\
			       MU_FILE_SAFETY_LINKED_WRDIR)

#define SSL_KEY_FILE_CHECKS   (MU_FILE_SAFETY_ALL & ~MU_FILE_SAFETY_OWNER_MISMATCH)

#define SSL_CA_FILE_CHECKS    (MU_FILE_SAFETY_GROUP_WRITABLE |		\
			       MU_FILE_SAFETY_GROUP_WRITABLE |		\
			       MU_FILE_SAFETY_LINKED_WRDIR)

static struct mu_tls_module_config tls_settings = {
    1,                   /* enabled by default */

    NULL,                /* Certificate file */
    SSL_CERT_FILE_CHECKS,

    NULL,                /* Key file */ 
    SSL_KEY_FILE_CHECKS, /* Stringent safety checks for keys */

    NULL,                /* CA file */
    SSL_CA_FILE_CHECKS
};

static int
cb2_safety_checks (const char *name, void *data)
{
  int defval;
  
  if (data == &tls_settings.ssl_key_safety_checks)
    defval = SSL_KEY_FILE_CHECKS;
  else if (data == &tls_settings.ssl_cert_safety_checks)
    defval = SSL_CERT_FILE_CHECKS;
  else if (data == &tls_settings.ssl_cafile_safety_checks)
    defval = SSL_CA_FILE_CHECKS;
  else
    {
      mu_error (_("INTERNAL ERROR at %s:%d: unknown default value?"),
		__FILE__, __LINE__);
      defval = MU_FILE_SAFETY_ALL;
    }
  if (mu_file_safety_compose (data, name, defval))
    mu_error (_("unknown keyword: %s"), name);
  return 0;
}

static int
cb_safety_checks (void *data, mu_config_value_t *arg)
{
  return mu_cfg_string_value_cb (arg, cb2_safety_checks, data);
}

static struct mu_cfg_param mu_tls_param[] = {
  { "enable", mu_cfg_bool, &tls_settings.enable, 0, NULL,
    N_("Enable TLS encryption.") },
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


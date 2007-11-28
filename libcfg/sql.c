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
#include "mailutils/libcfg.h"
#include <mailutils/sql.h>
#include <mailutils/mutil.h>

static struct mu_sql_module_config sql_settings;



/* Resource file configuration */
static int
cb_password_type (mu_debug_t debug, void *data, char *arg)
{
  if (mu_sql_decode_password_type (arg, &sql_settings.password_type))
    mu_cfg_format_error (debug, MU_DEBUG_ERROR,
			 _("Unknown password type `%s'"),
			 arg);
  return 0;
}

static int
cb_field_map (mu_debug_t debug, void *data, char *arg)
{
  int err;
  int rc = mutil_parse_field_map (arg, &sql_settings.field_map, &err);
  if (rc)
    mu_cfg_format_error (debug, MU_DEBUG_ERROR, _("Error near element %d: %s"),
			 err, mu_strerror (rc));
  return 0;
}

static struct mu_cfg_param mu_sql_param[] = {
  { "interface", mu_cfg_string, &sql_settings.interface },
  { "getwpnam", mu_cfg_string, &sql_settings.getpwnam_query },
  { "getpwuid", mu_cfg_string, &sql_settings.getpwuid_query },
  { "getpass", mu_cfg_string, &sql_settings.getpass_query },
  { "host", mu_cfg_string, &sql_settings.host },
  { "user", mu_cfg_string, &sql_settings.user },
  { "passwd", mu_cfg_string, &sql_settings.passwd },
  { "db", mu_cfg_string, &sql_settings.db },
  { "port", mu_cfg_int, &sql_settings.port },
  { "password-type", mu_cfg_callback, NULL, cb_password_type },
  { "field-map", mu_cfg_callback, NULL, cb_field_map },
  { NULL }
};

DCL_CFG_CAPA (sql);

     

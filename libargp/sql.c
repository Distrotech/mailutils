/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "mailutils/libargp.h"
#include "mailutils/sql.h"
#include "mailutils/mutil.h"

static int sql_requested;
static struct mu_sql_module_config sql_config;
/* Command-line configuration */

enum {
  OPT_SQL_INTERFACE = 256,        
  OPT_SQL_GETPWNAM,         
  OPT_SQL_GETPWUID,         
  OPT_SQL_GETPASS,          
  OPT_SQL_HOST,             
  OPT_SQL_USER,             
  OPT_SQL_PASSWD,           
  OPT_SQL_DB,               
  OPT_SQL_PORT,             
  OPT_SQL_MU_PASSWORD_TYPE, 
  OPT_SQL_FIELD_MAP,        
};

static struct argp_option mu_sql_argp_option[] = {
  {"sql-interface", OPT_SQL_INTERFACE, N_("NAME"), OPTION_HIDDEN,
   N_("Type of SQL interface to use"), },
  {"sql-getpwnam", OPT_SQL_GETPWNAM, N_("QUERY"), OPTION_HIDDEN,
   N_("SQL query to retrieve a passwd entry based on username"), 0},
  {"sql-getpwuid", OPT_SQL_GETPWUID, N_("QUERY"), OPTION_HIDDEN,
   N_("SQL query to retrieve a passwd entry based on UID"), 0},
  {"sql-getpass", OPT_SQL_GETPASS, N_("QUERY"), OPTION_HIDDEN,
   N_("SQL query to retrieve a password from the database"), 0},
  {"sql-host", OPT_SQL_HOST, N_("HOSTNAME"), OPTION_HIDDEN,
   N_("Name or IP of MySQL server to connect to"), 0},
  {"sql-user", OPT_SQL_USER, N_("NAME"), OPTION_HIDDEN,
   N_("SQL user name"), 0},
  {"sql-passwd", OPT_SQL_PASSWD, N_("STRING"), OPTION_HIDDEN,
   N_("SQL connection password"), 0},
  {"sql-db", OPT_SQL_DB, N_("STRING"), OPTION_HIDDEN,
   N_("Name of the database to connect to"), 0},
  {"sql-port", OPT_SQL_PORT, N_("NUMBER"), OPTION_HIDDEN,
   N_("Port to use"), 0},
  {"sql-password-type", OPT_SQL_MU_PASSWORD_TYPE, N_("STRING"), OPTION_HIDDEN,
   N_("Type of password returned by --sql-getpass query. STRING is one of: plain, hash, scrambled"), 0},
  {"sql-field-map", OPT_SQL_FIELD_MAP, N_("MAP"), OPTION_HIDDEN,
   N_("Declare a name translation map for SQL fields in results of sql-getpwnam and "
      "sql-getpwuid queries"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t
mu_sql_argp_parser (int key, char *arg, struct argp_state *state)
{
  int rc, err;
  
  switch (key)
    {
    case OPT_SQL_INTERFACE:
      sql_config.interface = arg;
      sql_requested = 1;
      break;
      
    case OPT_SQL_GETPWNAM:
      sql_requested = 1;
      sql_config.getpwnam_query = arg;
      break;

    case OPT_SQL_GETPWUID:
      sql_requested = 1;
      sql_config.getpwuid_query = arg;
      break;

    case OPT_SQL_GETPASS:
      sql_requested = 1;
      sql_config.getpass_query = arg;
      break;

    case OPT_SQL_HOST:
      sql_requested = 1;
      sql_config.host = arg;
      break;

    case OPT_SQL_USER:
      sql_requested = 1;
      sql_config.user = arg;
      break;

    case OPT_SQL_PASSWD:
      sql_requested = 1;
      sql_config.passwd = arg;
      break;

    case OPT_SQL_DB:
      sql_requested = 1;
      sql_config.db = arg;
      break;

    case OPT_SQL_PORT:
      sql_requested = 1;
      sql_config.port = strtoul (arg, NULL, 0);
      break;

    case OPT_SQL_MU_PASSWORD_TYPE:
      if (mu_sql_decode_password_type (arg, &sql_config.password_type))
	argp_error (state, _("Unknown password type `%s'"), arg);
      sql_requested = 1;
      break;

    case OPT_SQL_FIELD_MAP:
      rc = mutil_parse_field_map (arg, &sql_config.field_map, &err);
      if (rc)
	argp_error (state, _("Error near element %d: %s"),
		    err, mu_strerror (rc));
      sql_requested = 1;
      break;
      
    case ARGP_KEY_FINI:
      if (sql_requested)
	mu_gocs_store ("sql", &sql_config);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp mu_sql_argp = {
  mu_sql_argp_option,
  mu_sql_argp_parser,
};

struct argp_child mu_sql_argp_child = {
  &mu_sql_argp,
  0,
  NULL,
  0
};

struct mu_cmdline_capa mu_sql_cmdline = {
  "sql", &mu_sql_argp_child
};

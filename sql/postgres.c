/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2005, 2007 Free Software Foundation, Inc.

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

#ifdef HAVE_PGSQL
#include <mailutils/mailutils.h>
#include <mailutils/sql.h>

#include <libpq-fe.h>
#include <ctype.h>

static char *
chop (char *str)
{
  int len;
  
  for (len = strlen(str); len > 0 && isspace(str[len-1]); len--)
    ;
  str[len] = 0;
  return str;
}

struct mu_pgsql_data
{
  PGconn  *pgconn;
  PGresult *res;
};


static int
init (mu_sql_connection_t conn)
{
  struct mu_pgsql_data *dp = calloc (1, sizeof (*dp));
  if (!dp)
    return ENOMEM;
  conn->data = dp;
  return 0;
}

static int
destroy (mu_sql_connection_t conn)
{
  struct mu_pgsql_data *dp = conn->data;
  free (dp);
  conn->data = NULL;
  return 0;
}

static int
connect (mu_sql_connection_t conn)
{
  struct mu_pgsql_data *dp = conn->data;
  char portbuf[16];
  char *portstr;
        
  if (conn->port == 0)
    portstr = NULL;
  else
    {
      portstr = portbuf;
      snprintf (portbuf, sizeof (portbuf), "%d", conn->port);
    }
                
  dp->pgconn = PQsetdbLogin (conn->server, portstr, NULL, NULL,
			     conn->dbname, conn->login, conn->password);
        
  if (PQstatus (dp->pgconn) == CONNECTION_BAD)
    return MU_ERR_SQL;
  return 0;
}

static int 
disconnect (mu_sql_connection_t conn)
{
  struct mu_pgsql_data *dp = conn->data;
  PQfinish (dp->pgconn);
  return 0;
}

static int
query (mu_sql_connection_t conn, char *query)
{
  struct mu_pgsql_data *dp = conn->data;
  ExecStatusType stat;

  dp->res = PQexec (dp->pgconn, query);
  if (dp->res == NULL)
    return MU_ERR_SQL;
        
  stat = PQresultStatus (dp->res);

  if (stat != PGRES_COMMAND_OK && stat != PGRES_TUPLES_OK)
    return MU_ERR_SQL;

  return 0;
}

static int
store_result (mu_sql_connection_t conn)
{
  return 0;
}

static int
release_result (mu_sql_connection_t conn)
{
  struct mu_pgsql_data *dp = conn->data;
  PQclear (dp->res);
  dp->res = NULL;
  return 0;
}

static int
num_columns (mu_sql_connection_t conn, size_t *np)
{
  struct mu_pgsql_data *dp = conn->data;
  if (!dp->res)
    return MU_ERR_NO_RESULT;
  *np = PQnfields (dp->res);
  return 0;
}

static int
num_tuples (mu_sql_connection_t conn, size_t *np)
{
  struct mu_pgsql_data *dp = conn->data;
  if (!dp->res)
    return MU_ERR_NO_RESULT;
  *np = PQntuples (dp->res);
  return 0;
}

static int
get_column (mu_sql_connection_t conn, size_t nrow, size_t ncol, char **pdata)
{
  struct mu_pgsql_data *dp = conn->data;
  if (!dp->res)
    return MU_ERR_NO_RESULT;
  *pdata = chop (PQgetvalue (dp->res, nrow, ncol));
  return 0;
}

static int
get_field_number (mu_sql_connection_t conn, const char *fname, size_t *fno)
{
  struct mu_pgsql_data *dp = conn->data;
  if (!dp->res)
    return MU_ERR_NO_RESULT;
  if ((*fno = PQfnumber (dp->res, fname)) == -1)
    return MU_ERR_NOENT;
  return 0;
}

static const char *
errstr (mu_sql_connection_t conn)
{
  struct mu_pgsql_data *dp = conn->data;
  return PQerrorMessage (dp->pgconn);
}


MU_DECL_SQL_DISPATCH_T(postgres) = {
  "postgres",
  5432,
  init,
  destroy,
  connect,
  disconnect,
  query,
  store_result,
  release_result,
  num_tuples,
  num_columns,
  get_column,
  get_field_number,
  errstr,
};

#endif

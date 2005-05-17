/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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

#include <mailutils/mailutils.h>
#include <mailutils/sql.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

struct mu_odbc_data
{
  SQLHENV env;        /* Environment */
  SQLHDBC dbc;        /* DBC */
                      /* Result data: */ 
  SQLHSTMT stmt;      /* Statement being executed */
  list_t result;      /* List of returned field values */
                      /* Error reporting: */
  struct odbc_err_buffer
  {
    SQLSMALLINT handle_type;   /* Type of the handle */
    SQLHANDLE handle;          /* Handle that caused the error */  
    char *what;                /* Name of the function that failed */ 
    SQLCHAR *msg;              /* Error message buffer */
    char *text;                /* Error text buffer */
  } err;
};

static void
mu_odbc_diag(struct mu_odbc_data *dp,
	     SQLSMALLINT handle_type, SQLHANDLE handle, char *what)
{
  dp->err.what = what;
  dp->err.handle_type = handle_type;
  dp->err.handle = handle;
}

/* ************************************************************************* */
/* Interface routines */

static int
init (mu_sql_connection_t conn)
{
  struct mu_odbc_data *dp = calloc (1, sizeof (*dp));
  if (!dp)
    return ENOMEM;
  conn->data = dp;
  return 0;
}

static int
destroy (mu_sql_connection_t conn)
{
  struct mu_odbc_data *dp = conn->data;
  free (dp->err.msg);
  free (dp->err.text);
  if (dp->stmt)
    SQLFreeHandle (SQL_HANDLE_STMT, dp->stmt);
  free (dp);
  conn->data = NULL;
  return 0;
}

static int
connect (mu_sql_connection_t conn)
{
  struct mu_odbc_data *dp = conn->data;
  long rc;

  rc = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &dp->env);
  if (rc != SQL_SUCCESS)
    {
      mu_odbc_diag (dp, SQL_HANDLE_ENV, dp->env, "SQLAllocHandle");
      return MU_ERR_SQL;
    }
  
  rc = SQLSetEnvAttr (dp->env, SQL_ATTR_ODBC_VERSION,
		      (void*)SQL_OV_ODBC3, 0);
  if (rc != SQL_SUCCESS)
    {
      mu_odbc_diag (dp, SQL_HANDLE_ENV, dp->dbc, "SQLSetEnvAttr");
      return MU_ERR_SQL;
    }

  rc = SQLAllocHandle (SQL_HANDLE_DBC, dp->env, &dp->dbc);
  if (rc != SQL_SUCCESS)
    {
      mu_odbc_diag (dp, SQL_HANDLE_DBC, dp->dbc, "SQLAllocHandle");
      return MU_ERR_SQL;
    }
  
  rc = SQLConnect(dp->dbc,
		  (SQLCHAR*)conn->dbname, SQL_NTS,
		  (SQLCHAR*)conn->login, SQL_NTS,
		  (SQLCHAR*)conn->password, SQL_NTS);
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
      mu_odbc_diag (dp, SQL_HANDLE_DBC, dp->dbc, "SQLConnect");
      return MU_ERR_SQL;
    }
        
  return 0;
}

static int 
disconnect (mu_sql_connection_t conn)
{
  struct mu_odbc_data *dp = conn->data;   
  SQLDisconnect (dp->dbc);
  SQLFreeHandle (SQL_HANDLE_ENV, dp->env);
  return 0;
}

static int
query (mu_sql_connection_t conn, char *query)
{
  struct mu_odbc_data *dp = conn->data;
  long rc;

  if (dp->stmt)
    SQLFreeHandle (SQL_HANDLE_STMT, dp->stmt);
  
  rc = SQLAllocHandle (SQL_HANDLE_STMT, dp->dbc, &dp->stmt);     
  if (rc != SQL_SUCCESS)
    {
      mu_odbc_diag (dp, SQL_HANDLE_DBC, dp->dbc, "SQLAllocHandle");
      return MU_ERR_SQL;
    }

  /* FIXME: In some implementations only default (forward only) cursors
     may be available. Do we need a sequential access method after all?
     FIXME2: On SQL_SUCCESS_WITH_INFO no info is output */
  rc = SQLSetStmtAttr (dp->stmt,
		       SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_DYNAMIC,
		       0);
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
      mu_odbc_diag (dp, SQL_HANDLE_STMT, dp->stmt, "SQLSetStmtAttr");
      return MU_ERR_SQL;
    }

  rc = SQLExecDirect (dp->stmt, query, SQL_NTS);   
  if (rc != SQL_SUCCESS)
    {
      mu_odbc_diag (dp, SQL_HANDLE_STMT, dp->stmt, "SQLExecDirect");
      return MU_ERR_SQL;
    }

  return 0;
}

static int
store_result (mu_sql_connection_t conn)
{
  struct mu_odbc_data *dp = conn->data;
  list_create (&dp->result);
  return 0;
}

static int
free_char_data (void *item, void *data ARG_UNUSED)
{
  free (item);
  return 0;
}

static int
release_result (mu_sql_connection_t conn)
{
  struct mu_odbc_data *dp = conn->data;
  list_do (dp->result, free_char_data, NULL);
  list_destroy (&dp->result);
  return 0;
}

static int
num_columns (mu_sql_connection_t conn, size_t *np)
{
  struct mu_odbc_data *dp = conn->data;
  SQLSMALLINT  count;
  
  if (SQLNumResultCols (dp->stmt, &count) != SQL_SUCCESS)
    {
      mu_odbc_diag (dp, SQL_HANDLE_STMT, dp->stmt, "SQLNumResultCount");
      return MU_ERR_SQL;
    }
  *np = count;
  return 0;
}

static int
num_tuples (mu_sql_connection_t conn, size_t *np)
{
  struct mu_odbc_data *dp = conn->data;
  SQLINTEGER count;

  if (SQLRowCount (dp->stmt, &count) != SQL_SUCCESS)
    {
      mu_odbc_diag (dp, SQL_HANDLE_STMT, dp->stmt, "SQLRowCount");
      return MU_ERR_SQL;
    }
  *np = count;      
  return 0;
}

static int
get_column (mu_sql_connection_t conn, size_t nrow, size_t ncol, char **pdata)
{
  struct mu_odbc_data *dp = conn->data;
  char buffer[1024];
  SQLINTEGER size;
  
  if (SQLFetchScroll (dp->stmt, SQL_FETCH_ABSOLUTE, nrow + 1) != SQL_SUCCESS)
    {
      mu_odbc_diag (dp, SQL_HANDLE_STMT, dp->stmt, "SQLFetchScroll");
      return MU_ERR_SQL;
    }

  if (SQLGetData (dp->stmt, ncol + 1, SQL_C_CHAR,
		  buffer, sizeof buffer, &size) != SQL_SUCCESS)
    {
      mu_odbc_diag (dp, SQL_HANDLE_STMT, dp->stmt, "SQLGetData");
      return MU_ERR_SQL;
    }
  
  *pdata = strdup (buffer);
  list_append (dp->result, *pdata);
  return 0;
}

#define DEFAULT_ERROR_BUFFER_SIZE 1024

static const char *
errstr (mu_sql_connection_t conn)
{
  struct mu_odbc_data *dp = conn->data;   
  char state[16];
  char nbuf[64];
  SQLINTEGER nerror;
  SQLSMALLINT msglen;
  size_t length;

  if (!dp->err.what)
    return mu_strerror (0);
  
  if (!dp->err.msg)
    {
      dp->err.msg = malloc (DEFAULT_ERROR_BUFFER_SIZE);
      if (!dp->err.msg)
	return mu_strerror (ENOMEM);
    }
  
  SQLGetDiagRec (dp->err.handle_type,
		 dp->err.handle,
		 1,
		 state,
		 &nerror,
		 dp->err.msg, DEFAULT_ERROR_BUFFER_SIZE, &msglen);
  
  snprintf (nbuf, sizeof nbuf, "%d", (int) nerror);
  length = strlen (dp->err.what) + 1
             + strlen (state) + 1
             + strlen (nbuf) + 1
             + strlen (dp->err.msg) + 1;
  if (dp->err.text)
    free (dp->err.text);
  dp->err.text = malloc (length);
  if (!dp->err.text)
    return dp->err.msg;
  
  snprintf (dp->err.text, length, "%s %s %s %s", dp->err.what, state, nbuf,
	    dp->err.msg);
  return dp->err.text;
}

MU_DECL_SQL_DISPATCH_T(odbc) = {
  "odbc",
  0,
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
  errstr,
};

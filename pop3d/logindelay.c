/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007, 2009-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "pop3d.h"

#ifdef ENABLE_LOGIN_DELAY

static mu_dbm_file_t
open_stat_db (int mode)
{
  mu_dbm_file_t db;
  int rc;

  rc = mu_dbm_create (login_stat_file, &db, DEFAULT_GROUP_DB_SAFETY);
  if (rc)
    {
      mu_diag_output (MU_DIAG_ERROR, _("unable to create statistics db"));
      return NULL;
    }

  rc = mu_dbm_safety_check (db);
  if (rc && rc != ENOENT)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("statistics db fails safety check: %s"),
		      mu_strerror (rc));
      mu_dbm_destroy (&db);
      return NULL;
    }
  
  rc = mu_dbm_open (db, mode, 0660);
  if (rc)
    {
      mu_diag_output (MU_DIAG_ERROR, _("unable to open statistics db: %s"),
		      mu_dbm_strerror (db));
      mu_dbm_destroy (&db);
    }
  return db;
}

int
check_login_delay (char *username)
{
  time_t now, prev_time;
  mu_dbm_file_t db;
  struct mu_dbm_datum key, data;
  char text[64], *p;
  int rc;

  if (login_delay == 0)
    return 0;
  
  time (&now);
  db = open_stat_db (MU_STREAM_READ);
  if (!db)
    return 0;
  
  memset (&key, 0, sizeof key);
  key.mu_dptr = username;
  key.mu_dsize = strlen (username);
  memset (&data, 0, sizeof data);

  rc = mu_dbm_fetch (db, &key, &data);
  if (rc)
    {
      if (rc != MU_ERR_NOENT)
	mu_diag_output (MU_DIAG_ERROR, _("cannot fetch login delay data: %s"),
			mu_dbm_strerror (db));
      mu_dbm_destroy (&db);
      return 0;
    }

  if (data.mu_dsize > sizeof (text) - 1)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("invalid entry for '%s': wrong timestamp size"),
		      username);
      mu_dbm_destroy (&db);
      return 0;
    }
  mu_dbm_destroy (&db);

  memcpy (text, data.mu_dptr, data.mu_dsize);
  text[data.mu_dsize] = 0;
  mu_dbm_datum_free (&data);
  
  prev_time = strtoul (text, &p, 0);
  if (*p)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("malformed timestamp for '%s': %s"),
		      username, text);
      return 0;
    }

  return now - prev_time < login_delay;
}

void
update_login_delay (char *username)
{
  time_t now;
  mu_dbm_file_t db;
  struct mu_dbm_datum key, data;
  char text[64];
  
  if (login_delay == 0 || username == NULL)
    return;

  time (&now);
  db = open_stat_db (MU_STREAM_RDWR);
  if (!db)
    return;
  
  memset(&key, 0, sizeof(key));
  memset(&data, 0, sizeof(data));
  key.mu_dptr = username;
  key.mu_dsize = strlen (username);
  snprintf (text, sizeof text, "%lu", (unsigned long) now);
  data.mu_dptr = text;
  data.mu_dsize = strlen (text);
  if (mu_dbm_store (db, &key, &data, 1))
    mu_error (_("%s: cannot store datum %s/%s: %s"),
	      login_stat_file, username, text,
	      mu_dbm_strerror (db));
  
  mu_dbm_destroy (&db);
}

void
login_delay_capa (const char *name, struct pop3d_session *session)
{
  mu_dbm_file_t db;

  if (login_delay && (db = open_stat_db (MU_STREAM_RDWR)) != NULL)
    {
      pop3d_outf ("%lu\n", (unsigned long) login_delay);
      mu_dbm_destroy (&db);
    }
}
#endif

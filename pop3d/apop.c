/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

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

/*
  APOP name digest

  Arguments:
  a string identifying a mailbox and a MD5 digest string
  (both required)

  Restrictions:
  may only be given in the AUTHORIZATION state after the POP3
  greeting or after an unsuccessful USER or PASS command

  When the POP3 server receives the APOP command, it verifies
  the digest provided.  If the digest is correct, the POP3
  server issues a positive response, and the POP3 session
  enters the TRANSACTION state.  Otherwise, a negative
  response is issued and the POP3 session remains in the
  AUTHORIZATION state.  */

/* Check if a username exists in APOP password file
   returns pointer to password if found, otherwise NULL */

char *
pop3d_apopuser (const char *user)
{
  char *password = NULL;
  int rc;
  
#ifdef ENABLE_DBM
  {
    size_t len;
    mu_dbm_file_t db;
    struct mu_dbm_datum key, data;

    rc = mu_dbm_create (apop_database_name, &db, apop_database_safety);
    if (rc)
      {
	mu_diag_output (MU_DIAG_ERROR, _("unable to create APOP db"));
	return NULL;
      }

    if (apop_database_owner_set)
      mu_dbm_safety_set_owner (db, apop_database_owner);
    
    rc = mu_dbm_safety_check (db);
    if (rc)
      {
	mu_diag_output (MU_DIAG_ERROR,
			_("APOP file %s fails safety check: %s"),
			apop_database_name, mu_strerror (rc));
	mu_dbm_destroy (&db);
	return NULL;
      }
    
    rc = mu_dbm_open (db, MU_STREAM_READ, 0600);
    if (rc)
      {
	mu_diag_output (MU_DIAG_ERROR, _("unable to open APOP db: %s"),
			mu_strerror (rc));
	return NULL;
      }

    memset (&key, 0, sizeof key);
    memset (&data, 0, sizeof data);

    key.mu_dptr = (void *)user;
    key.mu_dsize = strlen (user);

    rc = mu_dbm_fetch (db, &key, &data);
    if (rc == MU_ERR_NOENT)
      {
	mu_dbm_destroy (&db);
	return NULL;
      }
    else if (rc)
      {
	mu_diag_output (MU_DIAG_ERROR,
			_("cannot fetch APOP data: %s"),
			mu_dbm_strerror (db));
	mu_dbm_destroy (&db);
	return NULL;
      }
    mu_dbm_destroy (&db);
    len = data.mu_dsize;
    password = malloc (len + 1);
    if (password == NULL)
      {
	mu_dbm_datum_free (&data);
	return NULL;
      }
    memcpy (password, data.mu_dptr, len);
    password[len] = 0;
    mu_dbm_datum_free (&data);
    return password;
  }
#else /* !ENABLE_DBM */
  {
    char *buf = NULL;
    size_t size = 0, n;
    size_t ulen;
    mu_stream_t apop_stream;

    rc = mu_file_safety_check (apop_database_name, apop_database_safety,
			       apop_database_owner, NULL);
    if (rc)
      {
	mu_diag_output (MU_DIAG_ERROR,
			_("APOP file %s fails safety check: %s"),
			apop_database_name, mu_strerror (rc));
	return NULL;
      }

    rc = mu_file_stream_create (&apop_stream, apop_database_name,
				MU_STREAM_READ);
    if (rc)
      {
	mu_diag_output (MU_DIAG_INFO,
			_("unable to open APOP password file %s: %s"),
			apop_database_name, mu_strerror (rc));
	return NULL;
      }
    
    ulen = strlen (user);
    while (mu_stream_getline (apop_stream, &buf, &size, &n) == 0 && n > 0)
      {
	char *p, *start = mu_str_stripws (buf);

	if (!*start || *start == '#')
	  continue;
	p = strchr (start, ':');
	if (!p)
	  continue;
	if (p - start == ulen && memcmp (user, start, ulen) == 0)
	  {
	    p = mu_str_skip_class (p + 1, MU_CTYPE_SPACE);
	    if (*p)
	      password = mu_strdup (p);
	    break;
	  }
      }
    free (buf);
    mu_stream_unref (apop_stream);
    return password;
  }
#endif
}

int
pop3d_apop (char *arg, struct pop3d_session *sess)
{
  char *p, *password, *user_digest, *user;
  struct mu_md5_ctx md5context;
  unsigned char md5digest[16];
  char buf[2 * 16 + 1];
  size_t i;
  
  if (state != AUTHORIZATION)
    return ERR_WRONG_STATE;

  if (strlen (arg) == 0)
    return ERR_BAD_ARGS;

  pop3d_parse_command (arg, &user, &user_digest);

  password = pop3d_apopuser (user);
  if (password == NULL)
    {
      mu_diag_output (MU_DIAG_INFO,
		      _("password for `%s' not found in the database"),
	      user);
      return ERR_BAD_LOGIN;
    }

  mu_md5_init_ctx (&md5context);
  mu_md5_process_bytes (md5shared, strlen (md5shared), &md5context);
  mu_md5_process_bytes (password, strlen (password), &md5context);
  free (password);
  mu_md5_finish_ctx (&md5context, md5digest);

  for (i = 0, p = buf; i < 16; i++, p += 2)
      sprintf (p, "%02x", md5digest[i]);
  *p = 0;

  if (strcmp (user_digest, buf))
    {
      mu_diag_output (MU_DIAG_INFO, _("APOP failed for `%s'"), user);
      return ERR_BAD_LOGIN;
    }

  auth_data = mu_get_auth_by_name (user);
  if (auth_data == NULL)
    return ERR_BAD_LOGIN;

  return pop3d_begin_session ();
}

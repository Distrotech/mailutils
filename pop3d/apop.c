/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005, 2007, 2009, 2010 Free
   Software Foundation, Inc.

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
  
#ifdef USE_DBM
  {
    size_t len;
    DBM_FILE db;
    DBM_DATUM key, data;

    int rc = mu_dbm_open (APOP_PASSFILE, &db, MU_STREAM_READ, 0600);
    if (rc)
      {
	mu_diag_output (MU_DIAG_ERROR, _("unable to open APOP db: %s"),
		mu_strerror (errno));
	return NULL;
      }

    memset (&key, 0, sizeof key);
    memset (&data, 0, sizeof data);

    MU_DATUM_PTR (key) = (void*) user;
    MU_DATUM_SIZE (key) = strlen (user);

    rc = mu_dbm_fetch (db, key, &data);
    mu_dbm_close (db);
    if (rc)
      {
	mu_diag_output (MU_DIAG_ERROR,
			_("cannot fetch APOP data: %s"), mu_strerror (errno));
	return NULL;
      }
    len = MU_DATUM_SIZE (data);
    password = malloc (len + 1);
    if (password == NULL)
      {
	mu_dbm_datum_free (&data);
	return NULL;
      }
    memcpy (password, MU_DATUM_PTR (data), len);
    password[len] = 0;
    mu_dbm_datum_free (&data);
    return password;
  }
#else /* !USE_DBM */
  {
    char *buf = NULL;
    size_t size = 0;
    size_t ulen;
    FILE *apop_file;

    if (mu_check_perm (APOP_PASSFILE, 0600))
      {
	mu_diag_output (MU_DIAG_INFO,
			_("bad permissions on APOP password file"));
	return NULL;
    }

    apop_file = fopen (APOP_PASSFILE, "r");
    if (apop_file == NULL)
      {
	mu_diag_output (MU_DIAG_INFO,
			_("unable to open APOP password file %s: %s"),
			APOP_PASSFILE, mu_strerror (errno));
	return NULL;
      }

    ulen = strlen (user);
    while (getline (&buf, &size, apop_file) > 0)
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
	      password = strdup (p);
	    break;
	  }
      }

    fclose (apop_file);
    return password;
  }
#endif
}

int
pop3d_apop (char *arg)
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

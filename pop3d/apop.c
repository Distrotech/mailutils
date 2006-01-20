/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

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
  char *password;
  char buf[POP_MAXCMDLEN];

#ifdef USE_DBM
  {
    DBM_FILE db;
    DBM_DATUM key, data;

    int rc = mu_dbm_open (APOP_PASSFILE, &db, MU_STREAM_READ, 0600);
    if (rc)
      {
	if (rc == -1)
	  syslog (LOG_INFO, _("Bad permissions on APOP password db"));
	else
	  syslog (LOG_ERR, _("Unable to open APOP db: %s"),
		  mu_strerror (rc));
	return NULL;
      }

    memset (&key, 0, sizeof key);
    memset (&data, 0, sizeof data);

    strncpy (buf, user, sizeof buf);
    /* strncpy () is lame and does not NULL terminate.  */
    buf[sizeof (buf) - 1] = '\0';
    MU_DATUM_PTR(key) = buf;
    MU_DATUM_SIZE(key) = strlen (buf);

    rc = mu_dbm_fetch (db, key, &data);
    mu_dbm_close (db);
    if (rc)
      {
	syslog (LOG_ERR, _("Cannot fetch APOP data: %s"), mu_strerror (rc));
	return NULL;
      }
    password = calloc (MU_DATUM_SIZE(data) + 1, sizeof (*password));
    if (password == NULL)
      return NULL;

    sprintf (password, "%.*s", (int) MU_DATUM_SIZE(data),
	     (char*) MU_DATUM_PTR(data));
    return password;
  }
#else /* !USE_DBM */
  {
    char *tmp;
    FILE *apop_file;

    if (mu_check_perm (APOP_PASSFILE, 0600))
      {
	syslog (LOG_INFO, _("Bad permissions on APOP password file"));
	return NULL;
    }

    apop_file = fopen (APOP_PASSFILE, "r");
    if (apop_file == NULL)
      {
	syslog (LOG_INFO, _("Unable to open APOP password file %s"),
		strerror (errno));
	return NULL;
      }

    password = calloc (APOP_DIGEST, sizeof (*password));
    if (password == NULL)
      {
	fclose (apop_file);
	pop3d_abquit (ERR_NO_MEM);
      }

    while (fgets (buf, sizeof (buf) - 1, apop_file) != NULL)
      {
	tmp = strchr (buf, ':');
	if (tmp == NULL)
	  continue;
	*tmp++ = '\0';

	if (strncmp (user, buf, strlen (user)))
	  continue;

	strncpy (password, tmp, APOP_DIGEST);
	/* strncpy () is lame and does not NULL terminate.  */
	password[APOP_DIGEST - 1] = '\0';
	tmp = strchr (password, '\n');
	if (tmp)
	  *tmp = '\0';
	break;
      }

    fclose (apop_file);
    if (*password == '\0')
      {
	free (password);
	return NULL;
      }

    return password;
  }
#endif
}

int
pop3d_apop (const char *arg)
{
  char *tmp, *user_digest, *user, *password;
  struct mu_auth_data *auth;
  char buf[POP_MAXCMDLEN];
  struct md5_ctx md5context;
  unsigned char md5digest[16];
  int status;
  int lockit = 1;
  char *mailbox_name = NULL;

  if (state != AUTHORIZATION)
    return ERR_WRONG_STATE;

  if (strlen (arg) == 0)
    return ERR_BAD_ARGS;

  user = pop3d_cmd (arg);
  if (strlen (user) > (POP_MAXCMDLEN - APOP_DIGEST))
    {
      syslog (LOG_INFO, _("User name too long: %s"), user);
      free (user);
      return ERR_BAD_ARGS;
    }
  user_digest = pop3d_args (arg);

  password = pop3d_apopuser (user);
  if (password == NULL)
    {
      syslog (LOG_INFO, _("Password for `%s' not found in the database"),
	      user);
      free (user);
      free (user_digest);
      return ERR_BAD_LOGIN;
    }

  md5_init_ctx (&md5context);
  md5_process_bytes (md5shared, strlen (md5shared), &md5context);
  md5_process_bytes (password, strlen (password), &md5context);
  free (password);
  md5_finish_ctx (&md5context, md5digest);

  {
    int i;
    tmp = buf;
    for (i = 0; i < 16; i++, tmp += 2)
      sprintf (tmp, "%02x", md5digest[i]);
  }

  *tmp++ = '\0';

  if (strcmp (user_digest, buf))
    {
      syslog (LOG_INFO, _("APOP failed for `%s'"), user);
      free (user);
      free (user_digest);
      return ERR_BAD_LOGIN;
    }

  free (user_digest);
  auth = mu_get_auth_by_name (user);
  free (user);
  if (auth == NULL)
    return ERR_BAD_LOGIN;

  /* Reset the uid.  */
  if (auth->change_uid && setuid (auth->uid) == -1)
    {
      syslog (LOG_INFO, _("Cannot change to uid %s: %m"),
	      mu_umaxtostr (0, auth->uid));
      mu_auth_data_free (auth);
      return ERR_BAD_LOGIN;
    }

  if ((status = mu_mailbox_create (&mbox, auth->mailbox)) != 0
      || (status = mu_mailbox_open (mbox, MU_STREAM_RDWR)) != 0)
    {
      mu_mailbox_destroy (&mbox);
      /* For non existent mailbox, we fake.  */
      if (status == ENOENT)
	{
	  if (mu_mailbox_create (&mbox, "/dev/null") != 0
	      || mu_mailbox_open (mbox, MU_STREAM_READ) != 0)
	    {
	      syslog (LOG_ERR, _("Cannot create temporary mailbox: %s"),
		      mu_strerror (status));
	      mu_auth_data_free (auth);
	      free (mailbox_name);
	      state = AUTHORIZATION;
	      return ERR_UNKNOWN;
	    }
	}
      else
	{
	  syslog (LOG_ERR, _("Cannot open mailbox %s: %s"),
		  auth->mailbox,
		  mu_strerror (status));
	  state = AUTHORIZATION;
	  mu_auth_data_free (auth);
	  return ERR_MBOX_LOCK;
	}
      lockit = 0; /* Do not attempt to lock /dev/null ! */
    }

  if (lockit && pop3d_lock())
    {
      mu_auth_data_free (auth);
      mu_mailbox_close(mbox);
      mu_mailbox_destroy(&mbox);
      state = AUTHORIZATION;
      return ERR_MBOX_LOCK;
    }

  state = TRANSACTION;
  username = strdup (auth->name);
  if (username == NULL)
    pop3d_abquit (ERR_NO_MEM);
  pop3d_outf ("+OK opened mailbox for %s\r\n", username);
  mu_auth_data_free (auth);

  /* mailbox name */
  {
    mu_url_t url = NULL;
    mu_mailbox_get_url (mbox, &url);
    syslog (LOG_INFO, _("User `%s' logged in with mailbox `%s'"),
            username, mu_url_to_string (url));
  }
  return OK;
}

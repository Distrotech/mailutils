/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "pop3d.h"

#ifdef HAVE_MYSQL
#include "../MySql/MySql.h"
#endif

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
  int rc;
  char buf[POP_MAXCMDLEN];

#ifdef USE_DBM
  {
    DBM_FILE db;
    DBM_DATUM key, data;

    rc = mu_dbm_open (APOP_PASSFILE, &db, MU_STREAM_READ, 0600);
    if (rc)
      {
	if (rc == -1)
	  syslog (LOG_INFO, "Bad permissions on APOP password db");
	else
	  syslog (LOG_ERR, "Unable to open APOP db: %s",
		  strerror (rc));
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
	syslog (LOG_ERR, "Can't fetch APOP data: %s", strerror (rc));
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
	syslog (LOG_INFO, "Bad permissions on APOP password file");
	return NULL;
    }

    apop_file = fopen (APOP_PASSFILE, "r");
    if (apop_file == NULL)
      {
	syslog (LOG_INFO, "Unable to open APOP password file %s",
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
  struct passwd *pw;
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
      free (user);
      return ERR_BAD_ARGS;
    }
  user_digest = pop3d_args (arg);

  password = pop3d_apopuser (user);
  if (password == NULL)
    {
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
      free (user);
      free (user_digest);
      return ERR_BAD_LOGIN;
    }

  free (user_digest);
  pw = getpwnam (user);
#ifdef HAVE_MYSQL
  if (!pw)
    pw = getMpwnam (user);
#endif /* HAVE_MYSQL */
  free (user);
  if (pw == NULL)
    return ERR_BAD_LOGIN;

  /* Reset the uid.  */
  if (setuid (pw->pw_uid) == -1)
    return ERR_BAD_LOGIN;

  mailbox_name  = calloc (strlen (maildir) + 1
			  + strlen (pw->pw_name) + 1, 1);
  sprintf (mailbox_name, "%s%s", maildir, pw->pw_name);

  if ((status = mailbox_create (&mbox, mailbox_name)) != 0
      || (status = mailbox_open (mbox, MU_STREAM_RDWR)) != 0)
    {
      mailbox_destroy (&mbox);
      /* For non existent mailbox, we fake.  */
      if (status == ENOENT)
	{
	  if (mailbox_create (&mbox, "/dev/null") != 0
	      || mailbox_open (mbox, MU_STREAM_READ) != 0)
	    {
	      free (mailbox_name);
	      state = AUTHORIZATION;
	      return ERR_UNKNOWN;
	    }
	}
      else
	{
	  free (mailbox_name);
	  state = AUTHORIZATION;
	  return ERR_MBOX_LOCK;
	}
      lockit = 0; /* Do not attempt to lock /dev/null ! */
    }
  free (mailbox_name);

  if (lockit && pop3d_lock())
    {
      mailbox_close(mbox);
      mailbox_destroy(&mbox);
      state = AUTHORIZATION;
      return ERR_MBOX_LOCK;
    }

  state = TRANSACTION;
  username = strdup (pw->pw_name);
  if (username == NULL)
    pop3d_abquit (ERR_NO_MEM);
  fprintf (ofile, "+OK opened mailbox for %s\r\n", username);
  /* mailbox name */
  {
    url_t url = NULL;
    mailbox_get_url (mbox, &url);
    syslog (LOG_INFO, "User '%s' logged in with mailbox '%s'",
            username, url_to_string (url));
  }
  return OK;
}

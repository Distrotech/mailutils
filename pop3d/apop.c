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
  struct stat st;

  /* Check the mode, for security reasons.  */
#ifdef WITH_BDB2
  if (stat (APOP_PASSFILE ".db", &st) != -1)
#else
  if (stat (APOP_PASSFILE ".passwd", &st) != -1)
#endif
    if ((st.st_mode & 0777) != 0600)
      {
	syslog (LOG_INFO, "Bad permissions on APOP password file");
	return NULL;
      }

#ifdef WITH_BDB2
  {
    int status;
    DB *dbp;
    DBT key, data;
    status = db_open (APOP_PASSFILE ".db", DB_HASH, DB_RDONLY, 0600, NULL,
		      NULL, &dbp);
    if (status != 0)
      {
	syslog (LOG_ERR, "Unable to open APOP db: %s", strerror (status));
	return NULL;
      }

    memset (&key, 0, sizeof DBT);
    memset (&data, 0, sizeof DBT);

    strncpy (buf, user, sizeof buf);
    /* strncpy () is lame and does not NULL terminate.  */
    buf[sizeof (buf) - 1] = '\0';
    key.data = buf;
    key.size = strlen (buf);
    status = dbp->get (dbp, NULL, &key, &data, 0);
    if (status != 0)
      {
	syslog (LOG_ERR, "db_get error: %s", strerror (status));
	dbp->close (dbp, 0);
	return NULL;
      }

    password = calloc (data.size + 1, sizeof (*password));
    if (password == NULL)
      {
	dbp->close (dbp, 0);
	return NULL;
      }

    sprintf (password, "%.*s", (int) data.size, (char *) data.data);
    dbp->close (dbp, 0);
    return password;
  }
#else /* !WITH_BDBD2 */
  {
    char *tmp;
    FILE *apop_file;
    apop_file = fopen (APOP_PASSFILE ".passwd", "r");
    if (apop_file == NULL)
      {
	syslog (LOG_INFO, "Unable to open APOP password file");
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
#endif /* WITH_BDB2 */
}

int
pop3d_apop (const char *arg)
{
  char *tmp, *user_digest, *password;
  struct passwd *pw;
  char buf[POP_MAXCMDLEN];
  struct md5_ctx md5context;
  unsigned char md5digest[16];
  int status;
  int lockit = 1;

  if (state != AUTHORIZATION)
    return ERR_WRONG_STATE;

  if (strlen (arg) == 0)
    return ERR_BAD_ARGS;

  username = pop3d_cmd (arg);
  if (strlen (username) > (POP_MAXCMDLEN - APOP_DIGEST))
    {
      free (username);
      username = NULL;
      return ERR_BAD_ARGS;
    }
  user_digest = pop3d_args (arg);

  password = pop3d_apopuser (username);
  if (password == NULL)
    {
      free (username);
      username = NULL;
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
      free (username);
      username = NULL;
      free (user_digest);
      return ERR_BAD_LOGIN;
    }

  free (user_digest);
  pw = getpwnam (username);
  if (pw == NULL)
    {
      free (username);
      username = NULL;
      return ERR_BAD_LOGIN;
    }

  /* Reset the uid.  */
  if (setuid (pw->pw_uid) == -1)
    {
      free (username);
      username = NULL;
      return ERR_BAD_LOGIN;
    }

  if ((status = mailbox_create_default (&mbox, username)) != 0
      || (status = mailbox_open (mbox, MU_STREAM_RDWR)) != 0)
    {
      mailbox_destroy (&mbox);
      /* For non existent mailbox, we fake.  */
      if (status == ENOENT)
	{
	  if (mailbox_create (&mbox, "/dev/null") != 0
	      || mailbox_open (mbox, MU_STREAM_READ) != 0)
	    {
	      free (username);
	      username = NULL;
	      state = AUTHORIZATION;
	      return ERR_UNKNOWN;
	    }
	}
      else
	{
	  free (username);
	  username = NULL;
	  state = AUTHORIZATION;
	  return ERR_MBOX_LOCK;
	}
      lockit = 0; /* Do not attempt to lock /dev/null ! */
    }

  if (lockit && pop3d_lock())
    {
      mailbox_close(mbox);
      mailbox_destroy(&mbox);
      state = AUTHORIZATION;
      free (username);
      username = NULL;
      return ERR_MBOX_LOCK;
    }

  state = TRANSACTION;
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

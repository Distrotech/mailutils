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

/* Check if a username exists in APOP password file
   returns pinter to password if found, otherwise NULL */
char *
pop3_apopuser (const char *user)
{
  char *password;
  char buf[POP_MAXCMDLEN];
  struct stat st;
#ifdef BDB2
  int errno;
  DB *dbp;
  DBT key, data;

  if (stat (APOP_PASSFILE ".db", &st) != -1)
#else
  char *tmp;
  FILE *apop_file;

  if (stat (APOP_PASSFILE ".passwd", &st) != -1)
#endif
    if ((st.st_mode & 0777) != 0600)
      {
	syslog (LOG_INFO, "Bad permissions on APOP password file");
	return NULL;
      }

#ifdef BDB2
  errno = db_open (APOP_PASSFILE ".db", DB_HASH, DB_RDONLY, 0600, NULL,
		   NULL, &dbp);
  if (errno != 0)
    {
      syslog (LOG_ERR, "Unable to open APOP database: %s", strerror (errno));
      return NULL;
    }

  memset (&key, 0, sizeof (DBT));
  memset (&data, 0, sizeof (DBT));

  strncpy (buf, user, sizeof (buf));
  key.data = buf;
  key.size = strlen (user);
  errno = dbp->get (dbp, NULL, &key, &data, 0);
  if (errno != 0)
    {
      syslog (LOG_ERR, "db_get error: %s", strerror (errno));
      dbp->close (dbp, 0);
      return NULL;
    }

  password = malloc (sizeof (char) * data.size);
  if (password == NULL)
    {
      dbp->close (dbp, 0);
      return NULL;
    }

  sprintf (password, "%.*s", (int) data.size, (char *) data.data);
  dbp->close (dbp, 0);
  return password;
#else /* !BDBD2 */
  apop_file = fopen (APOP_PASSFILE ".passwd", "r");
  if (apop_file == NULL)
    {
      syslog (LOG_INFO, "Unable to open APOP password file");
      return NULL;
    }

  password = malloc (sizeof (char) * APOP_DIGEST);
  if (password == NULL)
    {
      fclose (apop_file);
      pop3_abquit (ERR_NO_MEM);
    }
  password[0] = '\0';

  while (fgets (buf, sizeof (buf) - 1, apop_file) != NULL)
    {
      tmp = strchr (buf, ':');
      if (tmp == NULL)
	continue;
      *tmp = '\0';
      tmp++;

      if (strncmp (user, buf, strlen (user)))
	continue;

      strncpy (password, tmp, strlen (tmp));
      tmp = strchr (password, '\n');
      if (tmp)
	*tmp = '\0';
      break;
    }

  fclose (apop_file);
  if (strlen (password) == 0)
    {
      free (password);
      return NULL;
    }

  return password;
#endif /* BDB2 */
}

int
pop3_apop (const char *arg)
{
  char *tmp, *user_digest, *password;
  struct passwd *pw;
  char buf[POP_MAXCMDLEN];
  struct md5_ctx md5context;
  unsigned char md5digest[16];
  int i;

  if (state != AUTHORIZATION)
    return ERR_WRONG_STATE;

  if (strlen (arg) == 0)
    return ERR_BAD_ARGS;

  username = pop3_cmd (arg);
  if (strlen (username) > (POP_MAXCMDLEN - APOP_DIGEST))
    {
      free (username);
      return ERR_BAD_ARGS;
    }
  user_digest = pop3_args (arg);

  password = pop3_apopuser (username);
  if (password == NULL)
    {
      free (username);
      free (user_digest);
      return ERR_BAD_LOGIN;
    }

  md5_init_ctx (&md5context);
  md5_process_bytes (md5shared, strlen (md5shared), &md5context);
  md5_process_bytes (password, strlen (password), &md5context);
  free (password);
  md5_finish_ctx (&md5context, md5digest);

  tmp = buf;
  for (i = 0; i < 16; i++, tmp += 2)
    sprintf (tmp, "%02x", md5digest[i]);

  *tmp++ = '\0';

  if (strcmp (user_digest, buf))
    {
      free (username);
      free (user_digest);
      return ERR_BAD_LOGIN;
    }

  free (user_digest);
  pw = getpwnam (username);
  if (pw == NULL)
    {
      free (username);
      return ERR_BAD_LOGIN;
    }

  /* Reset the uid.  */
  /* FIXME: How about the gid.  */
  if (setuid (pw->pw_uid) == -1)
    {
      free (username);
      return ERR_BAD_LOGIN;
    }

  if (mailbox_create_default (&mbox, username) != 0)
    {
      free (username);
      state = AUTHORIZATION;
      return ERR_UNKNOWN;
    }
  else if (mailbox_open (mbox, MU_STREAM_RDWR) != 0)
    {
      free (username);
      state = AUTHORIZATION;
      return ERR_MBOX_LOCK;
    }

  state = TRANSACTION;
  fprintf (ofile, "+OK opened mailbox for %s\r\n", username);
  /* FIXME: how to get mailbox name? */
  syslog (LOG_INFO, "User %s logged in with mailbox %s", username, NULL);
  return OK;
}

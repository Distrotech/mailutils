/* GNU POP3 - a small, fast, and efficient POP3 daemon
   Copyright (C) 1999 Jakob 'sparky' Kaivo <jkaivo@nodomainname.net>

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

#ifdef _USE_APOP

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
  if ((errno = db_open (APOP_PASSFILE ".db", DB_HASH, DB_RDONLY, 0600,
			NULL, NULL, &dbp)) != 0)
    {
      syslog (LOG_ERR, "Unable to open APOP database: %s", strerror (errno));
      return NULL;
    }

  memset (&key, 0, sizeof (DBT));
  memset (&data, 0, sizeof (DBT));

  strncpy (buf, user, sizeof (buf));
  key.data = buf;
  key.size = strlen (user);
  if ((errno = dbp->get (dbp, NULL, &key, &data, 0)) != 0)
    {
      syslog (LOG_ERR, "db_get error: %s", strerror (errno));
      dbp->close (dbp, 0);
      return NULL;
    }

  if ((password = malloc (sizeof (char) * data.size)) == NULL)
    {
      dbp->close (dbp, 0);
      return NULL;
    }

  sprintf (password, "%.*s", (int) data.size, (char *) data.data);
  dbp->close (dbp, 0);
  return password;
#else /* !BDBD2 */
  if ((apop_file = fopen (APOP_PASSFILE ".passwd", "r")) == NULL)
    {
      syslog (LOG_INFO, "Unable to open APOP password file");
      return NULL;
    }

  if ((password = malloc (sizeof (char) * APOP_DIGEST)) == NULL)
    {
      fclose (apop_file);
      pop3_abquit (ERR_NO_MEM);
    }
  password[0] = '\0';

  while (fgets (buf, sizeof (buf) - 1, apop_file) != NULL)
    {
      tmp = index (buf, ':');
      *tmp = '\0';
      tmp++;

      if (strncmp (user, buf, strlen (user)))
	continue;

      strncpy (password, tmp, strlen (tmp));
      tmp = index (password, '\n');
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

  tmp = pop3_cmd (arg);
  if (strlen (tmp) > (POP_MAXCMDLEN - APOP_DIGEST))
    return ERR_BAD_ARGS;
  if ((username = strdup (tmp)) == NULL)
    pop3_abquit (ERR_NO_MEM);
  free (tmp);

  tmp = pop3_args (arg);
  if (strlen (tmp) > APOP_DIGEST)
    return ERR_BAD_ARGS;
  if ((user_digest = strdup (tmp)) == NULL)
    pop3_abquit (ERR_NO_MEM);
  free (tmp);

  if ((password = pop3_apopuser (username)) == NULL)
    return ERR_BAD_LOGIN;

  md5_init_ctx (&md5context);
  md5_process_bytes (md5shared, strlen (md5shared), &md5context);
  md5_process_bytes (password, strlen (password), &md5context);
  /* pop3_wipestr (password); */
  free (password);
  md5_finish_ctx (&md5context, md5digest);

  tmp = buf;
  for (i = 0; i < 16; i++, tmp += 2)
    sprintf (tmp, "%02x", md5digest[i]);

  *tmp++ = '\0';

  if (strcmp (user_digest, buf))
    return ERR_BAD_LOGIN;

  if ((pw = getpwnam (username)) == NULL)
    return ERR_BAD_LOGIN;

#ifdef MAILSPOOLHOME
  mailbox = malloc ((strlen (pw->pw_dir) + strlen (MAILSPOOLHOME) + 1) * sizeof (char));
  if (mailbox == NULL)
    pop3_abquit (ERR_NO_MEM);
  strcpy (mailbox, pw->pw_dir);
  strcat (mailbox, MAILSPOOLHOME);
  mbox = fopen (mailbox, "r");
  if (mbox == NULL)
    {
      free (mailbox);
      chdir (_PATH_MAILDIR);
#endif
      mailbox = malloc (sizeof (char) * (strlen (_PATH_MAILDIR) + strlen (username) + 2));
      strcpy (mailbox, _PATH_MAILDIR "/");
      strcat (mailbox, username);
      if (mailbox == NULL)
	pop3_abquit (ERR_NO_MEM);

      mbox = fopen (mailbox, "r");
      if (mbox == NULL)
	{
	  mailbox = strdup ("/dev/null");
	  if (mailbox == NULL)
	    pop3_abquit (ERR_NO_MEM);
	  mbox = fopen (mailbox, "r");
	}
#ifdef MAILSPOOLHOME
    }
  else
#endif
  if (pop3_lock () != OK)
    {
      pop3_unlock ();
      free (mailbox);
      state = AUTHORIZATION;
      return ERR_MBOX_LOCK;
    }

  if ((pw->pw_uid < 1)
#ifdef MAILSPOOLHOME
  /* Drop mail group for home dirs */
      || setgid (pw->pw_gid) == -1
#endif
      || setuid (pw->pw_uid) == -1)
    {
      pop3_unlock ();
      free (mailbox);
      return ERR_BAD_LOGIN;
    }

  state = TRANSACTION;
  messages = NULL;
  pop3_getsizes ();
  fprintf (ofile, "+OK opened mailbox for %s\r\n", username);
  syslog (LOG_INFO, "User %s logged in with mailbox %s", username, mailbox);
  cursor = 0;
  return OK;
}

#else

int
pop3_apop (const char *foo)
{
  return ERR_NOT_IMPL;
}

#endif

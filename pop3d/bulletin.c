/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007 Free Software Foundation, Inc.

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

static char *bulletin_mbox_name;
static char *bulletin_db_name;

void
set_bulletin_db (char *file)
{
  bulletin_db_name = file;
}

static void
close_bulletin_mailbox (mu_mailbox_t *pmbox)
{
  if (pmbox)
    {
      mu_mailbox_close (*pmbox);
      mu_mailbox_destroy (pmbox);
    }
}

static int
open_bulletin_mailbox (mu_mailbox_t *pmbox)
{
  int status;
  mu_mailbox_t tmbox;
  
  if ((status = mu_mailbox_create (&tmbox, bulletin_mbox_name)) != 0)
    {
      mu_error (_("Cannot create bulletin mailbox `%s': %s"),
		bulletin_mbox_name, mu_strerror (status));
      return 1;
    }

  if ((status = mu_mailbox_open (tmbox, MU_STREAM_READ)) != 0)
    {
      mu_mailbox_destroy (pmbox);
      mu_error (_("Cannot open bulletin mailbox `%s': %s"),
		bulletin_mbox_name, mu_strerror (status));
      return 1;
    }
  if (!pmbox)
    close_bulletin_mailbox (&tmbox);
  else
    *pmbox = tmbox;
  return 0;
}

int
set_bulletin_source (char *source)
{
  bulletin_mbox_name = source;
  return 0;
}

static int
read_popbull_file (size_t *pnum)
{
  int rc = 1;
  FILE *fp;
  char *filename = mu_tilde_expansion ("~/.popbull", "/", auth_data->dir);
  
  if (!filename)
    return 1;
  fp = fopen (filename, "r");
  if (fp)
    {
      char buf[128];
      char *p = fgets (buf, sizeof buf, fp);
      if (p)
	{
	  *pnum = strtoul (buf, &p, 0);
	  rc = *p && !isspace (*p);
	}
      fclose (fp);
    }
  return rc;
}

static int
write_popbull_file (size_t num)
{
  int rc = 1;
  FILE *fp;
  char *filename = mu_tilde_expansion ("~/.popbull", "/", auth_data->dir);
  
  if (!filename)
    return 1;
  fp = fopen (filename, "w");
  if (fp)
    {
      fprintf (fp, "%s\n", mu_umaxtostr (0, num));
      fclose (fp);
      rc = 0;
    }
  return rc;
}

#ifdef USE_DBM
int
read_bulletin_db (size_t *pnum)
{
  DBM_FILE db;
  DBM_DATUM key, data;
  int rc;
  char sbuf[128];
  char *bufptr;
  char *buf = NULL;
  size_t s;
  char *p;
  
  rc = mu_dbm_open (bulletin_db_name, &db, MU_STREAM_READ, 0660);
  if (rc)
    {
      mu_error (_("Unable to open bulletin db for reading: %s"),
		mu_strerror (errno));
      return rc;
    }

  memset (&key, 0, sizeof key);
  memset (&data, 0, sizeof data);

  MU_DATUM_PTR(key) = username;
  MU_DATUM_SIZE(key) = strlen (username);

  rc = mu_dbm_fetch (db, key, &data);

  if (rc)
    {
      mu_error (_("Cannot fetch bulletin db data: %s"),
		mu_strerror (errno));
      mu_dbm_close (db);
      return 1;
    }
  
  s = MU_DATUM_SIZE (data);
  if (s < sizeof sbuf)
    bufptr = sbuf;
  else
    {
      buf = malloc (s + 1);
      if (!buf)
	{
	  mu_error("%s", mu_strerror (errno));
	  return 1;
	}
      bufptr = buf;
    }

  memcpy (bufptr, MU_DATUM_PTR (data), s);
  bufptr[s] = 0;
  mu_dbm_datum_free(&data);
  mu_dbm_close (db);

  rc = 1;
  *pnum = strtoul (bufptr, &p, 0);
  if (*p == 0)
    rc = 0;
  else
    {
#ifdef QPOPPER_COMPAT
      if (s == sizeof long)
	{
	  long n;

	  n = *(long*)MU_DATUM_PTR (data);
	  if (n >= 0)
	    {
	      syslog (LOG_INFO,
		      _("assuming bulletin database is in qpopper format"));
	      *pnum = n;
	      rc = 0;
	    }
	}
      else
#endif /* QPOPPER_COMPAT */
	{
	  mu_error (_("Wrong bulletin database format for `%s'"),
		    username);
	}
    }
  
  free (buf);
  return rc;
}

int
write_bulletin_db (size_t num)
{
  DBM_FILE db;
  DBM_DATUM key, data;
  int rc;
  char *p;
  
  rc = mu_dbm_open (bulletin_db_name, &db, MU_STREAM_RDWR, 0660);
  if (rc)
    {
      mu_error (_("Unable to open bulletin db for writing: %s"),
		mu_strerror (errno));
      return rc;
    }

  memset (&key, 0, sizeof key);
  memset (&data, 0, sizeof data);

  MU_DATUM_PTR (key) = username;
  MU_DATUM_SIZE (key) = strlen (username);
  p = mu_umaxtostr (0, num);
  MU_DATUM_PTR (data) = p;
  MU_DATUM_SIZE (data) = strlen (p);

  rc = mu_dbm_insert (db, key, data, 1);
  if (rc)
    mu_error (_("Cannot store datum in bulletin db"));

  mu_dbm_close (db);
  return rc;
}
#endif /* USE_DBM */
      
size_t
get_last_delivered_num ()
{
  size_t num = 0;
  
#ifdef USE_DBM  
  if (bulletin_db_name && read_bulletin_db (&num) == 0)
    return num;
#endif

  read_popbull_file (&num);
  return num;
}

void
store_last_delivered_num (size_t num)
{
#ifdef USE_DBM  
  if (bulletin_db_name && write_bulletin_db (num) == 0)
    return;
#endif
  write_popbull_file (num);
}

void
deliver_pending_bulletins ()
{
  mu_mailbox_t bull;
  int rc;
  size_t lastnum, total;

  if (!bulletin_mbox_name)
    return;
  
  rc = open_bulletin_mailbox (&bull);
  if (rc)
    return;
  lastnum = get_last_delivered_num ();

  rc = mu_mailbox_messages_count (bull, &total);
  if (rc)
    mu_error (_("Cannot count bulletins: %s"), mu_strerror (rc));
  else
    {
      syslog (LOG_DEBUG,
	      "user %s, last bulletin %lu, total bulletins %lu",
	      username, (unsigned long) lastnum, (unsigned long) total);
	  
      if (lastnum < total)
	{
	  size_t i;
	  size_t count = total - lastnum;
  
	  syslog (LOG_INFO,
		  ngettext ("user %s: delivering %lu pending bulletin",
			    "user %s: delivering %lu pending bulletins",
			    count),
		  username, (unsigned long) count);

	  for (i = lastnum + 1; i <= total; i++)
	    {
	      int rc;
	      mu_message_t msg;
	  
	      if ((rc = mu_mailbox_get_message (bull, i, &msg)) != 0)
		{
		  mu_error (_("Cannot read bulletin %lu: %s"),
			    (unsigned long) i, mu_strerror (rc));
		  break;
		}
	
	      if ((rc = mu_mailbox_append_message (mbox, msg)) != 0)
		{
		  mu_error (_("Cannot append message %lu: %s"),
			    (unsigned long) i, mu_strerror (rc));
		  break;
		}
	    }
	  store_last_delivered_num (i - 1);
	}
    }
    
  close_bulletin_mailbox (&bull);
}
    

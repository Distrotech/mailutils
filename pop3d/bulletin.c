/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007-2012 Free Software Foundation, Inc.

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

static char *bulletin_mbox_name;
static char *bulletin_db_name;

void
set_bulletin_db (const char *file)
{
  bulletin_db_name = mu_strdup (file);
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
      mu_error (_("cannot create bulletin mailbox `%s': %s"),
		bulletin_mbox_name, mu_strerror (status));
      return 1;
    }

  if ((status = mu_mailbox_open (tmbox, MU_STREAM_READ)) != 0)
    {
      mu_mailbox_destroy (&tmbox);
      mu_error (_("cannot open bulletin mailbox `%s': %s"),
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
set_bulletin_source (const char *source)
{
  bulletin_mbox_name = mu_strdup (source);
  return 0;
}

static int
read_popbull_file (size_t *pnum)
{
  int rc = 1;
  FILE *fp;
  char *filename = mu_tilde_expansion ("~/.popbull", MU_HIERARCHY_DELIMITER,
				       auth_data->dir);
  
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
	  rc = *p && !mu_isspace (*p);
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
  char *filename = mu_tilde_expansion ("~/.popbull", MU_HIERARCHY_DELIMITER,
				       auth_data->dir);
  
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

#ifdef ENABLE_DBM
int
read_bulletin_db (size_t *pnum)
{
  mu_dbm_file_t db;
  struct mu_dbm_datum key, data;
  int rc;
  char sbuf[128];
  char *bufptr;
  char *buf = NULL;
  size_t s;
  char *p;

  rc = mu_dbm_create (bulletin_db_name, &db, DEFAULT_GROUP_DB_SAFETY);
  if (rc)
    {
      mu_diag_output (MU_DIAG_ERROR, _("unable to create bulletin db"));
      return rc;
    }

  rc = mu_dbm_safety_check (db);
  if (rc)
    {
      if (rc == ENOENT)
	{
	  *pnum = 0;
	  return 0;
	}
      mu_diag_output (MU_DIAG_ERROR,
		      _("bulletin db %s fails safety check: %s"),
		      bulletin_db_name, mu_strerror (rc));
      mu_dbm_destroy (&db);
      return 1;
    }
  
  rc = mu_dbm_open (db, MU_STREAM_READ, 0660);
  if (rc)
    {
      mu_error (_("unable to open bulletin db for reading: %s"),
		mu_strerror (rc));
      return rc;
    }

  memset (&key, 0, sizeof key);
  memset (&data, 0, sizeof data);

  key.mu_dptr = username;
  key.mu_dsize = strlen (username);

  rc = mu_dbm_fetch (db, &key, &data);

  if (rc == MU_ERR_NOENT)
    {
      *pnum = 0;
      return 0;
    }
  else if (rc)
    {
      mu_error (_("cannot fetch bulletin db data: %s"),
		mu_dbm_strerror (db));
      mu_dbm_destroy (&db);
      return 1;
    }
  mu_dbm_destroy (&db);
  
  s = data.mu_dsize;
  if (s < sizeof sbuf)
    bufptr = sbuf;
  else
    {
      buf = malloc (s + 1);
      if (!buf)
	{
	  mu_error ("%s", mu_strerror (errno));
	  return 1;
	}
      bufptr = buf;
    }

  memcpy (bufptr, data.mu_dptr, s);
  bufptr[s] = 0;
  mu_dbm_datum_free (&data);

  rc = 1;
  *pnum = strtoul (bufptr, &p, 0);
  if (*p == 0)
    rc = 0;
  else
    {
      mu_error (_("wrong bulletin database format for `%s'"),
		username);
    }
  
  free (buf);
  return rc;
}

int
write_bulletin_db (size_t num)
{
  mu_dbm_file_t db;
  struct mu_dbm_datum key, data;
  int rc;
  const char *p;
  
  rc = mu_dbm_create (bulletin_db_name, &db, DEFAULT_GROUP_DB_SAFETY);
  if (rc)
    {
      mu_diag_output (MU_DIAG_ERROR, _("unable to create bulletin db"));
      return rc;
    }

  rc = mu_dbm_safety_check (db);
  if (rc && rc != ENOENT)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("bulletin db %s fails safety check: %s"),
		      bulletin_db_name, mu_strerror (rc));
      mu_dbm_destroy (&db);
      return rc;
    }

  rc = mu_dbm_open (db, MU_STREAM_RDWR, 0660);
  if (rc)
    {
      mu_error (_("unable to open bulletin db for writing: %s"),
		mu_strerror (rc));
      mu_dbm_destroy (&db);
      return rc;
    }

  memset (&key, 0, sizeof key);
  memset (&data, 0, sizeof data);

  key.mu_dptr = username;
  key.mu_dsize = strlen (username);
  p = mu_umaxtostr (0, num);
  data.mu_dptr = (char *) p;
  data.mu_dsize = strlen (p);

  rc = mu_dbm_store (db, &key, &data, 1);
  if (rc)
    mu_error (_("cannot store datum in bulletin db: %s"),
	      mu_dbm_strerror (db));
  
  mu_dbm_destroy (&db);
  return rc;
}
#endif /* ENABLE_DBM */
      
int
get_last_delivered_num (size_t *pret)
{
#ifdef ENABLE_DBM  
  if (bulletin_db_name && read_bulletin_db (pret) == 0)
    return 0;
#endif
  return read_popbull_file (pret);
}

void
store_last_delivered_num (size_t num)
{
#ifdef ENABLE_DBM  
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
  if (rc || get_last_delivered_num (&lastnum))
    return;

  rc = mu_mailbox_messages_count (bull, &total);
  if (rc)
    mu_error (_("cannot count bulletins: %s"), mu_strerror (rc));
  else
    {
      mu_diag_output (MU_DIAG_DEBUG,
	      "user %s, last bulletin %lu, total bulletins %lu",
	      username, (unsigned long) lastnum, (unsigned long) total);
	  
      if (lastnum < total)
	{
	  size_t i;
	  size_t count = total - lastnum;
  
	  mu_diag_output (MU_DIAG_INFO,
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
		  mu_error (_("cannot read bulletin %lu: %s"),
			    (unsigned long) i, mu_strerror (rc));
		  break;
		}
	
	      if ((rc = mu_mailbox_append_message (mbox, msg)) != 0)
		{
		  mu_error (_("cannot append message %lu: %s"),
			    (unsigned long) i, mu_strerror (rc));
		  break;
		}
	    }
	  store_last_delivered_num (i - 1);
	}
    }
    
  close_bulletin_mailbox (&bull);
}
    

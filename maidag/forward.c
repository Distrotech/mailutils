/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007-2012 Free Software Foundation,
   Inc.

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

/* ".forward" support for GNU Maidag */

#include "maidag.h"

/* Auxiliary functions */

/* Forward message MSG to given EMAIL, using MAILER and sender address FROM */
static int
forward_to_email (mu_message_t msg, mu_address_t from,
		  mu_mailer_t mailer, const char *email)
{
  mu_address_t to;
  int rc;
  
  rc = mu_address_create (&to, email);
  if (rc)
    {
      mu_error (_("%s: cannot create email: %s"), email, mu_strerror (rc));
      return 1;
    }

  rc = mu_mailer_send_message (mailer, msg, from, to);
  if (rc)
    mu_error (_("Sending message to `%s' failed: %s"),
	      email, mu_strerror (rc));
  mu_address_destroy (&to);
  return rc;
}

/* Create a mailer if it does not already exist.*/
int
forward_mailer_create (mu_mailer_t *pmailer)
{
  int rc;

  if (*pmailer == NULL)
    {
      rc = mu_mailer_create (pmailer, NULL);
      if (rc)
	{
	  const char *url = NULL;
	  mu_mailer_get_url_default (&url);
	  mu_error (_("Creating mailer `%s' failed: %s"),
		    url, mu_strerror (rc));
	  return 1;
	}

  
      rc = mu_mailer_open (*pmailer, 0);
      if (rc)
	{
	  const char *url = NULL;
	  mu_mailer_get_url_default (&url);
	  mu_error (_("Opening mailer `%s' failed: %s"),
		    url, mu_strerror (rc));
	  mu_mailer_destroy (pmailer);
	  return 1;
	}
    }
  return 0;
}

/* Create *PFROM (if it is NULL), from the envelope sender address of MSG. */
static int
create_from_address (mu_message_t msg, mu_address_t *pfrom)
{
  if (!*pfrom)
    {
      mu_envelope_t envelope;
      const char *str;
      int status = mu_message_get_envelope (msg, &envelope);
      if (status)
	{
	  mu_error (_("cannot get envelope: %s"), mu_strerror (status));
	  return 1;
	}
      status = mu_envelope_sget_sender (envelope, &str);
      if (status)
	{
	  mu_error (_("cannot get envelope sender: %s"), mu_strerror (status));
	  return 1;
	}
      status = mu_address_create (pfrom, str);
      if (status)
	{
	  mu_error (_("%s: cannot create email: %s"), str,
		    mu_strerror (status));
	  return 1;
	}
    }
  return 0;
}       


/* Forward message MSG as requested by file FILENAME.
   MYNAME gives local user name. */
enum maidag_forward_result
process_forward (mu_message_t msg, char *filename, const char *myname)
{
  int rc;
  mu_stream_t file;
  size_t size = 0, n;
  char *buf = NULL;
  enum maidag_forward_result result = maidag_forward_ok;
  mu_mailer_t mailer = NULL;
  mu_address_t from = NULL;

  rc = mu_file_stream_create (&file, filename, MU_STREAM_READ);
  if (rc)
    {
      mu_error (_("%s: cannot open forward file: %s"),
		filename, mu_strerror (rc));
      return maidag_forward_error;
    }

  while (mu_stream_getline (file, &buf, &size, &n) == 0 && n > 0)
    {
      char *p;

      mu_rtrim_class (buf, MU_CTYPE_SPACE);
      p = mu_str_skip_class (buf, MU_CTYPE_SPACE);

      if (*p && *p != '#')
	{
	  if (strchr (p, '@'))
	    {
	      if (create_from_address (msg, &from)
		  || forward_mailer_create (&mailer)
		  || forward_to_email (msg, from, mailer, p))
		result = maidag_forward_error;
	    }
	  else 
	    {
	      if (*p == '\\')
		p++;
	      if (strcmp (p, myname) == 0)
		{
		  if (result == maidag_forward_ok)
		    result = maidag_forward_metoo;
		}
	      else if (deliver_to_user (msg, p, NULL))
		result = maidag_forward_error;
	    }
	}
    }

  mu_address_destroy (&from);
  if (mailer)
    {
      mu_mailer_close (mailer);
      mu_mailer_destroy (&mailer);
    }
  free (buf);
  mu_stream_destroy (&file);
  return result;
}


static mu_list_t idlist;

/* Check if the forward file FWFILE for user given by AUTH exists, and if
   so, use to to forward message MSG. */
enum maidag_forward_result
maidag_forward (mu_message_t msg, struct mu_auth_data *auth, char *fwfile)
{
  struct stat st;
  char *filename;
  enum maidag_forward_result result = maidag_forward_none;
  int rc;
  
  if (fwfile[0] != '/')
    {
      if (stat (auth->dir, &st))
	{
	  if (errno == ENOENT)
	    /* FIXME: a warning, maybe? */;
	  else if (!S_ISDIR (st.st_mode))
	    mu_error (_("%s: not a directory"), auth->dir);
	  else
	    mu_error (_("%s: cannot stat directory: %s"),
		      auth->dir, mu_strerror (errno));
	  return maidag_forward_none;
	}
      filename = mu_make_file_name (auth->dir, fwfile);
    }
  else
    filename = strdup (fwfile);
  
  if (!filename)
    {
      mu_error ("%s", mu_strerror (errno));
      return maidag_forward_error;
    }

  if (!idlist)
    mu_list_create (&idlist);

  rc = mu_file_safety_check (filename, forward_file_checks,
			     auth->uid, idlist);
  if (rc == 0)
    result = process_forward (msg, filename, auth->name);
  else if (rc == MU_ERR_EXISTS)
    mu_diag_output (MU_DIAG_NOTICE,
		    _("skipping forward file %s: already processed"),
		    filename);
  else
    mu_error (_("ignoring forward file %s: %s"),
	      filename, mu_strerror (rc));
  
  free (filename);
  return result;
}


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

#include "maidag.h"

static mu_message_t
make_tmp (const char *from)
{
  int rc;
  mu_stream_t in, out;
  char *buf = NULL;
  size_t size = 0, n;
  mu_message_t mesg;
  
  rc = mu_stdio_stream_create (&in, MU_STDIN_FD, MU_STREAM_READ);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stdio_stream_create",
		       "MU_STDIN_FD", rc);
      exit (EX_TEMPFAIL);
    } 

  rc = mu_temp_file_stream_create (&out, NULL, 0);
  if (rc)
    {
      maidag_error (_("unable to open temporary file: %s"), mu_strerror (rc));
      exit (EX_TEMPFAIL);
    }

  rc = mu_stream_getline (in, &buf, &size, &n);
  if (rc)
    {
      maidag_error (_("read error: %s"), mu_strerror (rc));
      mu_stream_destroy (&in);
      mu_stream_destroy (&out);
      exit (EX_TEMPFAIL);
    }
  if (n == 0)
    {
      maidag_error (_("unexpected EOF on input"));
      mu_stream_destroy (&in);
      mu_stream_destroy (&out);
      exit (EX_TEMPFAIL);
    } 

  if (n >= 5 && memcmp (buf, "From ", 5))
    {
      struct mu_auth_data *auth = NULL;
      if (!from)
	{
	  auth = mu_get_auth_by_uid (getuid ());
	  if (auth)
	    from = auth->name;
	}
      if (from)
	{
	  time_t t;
	  struct tm *tm;
	  
	  time (&t);
	  tm = gmtime (&t);
	  mu_stream_printf (out, "From %s ", from);
	  mu_c_streamftime (out, "%c%n", tm, NULL);
	}
      else
	{
	  maidag_error (_("cannot determine sender address"));
	  mu_stream_destroy (&in);
	  mu_stream_destroy (&out);
	  exit (EX_TEMPFAIL);
	}
      if (auth)
	mu_auth_data_free (auth);
    }

  mu_stream_write (out, buf, n, NULL);
  free (buf);
  
  rc = mu_stream_copy (out, in, 0, NULL);
  mu_stream_destroy (&in);
  if (rc)
    {
      maidag_error (_("copy error: %s"), mu_strerror (rc));
      mu_stream_destroy (&out);
      exit (EX_TEMPFAIL);
    }

  rc = mu_stream_to_message (out, &mesg);
  mu_stream_destroy (&out);
  if (rc)
    {
      maidag_error (_("error creating temporary message: %s"),
		    mu_strerror (rc));
      exit (EX_TEMPFAIL);
    }

  return mesg;
}

int
maidag_stdio_delivery (maidag_delivery_fn delivery_fun, int argc, char **argv)
{
  mu_message_t mesg = make_tmp (sender_address);
  
  if (multiple_delivery)
    multiple_delivery = argc > 1;

  for (; *argv; argv++)
    {
      delivery_fun (mesg, *argv, NULL);
      if (multiple_delivery)
	exit_code = EX_OK;
    }
  return exit_code;
}

static int biff_fd = -1;
static struct sockaddr_in biff_in;
static const char *biff_user_name;

static int
notify_action (mu_observer_t obs, size_t type, void *data, void *action_data)
{
  if (type == MU_EVT_MAILBOX_MESSAGE_APPEND && biff_user_name)
    {
      mu_message_qid_t qid = data;
      mu_mailbox_t mbox = mu_observer_get_owner (obs);
      mu_url_t url;
      char *buf;
      
      mu_mailbox_get_url (mbox, &url);
      mu_asprintf (&buf, "%s@%s:%s", biff_user_name,
		   qid, mu_url_to_string (url));
      if (buf)
	{
	  sendto (biff_fd, buf, strlen (buf), 0,
		  (struct sockaddr *)&biff_in, sizeof biff_in);
	  free (buf);
	}
    }
  return 0;
}

static void
attach_notify (mu_mailbox_t mbox)
{
  struct servent *sp;
  mu_observer_t observer;
  mu_observable_t observable;

  if (biff_fd == -1)
    {
      if ((sp = getservbyname ("biff", "udp")) == NULL)
	{
	  biff_fd = -2;
	  return;
	}
      biff_in.sin_family = AF_INET;
      biff_in.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
      biff_in.sin_port = sp->s_port;
      
      biff_fd = socket (PF_INET, SOCK_DGRAM, 0);
      if (biff_fd < 0)
	{
	  biff_fd = -2;
	  return;
	}
    }

  if (biff_fd)
    {
      mu_observer_create (&observer, mbox);
      mu_observer_set_action (observer, notify_action, mbox);
      mu_mailbox_get_observable (mbox, &observable);
      mu_observable_attach (observable, MU_EVT_MAILBOX_MESSAGE_APPEND, 
                            observer);
    }
}  

int
deliver_to_mailbox (mu_mailbox_t mbox, mu_message_t msg,
		    struct mu_auth_data *auth,
		    char **errp)
{
  int status;
  char *path;
  mu_url_t url = NULL;
  mu_locker_t lock;
  int failed = 0;
  
  mu_mailbox_get_url (mbox, &url);
  path = (char*) mu_url_to_string (url);

  status = mu_mailbox_open (mbox, MU_STREAM_APPEND|MU_STREAM_CREAT);
  if (status != 0)
    {
      maidag_error (_("cannot open mailbox %s: %s"), 
                    path, mu_strerror (status));
      return EX_TEMPFAIL;
    }

  attach_notify (mbox);
  
  /* FIXME: This is superfluous, as mu_mailbox_append_message takes care
     of locking anyway. But I leave it here for the time being. */
  mu_mailbox_get_locker (mbox, &lock);

  if (lock)
    {
      status = mu_locker_lock (lock);

      if (status)
	{
	  maidag_error (_("cannot lock mailbox `%s': %s"), path,
		        mu_strerror (status));
	  exit_code = EX_TEMPFAIL;
	  return EX_TEMPFAIL;
	}
    }
  
#if defined(USE_MAILBOX_QUOTAS)
  if (auth)
    {
      mu_off_t n;
      size_t msg_size;
      mu_off_t mbsize;
      
      if ((status = mu_mailbox_get_size (mbox, &mbsize)))
	{
	  maidag_error (_("cannot get size of mailbox %s: %s"),
			path, mu_strerror (status));
	  if (status == ENOSYS)
	    mbsize = 0; /* Try to continue anyway */
	  else
	    return EX_TEMPFAIL;
	}
    
      switch (check_quota (auth, mbsize, &n))
	{
	case MQUOTA_EXCEEDED:
	  maidag_error (_("%s: mailbox quota exceeded for this recipient"),
			auth->name);
	  if (errp)
	    mu_asprintf (errp, "%s: mailbox quota exceeded for this recipient",
		         auth->name);
	  exit_code = EX_QUOTA();
	  failed++;
	  break;
	  
	case MQUOTA_UNLIMITED:
	  break;
	  
	default:
	  if ((status = mu_message_size (msg, &msg_size)))
	    {
	      maidag_error (_("cannot get message size (input message %s): %s"),
			    path, mu_strerror (status));
	      exit_code = EX_UNAVAILABLE;
	      failed++;
	    }
	  else if (msg_size > n)
	    {
	      maidag_error (_("%s: message would exceed maximum mailbox size for "
			      "this recipient"),
			    auth->name);
	      if (errp)
		mu_asprintf (errp,
			     "%s: message would exceed maximum mailbox size "
			     "for this recipient",
			     auth->name);
	      exit_code = EX_QUOTA();
	      failed++;
	    }
	  break;
	}
    }
#endif
  
  if (!failed && switch_user_id (auth, 1) == 0)
    {
      status = mu_mailbox_append_message (mbox, msg);
      if (status)
	{
	  maidag_error (_("error writing to mailbox %s: %s"),
		        path, mu_strerror (status));
	  failed++;
	}
      else
	{
	  status = mu_mailbox_sync (mbox);
	  if (status)
	    {
	      maidag_error (_("error flushing mailbox %s: %s"),
			    path, mu_strerror (status));
	      failed++;
	    }
	}
      switch_user_id (auth, 0);
    }

  mu_mailbox_close (mbox);
  mu_locker_unlock (lock);
  return failed ? exit_code : 0;
}

static int
is_remote_url (mu_url_t url)
{
  int rc, res;

  if (!url)
    return 0;
  
  rc = mu_registrar_test_local_url (url, &res);
  return rc == 0 && res == 0;
}

static int
do_delivery (mu_url_t url, mu_message_t msg, const char *name, char **errp)
{
  struct mu_auth_data *auth = NULL;
  mu_mailbox_t mbox;
  int status;

  if (name && !is_remote_url (url))
    {
      auth = mu_get_auth_by_name (name);
      if (!auth)
	{
	  maidag_error (_("%s: no such user"), name);
	  if (errp)
	    mu_asprintf (errp, "%s: no such user", name);
	  exit_code = EX_NOUSER;
	  return EX_NOUSER;
	}

      if (current_uid)
	auth->change_uid = 0;

      if (switch_user_id (auth, 1))
	return EX_TEMPFAIL;
      status = script_apply (msg, auth);
      if (switch_user_id (auth, 0))
	return EX_TEMPFAIL;
      if (status)
	{
	  exit_code = EX_OK;
	  mu_auth_data_free (auth);
	  return 0;
	}
 
      if (forward_file)
	switch (maidag_forward (msg, auth, forward_file))
	  {
	  case maidag_forward_none:
	  case maidag_forward_metoo:
	    break;
	    
	  case maidag_forward_ok:
	    mu_auth_data_free (auth);
	    return 0;

	  case maidag_forward_error:
	    mu_auth_data_free (auth);
	    return exit_code = EX_TEMPFAIL;
	  }
    }
  
  if (!url)
    {
      status = mu_url_create (&url, auth->mailbox);
      if (status)
	{
	  maidag_error (_("cannot create URL for %s: %s"),
			auth->mailbox, mu_strerror (status));
	  return exit_code = EX_UNAVAILABLE;
	}
    }      

  status = mu_mailbox_create_from_url (&mbox, url);

  if (status)
    {
      maidag_error (_("cannot open mailbox %s: %s"),
		    mu_url_to_string (url),
		    mu_strerror (status));
      mu_url_destroy (&url);
      mu_auth_data_free (auth);
      return EX_TEMPFAIL;
    }

  biff_user_name = name;
  
  /* Actually open the mailbox. Switch to the user's euid to make
     sure the maildrop file will have right privileges, in case it
     will be created */
  if (switch_user_id (auth, 1))
    return EX_TEMPFAIL;
  status = deliver_to_mailbox (mbox, msg, auth, errp);
  if (switch_user_id (auth, 0))
    return EX_TEMPFAIL;

  mu_auth_data_free (auth);
  mu_mailbox_destroy (&mbox);

  return status;
}

int
deliver_to_url (mu_message_t msg, char *dest_id, char **errp)
{
  int status;
  const char *name;
  mu_url_t url = NULL;
  
  status = mu_url_create (&url, dest_id);
  if (status)
    {
      maidag_error (_("%s: cannot create url: %s"), dest_id,
		    mu_strerror (status));
      return EX_NOUSER;
    }
  status = mu_url_sget_user (url, &name);
  if (status == MU_ERR_NOENT)
    name = NULL;
  else if (status)
    {
      maidag_error (_("%s: cannot get user name from url: %s"),
		    dest_id, mu_strerror (status));
      mu_url_destroy (&url);
      return EX_NOUSER;
    }
  return do_delivery (url, msg, name, errp);
}
  
int
deliver_to_user (mu_message_t msg, char *dest_id, char **errp)
{
  return do_delivery (NULL, msg, dest_id, errp);
}

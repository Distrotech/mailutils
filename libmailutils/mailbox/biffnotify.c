/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2014-2016 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mailutils/errno.h>
#include <mailutils/observer.h>
#include <mailutils/io.h>
#include <mailutils/url.h>
#include <mailutils/mu_auth.h>
#include <mailutils/sys/mailbox.h>

static int
get_notify_fd (mu_mailbox_t mbox)
{
  int fd = mbox->notify_fd;
  
  if (fd == -1)
    {
      struct sockaddr_in biff_in;
      struct servent *sp;
      
      if ((sp = getservbyname ("biff", "udp")) == NULL)
	return -1;
      
      biff_in.sin_family = AF_INET;
      biff_in.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
      biff_in.sin_port = sp->s_port;
      
      fd = socket (PF_INET, SOCK_DGRAM, 0);
      if (fd < 0)
	return -1;

      mbox->notify_sa = malloc (sizeof (biff_in));

      if (!mbox->notify_sa)
	{
	  close (fd);
	  return -1;
	}
      memcpy (mbox->notify_sa, &biff_in, sizeof (biff_in));
      mbox->notify_fd = fd;
    }
  return fd;
}

static int
biff_notify (mu_observer_t obs, size_t type, void *data, void *action_data)
{
  mu_mailbox_t mbox = mu_observer_get_owner (obs);

  if (type == MU_EVT_MAILBOX_MESSAGE_APPEND && mbox->notify_user)
    {
      mu_message_qid_t qid = data;
      mu_url_t url;
      char *buf;
      int fd = get_notify_fd (mbox);

      if (fd < 0)
	{
	  mu_observable_t observable;
	  mu_mailbox_get_observable (mbox, &observable);
	  mu_observable_detach (observable, obs);
	  return 0;
	}
      
      mu_mailbox_get_url (mbox, &url);
      if (mu_asprintf (&buf, "%s@%s:%s", mbox->notify_user,
		       qid, mu_url_to_string (url)))
	return 0;
      
      mu_mailbox_flush (mbox, 0);
      if (buf)
	{
	  sendto (fd, buf, strlen (buf), 0,
		  mbox->notify_sa, sizeof (struct sockaddr_in));
	  /*FIXME: on error?*/
	  free (buf);
	}
    }
  return 0;
}

int
mu_mailbox_set_notify (mu_mailbox_t mbox, const char *user)
{
  mu_observer_t observer;
  mu_observable_t observable;

  if (!mbox)
    return EINVAL;

  if (user)
    user = strdup (user);
  else
    {
      struct mu_auth_data *auth = NULL;

      auth = mu_get_auth_by_uid (getuid ());
      if (!auth)
	return MU_ERR_NO_SUCH_USER;
      user = strdup (auth->name);
    }

  if (!user)
    return ENOMEM;

  if (mbox->notify_user)
    free (mbox->notify_user);
  mbox->notify_user = (char*) user;

  if (!(mbox->flags & _MU_MAILBOX_NOTIFY))
    {
      mu_observer_create (&observer, mbox);
      mu_observer_set_action (observer, biff_notify, mbox);
      mu_mailbox_get_observable (mbox, &observable);
      mu_observable_attach (observable, MU_EVT_MAILBOX_MESSAGE_APPEND, 
			    observer);

      mbox->flags |= _MU_MAILBOX_NOTIFY;
    }
  
  return 0;
}

int
mu_mailbox_unset_notify (mu_mailbox_t mbox)
{
  if (!mbox)
    return EINVAL;
  if (!mbox->notify_user)
    return EINVAL;
  free (mbox->notify_user);
  mbox->notify_user = NULL;
  close (mbox->notify_fd);
  mbox->notify_fd = -1;
  free (mbox->notify_sa);
  mbox->notify_sa = NULL;
  return 0;
}
  

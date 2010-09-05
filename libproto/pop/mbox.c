/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2003, 2005, 2007, 2009, 2010 Free
   Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_POP

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <mailutils/pop3.h>
#include <mailutils/attribute.h>
#include <mailutils/auth.h>
#include <mailutils/body.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/header.h>
#include <mailutils/message.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>
#include <mailutils/secret.h>
#include <mailutils/tls.h>
#include <mailutils/md5.h>
#include <mailutils/io.h>
#include <mailutils/mutil.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>
#include <mailutils/opool.h>

#include <mailutils/sys/folder.h>
#include <mailutils/sys/mailbox.h>
#include <mailutils/sys/registrar.h>
#include <mailutils/sys/url.h>

#define _POP3_MSG_INBODY  0x01
#define _POP3_MSG_SKIPHDR 0x02
#define _POP3_MSG_SKIPBDY 0x04

struct _pop3_message
{
  int flags;
  size_t body_size;
  size_t header_size;
  size_t body_lines;
  size_t header_lines;
  size_t message_size;
  size_t num;
  char *uidl; /* Cache the uidl string.  */
  int attr_flags;
  mu_message_t message;
  struct _pop3_mailbox *mpd; /* Back pointer.  */
};
  
struct _pop3_mailbox
{
  mu_pop3_t pop3;             /* mu_pop3_t is the working horse */
  int pops;                   /* true if pop3 over SSL is being used */
  int is_updated;             /* true if the mailbox info is up to date */
  
  size_t msg_count;           /* Number of messages in the mailbox */
  mu_off_t total_size;        /* Total mailbox size. */
  struct _pop3_message **msg; /* Array of messages */
  size_t msg_max;             /* Actual size of the array */  
  mu_mailbox_t mbox;          /* MU mailbox corresponding to this one. */

  char *user;     /* Temporary holders for user and passwd.  */
  mu_secret_t secret;
};

static int
pop_open (mu_mailbox_t mbox, int flags)
{
  struct _pop3_mailbox *mpd = mbox->data;
  int status;
  const char *host;
  long port = mpd->pops ? MU_POPS_PORT : MU_POP_PORT;
  mu_stream_t stream;
  
  /* Sanity checks. */
  if (mpd == NULL)
    return EINVAL;
  
  /* Fetch the pop server name and the port in the mu_url_t.  */
  status = mu_url_sget_host (mbox->url, &host);
  if (status != 0)
    return status;
  mu_url_get_port (mbox->url, &port);

  mbox->flags = flags;

  status = mu_tcp_stream_create (&stream, host, port, mbox->flags);
  if (status)
    return status;
#ifdef WITH_TLS
  if (mpd->pops)
    {
      mu_stream_t newstr;
      
      status = mu_stream_open (stream);
      if (status)
	{
	  mu_stream_destroy (&stream);
	  return status;
	}
      
      status = mu_tls_client_stream_create (&newstr, stream, stream, 0);
      mu_stream_unref (stream);
      if (status)
	{
	  mu_error ("pop_open: mu_tls_client_stream_create: %s",
		    mu_strerror (status));
	  return status;
	}
      stream = newstr;
    }
#endif /* WITH_TLS */

  /* FIXME: How to configure buffer size? */
  mu_stream_set_buffer (stream, mu_buffer_line, 1024);

  status = mu_pop3_create (&mpd->pop3);
  if (status)
    {
      mu_stream_destroy (&stream);
      return status;
    }
  mu_pop3_set_carrier (mpd->pop3, stream);

  if (mu_debug_check_level (mbox->debug, MU_DEBUG_PROT))
    mu_pop3_trace (mpd->pop3, MU_POP3_TRACE_SET);

  do
    {
      status = mu_pop3_connect (mpd->pop3);
      if (status)
	break;

      status = mu_pop3_capa (mpd->pop3, 1, NULL);
      if (status)
	break;

      if (WITH_TLS && !mpd->pops &&
	  mu_pop3_capa_test (mpd->pop3, "STLS", NULL) == 0)
	{
	  status = mu_pop3_stls (mpd->pop3);
	  if (status)
	    break;
	}

      status = mu_authority_authenticate (mbox->folder->authority);
    }
  while (0);
  
  if (status)
    mu_pop3_destroy (&mpd->pop3);
  return status;
}
  
static int
pop_close (mu_mailbox_t mbox)
{
  struct _pop3_mailbox *mpd = mbox->data;
  int status;
  
  status = mu_pop3_quit (mpd->pop3);
  if (status)
    mu_error ("mu_pop3_quit failed: %s", mu_strerror (status));
  status = mu_pop3_disconnect (mpd->pop3);
  if (status)
    mu_error ("mu_pop3_disconnect failed: %s", mu_strerror (status));
  mu_pop3_destroy (&mpd->pop3);
  return 0;
}

static void
pop_destroy (mu_mailbox_t mbox)
{
  struct _pop3_mailbox *mpd = mbox->data;
  if (mpd)
    {
       size_t i;
      mu_monitor_wrlock (mbox->monitor);
      /* Destroy the pop messages and ressources associated to them.  */
      for (i = 0; i < mpd->msg_count; i++)
	{
	  if (mpd->msg[i])
	    {
	      mu_message_destroy (&mpd->msg[i]->message, mpd->msg[i]);
	      if (mpd->msg[i]->uidl)
		free (mpd->msg[i]->uidl);
	      free (mpd->msg[i]);
	    }
	}
      mu_pop3_destroy (&mpd->pop3);
      if (mpd->user)
	free (mpd->user);
      if (mpd->secret)
	mu_secret_unref (mpd->secret);
   }
}

/* Update and scanning.  */
static int
pop_is_updated (mu_mailbox_t mbox)
{
  struct _pop3_mailbox *mpd = mbox->data;
  if (mpd == NULL)
    return 0;
  return mpd->is_updated;
}
      
/* Return the number of messages in the mailbox */
static int
pop_messages_count (mu_mailbox_t mbox, size_t *pcount)
{
  struct _pop3_mailbox *mpd = mbox->data;
  int status;
  
  if (mpd == NULL)
    return EINVAL;

  if (pop_is_updated (mbox))
    {
      if (pcount)
	*pcount = mpd->msg_count;
      return 0;
    }

  status = mu_pop3_stat (mpd->pop3, &mpd->msg_count, &mpd->total_size);
  if (status == 0)
    {
      if (pcount)
	*pcount = mpd->msg_count;
      mpd->is_updated = 1;
    }
  return status;
}
  
static int
pop_scan (mu_mailbox_t mbox, size_t msgno, size_t *pcount)
{
  int status;
  size_t i;
  size_t count = 0;

  status = pop_messages_count (mbox, &count);
  if (status != 0)
    return status;
  if (pcount)
    *pcount = count;
  if (mbox->observable == NULL)
    return 0;
  for (i = msgno; i <= count; i++)
    {
      size_t tmp = i;
      if (mu_observable_notify (mbox->observable, MU_EVT_MESSAGE_ADD,
				&tmp) != 0)
	break;
      if (((i +1) % 10) == 0)
	{
	  mu_observable_notify (mbox->observable, MU_EVT_MAILBOX_PROGRESS,
				NULL);
	}
    }
  return 0;
}

/* There's no way to retrieve this info via POP3 */
static int
pop_message_unseen (mu_mailbox_t mbox, size_t *punseen)
{
  size_t count = 0;
  int status = pop_messages_count (mbox, &count);
  if (status != 0)
    return status;
  if (punseen)
    *punseen = (count > 0) ? 1 : 0;
  return 0;
}

static int
pop_get_size (mu_mailbox_t mbox, mu_off_t *psize)
{
  struct _pop3_mailbox *mpd = mbox->data;
  int status = 0;

  if (mpd == NULL)
    return EINVAL;

  if (!pop_is_updated (mbox))
    status = pop_messages_count (mbox, NULL);
  if (psize)
    *psize = mpd->total_size;
  return status;
}


static int
pop_create_message (struct _pop3_message *mpm, struct _pop3_mailbox *mpd)
{
  int status;
  mu_message_t msg;
  
  status = mu_message_create (&msg, mpm);
  if (status)
    return status;
  // FIXME...
  mpm->message = msg;
  return 0;
}


/* ------------------------------------------------------------------------- */
/* Header */

static int
pop_header_fill (void *data, char **pbuf, size_t *plen)
{
  struct _pop3_message *mpm = data;
  struct _pop3_mailbox *mpd = mpm->mpd;
  mu_stream_t stream;
  mu_opool_t opool;
  int status;
  
  status = mu_opool_create (&opool, 0);
  if (status)
    return status;
  
  if (mu_pop3_capa_test (mpd->pop3, "TOP", NULL) == 0)
    status = mu_pop3_top (mpd->pop3, mpm->num, 0, &stream);
  else
    status = mu_pop3_retr (mpd->pop3, mpm->num, &stream);

  if (status == 0)
    {
      size_t size = 0;
      char *buf = NULL;
      size_t n;

      while (mu_stream_getline (stream, &buf, &size, &n) == 0
	     && n > 0)
	{
	  size_t len = mu_rtrim_cset (buf, "\r\n");
	  if (len == 0)
	    break;
	  mu_opool_append (opool, buf, len);
	  mu_opool_append_char (opool, '\n');
	}

      if (!mu_stream_eof (stream))
	/* Drain the stream. */
	while (mu_stream_getline (stream, &buf, &size, &n) == 0
	       && n > 0);
      mu_stream_destroy (&stream);

      n = mu_opool_size (opool);
      if (n > size)
	{
	  char *p = realloc (buf, n);
	  if (!p)
	    {
	      free (buf);
	      mu_opool_destroy (&opool);
	      return ENOMEM;
	    }
	  buf = p;
	}

      mu_opool_copy (opool, buf, n);
      *pbuf = buf;
      *plen = n;
      status = 0;
    }
  mu_opool_destroy (&opool);
  
  return status;
}

static int
pop_create_header (struct _pop3_message *mpm)
{
  int status;
  mu_header_t header = NULL;

  status = mu_header_create (&header, NULL, 0);
  if (status)
    return status;
  mu_header_set_fill (header, pop_header_fill, mpm);
  mu_message_set_header (mpm->message, header, mpm);
  return 0;
}


/* ------------------------------------------------------------------------- */
/* Attributes */

/* There is no POP3 command to return message attributes, therefore we
   have to recurse to reading the "Status:" header.  Unfortunately, some
   servers remove it when you dowload a message, and in this case a message
   will always look like new even if you already read it.  There is also
   no way to set an attribute on remote mailbox via the POP server and
   many server once you do a RETR (and in some cases a TOP) will mark the
   message as read (Status: RO).  Even worse, some servers may be configured
   to delete after the RETR, and some go as much as deleting after the TOP,
   since technicaly you can download a message via TOP without RETR'eiving
   it.  */
static int
pop_get_attribute (mu_attribute_t attr, int *pflags)
{
  struct _pop3_message *mpm = mu_attribute_get_owner (attr);
  char hdr_status[64];
  mu_header_t header = NULL;

  if (mpm == NULL || pflags == NULL)
    return EINVAL;
  if (mpm->attr_flags == 0)
    {
      hdr_status[0] = '\0';

      mu_message_get_header (mpm->message, &header);
      mu_header_get_value (header, MU_HEADER_STATUS,
			   hdr_status, sizeof hdr_status, NULL);
      mu_string_to_flags (hdr_status, &mpm->attr_flags);
    }
  *pflags = mpm->attr_flags;
  return 0;
}

static int
pop_set_attribute (mu_attribute_t attr, int flags)
{
  struct _pop3_message *mpm = mu_attribute_get_owner (attr);

  if (mpm == NULL)
    return EINVAL;
  mpm->attr_flags |= flags;
  return 0;
}

static int
pop_unset_attribute (mu_attribute_t attr, int flags)
{
  struct _pop3_message *mpm = mu_attribute_get_owner (attr);

  if (mpm == NULL)
    return EINVAL;
  mpm->attr_flags &= ~flags;
  return 0;
}

static int
pop_create_attribute (struct _pop3_message *mpm)
{
  int status;
  mu_attribute_t attribute;
  
  status = mu_attribute_create (&attribute, mpm);
  if (status)
    return status;

  mu_attribute_set_get_flags (attribute, pop_get_attribute, mpm);
  mu_attribute_set_set_flags (attribute, pop_set_attribute, mpm);
  mu_attribute_set_unset_flags (attribute, pop_unset_attribute, mpm);
  mu_message_set_attribute (mpm->message, attribute, mpm);
  return 0;
}

/* ------------------------------------------------------------------------- */
/* Body */

static int
pop_create_body (struct _pop3_message *mpm)
{
  /* FIXME */
  return ENOSYS;
}


/* FIXME: change prototype to avoid fixed size buffer */
static int
pop_uidl (mu_message_t msg, char *buffer, size_t buflen, size_t *pnwriten)
{
  struct _pop3_message *mpm = mu_message_get_owner (msg);
  struct _pop3_mailbox *mpd = mpm->mpd;
  size_t len;
  
  if (!mpm->uidl)
    {
      if (mu_pop3_capa_test (mpd->pop3, "UIDL", NULL) == 0)
	{
	  int status = mu_pop3_uidl (mpd->pop3, mpm->num, &mpm->uidl);
	  if (status)
	    return status;
	}
      else
	return ENOSYS;
    }

  len = strlen (mpm->uidl);
  if (buffer)
    {
      buflen--; /* Leave space for the null.  */
      buflen = (len > buflen) ? buflen : len;
      memcpy (buffer, mpm->uidl, buflen);
      buffer[buflen] = 0;
    }
  else
    buflen = len;
  if (pnwriten)
    *pnwriten = buflen;
  return 0;
}

static int
pop_uid (mu_message_t msg,  size_t *puid)
{
  struct _pop3_message *mpm = mu_message_get_owner (msg);
  if (puid)
    *puid = mpm->num;
  return 0;
}

static int
pop_get_message (mu_mailbox_t mbox, size_t msgno, mu_message_t *pmsg)
{
  struct _pop3_mailbox *mpd = mbox->data;
  struct _pop3_message *mpm;
  int status;

  /* Sanity checks. */
  if (pmsg == NULL || mpd == NULL)
    return EINVAL;

  /* If we did not start a scanning yet do it now.  */
  if (!pop_is_updated (mbox))
    pop_scan (mbox, 1, NULL);

  if (msgno > mpd->msg_count)
    return EINVAL;

  if (!mpd->msg)
    {
      mpd->msg = calloc (mpd->msg_count, sizeof (mpd->msg[0]));
      if (!mpd->msg)
	return ENOMEM;
    }
  if (mpd->msg[msgno])
    {
      *pmsg = mpd->msg[msgno]->message;
      return 0;
    }

  mpm = calloc (1, sizeof (*mpm));
  if (mpm == NULL)
    return ENOMEM;

  /* Back pointer.  */
  mpm->mpd = mpd;
  mpm->num = msgno;
  
  status = pop_create_message (mpm, mpd);
  if (status)
    {
      free (mpm);
      return status;
    }

  do
    {
      status = pop_create_header (mpm);
      if (status)
	break;
      status = pop_create_attribute (mpm);
      if (status)
	break;
      status = pop_create_body (mpm);
    }
  while (0);
  
  if (status)
    {
      mu_message_destroy (&mpm->message, mpm);
      free (mpm);
      return status;
    }

  if (mu_pop3_capa_test (mpd->pop3, "UIDL", NULL) == 0)
    mu_message_set_uidl (mpm->message, pop_uidl, mpm);

  mu_message_set_uid (mpm->message, pop_uid, mpm);

  mpd->msg[msgno] = mpm;
  mu_message_set_mailbox (mpm->message, mbox, mpm);
  *pmsg = mpm->message;
  return 0;
}

static int
_pop3_mailbox_init (mu_mailbox_t mbox, int pops)
{
  struct _pop3_mailbox *mpd;
  int status = 0;

  /* Allocate specifics for pop data.  */
  mpd = mbox->data = calloc (1, sizeof (*mpd));
  if (mbox->data == NULL)
    return ENOMEM;

  mpd->pop3 = NULL;
  
  mpd->mbox = mbox;		/* Back pointer.  */
  mpd->pops = pops;

  /* Initialize the structure.  */
  mbox->_destroy = pop_destroy;

  mbox->_open = pop_open;
  mbox->_close = pop_close;
  mbox->_messages_count = pop_messages_count;
  /* FIXME: There is no way to retrieve the number of recent messages via
     POP3 protocol, so we consider all messages recent. */
  mbox->_messages_recent = pop_messages_count;
  mbox->_is_updated = pop_is_updated;
  mbox->_scan = pop_scan;
  mbox->_message_unseen = pop_message_unseen;
  mbox->_get_size = pop_get_size;
  
#if 0 //FIXME
  /* Messages.  */
  mbox->_get_message = pop_get_message;
  mbox->_expunge = pop_expunge;



  /* Set our properties.  */
  {
    mu_property_t property = NULL;
    mu_mailbox_get_property (mbox, &property);
    mu_property_set_value (property, "TYPE", "POP3", 1);
  }

#endif
  /* Hack! POP does not really have a folder.  */
  mbox->folder->data = mbox;
  return status;
}

int
_mailbox_pop_init (mu_mailbox_t mbox)
{
  return _pop3_mailbox_init (mbox, 0);
}

int
_mailbox_pops_init (mu_mailbox_t mbox)
{
  return _pop3_mailbox_init (mbox, 1);
}


/* Authentication */

/* Extract the User from the URL or the ticket.  */
static int
pop_get_user (mu_authority_t auth)
{
  mu_folder_t folder = mu_authority_get_owner (auth);
  mu_mailbox_t mbox = folder->data;
  struct _pop3_mailbox *mpd = mbox->data;
  mu_ticket_t ticket = NULL;
  int status;
  /*  Fetch the user from them.  */

  mu_authority_get_ticket (auth, &ticket);
  if (mpd->user)
    {
      free (mpd->user);
      mpd->user = NULL;
    }
  /* Was it in the URL? */
  status = mu_url_aget_user (mbox->url, &mpd->user);
  if (status == MU_ERR_NOENT)
    status = mu_ticket_get_cred (ticket, mbox->url, "Pop User: ",
				 &mpd->user, NULL);
  if (status == MU_ERR_NOENT || mpd->user == NULL)
    return MU_ERR_NOUSERNAME;  
  return status;
}

/* Extract the User from the URL or the ticket.  */
static int
pop_get_passwd (mu_authority_t auth)
{
  mu_folder_t folder = mu_authority_get_owner (auth);
  mu_mailbox_t mbox = folder->data;
  struct _pop3_mailbox *mpd = mbox->data;
  mu_ticket_t ticket = NULL;
  int status;

  mu_authority_get_ticket (auth, &ticket);
  /* Was it in the URL? */
  status = mu_url_get_secret (mbox->url, &mpd->secret);
  if (status == MU_ERR_NOENT)
    status = mu_ticket_get_cred (ticket, mbox->url, "Pop Passwd: ",
				 NULL, &mpd->secret);
  if (status == MU_ERR_NOENT || !mpd->secret)
    /* FIXME: Is this always right? The user might legitimately have
       no password */
    return MU_ERR_NOPASSWORD;
  return 0;
}

int
_pop_apop (mu_authority_t auth)
{
  mu_folder_t folder = mu_authority_get_owner (auth);
  mu_mailbox_t mbox = folder->data;
  struct _pop3_mailbox *mpd = mbox->data;
  int status;
  
  status = pop_get_user (auth);
  if (status)
    return status;

  /* Fetch the secret from them.  */
  status = pop_get_passwd (auth);
  if (status)
    return status;
  status = mu_pop3_apop (mpd->pop3, mpd->user,
			 mu_secret_password (mpd->secret));
  mu_secret_password_unref (mpd->secret);
  mu_secret_unref (mpd->secret);
  mpd->secret = NULL;
  free (mpd->user);
  mpd->user = NULL;
  return status;
}

int
_pop_user (mu_authority_t auth)
{
  mu_folder_t folder = mu_authority_get_owner (auth);
  mu_mailbox_t mbox = folder->data;
  struct _pop3_mailbox *mpd = mbox->data;
  int status;
  
  status = pop_get_user (auth);
  if (status)
    return status;
  status = mu_pop3_user (mpd->pop3, mpd->user);
  if (status == 0)
    {
      /* Fetch the secret from them.  */
      status = pop_get_passwd (auth);
      if (status == 0) 
	{
	  status = mu_pop3_pass (mpd->pop3, mu_secret_password (mpd->secret));
	  mu_secret_password_unref (mpd->secret);
	  mu_secret_unref (mpd->secret);
	  mpd->secret = NULL;
	}
    }
  free (mpd->user);
  mpd->user = NULL;
  return status;
}


#endif /* ENABLE_POP */

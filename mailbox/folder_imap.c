/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fnmatch.h>

#include <imap0.h>

/* Variable use for the registrar.  */
static struct _record _imap_record =
{
  MU_IMAP_SCHEME,
  _url_imap_init,     /* url entry.  */
  _mailbox_imap_init, /* Mailbox entry.  */
  NULL,              /* Mailer entry.  */
  _folder_imap_init,  /* Folder entry.  */
  NULL, /* No need for a back pointer.  */
  NULL, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};

/* We export this variable: url parsing and the initialisation of the mailbox,
   via the register entry/record.  */
record_t imap_record = &_imap_record;

/* Concrete IMAP implementation.  */
static void folder_imap_destroy    __P ((folder_t));
static int folder_imap_delete      __P ((folder_t, const char *));
static int folder_imap_list        __P ((folder_t, const char *, const char *,
					 struct folder_list *));
static int folder_imap_lsub        __P ((folder_t, const char *, const char *,
					 struct folder_list *));
static int folder_imap_rename      __P ((folder_t, const char *,
					 const char *));
static int folder_imap_subscribe   __P ((folder_t, const char *));
static int folder_imap_unsubscribe __P ((folder_t, const char *));

/* Private */
/* static int imap_readline (f_imap_t); */
static int imap_fetch          __P ((f_imap_t));
static int imap_flags          __P ((f_imap_t));
static int imap_bodystructure  __P ((f_imap_t));
static int imap_literal_string __P ((f_imap_t));
static int imap_string         __P ((f_imap_t));
static int imap_quoted_string  __P ((f_imap_t, char *));

/* Initialize the concrete IMAP mailbox: overload the folder functions  */
int
_folder_imap_init (folder_t folder)
{
  f_imap_t f_imap;
  f_imap = folder->data = calloc (1, sizeof (*f_imap));
  if (f_imap == NULL)
    return ENOMEM;
  f_imap->folder = folder;
  f_imap->state = IMAP_NO_STATE;

  folder->_destroy = folder_imap_destroy;

  folder->_open = folder_imap_open;
  folder->_close = folder_imap_close;

  folder->_list = folder_imap_list;
  folder->_lsub = folder_imap_lsub;
  folder->_subscribe = folder_imap_subscribe;
  folder->_unsubscribe = folder_imap_unsubscribe;
  folder->_delete = folder_imap_delete;
  folder->_rename = folder_imap_rename;

  return 0;
}

/* Destroy the resources.  */
static void
folder_imap_destroy (folder_t folder)
{
  if (folder->data)
    {
      f_imap_t f_imap = folder->data;
      if (f_imap->buffer)
	free (f_imap->buffer);
      if (f_imap->capa)
	free (f_imap->capa);
      free (f_imap);
      folder->data = NULL;
    }
}

/* Simple User/pass authentication for imap.  */
static int
imap_user (authority_t auth)
{
  folder_t folder = authority_get_owner (auth);
  f_imap_t f_imap = folder->data;
  ticket_t ticket;
  int status = 0;

  switch (f_imap->state)
    {
    case IMAP_AUTH:
      {
	/* Grab the User and Passwd information.  */
	size_t n = 0;
	authority_get_ticket (auth, &ticket);
	if (f_imap->user)
	  free (f_imap->user);
	if (f_imap->passwd)
	  free (f_imap->passwd);
	/* Was it in the URL?  */
	status = url_get_user (folder->url, NULL, 0, &n);
        if (status != 0 || n == 0)
	  ticket_pop (ticket, "Imap User: ",  &f_imap->user);
	else
	  {
	    f_imap->user = calloc (1, n + 1);
	    url_get_user (folder->url, f_imap->user, n + 1, NULL);
	  }
	ticket_pop (ticket, "Imap Passwd: ",  &f_imap->passwd);
	if (f_imap->user == NULL || f_imap->passwd == NULL)
	  {
	    CHECK_ERROR_CLOSE (folder, f_imap, EINVAL);
	  }
	status = imap_writeline (f_imap, "g%d LOGIN %s %s\r\n",
				 f_imap->seq++, f_imap->user, f_imap->passwd);
	CHECK_ERROR_CLOSE(folder, f_imap, status);
	FOLDER_DEBUG1 (folder, MU_DEBUG_PROT, "LOGIN %s *\n", f_imap->user);
	free (f_imap->user);
	f_imap->user = NULL;
	/* We have to nuke the passwd.  */
	memset (f_imap->passwd, '\0', strlen (f_imap->passwd));
	free (f_imap->passwd);
	f_imap->passwd = NULL;
	f_imap->state = IMAP_LOGIN;
      }

    case IMAP_LOGIN:
      /* Send it across.  */
      imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      /* Clear the buffer it contains the passwd. */
      memset (f_imap->buffer, '\0', f_imap->buflen);
      f_imap->state = IMAP_LOGIN_ACK;

    case IMAP_LOGIN_ACK:
      /* Get the login ack.  */
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_AUTH_DONE;

    default:
      break;  /* We're outta here.  */
    }
  CLEAR_STATE (f_imap);
  return 0;
}

/* Create/Open the stream for IMAP.  */
int
folder_imap_open (folder_t folder, int flags)
{
  f_imap_t f_imap = folder->data;
  char *host;
  long port = 143; /* default imap port.  */
  int status = 0;
  size_t len = 0;
  int preauth = 0; /* Do we have "preauth"orisation ?  */

  /* If we are already open for business, noop.  */
  if (f_imap->isopen)
    return 0;

  /* Fetch the pop server name and the port in the url_t.  */
  status = url_get_host (folder->url, NULL, 0, &len);
  if (status != 0)
    return status;
  host = alloca (len + 1);
  url_get_host (folder->url, host, len + 1, NULL);
  url_get_port (folder->url, &port);

  folder->flags = flags | MU_STREAM_IMAP;

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      /* allocate working io buffer.  */
      if (f_imap->buffer == NULL)
        {
	  /* There is no particular limit on the length of a command/response
	     in IMAP. We start with 255, which is quite reasonnable and grow
	     as we go along.  */
          f_imap->buflen = 255;
          f_imap->buffer = calloc (f_imap->buflen + 1, sizeof (char));
          if (f_imap->buffer == NULL)
            {
              CHECK_ERROR (f_imap, ENOMEM);
            }
        }
      else
        {
	  /* Clear from any residue.  */
          memset (f_imap->buffer, '\0', f_imap->buflen);
        }
      f_imap->ptr = f_imap->buffer;

      /* Create the networking stack.  */
      if (folder->stream == NULL)
        {
          status = tcp_stream_create (&(folder->stream));
          CHECK_ERROR (f_imap, status);
	  /* Ask for the stream internal buffering mechanism scheme.  */
	  stream_setbufsiz (folder->stream, BUFSIZ);
        }
      else
        stream_close (folder->stream);
      FOLDER_DEBUG2 (folder, MU_DEBUG_PROT, "imap_open (%s:%d)\n", host, port);
      f_imap->state = IMAP_OPEN_CONNECTION;

    case IMAP_OPEN_CONNECTION:
      /* Establish the connection.  */
      status = stream_open (folder->stream, host, port, folder->flags);
      CHECK_EAGAIN (f_imap, status);
      /* Can't recover bailout.  */
      CHECK_ERROR_CLOSE (folder, f_imap, status);
      f_imap->state = IMAP_GREETINGS;

    case IMAP_GREETINGS:
      {
        /* Swallow the greetings.  */
        status = imap_readline (f_imap);
        CHECK_EAGAIN (f_imap, status);
	f_imap->ptr = f_imap->buffer;
        FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);
	/* Are they open for business ?  The server send an untag response
	   for greeting. Thenically it can be OK/PREAUTH/BYE.  The BYE is
	   the one that we do not want, server being unfriendly.  */
	preauth = (strncasecmp (f_imap->buffer, "* PREAUTH", 9) == 0);
        if (strncasecmp (f_imap->buffer, "* OK", 4) != 0 && ! preauth)
          {
            CHECK_ERROR_CLOSE (folder, f_imap, EACCES);
          }

        if (folder->authority == NULL && !preauth)
          {
	    char auth[64] = "";
	    size_t n = 0;
            url_get_auth (folder->url, auth, 64, &n);
            if (n == 0 || strcasecmp (auth, "*") == 0)
              {
                authority_create (&(folder->authority), folder->ticket,
				  folder);
                authority_set_authenticate (folder->authority, imap_user,
					    folder);
              }
	    else
              {
		/* No other type of Authentication is supported yet.  */
                /* What can we do ? flag an error ?  */
              }
          }
        f_imap->state = IMAP_AUTH;
      }

    case IMAP_AUTH:
    case IMAP_LOGIN:
    case IMAP_LOGIN_ACK:
      if (!preauth)
	{
	  status = authority_authenticate (folder->authority);
	  CHECK_EAGAIN (f_imap, status);
	}

    case IMAP_AUTH_DONE:
    default:
      break;
    }
  f_imap->state = IMAP_NO_STATE;
  f_imap->isopen = 1;
  return 0;
}

/* Shutdown the connection.  */
int
folder_imap_close (folder_t folder)
{
  f_imap_t f_imap = folder->data;
  int status = 0;

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap, "g%d LOGOUT\r\n", f_imap->seq++);
      CHECK_ERROR (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_LOGOUT;

    case IMAP_LOGOUT:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_LOGOUT_ACK;

    case IMAP_LOGOUT_ACK:
      /* Check for "* Bye" from the imap server.  */
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);
      /* This is done when we received the BYE in the parser code.  */
      /* stream_close (folder->stream); */
      /* f_imap->isopen = 0 ; */

    default:
      break;
    }
  f_imap->state = IMAP_NO_STATE;
  return 0;
}

/* Remove a mailbox.  */
static int
folder_imap_delete (folder_t folder, const char *name)
{
  f_imap_t f_imap = folder->data;
  int status = 0;

  if (name == NULL)
    return EINVAL;
  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap, "g%d DELETE %s\r\n", f_imap->seq++,
			       name);
      CHECK_ERROR (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_DELETE;

    case IMAP_DELETE:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_DELETE_ACK;

    case IMAP_DELETE_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);

    default:
      break;
    }
  f_imap->state = IMAP_NO_STATE;
  return status;
}

/* Since the mailutils API does not offer recursive listing. There is no need
   to follow IMAP "bizarre" recursive rules. The use of '%' is sufficient.  So
   the approach is everywhere there is a regex in the path we change that
   branch for '%' and do the matching ourself with fnmatch().  */
static int
folder_imap_list (folder_t folder, const char *ref, const char *name,
		  struct folder_list *pflist)
{
  f_imap_t f_imap = folder->data;
  int status = 0;
  char *path = NULL;

  /* NOOP.  */
  if (pflist == NULL)
    return EINVAL;

  if (ref == NULL)
    ref = "";
  if (name == NULL)
    name = "";

  path = strdup ("");
  if (path == NULL)
    return ENOMEM;

  /* We break the string to pieces and change the occurences of "*?[" for
     the imap magic "%" for expansion.  Then reassemble the string:
     "/home/?/Mail/a*lain*" --> "/usr/%/Mail/%".  */
  {
    int done = 0;
    size_t i;
    char **node = NULL;
    size_t nodelen = 0;
    const char *p = name;
    /* Disassemble.  */
    while (!done && *p)
      {
	char **n;
	n = realloc (node, (nodelen + 1) * sizeof (*node));
	if (n == NULL)
	  break;
	node = n;
	if (*p == '/')
	  {
	    node[nodelen] = strdup ("/");
	    p++;
	  }
	else
	  {
	    const char *s = strchr (p, '/');
	    if (s)
	      {
		node[nodelen] = calloc (s - p + 1,  sizeof (char));
		if (node[nodelen])
		  memcpy (node[nodelen], p, s - p);
		p = s;
	      }
	    else
	      {
		node[nodelen] = strdup (p);
		done = 1;
	      }
	    if (node[nodelen] && strpbrk (node[nodelen], "*?["))
	      {
		free (node[nodelen]);
		node[nodelen] = strdup ("%");
            }
	  }
	nodelen++;
	if (done)
	  break;
      }
    /* Reassemble.  */
    for (i = 0; i < nodelen; i++)
      {
	if (node[i])
	  {
	    char *pth;
	    pth = realloc (path, strlen (path) + strlen (node[i]) + 1);
	    if (pth)
	      {
		path = pth;
		strcat (path, node[i]);
	      }
	    free (node[i]);
	  }
      }
    if (node)
      free (node);
  }

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap, "g%d LIST \"%s\" \"%s\"\r\n",
			       f_imap->seq++, ref, path);
      free (path);
      CHECK_ERROR (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_LIST;

    case IMAP_LIST:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_LIST_ACK;

    case IMAP_LIST_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);

    default:
      break;
    }

  /* Build the folder list.  */
  if (f_imap->callback.flist.num > 0)
    {
      struct list_response **plist = NULL;
      size_t num = f_imap->callback.flist.num;
      size_t j = 0;
      plist = calloc (num, sizeof (*plist));
      if (plist)
	{
	  size_t i;
	  for (i = 0; i < num; i++)
	    {
	      struct list_response *lr = f_imap->callback.flist.element[i];
	      //printf ("%s --> %s\n", lr->name, name);
	      if (fnmatch (name, lr->name, 0) == 0)
		{
		  plist[i] = calloc (1, sizeof (**plist));
		  if (plist[i] == NULL
		      || (plist[i]->name = strdup (lr->name)) == NULL)
		    {
		      break;
		    }
		  plist[i]->type = lr->type;
		  plist[i]->separator = lr->separator;
		  j++;
		}
	    }
	}
      pflist->element = plist;
      pflist->num = j;
    }
  folder_list_destroy (&(f_imap->callback.flist));
  f_imap->state = IMAP_NO_STATE;
  return status;
}

static int
folder_imap_lsub (folder_t folder, const char *ref, const char *name,
		  struct folder_list *pflist)
{
  f_imap_t f_imap = folder->data;
  int status = 0;

  /* NOOP.  */
  if (pflist == NULL)
    return EINVAL;

  if (ref == NULL) ref = "";
  if (name == NULL) name = "";

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap, "g%d LSUB \"%s\" \"%s\"\r\n",
			       f_imap->seq++, ref, name);
      CHECK_ERROR (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_LSUB;

    case IMAP_LSUB:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_LSUB_ACK;

    case IMAP_LSUB_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);

    default:
      break;
    }

  /* Build the folder list.  */
  if (f_imap->callback.flist.num > 0)
    {
      struct list_response **plist = NULL;
      size_t num = f_imap->callback.flist.num;
      size_t j = 0;
      plist = calloc (num, sizeof (*plist));
      if (plist)
	{
	  size_t i;
	  for (i = 0; i < num; i++)
	    {
	      struct list_response *lr = f_imap->callback.flist.element[i];
	      //printf ("%s --> %s\n", lr->name, name);
	      plist[i] = calloc (1, sizeof (**plist));
	      if (plist[i] == NULL
		  || (plist[i]->name = strdup (lr->name)) == NULL)
		{
		  break;
		}
	      plist[i]->type = lr->type;
	      plist[i]->separator = lr->separator;
	      j++;
	    }
	}
      pflist->element = plist;
      pflist->num = j;
      folder_list_destroy (&(f_imap->callback.flist));
    }
  f_imap->state = IMAP_NO_STATE;
  f_imap->state = IMAP_NO_STATE;
  return 0;
}

static int
folder_imap_rename (folder_t folder, const char *oldpath, const char *newpath)
{
  f_imap_t f_imap = folder->data;
  int status = 0;
  if (oldpath == NULL || newpath == NULL)
    return EINVAL;

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap, "g%d RENAME %s %s\r\n",
			       f_imap->seq++, oldpath, newpath);
      CHECK_ERROR (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_RENAME;

    case IMAP_RENAME:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_RENAME_ACK;

    case IMAP_RENAME_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);

    default:
      break;
    }
  f_imap->state = IMAP_NO_STATE;
  return status;
}

static int
folder_imap_subscribe (folder_t folder, const char *name)
{
  f_imap_t f_imap = folder->data;
  int status = 0;

  if (name == NULL)
    return EINVAL;
  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap, "g%d SUBSCRIBE %s\r\n",
			       f_imap->seq++, name);
      CHECK_ERROR (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_SUBSCRIBE;

    case IMAP_SUBSCRIBE:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_SUBSCRIBE_ACK;

    case IMAP_SUBSCRIBE_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);

    default:
      break;
    }
  f_imap->state = IMAP_NO_STATE;
  return status;
}

static int
folder_imap_unsubscribe (folder_t folder, const char *name)
{
  f_imap_t f_imap = folder->data;
  int status = 0;

  if (name == NULL)
    return EINVAL;
  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap, "g%d UNSUBSCRIBE %s\r\n",
			       f_imap->seq++, name);
      CHECK_ERROR (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_UNSUBSCRIBE;

    case IMAP_UNSUBSCRIBE:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_UNSUBSCRIBE_ACK;

    case IMAP_UNSUBSCRIBE_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);

    default:
      break;
    }
  f_imap->state = IMAP_NO_STATE;
  return status;
}

/*  Implementation.  */

/* A literal is a sequence of zero or more octets (including CR and LF),
   prefix-quoted with an octet count in the form of an open brace ("{"),
   the number of octets, close brace ("}"), and CRLF.  The sequence is read
   and put in the callback buffer, hopefully the callee did have enough
   room.  Since we are using buffering on the stream(internal stream buffer)
   We should have lots of room.  */
static int
imap_literal_string (f_imap_t f_imap)
{
  size_t len, total0, len0;
  int status = 0;
  for (len0 = len = total0 = 0; total0 < f_imap->callback.nleft;
       total0 += len0)
    {
      status = imap_readline (f_imap);
      if (status != 0)
	{
	  /* Return what we got so far.  */
	  break;
	}
      f_imap->ptr = f_imap->buffer;
      /* How much ?  */
      len = len0 =  f_imap->nl - f_imap->buffer;
      /* Check if the last read did not finish on a line do not copy in
	 callback buffer the terminating sequence ")\r\n".  We are doing this
	 by checking if the amounth (total0) we got so far correspond to what
	 we expect + the line we just read (+1 taking to account the '\r') if
	 yes then we should strip the termination string ")\r\n".  */
      if ((len0 + 1 + total0) >= f_imap->callback.nleft)
	{
	  char *seq = memchr (f_imap->buffer, ')', len0);
	  if (seq)
	    {
	      len = seq - f_imap->buffer;
	      /* Bad luck, if it is \r meaning the imap server  could not add
		 the adjacent \n in the substring, maybe because it did not
		 fit the number of byte we ask, skip it here.  */
	      /* FIXME: Can this be dangerous ?  It can be if a the message
		 contains char that is not 7bit ascii and no encoding were use
		 of that message.  A solution would be to wait for the next
		 char and if it is '\n' skip it if not use it, but that is to
		 much work especially for nonblocking, the message should have
		 been encoded.  */
	      if (*(seq -1) == '\r')
		len--;
	    }
	}
      /* Put the sequence of string in the callback structre for the calle to
	 use.  */
      if (f_imap->callback.total < f_imap->callback.buflen)
	{
	  /* Check how much we can fill the buffer.  */
	  int x = (f_imap->callback.buflen - f_imap->callback.total) - len;
	  x = (x >= 0) ? len : -x;
	  memcpy (f_imap->callback.buffer + f_imap->callback.total,
		  f_imap->buffer, x);
	  f_imap->callback.total += x;

	  /* Depending on the type of request we incremente the xxxx_lines
	     and  xxxx_sizes.  */
	  if (len == len0 && f_imap->callback.msg_imap)
	    {
	      switch (f_imap->callback.type)
		{
		case IMAP_HEADER:
		  f_imap->callback.msg_imap->header_lines++;
		  f_imap->callback.msg_imap->header_size +=

		    f_imap->callback.total;
		  break;

		case IMAP_BODY:
		  f_imap->callback.msg_imap->body_lines++;
		  f_imap->callback.msg_imap->body_size +=
		    f_imap->callback.total;
		  break;

		case IMAP_MESSAGE:
		  f_imap->callback.msg_imap->message_lines++;

		default:
		  break;
		}
	    }
	}
      else
	{
	  /* This should never happen, the amount of in the literal string
	     response is the same or less that we requested in the buffer
	     of the callback.  They should always be enough room.  */
	  break;
	}
      /* count the '\r' too, since it was strip in imap_readline ()  */
      len0 += 1;
    }
  f_imap->callback.nleft -= total0;
  return status;
}

/* A quoted string is a sequence of zero or more 7-bit characters,
   excluding CR and LF, with double quote (<">) characters at each end.
   Same thing as the literal, diferent format the result is put in the
   callback buffer for the mailbox/calle.  */
static int
imap_quoted_string (f_imap_t f_imap, char *remainder)
{
  char *bquote;
  bquote = memchr (remainder, '"', strlen (remainder));
  if (bquote)
    {
      char *equote = memchr (bquote + 1, '"', strlen (bquote + 1));
      if (equote)
	{
	  /* {De,In}cremente by one to not count the quotes.  */
	  bquote++;
	  equote--;
	  f_imap->callback.total = (equote - bquote) + 1;
	  /* Fill the call back buffer. The if is redundant there should always
	     be enough room since the request is base on the buffer size.  */
	  if (f_imap->callback.total <= f_imap->callback.buflen)
	    {
	      int nl = 0;
	      memcpy (f_imap->callback.buffer, bquote, f_imap->callback.total);
	      while (bquote && ((equote - bquote) >= 0))
		{
		  bquote = memchr (f_imap->callback.buffer, '\n',
				   (equote - bquote) + 1);
		  if (bquote)
		    {
		      nl++;
		      bquote++;
		    }
		  else
		    break;
		}

	      /* Depending on the type of request we incremente the xxxx_lines
		 and  xxxx_sizes.  */
	      switch (f_imap->callback.type)
		{
		case IMAP_HEADER:
		  f_imap->callback.msg_imap->header_lines += nl;
		  f_imap->callback.msg_imap->header_size +=
		    f_imap->callback.total;
		  break;

		case IMAP_BODY:
		  f_imap->callback.msg_imap->body_lines += nl;
		  f_imap->callback.msg_imap->body_size +=
		    f_imap->callback.total;
		  break;

		case IMAP_MESSAGE:
		  f_imap->callback.msg_imap->message_lines += nl;

		default:
		  break;
		}
	    }
	}
    }
  return 0;
}

/* Find which type of string the response is: literal or quoted and let the
   function fill the callback buffer.  */
static int
imap_string (f_imap_t f_imap)
{
  int status = 0;
  size_t len = f_imap->nl - f_imap->buffer;
  char *bcurl = memchr (f_imap->buffer, '{', len);
  if (bcurl)
    {
      char *ecurl;
      len = f_imap->nl - bcurl;
      ecurl = memchr (bcurl, '}', len);
      if (ecurl)
	{
	  *ecurl = '\0';
	  f_imap->callback.nleft = strtol (bcurl + 1, NULL, 10);
	  /* We had 3 for ")\r\n" terminates the fetch command.  */
	  f_imap->callback.nleft += 3;
	}
      f_imap->ptr = f_imap->buffer;
      status = imap_literal_string (f_imap);
    }
  else
    {
      char *bracket = memchr (f_imap->buffer, ']', len);
      if (bracket == NULL)
	bracket = f_imap->buffer;
      f_imap->ptr = f_imap->buffer;
      status = imap_quoted_string (f_imap, bracket);
    }
  return status;
}

/* FIXME: does not worl for nobloking.  */
static int
imap_list (f_imap_t f_imap)
{
  char *tok;
  char *sp = NULL;
  size_t len = f_imap->nl - f_imap->buffer - 1;
  char *buffer;
  struct list_response **plr;
  struct list_response *lr;

  buffer = alloca (len);
  memcpy (buffer, f_imap->buffer, len);
  buffer[len] = '\0';
  plr = realloc (f_imap->callback.flist.element,
		 (f_imap->callback.flist.num + 1) * sizeof (*plr));
  if (plr == NULL)
    return ENOMEM;
  f_imap->callback.flist.element = plr;
  lr = plr[f_imap->callback.flist.num] = calloc (1, sizeof (*lr));
  if (lr == NULL)
    return ENOMEM;
  (f_imap->callback.flist.num)++;

  /* Glob untag.  */
  tok = strtok_r (buffer, " ", &sp);
  /* Glob LIST.  */
  tok = strtok_r (NULL, " ", &sp);
  /* Get the attibutes.  */
  tok = strtok_r (NULL, ")", &sp);
  if (tok)
    {
      char *s = NULL;
      char *p = tok;
      while ((tok = strtok_r (p, " ()", &s)) != NULL)
	{
	  if (strcasecmp (tok, "\\Noselect") == 0)
	    {
	    }
	  else if (strcasecmp (tok, "\\Marked") == 0)
	    {
	    }
	  else if (strcasecmp (tok, "\\Unmarked") == 0)
	    {
	    }
	  else if (strcasecmp (tok, "\\Noinferiors") == 0)
	    {
	      lr->type |= MU_FOLDER_ATTRIBUTE_FILE;
	    }
	  else
	    {
	      lr->type |= MU_FOLDER_ATTRIBUTE_DIRECTORY;
	    }
	  p = NULL;
	}
    }
  /* Hiearchy delimeter.  */
  tok = strtok_r (NULL, " ", &sp);
  if (tok && strlen (tok) > 2)
    lr->separator = tok[1];
  /* The path.  */
  tok = strtok_r (NULL, " ", &sp);
  if (tok)
    {
      char *s = strchr (tok, '{');
      if (s)
	{
	  size_t n = strtoul (s + 1, NULL, 10);
	  lr->name = calloc (n + 1, sizeof (char));
	  f_imap->ptr = f_imap->buffer;
	  imap_readline (f_imap);
	  memcpy (lr->name, f_imap->buffer, n);
	  lr->name[n] = '\0';
	}
      else
	lr->name = strdup (tok);
    }
  return 0;
}

/* Helping function to figure out the section name of the message: for example
   a 2 part message with the first part being sub in two will be:
   {1}, {1,1} {1,2}  The first subpart of the message and its sub parts
   {2}  The second subpar of the message.  */
char *
section_name (msg_imap_t msg_imap)
{
  size_t sectionlen = 0;
  char *section = strdup ("");

  /* Build the section name, but it is in reverse.  */
  for (; msg_imap; msg_imap = msg_imap->parent)
    {
      if (msg_imap->part != 0)
	{
	  char *tmp;
	  char part[64];
	  size_t partlen;
	  snprintf (part, sizeof (partlen), "%d", msg_imap->part);
	  partlen = strlen (part);
	  tmp = realloc (section, sectionlen + partlen + 2);
	  if (tmp == NULL)
	    break;
	  section = tmp;
	  memset (section + sectionlen, '\0', partlen + 2);
	  if (sectionlen != 0)
	    strcat (section, ".");
	  strcat (section, part);
	  sectionlen = strlen (section);
	}
    }

  /* Reverse the string.  */
  if (section)
    {
      char *begin, *last;
      char c;
      for (begin = section, last = section + sectionlen - 1; begin < last;
	   begin++, last--)
	{
	  c = *begin;
	  *begin = *last;
	  *last = c;
	}
    }
  return section;
}

/* FIXME: This does not work for nonblocking.  */
/* Recursive call, to parse the dismay of parentesis "()" in a BODYSTRUCTURE
   call, not we use the short form of BODYSTRUCTURE, BODY with no argument.  */
static int
imap_bodystructure0 (msg_imap_t msg_imap, char **buffer, char **base)
{
  int paren = 0;
  int no_arg = 0;
  int status = 0;
  char *p = memchr (*buffer, '(', strlen (*buffer));

  if (p == NULL)
    return 0;

  paren++;
  no_arg++;

  for (p = (p + 1); *p != '\0'; p++)
    {
      if (*p == '(')
	{
	  paren++;
	  /* If we did not parse any argment it means that it is a subpart of
	     the message.  We create a new concrete msg_api for it. */
	  if (no_arg)
	    {
	      msg_imap_t new_part;
	      msg_imap_t *tmp;
	      tmp = realloc (msg_imap->parts,
			     ((msg_imap->num_parts + 1) * sizeof (*tmp)));
	      if (tmp)
		{
		  msg_imap->parts = tmp;
		  new_part = calloc (1, sizeof (*new_part));
		  if (new_part)
		    {
		      msg_imap->parts[msg_imap->num_parts] = new_part;
		      new_part->part = ++(msg_imap->num_parts);
		      new_part->parent = msg_imap;
		      new_part->num = msg_imap->num;
		      new_part->m_imap = msg_imap->m_imap;
		      new_part->flags = msg_imap->flags;
		      //p++;
		      imap_bodystructure0 (new_part, &p, base);
		    }
		  else
		    status = ENOMEM;
		}
	      else
		status = ENOMEM;
	    }
	}
      if (*p == ')')
	{
	  no_arg = 1;
	  paren--;
	  /* Did we reach the same number of close paren ?  */
	  if (paren <= 0)
	    break;
	}
      /* Skip over quoted strings.  */
      else if (*p == '"')
	{
	  p++;
	  while (*p && *p != '"') p++;
	  if (*p == '\0') break;
	  no_arg = 0;
	}
      /* Dam !! a literal in the body description !!! We just read the line
         and continue the parsing with this newline.  */
      else if (*p == '{')
	{
	  size_t count;
	  f_imap_t f_imap = msg_imap->m_imap->f_imap;
	  f_imap->ptr = f_imap->buffer;
	  imap_readline (f_imap);
	  count = f_imap->nl - f_imap->buffer;
	  if (count)
	    {
	      char *tmp = calloc (count + 1, sizeof (char));
	      if (tmp)
		{
		  free (*base);
		  *base = tmp;
		  p = *base;
		  memcpy (p, f_imap->buffer, count);
		}
	    }
	}
      /* Skip the rests */
      else if (*p != ' ')
	no_arg = 0;
    } /* for */
  *buffer = p;
  return status;
}

/* Cover functions that calls the recursive imap_bodystructure0 to parse.  */
static int
imap_bodystructure (f_imap_t f_imap)
{
  char *p = strchr (f_imap->buffer, '(');
  if (p)
    {
      p++;
      p = strchr (p, '(');
      if (p && f_imap->callback.msg_imap)
	{
	  char *s = calloc (strlen (p) + 1, sizeof (*s));
	  char *base = s;
	  strcpy (s, p);
	  imap_bodystructure0 (f_imap->callback.msg_imap, &s, &base);
	  free (base);
	}
    }
  return 0;
}

/* Parse the flags untag responses.  */
static int
imap_flags (f_imap_t f_imap)
{
  char *flag;
  char *sp = NULL;
  char *flags = memchr (f_imap->buffer, '(', f_imap->nl - f_imap->buffer);
  msg_imap_t msg_imap = f_imap->callback.msg_imap;
  while ((flag = strtok_r (flags, " ()", &sp)) != NULL)
    {
      if (strcmp (flag, "\\Seen") == 0)
	{
	  if (msg_imap)
	    msg_imap->flags |= MU_ATTRIBUTE_SEEN;
	  else
	    f_imap->flags |= MU_ATTRIBUTE_SEEN;
	}
      else if (strcmp (flag, "\\Answered") == 0)
	{
	  if (msg_imap)
	    msg_imap->flags |= MU_ATTRIBUTE_ANSWERED;
	  else
	    f_imap->flags |= MU_ATTRIBUTE_ANSWERED;
	}
      else if (strcmp (flag, "\\Flagged") == 0)
	{
	  if (msg_imap)
	    msg_imap->flags |= MU_ATTRIBUTE_FLAGGED;
	  else
	    f_imap->flags |= MU_ATTRIBUTE_FLAGGED;
	}
      else if (strcmp (flag, "\\Deleted") == 0)
	{
	  if (msg_imap)
	    msg_imap->flags |= MU_ATTRIBUTE_DELETED;
	  else
	    f_imap->flags |= MU_ATTRIBUTE_DELETED;
	}
      else if (strcmp (flag, "\\Draft") == 0)
	{
	  if (msg_imap)
	    msg_imap->flags |= MU_ATTRIBUTE_DRAFT;
	  else
	    f_imap->flags |= MU_ATTRIBUTE_DRAFT;
	}
      flags = NULL;
    }
  return 0;
}

/* Parse imap unfortunately FETCH is use as response for many commands.  */
static int
imap_fetch (f_imap_t f_imap)
{
  char *command;
  size_t msgno = 0;
  size_t len = f_imap->nl - f_imap->buffer;
  m_imap_t m_imap = f_imap->selected;
  const char *delim = " ()<>";
  int status = 0;
  char *sp = NULL;

  /* If we have some data that was not clear it may mean that we returned
     EAGAIN etc ...  In any case we have to clear it first.  */
  if (f_imap->callback.nleft)
    return imap_literal_string (f_imap);

  /* We do not want to step over f_imap->buffer since it can be use further
     down the chain.  */
  command = alloca (len + 1);
  memcpy (command, f_imap->buffer, len + 1);

  /* We should have a mailbox selected.  */
  assert (m_imap != NULL);

  /* Parse the FETCH respones to extract the pieces.  */

  /* Skip untag '*'.  */
  command = strtok_r (command, delim, &sp);
  /* Get msgno.  */
  command = strtok_r (NULL, delim, &sp);
  if (command)
    msgno = strtol (command, NULL, 10);
  /* Skip FETCH .  */
  command = strtok_r (NULL, delim,  &sp);

  /* It is actually possible, but higly unlikely that we do not have the
     message yet, for example a "FETCH (FLAGS (\Recent))" notification
     for a newly messsage.  */
  if (f_imap->callback.msg_imap == NULL
      || f_imap->callback.msg_imap->num != msgno)
    {
      /* Find the imap mesg struct.  */
      size_t i;
      for (i = 0; i < m_imap->imessages_count; i++)
	{
	  if (m_imap->imessages[i] && m_imap->imessages[i]->num == msgno)
	    {
	      f_imap->callback.msg_imap = m_imap->imessages[i];
	      break;
	    }
	}
    }
  //assert (msg_imap != NULL);

  /* Get the command.  */
  command = strtok_r (NULL, delim, &sp);
  if (strncmp (command, "FLAGS", 5) == 0)
    {
      status = imap_flags (f_imap);
    }
  /* This the short form for BODYSTRUCTURE.  */
  else if (strcmp (command, "BODY") == 0)
    {
      status = imap_bodystructure (f_imap);
    }
  else if (strncmp (command, "INTERNALDATE", 12) == 0)
    {
      status = imap_string (f_imap);
    }
  else if (strncmp (command, "RFC822.SIZE", 10) == 0)
    {
      command = strtok_r (NULL, " ()\n", &sp);
      if (f_imap->callback.msg_imap)
	f_imap->callback.msg_imap->message_size = strtol (command, NULL, 10);
    }
  else if (strncmp (command, "BODY", 4) == 0)
    {
      status = imap_string (f_imap);
    }
  return status;
}

/* C99 says that a conforming implementations of snprintf () should return the
   number of char that would have been call but many GNU/Linux && BSD
   implementations return -1 on error.  Worse QnX/Neutrino actually does not
   put the terminal null char.  So let's try to cope.  */
int
imap_writeline (f_imap_t f_imap, const char *format, ...)
{
  int len;
  va_list ap;
  int done = 1;

  va_start(ap, format);
  do
    {
      len = vsnprintf (f_imap->buffer, f_imap->buflen - 1, format, ap);
      if (len < 0 || len >= (int)f_imap->buflen
          || !memchr (f_imap->buffer, '\0', len + 1))
        {
          f_imap->buflen *= 2;
          f_imap->buffer = realloc (f_imap->buffer, f_imap->buflen);
          if (f_imap->buffer == NULL)
            return ENOMEM;
          done = 0;
        }
      else
        done = 1;
    }
  while (!done);
  va_end(ap);
  f_imap->ptr = f_imap->buffer + len;
  return 0;
}

/* Cover to send requests.  */
int
imap_send (f_imap_t f_imap)
{
  int status = 0;
  size_t len;
  if (f_imap->ptr > f_imap->buffer)
    {
      len = f_imap->ptr - f_imap->buffer;
      status = stream_write (f_imap->folder->stream, f_imap->buffer, len,
			     0, &len);
      if (status == 0)
        {
          memmove (f_imap->buffer, f_imap->buffer + len, len);
          f_imap->ptr -= len;
        }
    }
  else
    {
      f_imap->ptr = f_imap->buffer;
      len = 0;
    }
  return status;
}

/* Read a complete line form the imap server. Transform CRLF to LF, put a null
   in the buffer when done.  Note f_imap->offset is not really of any use
   but rather to keep the stream internal buffer scheme happy, so we have to
   be in sync with the stream.  */
int
imap_readline (f_imap_t f_imap)
{
  size_t n = 0;
  size_t total = f_imap->ptr - f_imap->buffer;
  int status;

  /* Must get a full line before bailing out.  */
  do
    {
      status = stream_readline (f_imap->folder->stream, f_imap->buffer + total,
				f_imap->buflen - total,  f_imap->offset, &n);
      if (status != 0)
        return status;

      total += n;
      f_imap->offset += n;
      f_imap->nl = memchr (f_imap->buffer, '\n', total);
      if (f_imap->nl == NULL)  /* Do we have a full line.  */
        {
          /* Allocate a bigger buffer ?  */
          if (total >= f_imap->buflen -1)
            {
              f_imap->buflen *= 2;
              f_imap->buffer = realloc (f_imap->buffer, f_imap->buflen + 1);
              if (f_imap->buffer == NULL)
                return ENOMEM;
            }
        }
      f_imap->ptr = f_imap->buffer + total;
    }
  while (f_imap->nl == NULL);

  /* Conversion \r\n --> \n\0  */
  /* FIXME: We should use a property to enable or disable conversion.  */
  if (f_imap->nl > f_imap->buffer)
    {
      *(f_imap->nl - 1) = '\n';
      *(f_imap->nl) = '\0';
      f_imap->ptr = f_imap->nl;
    }
  return 0;
}

/*
  The parsing was inspired by this article form the BeNews channel: "BE
  ENGINEERING INSIGHTS: IMAP for the Masses." By Adam Haberlach adam@be.com

  The server responses are in three forms: status responses, server data,
  and command continuation request, ...
  An untagged response is indicated by the token "*" instead of a tag.
  Untagged status responses indicate server greeting, or server status
  that does not indicate the completion of a command (for example, an
  impending system shutdown alert).
  ....
  The client MUST be prepared to accept any response at all times.

  Status responses are OK, NO, BAD, PREAUTH and BYE.  OK, NO, and BAD
  may be tagged or untagged.  PREAUTH and BYE are always untagged.

  Server Responses - Status Responses
  BAD *|tag
  BYE *
  NO *|tag
  OK *|tag
  PREAUTH *

  The code for status responses are
  ALERT
  BADCHARSET(IMAPV)
  CAPABILITY(IMAPV)
  NEWNAME
  PARSE
  PERMANENTFLAGS
  READ-ONLY
  READ-WRITE
  TRYCREATE
  UIDNEXT
  UIDVALIDITY
  UNSEEN

  Server Responses - Server and Mailbox Status.
  These responses are always untagged.
  CAPABILITY *
  EXISTS *
  EXPUNGE *
  FLAGS *
  FETCH *
  LIST *
  LSUB *
  RECENT *
  SEARCH *
  STATUS *

  Server Responses - Command Continuation Request
  +

*/
int
imap_parse (f_imap_t f_imap)
{
  int done = 0;
  int status = 0;
  char empty[2];

  if (f_imap->callback.nleft)
    imap_literal_string (f_imap);

  /* We use that moronic hack to not check null for the tockenize strings.  */
  empty[0] = '\0';
  empty[1] = '\0';
  while (! done)
    {
      char *tag, *response, *remainder;
      char *buffer;
      status = imap_readline (f_imap);
      if (status != 0)
	{
	  break;
	}
      /* Comment out to see all reading traffic.  */
      /* fprintf (stderr, "\t\t%s", f_imap->buffer); */

      /* We do not want to step over f_imap->buffer since it can be use
	 further down the chain.  */
      buffer = alloca ((f_imap->ptr - f_imap->buffer) + 1);
      memcpy (buffer, f_imap->buffer, (f_imap->ptr - f_imap->buffer) + 1);

      /* Tokenize the string.  */
      {
	char *sp = NULL;
	tag = strtok_r (buffer, " ", &sp);
	response = strtok_r (NULL, " ", &sp);
	if (!response) response = empty;
	remainder = strtok_r (NULL, "\n", &sp);
	if (!remainder) remainder = empty;
      }

      /* Is the response untagged ?  */
      if (tag && tag[0] == '*')
	{

	  /* Is it a Status Response.  */
	  if (strcmp (response, "OK") == 0)
	    {
	      /* Check for status response [code].  */
	      if (*remainder == '[')
		{
		  char *cruft, *subtag;
		  char *sp = NULL;
		  remainder++;
		  cruft = strtok_r (remainder, "]", &sp);
		  if (!cruft) cruft = empty;
		  subtag = strtok_r (cruft, " ", &sp);
		  if (!subtag) subtag = empty;

		  if (strcmp (subtag, "ALERT") == 0)
		    {
		      /* The human-readable text contains a special alert that
			 MUST be presented to the user in a fashion that calls
			 the user's attention to the message.  */
		    }
		  else if (strcmp (subtag, "BADCHARSET") == 0)
		    {
		      /* Optionally followed by a parenthesized list of
			 charsets.  A SEARCH failed because the given charset
			 is not supported by this implementation.  If the
			 optional list of charsets is given, this lists the
			 charsets that are supported by this implementation. */
		    }
		  else if (strcmp (subtag, "CAPABILITY") == 0)
		    {
		      /* Followed by a list of capabilities.  This can appear
			 in the initial OK or PREAUTH response to transmit an
			 initial capabilities list.  This makes it unnecessary
			 for a client to send a separate CAPABILITY command if
			 it recognizes this response.  */
		      if (f_imap->capa)
			free (f_imap->capa);
		      f_imap->capa = strdup (cruft);
		    }
		  else if (strcmp (subtag, "NEWNAME") == 0)
		    {
		      /* Followed by a mailbox name and a new mailbox name.  A
			 SELECT or EXAMINE failed because the target mailbox
			 name (which once existed) was renamed to the new
			 mailbox name.  This is a hint to the client that the
			 operation can succeed if the SELECT or EXAMINE is
			 reissued with the new mailbox name. */
		    }
		  else if (strcmp (subtag, "PARSE") == 0)
		    {
		      /* The human-readable text represents an error in
			 parsing the [RFC-822] header or [MIME-IMB] headers
			 of a message in the mailbox.  */
		    }
		  else if (strcmp (subtag, "PERMANENTFLAGS") == 0)
		    {
		      /* Followed by a parenthesized list of flags, indicates
			 which of the known flags that the client can change
			 permanently.  Any flags that are in the FLAGS
			 untagged response, but not the PERMANENTFLAGS list,
			 can not be set permanently.  If the client attempts
			 to STORE a flag that is not in the PERMANENTFLAGS
			 list, the server will either ignore the change or
			 store the state change for the remainder of the
			 current session only. The PERMANENTFLAGS list can
			 also include the special flag \*, which indicates
			 that it is possible to create new keywords by
			 attempting to store those flags in the mailbox.  */
		    }
		  else if (strcmp (subtag, "READ-ONLY") == 0)
		    {
		      /* The mailbox is selected read-only, or its access
			 while selected has changed from read-write to
			 read-only.  */
		    }
		  else if (strcmp (subtag, "READ-WRITE") == 0)
		    {
		      /* The mailbox is selected read-write, or its access
			 while selected has changed from read-only to
			 read-write.  */
		    }
		  else if (strcmp (subtag, "TRYCREATE") == 0)
		    {
		      /* An APPEND or COPY attempt is failing because the
			 target mailbox does not exist (as opposed to some
			 other reason).  This is a hint to the client that
			 the operation can succeed if the mailbox is first
			 created by the CREATE command.  */
		    }
		  else if (strcmp (subtag, "UIDNEXT") == 0)
		    {
		      /* Followed by a decimal number, indicates the next
			 unique identifier value.  Refer to section 2.3.1.1
			 for more information.  */
		    }
		  else if (strcmp (subtag, "UIDVALIDITY") == 0)
		    {
		      /* Followed by a decimal number, indicates the unique
			 identifier validity value.  Refer to section 2.3.1.1
			 for more information.  */
		      char *value = strtok_r (NULL, " ", &sp);
		      f_imap->selected->uidvalidity = strtol (value, NULL, 10);
		    }
		  else if (strcmp (subtag, "UNSEEN") == 0)
		    {
		      /* Followed by a decimal number, indicates the number of
			 the first message without the \Seen flag set.  */
		      char *value = strtok_r (NULL, " ", &sp);
		      f_imap->selected->unseen = strtol (value, NULL, 10);
		    }
		  else
		    {
		      /* Additional response codes defined by particular
			 client or server implementations SHOULD be prefixed
			 with an "X" until they are added to a revision of
			 this protocol.  Client implementations SHOULD ignore
			 response codes that they do not recognize.  */
		    }
		} /* End of code.  */
	      else
		{
		  /* Not sure why we would get an untagged ok...but we do... */
		  /* Still should we be verbose about is ? */
		  printf("Untagged OK: %s\n", remainder);
		}
	    }
	  else if (strcmp (response, "NO") == 0)
	    {
	      /* This does not mean failure but rather a strong warning.  */
	      printf ("Untagged NO: %s\n", remainder);
	    }
	  else if (strcmp (response, "BAD") == 0)
	    {
	      /* We're dead, protocol/syntax error.  */
	      printf ("Untagged BAD: %s\n", remainder);
	    }
	  else if (strcmp (response, "PREAUTH") == 0)
	    {
	    }
	  else if (strcmp (response, "BYE") == 0)
	    {
	      /* We should close the stream. This is not recoverable.  */
	      done = 1;
	      f_imap->isopen = 0;
	      stream_close (f_imap->folder->stream);
	    }
	  else if (strcmp (response, "CAPABILITY") == 0)
	    {
	      if (f_imap->capa)
		free (f_imap->capa);
	      f_imap->capa = strdup (remainder);
	    }
	  else if (strcmp (remainder, "EXISTS") == 0)
	    {
	      f_imap->selected->messages_count = strtol (response, NULL, 10);
	    }
	  else if (strcmp (remainder, "EXPUNGE") == 0)
	    {
	    }
	  else if (strncmp (remainder, "FETCH", 5) == 0)
	    {
	      status = imap_fetch (f_imap);
	      if (status != 0)
		break;
	    }
	  else if (strcmp (response, "FLAGS") == 0)
	    {
	      /* Flags define on the mailbox not a message flags.  */
	      status = imap_flags (f_imap);
	    }
	  else if (strcmp (response, "LIST") == 0)
	    {
	      status = imap_list (f_imap);
	    }
	  else if (strcmp (response, "LSUB") == 0)
	    {
	      status = imap_list (f_imap);
	    }
	  else if (strcmp (remainder, "RECENT") == 0)
	    {
	      f_imap->selected->recent = strtol (response, NULL, 10);
	    }
	  else if (strcmp (response, "SEARCH") == 0)
	    {
	    }
	  else if (strcmp (response, "STATUS") == 0)
	    {
	    }
	  else
	    {
	      /* Once again, check for something strange.  */
	      printf("unknown untagged response: \"%s\"  %s\n",
		     response, remainder);
	    }
	}
      /* Continuation token.  */
      else if (tag && tag[0] == '+')
	{
	  done = 1;
	}
      else
	{
	  /* Every transaction ends with a tagged response.  */
	  done = 1;
	  if (strcmp (response, "OK") == 0)
	    {
	    }
	  else /* NO and BAD */
	    {
	      if (strncmp (remainder, "LOGIN", 5) == 0)
		{
		  observable_t observable = NULL;
		  folder_get_observable (f_imap->folder, &observable);
		  observable_notify (observable, MU_EVT_AUTHORITY_FAILED);
		}
	      printf("NO/Bad Tagged: %s %s\n", response, remainder);
	      status = EINVAL;
	    }
	}
      f_imap->ptr = f_imap->buffer;
    }
  return status;
}

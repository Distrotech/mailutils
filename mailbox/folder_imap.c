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
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fnmatch.h>

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <imap0.h>
#include <mailutils/error.h>

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

#ifndef HAVE_STRTOK_R
char *strtok_r                     __P ((char *, const char *, char **));
#endif
/* Concrete IMAP implementation.  */
static int folder_imap_open        __P ((folder_t, int));
static int folder_imap_create      __P ((folder_t));
static int folder_imap_close       __P ((folder_t));
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
/* FETCH  */
static int imap_fetch          __P ((f_imap_t));
static int imap_rfc822         __P ((f_imap_t, char **));
static int imap_rfc822_size    __P ((f_imap_t, char **));
static int imap_rfc822_header  __P ((f_imap_t, char **));
static int imap_rfc822_text    __P ((f_imap_t, char **));
static int imap_flags          __P ((f_imap_t, char **));
static int imap_bodystructure  __P ((f_imap_t, char **));
static int imap_body           __P ((f_imap_t, char **));
static int imap_uid            __P ((f_imap_t, char **));

/* String.  */
static int imap_literal_string __P ((f_imap_t, char **));
static int imap_string         __P ((f_imap_t, char **));
static int imap_quoted_string  __P ((f_imap_t, char **));

static int imap_token          __P ((char *, size_t, char **));

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
	status = imap_writeline (f_imap, "g%u LOGIN %s %s\r\n",
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
static int
folder_imap_open (folder_t folder, int flags)
{
  f_imap_t f_imap = folder->data;
  char *host;
  long port = 143; /* default imap port.  */
  int status = 0;
  size_t len = 0;
  int preauth = 0; /* Do we have "preauth"orisation ?  */

  /* If we are already open for business, noop.  */
  monitor_wrlock (folder->monitor);
  if (f_imap->isopen)
    {
      monitor_unlock (folder->monitor);
      if (flags & MU_STREAM_CREAT)
	status = folder_imap_create (folder);
      return 0;
    }
  monitor_unlock (folder->monitor);

  /* Fetch the pop server name and the port in the url_t.  */
  status = url_get_host (folder->url, NULL, 0, &len);
  if (status != 0)
    return status;
  host = alloca (len + 1);
  url_get_host (folder->url, host, len + 1, NULL);
  url_get_port (folder->url, &port);

  folder->flags = flags;

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
  monitor_wrlock (folder->monitor);
  f_imap->isopen++;
  monitor_unlock (folder->monitor);
  if (flags & MU_STREAM_CREAT)
    {
      status = folder_imap_create (folder);
      CHECK_EAGAIN (f_imap, status);
    }
  return 0;
}

/* Shutdown the connection.  */
static int
folder_imap_close (folder_t folder)
{
  f_imap_t f_imap = folder->data;
  int status = 0;

  monitor_wrlock (folder->monitor);
  f_imap->isopen--;
  if (f_imap->isopen)
    {
      monitor_unlock (folder->monitor);
      return 0;
    }
  monitor_unlock (folder->monitor);

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap, "g%u LOGOUT\r\n", f_imap->seq++);
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
  f_imap->selected = NULL;
  return 0;
}

/* Create a folder/mailbox.  */
static int
folder_imap_create (folder_t folder)
{
  f_imap_t f_imap = folder->data;
  int status = 0;

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      {
	char *path;
	size_t len;
	url_get_path (folder->url, NULL, 0, &len);
	if (len == 0)
	  return 0;
	path = calloc (len + 1, sizeof (*path));
	if (path == NULL)
	  return ENOMEM;
	url_get_path (folder->url, path, len + 1, NULL);
	status = imap_writeline (f_imap, "g%u CREATE %s\r\n", f_imap->seq++,
				 path);
	free (path);
	CHECK_ERROR (f_imap, status);
	FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);
	f_imap->state = IMAP_CREATE;
      }

    case IMAP_CREATE:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_DELETE_ACK;

    case IMAP_CREATE_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      FOLDER_DEBUG0 (folder, MU_DEBUG_PROT, f_imap->buffer);

    default:
      break;
    }
  f_imap->state = IMAP_NO_STATE;
  return status;
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
      status = imap_writeline (f_imap, "g%u DELETE %s\r\n", f_imap->seq++,
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
      status = imap_writeline (f_imap, "g%u LIST \"%s\" \"%s\"\r\n",
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
      status = imap_writeline (f_imap, "g%u LSUB \"%s\" \"%s\"\r\n",
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
      status = imap_writeline (f_imap, "g%u RENAME %s %s\r\n",
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
      status = imap_writeline (f_imap, "g%u SUBSCRIBE %s\r\n",
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
      status = imap_writeline (f_imap, "g%u UNSUBSCRIBE %s\r\n",
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
   room.  */
static int
imap_literal_string (f_imap_t f_imap, char **ptr)
{
  size_t len, len0, total;
  int status = 0;
  int nl;
  /* The (len + 1) in the for is to count the strip '\r' by imap_readline.  */
  for (len0 = len = total = 0; total < f_imap->callback.nleft; total += (len + 1))
    {
      status = imap_readline (f_imap);
      if (status != 0)
	{
	  /* Return what we got so far.  */
	  break;
	}
      f_imap->ptr = f_imap->buffer;

      /* How much ?  */
      len0 = len = f_imap->nl - f_imap->buffer;
      /* Check if the last read did not finish on a line, if yes do not copy in
	 callback buffer the terminating sequence ")\r\n".  We are doing this
         by checking if the amount(total) we got so far + the len of the line
         +1 (taking to account the strip '\r') goes behond the request.  */
      if ((total + len + 1) > f_imap->callback.nleft)
	{
	  len0 = len = f_imap->callback.nleft - total;
	  /* ALERT: if we ask for a substring, for example we have :
	     "123456\n", and ask for body[]<0.7> the server will send
	     body[] {7} --> "123456\r".  There was not enough space
	     to fit the nl .. annoying.  Take care of this here.  */
	  if (f_imap->buffer[len - 1] == '\r')
	    len0--;
	}

      if (f_imap->callback.total < f_imap->callback.buflen)
	{
	  /* Check how much we can fill the callback buffer.  */
          int x = (f_imap->callback.buflen - f_imap->callback.total) - len0;
          x = (x >= 0) ? len0 : (x + len0);
	  if (f_imap->callback.buffer)
	    memcpy (f_imap->callback.buffer + f_imap->callback.total,
		    f_imap->buffer, x);
          f_imap->callback.total += x;

	  /* Depending on the type of request we incremente the xxxx_lines
	     and  xxxx_sizes.  */
	  nl = (memchr (f_imap->buffer, '\n', len0)) ? 1 : 0;
	  if (f_imap->callback.msg_imap)
	    {
	      switch (f_imap->callback.type)
		{
		case IMAP_HEADER:
		  f_imap->callback.msg_imap->header_lines += nl;
		  f_imap->callback.msg_imap->header_size += x;
		  break;

		case IMAP_BODY:
		  f_imap->callback.msg_imap->body_lines += nl;
		  f_imap->callback.msg_imap->body_size += x;
		  break;

		case IMAP_MESSAGE:
		  f_imap->callback.msg_imap->message_lines += nl;
		  /* The message size is known by sending RFC822.SIZE.  */

		default:
		  break;
		}
	    }
	}
    }
  f_imap->callback.nleft -= total;
  /* Move the command buffer, or do a full readline.  */
  if (len == (size_t)(f_imap->nl - f_imap->buffer))
    {
      len = 0;
      status = imap_readline (f_imap);
    }
  *ptr = f_imap->buffer + len;
  return status;
}

/* A quoted string is a sequence of zero or more 7-bit characters,
   excluding CR and LF, with double quote (<">) characters at each end.
   Same thing as the literal, diferent format the result is put in the
   callback buffer for the mailbox/callee.  */
static int
imap_quoted_string (f_imap_t f_imap, char **ptr)
{
  char *bquote;
  int escaped = 0;
  (*ptr)++;
  bquote = *ptr;
  while (**ptr && (**ptr != '"' || escaped))
    {
      escaped = (**ptr == '\\') ? 1 : 0;
      (*ptr)++;
    }
  f_imap->callback.total = *ptr - bquote;
  /* Fill the call back buffer. The if is redundant there should always
     be enough room since the request is base on the buffer size.  */
  if (f_imap->callback.total <= f_imap->callback.buflen
      && f_imap->callback.buffer)
    memcpy (f_imap->callback.buffer, bquote, f_imap->callback.total);
  if (**ptr == '"')
    (*ptr)++;
  return 0;
}

/* Find which type of string the response is: literal or quoted and let the
   function fill the callback buffer.  */
static int
imap_string (f_imap_t f_imap, char **ptr)
{
  int status = 0;

  /* Skip whites.  */
  while (**ptr == ' ')
    (*ptr)++;
  switch (**ptr)
    {
    case '{':
      f_imap->callback.nleft = strtol ((*ptr) + 1, ptr, 10);
      if (**ptr == '}')
	{
	  (*ptr)++;
	  /* Reset the buffer to the beginning.  */
	  f_imap->ptr = f_imap->buffer;
	  status = imap_literal_string (f_imap, ptr);
	}
      break;
    case '"':
      status = imap_quoted_string (f_imap, ptr);
      break;
      /* NIL */
    case 'N':
    case 'n':
      (*ptr)++; /* N|n  */
      (*ptr)++; /* I|i  */
      (*ptr)++; /* L|l  */
      break;
    default:
      /* Problem.  */
      break;
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
	  snprintf (part, sizeof (partlen), "%u", msg_imap->part);
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

/* We do not pay particular attention to the content of the bodystructure
   but rather to the paremetized list layout to discover how many messages
   the originial message is composed of.  The information is later retrieve
   when needed via the body[header.fields] command.  Note that this function
   is recursive. */
static int
imap_bodystructure0 (msg_imap_t msg_imap, char **ptr)
{
  int paren = 0;
  int no_arg = 0;
  int status = 0;
  int have_size = 0;

  /* Skip space.  */
  while (**ptr == ' ')
    (*ptr)++;
  /* Pass lparen.  */
  if (**ptr == '(')
    {
      ++(*ptr);
      paren++;
      no_arg++;
    }
  /* NOTE : this loop has side effects in strtol() and imap_string(), the
     order of the if's are important.  */
  while (**ptr)
    {
      /* Skip the string argument.  */
      if (**ptr != '(' && **ptr != ')')
	{
	  char *start = *ptr;
	  /* FIXME: set the command callback if EAGAIN to resume.  */
          status = imap_string (msg_imap->m_imap->f_imap, ptr);
	  if (status != 0)
	    return status;
	  if (start != *ptr)
	    no_arg = 0;
	}

      if (isdigit ((unsigned)**ptr))
	{
	  char *start = *ptr;
	  size_t size = strtoul (*ptr, ptr, 10);
	  if (start != *ptr)
	    {
	      if (!have_size && msg_imap && msg_imap->parent)
		msg_imap->message_size = size;
	      have_size = 1;
	      no_arg = 0;
	    }
	}

      if (**ptr == '(')
	{
	  if (no_arg)
	    {
	      msg_imap_t new_part;
	      msg_imap_t *tmp;
	      tmp = realloc (msg_imap->parts,
			     ((msg_imap->num_parts + 1) * sizeof (*tmp)));
	      if (tmp)
		{
		  new_part = calloc (1, sizeof (*new_part));
		  if (new_part)
		    {
		      msg_imap->parts = tmp;
		      msg_imap->parts[msg_imap->num_parts] = new_part;
		      new_part->part = ++(msg_imap->num_parts);
		      new_part->parent = msg_imap;
		      new_part->num = msg_imap->num;
		      new_part->m_imap = msg_imap->m_imap;
		      new_part->flags = msg_imap->flags;
		      status = imap_bodystructure0 (new_part, ptr);
		      /* Jump up, the rparen been swallen already.  */
		      continue;
		    }
		  else
		    {
		      status = ENOMEM;
		      free (tmp);
		      break;
		    }
		}
	      else
		{
		  status = ENOMEM;
		  break;
		}
	    }
	  paren++;
        }

      if (**ptr == ')')
        {
          no_arg = 1;
          paren--;
          /* Did we reach the same number of close paren ?  */
          if (paren <= 0)
	    {
	      /* Swallow the rparen.  */
	      (*ptr)++;
	      break;
	    }
        }

      if (**ptr == '\0')
	break;

      (*ptr)++;
    }
  return status;
}

static int
imap_bodystructure (f_imap_t f_imap, char **ptr)
{
  return imap_bodystructure0 (f_imap->callback.msg_imap, ptr);
}

/* The Format for a FLAG response is :
   mailbox_data    ::=  "FLAGS" SPACE flag_list
   flag_list       ::= "(" #flag ")"
   flag            ::= "\Answered" / "\Flagged" / "\Deleted" /
   "\Seen" / "\Draft" / flag_keyword / flag_extension
   flag_extension  ::= "\" atom
   ;; Future expansion.  Client implementations
   ;; MUST accept flag_extension flags.  Server
   ;; implementations MUST NOT generate
   ;; flag_extension flags except as defined by
   ;; future standard or standards-track
   ;; revisions of this specification.
   flag_keyword    ::= atom

   S: * 14 FETCH (FLAGS (\Seen \Deleted))
   S: * FLAGS (\Answered \Flagged \Deleted \Seen \Draft)

   We assume that the '*' or the FLAGS keyword are strip.

   FIXME:  User flags are not take to account. */
static int
imap_flags (f_imap_t f_imap, char **ptr)
{
  char *flag;
  /* msg_imap may be null for an untag response deal with it.  */
  msg_imap_t msg_imap = f_imap->callback.msg_imap;

  /* Skip space.  */
  while (**ptr == ' ')
    (*ptr)++;
  /* Skip LPAREN.  */
  if (**ptr == '(')
    (*ptr)++;
  for (;**ptr && **ptr != ')'; ++(*ptr))
    {
      /* Skip space before next word.  */
      while (**ptr == ' ')
	(*ptr)++;
      /* Save the beginning of the word.  */
      flag = *ptr;
      /* Get the next word boundary.  */
      while (**ptr && **ptr != ' ' && **ptr != ')')
	++*ptr;
      /* Make a C string for the strcasecmp.  */
      **ptr = '\0';
      /* Bail out.  */
      if (*flag == '\0')
	break;

      /* Guess the flag.  */
      if (strcasecmp (flag, "\\Seen") == 0)
	{
	  if (msg_imap)
	    msg_imap->flags |= MU_ATTRIBUTE_SEEN;
	  else
	    f_imap->flags |= MU_ATTRIBUTE_SEEN;
	}
      else if (strcasecmp (flag, "\\Answered") == 0)
	{
	  if (msg_imap)
	    msg_imap->flags |= MU_ATTRIBUTE_ANSWERED;
	  else
	    f_imap->flags |= MU_ATTRIBUTE_ANSWERED;
	}
      else if (strcasecmp (flag, "\\Flagged") == 0)
	{
	  if (msg_imap)
	    msg_imap->flags |= MU_ATTRIBUTE_FLAGGED;
	  else
	    f_imap->flags |= MU_ATTRIBUTE_FLAGGED;
	}
      else if (strcasecmp (flag, "\\Deleted") == 0)
	{
	  if (msg_imap)
	    msg_imap->flags |= MU_ATTRIBUTE_DELETED;
	  else
	    f_imap->flags |= MU_ATTRIBUTE_DELETED;
	}
      else if (strcasecmp (flag, "\\Draft") == 0)
	{
	  if (msg_imap)
	    msg_imap->flags |= MU_ATTRIBUTE_DRAFT;
	  else
	    f_imap->flags |= MU_ATTRIBUTE_DRAFT;
	}
      else if (strcasecmp (flag, "\\Read") == 0)
	{
	  if (msg_imap)
	    msg_imap->flags |= MU_ATTRIBUTE_READ;
	  else
	    f_imap->flags |= MU_ATTRIBUTE_READ;
	}
    }
  return 0;
}

static int
imap_body (f_imap_t f_imap, char **ptr)
{
  int status;

  while (**ptr && **ptr == ' ')
    (*ptr)++;

  if (**ptr == '[')
    {
      char *sep = strchr (*ptr, ']');
      (*ptr)++; /* Move pass the '[' */
      if (sep)
	{
	  size_t len = sep - *ptr;
	  char *section = alloca (len + 1);
	  char *p = section;
	  strncpy (section, *ptr, len);
	  section[len] = '\0';
	  /* strupper.  */
	  for (; *p; p++)
	    if (isalpha((unsigned)*p))
	      *p = toupper ((unsigned)*p);
	  /* Check to see the callback type to update the line count.  */
	  if (!strstr (section, "FIELD"))
	    {
	      if (strstr (section, "MIME") || (strstr (section, "HEADER")))
		{
		  f_imap->callback.type = IMAP_HEADER;
		}
	      else if (strstr (section, "TEXT") || len > 0)
		{
		  f_imap->callback.type = IMAP_BODY;
		}
	      else if (len == 0) /* body[]  */
		{
		  f_imap->callback.type = IMAP_MESSAGE;
		}
	    }
	  sep++; /* Move pass the ']'  */
	  *ptr = sep;
	}
    }
  while (**ptr && **ptr == ' ')
    (*ptr)++;
  if (**ptr == '<')
    {
      char *sep = strchr (*ptr, '>');
      if (sep)
	{
	  sep++;
	  *ptr = sep;
	}
    }
  status = imap_string (f_imap, ptr);
  f_imap->callback.type = IMAP_MESSAGE;
  return status;
}

static int
imap_internaldate (f_imap_t f_imap, char **ptr)
{
  return imap_string (f_imap, ptr);
}

static int
imap_uid (f_imap_t f_imap, char **ptr)
{
  char token[128];
  imap_token (token, sizeof (token), ptr);
  if (f_imap->callback.msg_imap)
    f_imap->callback.msg_imap->uid = strtoul (token, NULL, 10);
  return 0;
}

static int
imap_rfc822 (f_imap_t f_imap, char **ptr)
{
  int status;
  f_imap->callback.type = IMAP_MESSAGE;
  status = imap_body (f_imap, ptr);
  f_imap->callback.type = 0;
  return status;
}

static int
imap_rfc822_size (f_imap_t f_imap, char **ptr)
{
  char token[128];
  imap_token (token, sizeof (token), ptr);
  if (f_imap->callback.msg_imap)
    f_imap->callback.msg_imap->message_size = strtoul (token, NULL, 10);
  return 0;
}

static int
imap_rfc822_header (f_imap_t f_imap, char **ptr)
{
  int status;
  f_imap->callback.type = IMAP_HEADER;
  status = imap_string (f_imap, ptr);
  f_imap->callback.type = 0;
  return status;
}

static int
imap_rfc822_text (f_imap_t f_imap, char **ptr)
{
  int status;
  f_imap->callback.type = IMAP_HEADER;
  status = imap_string (f_imap, ptr);
  f_imap->callback.type = 0;
  return status;
}

/* Parse imap unfortunately FETCH is use as response for many commands.
   We just use a small set an ignore the other ones :
   not use  : ALL
   use      : BODY
   not use  : BODY[<section>]<<partial>>
   use      : BODY.PEEK[<section>]<<partial>>
   not use  : BODYSTRUCTURE
   not use  : ENVELOPE
   not use  : FAST
   use      : FLAGS
   not use  : FULL
   use      : INTERNALDATE
   not use  : RFC822
   not use  : RFC822.HEADER
   use      : RFC822.SIZE
   not use  : RFC822.TEXT
   not use  : UID
 */
static int
imap_fetch (f_imap_t f_imap)
{
  char token[128];
  size_t msgno = 0;
  m_imap_t m_imap = f_imap->selected;
  int status = 0;
  char *sp = NULL;

  /* We should have a mailbox selected.  */
  assert (m_imap != NULL);

  /* Parse the FETCH respones to extract the pieces.  */
  sp = f_imap->buffer;

  /* Skip untag '*'.  */
  imap_token (token, sizeof (token), &sp);
  /* Get msgno.  */
  imap_token (token, sizeof (token), &sp);
  msgno = strtol (token, NULL,  10);
  /* Skip FETCH .  */
  imap_token (token, sizeof (token), &sp);

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

  while (*sp && *sp != ')')
    {
      /* Get the token.  */
      imap_token (token, sizeof (token), &sp);

      if (strncmp (token, "FLAGS", 5) == 0)
	{
	  status = imap_flags (f_imap, &sp);
	}
      else if (strcasecmp (token, "BODY") == 0)
	{
	  if (*sp == '[')
	    status = imap_body (f_imap, &sp);
	  else
	    status = imap_bodystructure (f_imap, &sp);
	}
      else if (strcasecmp (token, "BODYSTRUCTURE") == 0)
	{
	  status = imap_bodystructure (f_imap, &sp);
	}
      else if (strncmp (token, "INTERNALDATE", 12) == 0)
	{
	  status = imap_internaldate (f_imap, &sp);
	}
      else if (strncmp (token, "RFC822", 10) == 0)
	{
	  if (*sp == '.')
	    {
	      sp++;
	      imap_token (token, sizeof (token), &sp);
	      if (strcasecmp (token, "SIZE") == 0)
		{
		  status = imap_rfc822_size (f_imap, &sp);
		}
	      else if (strcasecmp (token, "TEXT") == 0)
		{
		  status = imap_rfc822_text (f_imap, &sp);
		}
	      else if (strcasecmp (token, "HEADER") == 0)
		{
		  status = imap_rfc822_header (f_imap, &sp);
		}
	      /* else mu_error ("not supported RFC822 option\n");  */
	    }
	  else
	    {
	      status = imap_rfc822 (f_imap, &sp);
	    }
	}
      else if (strncmp (token, "UID", 3) == 0)
	{
	  status = imap_uid (f_imap, &sp);
	}
      /* else mu_error ("not supported FETCH command\n");  */
    }
  return status;
}

static int
imap_token (char *buf, size_t len, char **ptr)
{
  char *start = *ptr;
  size_t i;
  /* Skip leading space.  */
  while (**ptr && **ptr == ' ')
    (*ptr)++;
  /* Break the string by token, when we recognise Atoms we stop.  */
  for (i = 1; **ptr && i < len; (*ptr)++, buf++, i++)
    {
      if (**ptr == ' ' || **ptr == '.'
	  || **ptr == '(' || **ptr == ')'
	  || **ptr == '[' || **ptr == ']'
	  || **ptr == '<' || **ptr  == '>')
	{
	  /* Advance.  */
	  if (start == (*ptr))
	    (*ptr)++;
	  break;
	}
      *buf = **ptr;
  }
  *buf = '\0';
  /* Skip trailing space.  */
  while (**ptr && **ptr == ' ')
    (*ptr)++;
  return  *ptr - start;;
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
  if (f_imap->ptr > f_imap->buffer)
    {
      size_t len;
      size_t n = 0;
      len = f_imap->ptr - f_imap->buffer;
      status = stream_write (f_imap->folder->stream, f_imap->buffer, len,
			     0, &n);
      if (status == 0)
        {
          memmove (f_imap->buffer, f_imap->buffer + n, len - n);
          f_imap->ptr -= n;
        }
    }
  else
    f_imap->ptr = f_imap->buffer;
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

      /* The server went away:  It maybe a timeout and some imap server
	 does not send the BYE.  Consider like an error.  */
      if (n == 0)
	return EIO;

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
#if 0
      /* Comment out to see all reading traffic.  */
      mu_error ("\t\t%s", f_imap->buffer);
#endif

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
	  if (strcasecmp (response, "OK") == 0)
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

		  if (strcasecmp (subtag, "ALERT") == 0)
		    {
		      /* The human-readable text contains a special alert that
			 MUST be presented to the user in a fashion that calls
			 the user's attention to the message.  */
		      mu_error ("ALERT: %s\n", (sp) ? sp : "");
		    }
		  else if (strcasecmp (subtag, "BADCHARSET") == 0)
		    {
		      /* Optionally followed by a parenthesized list of
			 charsets.  A SEARCH failed because the given charset
			 is not supported by this implementation.  If the
			 optional list of charsets is given, this lists the
			 charsets that are supported by this implementation. */
		    }
		  else if (strcasecmp (subtag, "CAPABILITY") == 0)
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
		  else if (strcasecmp (subtag, "NEWNAME") == 0)
		    {
		      /* Followed by a mailbox name and a new mailbox name.  A
			 SELECT or EXAMINE failed because the target mailbox
			 name (which once existed) was renamed to the new
			 mailbox name.  This is a hint to the client that the
			 operation can succeed if the SELECT or EXAMINE is
			 reissued with the new mailbox name. */
		    }
		  else if (strcasecmp (subtag, "PARSE") == 0)
		    {
		      /* The human-readable text represents an error in
			 parsing the [RFC-822] header or [MIME-IMB] headers
			 of a message in the mailbox.  */
		    }
		  else if (strcasecmp (subtag, "PERMANENTFLAGS") == 0)
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
		  else if (strcasecmp (subtag, "READ-ONLY") == 0)
		    {
		      /* The mailbox is selected read-only, or its access
			 while selected has changed from read-write to
			 read-only.  */
		    }
		  else if (strcasecmp (subtag, "READ-WRITE") == 0)
		    {
		      /* The mailbox is selected read-write, or its access
			 while selected has changed from read-only to
			 read-write.  */
		    }
		  else if (strcasecmp (subtag, "TRYCREATE") == 0)
		    {
		      /* An APPEND or COPY attempt is failing because the
			 target mailbox does not exist (as opposed to some
			 other reason).  This is a hint to the client that
			 the operation can succeed if the mailbox is first
			 created by the CREATE command.  */
		    }
		  else if (strcasecmp (subtag, "UIDNEXT") == 0)
		    {
		      /* Followed by a decimal number, indicates the next
			 unique identifier value.  Refer to section 2.3.1.1
			 for more information.  */
		      char *value = strtok_r (NULL, " ", &sp);
		      f_imap->selected->uidnext = strtol (value, NULL, 10);
		    }
		  else if (strcasecmp (subtag, "UIDVALIDITY") == 0)
		    {
		      /* Followed by a decimal number, indicates the unique
			 identifier validity value.  Refer to section 2.3.1.1
			 for more information.  */
		      char *value = strtok_r (NULL, " ", &sp);
		      f_imap->selected->uidvalidity = strtol (value, NULL, 10);
		    }
		  else if (strcasecmp (subtag, "UNSEEN") == 0)
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
		  mu_error ("Untagged OK: %s\n", remainder);
		}
	    }
	  else if (strcasecmp (response, "NO") == 0)
	    {
	      /* This does not mean failure but rather a strong warning.  */
	      mu_error ("Untagged NO: %s\n", remainder);
	    }
	  else if (strcasecmp (response, "BAD") == 0)
	    {
	      /* We're dead, protocol/syntax error.  */
	      mu_error ("Untagged BAD: %s\n", remainder);
	    }
	  else if (strcasecmp (response, "PREAUTH") == 0)
	    {
	    }
	  else if (strcasecmp (response, "BYE") == 0)
	    {
	      /* We should close the stream. This is not recoverable.  */
	      done = 1;
	      monitor_wrlock (f_imap->folder->monitor);
	      f_imap->isopen = 0;
	      f_imap->selected = NULL;
	      monitor_unlock (f_imap->folder->monitor);
	      stream_close (f_imap->folder->stream);
	    }
	  else if (strcasecmp (response, "CAPABILITY") == 0)
	    {
	      if (f_imap->capa)
		free (f_imap->capa);
	      f_imap->capa = strdup (remainder);
	    }
	  else if (strcasecmp (remainder, "EXISTS") == 0)
	    {
	      f_imap->selected->messages_count = strtol (response, NULL, 10);
	    }
	  else if (strcasecmp (remainder, "EXPUNGE") == 0)
	    {
	    }
	  else if (strncasecmp (remainder, "FETCH", 5) == 0)
	    {
	      status = imap_fetch (f_imap);
	      if (status != 0)
		break;
	    }
	  else if (strcasecmp (response, "FLAGS") == 0)
	    {
	      /* Flags define on the mailbox not a message flags.  */
	      status = imap_flags (f_imap, &remainder);
	    }
	  else if (strcasecmp (response, "LIST") == 0)
	    {
	      status = imap_list (f_imap);
	    }
	  else if (strcasecmp (response, "LSUB") == 0)
	    {
	      status = imap_list (f_imap);
	    }
	  else if (strcasecmp (remainder, "RECENT") == 0)
	    {
	      f_imap->selected->recent = strtol (response, NULL, 10);
	    }
	  else if (strcasecmp (response, "SEARCH") == 0)
	    {
	    }
	  else if (strcasecmp (response, "STATUS") == 0)
	    {
	    }
	  else
	    {
	      /* Once again, check for something strange.  */
	      mu_error ("unknown untagged response: \"%s\"  %s\n",
			response, remainder);
	    }
	}
      /* Continuation token ???.  */
      else if (tag && tag[0] == '+')
	{
	  done = 1;
	}
      else
	{
	  /* Every transaction ends with a tagged response.  */
	  done = 1;
	  if (strcasecmp (response, "OK") == 0)
	    {
	    }
	  else /* NO and BAD */
	    {
	      if (strncasecmp (remainder, "LOGIN", 5) == 0)
		{
		  observable_t observable = NULL;
		  folder_get_observable (f_imap->folder, &observable);
		  observable_notify (observable, MU_EVT_AUTHORITY_FAILED);
		}
	      mu_error ("NO/Bad Tagged: %s %s\n", response, remainder);
	      status = EINVAL;
	    }
	}
      f_imap->ptr = f_imap->buffer;
    }
  return status;
}

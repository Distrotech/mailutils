/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2008-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not,
   see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_PROG
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <mailutils/address.h>
#include <mailutils/argcv.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/header.h>
#include <mailutils/message.h>
#include <mailutils/observer.h>
#include <mailutils/progmailer.h>
#include <mailutils/wordsplit.h>

#include <mailutils/sys/url.h>
#include <mailutils/sys/mailer.h>
#include <mailutils/sys/registrar.h>

static int _url_prog_init     (mu_url_t);

static struct _mu_record _prog_record =
{
  MU_PROG_PRIO,
  MU_PROG_SCHEME,
  MU_RECORD_DEFAULT,
  /* FIXME: MU_URL_USER could be used to request running with this
     user privileges. */
  MU_URL_SCHEME | MU_URL_PATH | MU_URL_QUERY, 
  MU_URL_PATH,
  _url_prog_init,    /* url init.  */
  _mu_mailer_mailbox_init,  /* Mailbox entry.  */
  _mu_mailer_prog_init, /* Mailer entry.  */
  _mu_mailer_folder_init, /* Folder entry.  */
  NULL, /* No need for a back pointer.  */
  NULL, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};

mu_record_t mu_prog_record = &_prog_record;


static int
_url_prog_uplevel (const mu_url_t orig, mu_url_t *up)
{
  return MU_ERR_NOENT;
}

static int
_url_prog_init (mu_url_t url)
{
  url->_uplevel = _url_prog_uplevel;
  return 0;
}


static void prog_destroy (mu_mailer_t);
static int prog_open (mu_mailer_t, int);
static int prog_close (mu_mailer_t);
static int prog_send_message (mu_mailer_t, mu_message_t, mu_address_t,
			      mu_address_t);

int
_mu_mailer_prog_init (mu_mailer_t mailer)
{
  int status;
  mu_progmailer_t pm;

  status = mu_progmailer_create (&pm);
  if (status)
    return status;

  mailer->data = pm;
  mailer->_destroy = prog_destroy;
  mailer->_open = prog_open;
  mailer->_close = prog_close;
  mailer->_send_message = prog_send_message;
  
  return 0;
}

static void
prog_destroy (mu_mailer_t mailer)
{
  mu_progmailer_destroy ((mu_progmailer_t*)&mailer->data);
}

static int
prog_open (mu_mailer_t mailer, int flags)
{
  mu_progmailer_t pm = mailer->data;
  int status;
  const char *path;
  
  /* Sanity checks.  */
  if (pm == NULL)
    return EINVAL;

  mailer->flags = flags;

  if ((status = mu_url_sget_path (mailer->url, &path)))
    return status;
  
  if (access (path, X_OK) == -1)
    return errno;

  status = mu_progmailer_set_command (pm, path);
  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_TRACE, ("prog (%s)", path));
  return status;
}

static int
prog_close (mu_mailer_t mailer)
{
  return mu_progmailer_close (mailer->data);
}

struct prog_exp
{
  mu_message_t msg;
  mu_address_t sender_addr;
  char *sender_str;
  mu_address_t rcpt_addr;
  char *rcpt_str;
};

static const char *
_expand_sender (struct prog_exp *pe)
{
  if (!pe->sender_str &&
      mu_address_aget_email (pe->sender_addr, 1, &pe->sender_str))
    return NULL;
  return pe->sender_str;
}

static int
address_add (mu_address_t *paddr, const char *value)
{
  mu_address_t addr = NULL;
  int status;
  
  status = mu_address_create (&addr, value);
  if (status)
    return status;
  status = mu_address_union (paddr, addr);
  mu_address_destroy (&addr);
  return status;
}

static int
message_read_rcpt (mu_message_t msg, mu_address_t *paddr)
{
  mu_header_t header = NULL;
  const char *value;
  int status;
  
  status = mu_message_get_header (msg, &header);
  if (status)
    return status;
  
  status = mu_header_sget_value (header, MU_HEADER_TO, &value);

  if (status == 0)
    address_add (paddr, value);
  else if (status != MU_ERR_NOENT)
    return status;

  status = mu_header_sget_value (header, MU_HEADER_CC, &value);
  if (status == 0)
    address_add (paddr, value);
  else if (status != MU_ERR_NOENT)
    return status;

  status = mu_header_sget_value (header, MU_HEADER_BCC, &value);
  if (status == 0)
    address_add (paddr, value);
  else if (status != MU_ERR_NOENT)
    return status;
  return 0;
}

static const char *
_expand_rcpt (struct prog_exp *pe)
{
  int status;

  if (!pe->rcpt_str)
    {
      size_t i, count = 0;
      size_t len = 0;
      char *str;
      mu_address_t tmp_addr = NULL, addr;
      
      if (pe->rcpt_addr)
	addr = pe->rcpt_addr;
      else
	{
	  status = message_read_rcpt (pe->msg, &tmp_addr);
	  if (status)
	    {
	      mu_address_destroy (&tmp_addr);
	      return NULL;
	    }
	  addr = tmp_addr;
	}
	    
      mu_address_get_count (addr, &count);
      for (i = 1; i <= count; i++)
	{
	  const char *email;
	  if (i > 1)
	    len++;
	  if ((status = mu_address_sget_email (addr, i, &email)) != 0)
	    {
	      mu_address_destroy (&tmp_addr);
	      return NULL;
	    }
	  len += strlen (email);
	}

      str = malloc (len + 1);
      if (!str)
	{
	  mu_address_destroy (&tmp_addr);
	  return NULL;
	}
      pe->rcpt_str = str;
      
      for (i = 1; i <= count; i++)
	{
	  const char *email;
	  if (i > 1)
	    *str++ = ' ';
	  if (mu_address_sget_email (addr, i, &email))
	    continue;
	  strcpy (str, email);
	  str += strlen (email);
	}
      *str = 0;
      mu_address_destroy (&tmp_addr);
    }  
  return pe->rcpt_str;
}

#define SEQ(s, n, l) \
  (((l) == (sizeof(s) - 1)) && memcmp (s, n, l) == 0)

static const char *
prog_getvar (const char *name, size_t nlen, void *data)
{
  struct prog_exp *pe = data;
  
  if (SEQ ("sender", name, nlen))
    return _expand_sender (pe);
  if (SEQ ("rcpt", name, nlen))
    return _expand_rcpt (pe);
  return NULL;
}

static int
url_to_argv (mu_url_t url, mu_message_t msg,
	     mu_address_t from, mu_address_t to,
	     int *pargc, char ***pargv)
{
  int rc;
  struct prog_exp pe;
  char **query;
  size_t i;
  size_t argc;
  char **argv;
  struct mu_wordsplit ws;
  int wsflags;
  
  pe.msg = msg;
  pe.rcpt_addr = to;
  pe.sender_addr = from;
  pe.sender_str = pe.rcpt_str = NULL;

  ws.ws_getvar = prog_getvar;
  ws.ws_closure = &pe;
  wsflags = MU_WRDSF_NOSPLIT | MU_WRDSF_NOCMD |
            MU_WRDSF_GETVAR | MU_WRDSF_CLOSURE;

  rc = mu_url_sget_query (url, &argc, &query);
  if (rc)
    return rc;

  argv = calloc (argc + 2, sizeof (argv[0]));
  if (!argv)
    return ENOMEM;

  rc = mu_url_aget_path (url, &argv[0]);
  if (rc)
    {
      free (argv);
      return rc;
    }
  
  for (i = 0; i < argc; i++)
    {
      if (mu_wordsplit (query[i], &ws, wsflags))
	{
	  mu_argcv_free (i, argv);
	  mu_wordsplit_free (&ws);
	  return errno;
	}
      if (ws.ws_wordc == 0)
	argv[i+1] = strdup ("");
      else
	argv[i+1] = strdup (ws.ws_wordv[0]);
      if (!argv[i+1])
        {
          mu_argcv_free(i, argv);
          return ENOMEM;
        }
      wsflags |= MU_WRDSF_REUSE;
    }
  argv[i+1] = NULL;
  mu_wordsplit_free (&ws);
  free (pe.sender_str);
  free (pe.rcpt_str);
  
  *pargc = argc;
  *pargv = argv;
  return 0;
}

static int
prog_send_message (mu_mailer_t mailer, mu_message_t msg, mu_address_t from,
		   mu_address_t to)
{
  mu_progmailer_t pm = mailer->data;
  int argc;
  char **argv;
  int status;
  const char *command;

  status = mu_url_sget_path (mailer->url, &command);
  if (status && status != MU_ERR_NOENT)
    {
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("cannot get path from URL: %s",
		 mu_strerror (status)));
      return status;
    }
  status = mu_progmailer_set_command (pm, command);
  if (status)
    {
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("cannot set progmailer command: %s",
		 mu_strerror (status)));
      return status;
    }
      
  status = url_to_argv (mailer->url, msg, from, to, &argc, &argv);
  if (status)
    {
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("cannot convert URL to command line: %s",
		 mu_strerror (status)));
      return status;
    }

  status = mu_progmailer_open (pm, argv);
  if (status == 0)
    {
      status = mu_progmailer_send (pm, msg);
      if (status == 0)
	mu_observable_notify (mailer->observable, MU_EVT_MAILER_MESSAGE_SENT,
			      msg);
      else
	mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		  ("progmailer error: %s", mu_strerror (status)));
    }
  
  mu_argcv_free (argc, argv);
  return status;
}

#else
#include <stdio.h>
#include <mailutils/sys/registrar.h>
mu_record_t mu_prog_record = NULL;
#endif

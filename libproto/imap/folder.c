/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include <mailutils/imap.h>
#include <mailutils/diag.h>
#include <mailutils/debug.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/tls.h>
#include <mailutils/url.h>
#include <mailutils/list.h>
#include <mailutils/secret.h>
#include <mailutils/cstr.h>
#include <mailutils/sockaddr.h>
#include <mailutils/wordsplit.h>
#include <mailutils/sys/imap.h>
#include <mailutils/sys/folder.h>

static void
_mu_imap_folder_destroy (mu_folder_t folder)
{
  mu_imap_t imap = folder->data;
  if (imap)
    {
      mu_folder_close (folder);
      mu_imap_destroy (&imap);
      folder->data = imap;
    }
}

/* Imap callbacks */
static void
_mu_folder_preauth_callback (void *data, int code, size_t sdat, void *pdat)
{
  /*  mu_folder_t folder = data;*/
  const char *text = pdat;
  mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_TRACE1,
	    (_("IMAP server opened in preauth mode: %s"), text));
}

static void
_mu_folder_bye_callback (void *data, int code, size_t sdat, void *pdat)
{
  mu_folder_t folder = data;
  const char *text = pdat;
  mu_imap_t imap = folder->data;
  mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_TRACE1,
	    (_("IMAP server closing connection: %s"), text));
  /*FIXME:  mu_imap_disconnect (imap);*/
}

static void
_mu_folder_bad_callback (void *data, int code, size_t sdat, void *pdat)
{
  /*  mu_folder_t folder = data;*/
  const char *text = pdat;

  mu_error (_("IMAP server complains: %s"), text);
  mu_error (_("This probably indicates a bug in Mailutils client code."));
  mu_error (_("Please, report that to <%s>."), PACKAGE_BUGREPORT);
}

/* Set up an IMAP(S) connection for this folder */
static int
_mu_imap_folder_open (mu_folder_t folder, int flags)
{
  int rc;
  mu_imap_t imap = folder->data;
  struct mu_sockaddr *sa;
  struct mu_sockaddr_hints hints;
  char const *s;
  int tls;
  mu_stream_t transport;

  /* FIXME: This monitor business is suspicious */
  mu_monitor_wrlock (folder->monitor);
  rc = mu_imap_session_state (imap);
  mu_monitor_unlock (folder->monitor);
  if (rc != MU_IMAP_SESSION_INIT)
    return 0;

  mu_url_sget_scheme (folder->url, &s);
  tls = strcmp (s, "imaps") == 0;

  memset (&hints, 0, sizeof (hints));
  hints.flags = MU_AH_DETECT_FAMILY;
  hints.port = tls ? MU_IMAP_DEFAULT_SSL_PORT : MU_IMAP_DEFAULT_PORT;
  hints.protocol = IPPROTO_TCP;
  hints.socktype = SOCK_STREAM;

  rc = mu_sockaddr_from_url (&sa, folder->url, &hints);
  if (rc)
    {
      s = mu_url_to_string (folder->url);
      mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_ERROR,
		(_("cannot create sockaddr from URL %s: %s"),
		 s, mu_strerror (rc)));
      return rc;
    }

  rc = mu_tcp_stream_create_from_sa (&transport, sa, NULL, 0);
  if (rc)
    {
      s = mu_url_to_string (folder->url);
      mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_ERROR,
		(_("cannot create stream from URL %s: %s"),
		 s, mu_strerror (rc)));
      mu_sockaddr_free (sa);
      return rc;
    }
  
#ifdef WITH_TLS
  if (tls)
    {
      mu_stream_t tlsstream;
	      
      rc = mu_tls_client_stream_create (&tlsstream, transport, transport, 0);
      mu_stream_unref (transport);
      if (rc)
	{
	  mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_ERROR,
		    (_("cannot create TLS stream: %s"),
		     mu_strerror (rc)));
	  return rc;
	}
      transport = tlsstream;
    }
#endif

  mu_imap_set_carrier (imap, transport);

  if (mu_debug_level_p (MU_DEBCAT_FOLDER, MU_DEBUG_PROT) ||
      mu_debug_level_p (MU_DEBCAT_MAILBOX, MU_DEBUG_PROT))
    mu_imap_trace (imap, MU_IMAP_TRACE_SET);
  if (mu_debug_level_p (MU_DEBCAT_FOLDER, MU_DEBUG_TRACE6) ||
      mu_debug_level_p (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE6))
    mu_imap_trace_mask (imap, MU_IMAP_TRACE_SET, MU_XSCRIPT_SECURE);
  if (mu_debug_level_p (MU_DEBCAT_FOLDER, MU_DEBUG_TRACE7) ||
      mu_debug_level_p (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE7))
    mu_imap_trace_mask (imap, MU_IMAP_TRACE_SET, MU_XSCRIPT_PAYLOAD);

  /* Set callbacks */
  mu_imap_register_callback_function (imap, MU_IMAP_CB_PREAUTH,
				      _mu_folder_preauth_callback,
				      folder);
  mu_imap_register_callback_function (imap, MU_IMAP_CB_BYE,
				      _mu_folder_bye_callback,
				      folder);
  mu_imap_register_callback_function (imap, MU_IMAP_CB_BAD,
				      _mu_folder_bad_callback,
				      folder);
  rc = mu_imap_connect (imap);
  if (rc)
    {
      s = mu_url_to_string (folder->url);
      mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_ERROR,
		(_("failed to connect to %s: %s"),
		 s, mu_strerror (rc)));
      if (mu_imap_strerror (imap, &s))
	mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_ERROR,
		  (_("server response: %s"), s));
      mu_imap_destroy (&imap);
      return rc;
    }

#ifdef WITH_TLS
  if (!tls && mu_imap_capability_test (imap, "STARTTLS", NULL) == 0)
    {
      size_t parmc = 0;
      char **parmv = NULL;

      tls = 1;
      if (mu_url_sget_fvpairs (folder->url, &parmc, &parmv) == 0)
	{
	  size_t i;
      
	  for (i = 0; i < parmc; i++)
	    {
	      if (strcmp (parmv[i], "notls") == 0)
		tls = 0;
	      /*FIXME:
		else if (strncmp (parmv[i], "auth=", 5) == 0)
	      */
	      /* unrecognized arguments silently ignored */
	    }
	}

      if (tls)
	mu_imap_starttls (imap);
    }
#endif
  if (mu_imap_session_state (imap) == MU_IMAP_SESSION_NONAUTH)
    {
      rc = mu_authority_authenticate (folder->authority);
      if (rc)
	{
	  mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_ERROR,
		    (_("IMAP authentication: %s"),
		     mu_strerror (rc)));
	  mu_folder_close (folder);
	}
    }
    
  return rc;
}

/* Close existing connection */
static int
_mu_imap_folder_close (mu_folder_t folder)
{
  mu_imap_t imap = folder->data;

  if (mu_imap_session_state (imap) > MU_IMAP_SESSION_INIT)
    {
      mu_imap_clearerr (imap);
      mu_imap_logout (imap);
      mu_imap_disconnect (imap);
    }
  
  return 0;
}

struct enumerate_closure
{
  mu_folder_t folder;
  mu_folder_enumerate_fp fun;
  void *data;
};

static int
_enumerate_helper (void *item, void *data)
{
  struct mu_list_response *rp = item;
  struct enumerate_closure *clos = data;

  return clos->fun (clos->folder, rp, clos->data);
}

static int
_mu_imap_folder_list (mu_folder_t folder, const char *ref, void *name,
		      int flags, size_t max_level,
		      mu_list_t flist,
		      mu_folder_enumerate_fp efp, void *edp)
{
  mu_imap_t imap = folder->data;
  mu_list_t list;
  int rc = mu_imap_list_new (imap, ref, name, &list);

  if (rc)
    return rc;

  if (max_level ||
      (flags & MU_FOLDER_ATTRIBUTE_ALL) != MU_FOLDER_ATTRIBUTE_ALL)
    {
      /* Filter out the list, eliminating non-matching entries */
      mu_iterator_t itr;

      rc = mu_list_get_iterator (list, &itr);
      if (rc)
	{
	  mu_list_destroy (&list);
	  return rc;
	}

      for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  struct mu_list_response *rp;

	  mu_iterator_current (itr, (void**) &rp);
	  if (!(rp->type & flags) || (max_level && rp->level > max_level))
	    mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	}
      mu_iterator_destroy (&itr);
    }

  if (efp)
    {
      struct enumerate_closure clos;

      clos.folder = folder;
      clos.fun = efp;
      clos.data = edp;

      rc = mu_list_foreach (list, _enumerate_helper, &clos);
    }

  if (flist)
    mu_list_append_list (flist, list);

  mu_list_destroy (&list);

  return rc;
}

static int
_mu_imap_folder_lsub (mu_folder_t folder, const char *ref, const char *name,
		      mu_list_t flist)
{
  mu_imap_t imap = folder->data;
  return mu_imap_lsub (imap, ref, name, flist);
}

/* Subscribe to the named mailbox. */
static int
_mu_imap_folder_subscribe (mu_folder_t folder, const char *name)
{
  mu_imap_t imap = folder->data;
  return mu_imap_subscribe (imap, name);
}
/* Unsubscribe from the mailbox. */
static int
_mu_imap_folder_unsubscribe (mu_folder_t folder, const char *name)
{
  mu_imap_t imap = folder->data;
  return mu_imap_unsubscribe (imap, name);
}

/* Remove a mailbox.  */
static int
_mu_imap_folder_delete (mu_folder_t folder, const char *name)
{
  mu_imap_t imap = folder->data;
  return mu_imap_delete (imap, name);
}

/* Rename OLDPATH to NEWPATH */
static int
_mu_imap_folder_rename (mu_folder_t folder, const char *oldpath,
			const char *newpath)
{
  mu_imap_t imap = folder->data;
  return mu_imap_rename (imap, oldpath, newpath);
}

typedef int (*auth_method_t) (mu_authority_t);

static int
authenticate_imap_login (mu_authority_t auth)
{
  mu_folder_t folder = mu_authority_get_owner (auth);
  mu_imap_t imap = folder->data;
  mu_ticket_t ticket;
  char *user;
  int rc;
  mu_secret_t secret;
  
  rc = mu_imap_capability_test (imap, "LOGINDISABLED", NULL);
  if (rc == 0)
    {
      mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_ERROR,
		(_("IMAP LOGIN disabled")));
      return rc;
    }
  else if (rc != MU_ERR_NOENT)
    {
      mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_ERROR,
		(_("cannot test server capabilities (%s), continuing anyway"),
		 mu_strerror (rc)));
      return rc;
    }

  /* Grab the User and Passwd information.  */
  mu_authority_get_ticket (auth, &ticket);
  /* Was it in the URL?  */
  rc = mu_url_aget_user (folder->url, &user);
  if (rc == MU_ERR_NOENT)
    rc = mu_ticket_get_cred (ticket, folder->url, "Imap User: ", &user, NULL);
  if (rc == MU_ERR_NOENT || user == NULL)
    return MU_ERR_NOUSERNAME;
  else if (rc)
    return rc;

  rc = mu_url_get_secret (folder->url, &secret);
  if (rc == MU_ERR_NOENT)
    rc = mu_ticket_get_cred (ticket, folder->url,
			     "Imap Passwd: ", NULL, &secret);
	
  if (rc == MU_ERR_NOENT || !secret)
    {
      /* FIXME: Is this always right? The user might legitimately have
	 no password */
      free (user);
      return MU_ERR_NOPASSWORD;
    }
  else if (rc)
    {
      free (user);
      return rc;
    }
  rc = mu_imap_login_secret (imap, user, secret);
  mu_secret_unref (secret);
  return rc;
}

struct auth_tab
{
  char *name;
  auth_method_t method;
};

/* NOTE: The ordering of methods in this table is from most secure
   to less secure. */

static struct auth_tab auth_tab[] = {
  { "login", authenticate_imap_login },
  /*  { "anon",  authenticate_imap_sasl_anon },*/
  { NULL }
};

static auth_method_t
find_auth_method (const char *name)
{
  struct auth_tab *p;

  for (p = auth_tab; p->name; p++)
    if (mu_c_strcasecmp (p->name, name) == 0)
      return p->method;

  return NULL;
}

static int
authenticate_imap_select (mu_authority_t auth)
{
  struct auth_tab *p;
  int rc = ENOSYS;
  
  for (p = auth_tab; rc == ENOSYS && p->name; p++)
    rc = p->method (auth);

  return rc;
}

static int
folder_set_auth_method (mu_folder_t folder, auth_method_t method)
{
  if (!folder->authority)
    {
      int rc = mu_authority_create (&folder->authority, NULL, folder);
      if (rc)
	return rc;
    }
  return mu_authority_set_authenticate (folder->authority, method, folder);
}

static int
_imap_folder_setup_authority (mu_folder_t folder)
{
  int rc = 0;

  if (!folder->authority)
    {
      const char *auth;
      if (folder->url == NULL)
	return EINVAL;

      if (mu_url_sget_auth (folder->url, &auth))
	rc = folder_set_auth_method (folder, authenticate_imap_select);
      else if (strcmp (auth, "*") == 0)
	rc = folder_set_auth_method (folder, authenticate_imap_select);
      else 
	{
	  struct mu_wordsplit ws;

	  ws.ws_delim = ",";
	  if (mu_wordsplit (auth, &ws,
			    MU_WRDSF_NOVAR | MU_WRDSF_NOCMD |
			    MU_WRDSF_DELIM))
	    {
	      mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_ERROR,
			(_("cannot split out auth part: %s"),
			 mu_wordsplit_strerror (&ws)));
	      rc = MU_ERR_FAILURE;
	    }
	  else
	    {
	      int i;

	      for (i = 0; i < ws.ws_wordc; i++)
		{
		  auth_method_t method = find_auth_method (ws.ws_wordv[i]);
		  if (method)
		    rc = folder_set_auth_method (folder, method);
		  else
		    {
		      mu_debug (MU_DEBCAT_FOLDER, MU_DEBUG_ERROR,
				(_("unrecognized AUTH scheme %s"),
				 ws.ws_wordv[i]));
		      rc = MU_ERR_BAD_AUTH_SCHEME;
		    }
		}
	      mu_wordsplit_free (&ws);
	    }
	}
    }

  return rc;
}

int
_mu_imap_folder_init (mu_folder_t folder)
{
  mu_imap_t imap;
  int rc;

  rc = _imap_folder_setup_authority (folder);
  if (rc)
    return rc;
  
  rc = mu_imap_create (&imap);
  if (rc)
    return rc;

  folder->data = imap;

  folder->_destroy = _mu_imap_folder_destroy;

  folder->_open = _mu_imap_folder_open;
  folder->_close = _mu_imap_folder_close;

  folder->_list = _mu_imap_folder_list;
  folder->_lsub = _mu_imap_folder_lsub;

  folder->_subscribe = _mu_imap_folder_subscribe;
  folder->_unsubscribe = _mu_imap_folder_unsubscribe;
  folder->_delete = _mu_imap_folder_delete;
  folder->_rename = _mu_imap_folder_rename;
  return 0;
}

static struct _mu_record _imap_record =
{
  MU_IMAP_PRIO,
  MU_IMAP_SCHEME,
  MU_RECORD_DEFAULT,
  MU_URL_SCHEME | MU_URL_CRED | MU_URL_INET | MU_URL_PATH | MU_URL_PARAM,
  MU_URL_HOST,
  _mu_imap_url_init,     /* url entry.  */
  _mu_imap_mailbox_init, /* Mailbox entry.  */
  NULL,                  /* Mailer entry.  */
  _mu_imap_folder_init,  /* Folder entry.  */
  NULL, /* No need for a back pointer.  */
  NULL, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};

mu_record_t mu_imap_record = &_imap_record;

#ifdef WITH_TLS
static struct _mu_record _imaps_record =
{
  MU_IMAP_PRIO,
  MU_IMAPS_SCHEME,
  MU_RECORD_DEFAULT,
  MU_URL_SCHEME | MU_URL_CRED | MU_URL_INET | MU_URL_PATH | MU_URL_PARAM,
  MU_URL_HOST,
  _mu_imaps_url_init,     /* url entry.  */
  _mu_imap_mailbox_init,  /* Mailbox entry.  */
  NULL,                /* Mailer entry.  */
  _mu_imap_folder_init,   /* Folder entry.  */
  NULL, /* No need for a back pointer.  */
  NULL, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};
mu_record_t mu_imaps_record = &_imaps_record;
#else
mu_record_t mu_imaps_record = NULL;
#endif /* WITH_TLS */

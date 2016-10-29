/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005, 2007-2012, 2014-2016 Free Software Foundation,
   Inc.

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

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/mailbox.h>
#include <mailutils/radius.h>
#include <mailutils/wordsplit.h>
#include <mailutils/mu_auth.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/io.h>
#include <mailutils/cctype.h>
#include <mailutils/cli.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>

#ifdef ENABLE_RADIUS

#include <radius/radius.h>
#include <radius/debug.h>

static int radius_auth_enabled;

static int MU_User_Name;
static int MU_UID;
static int MU_GID;
static int MU_GECOS;
static int MU_Dir;
static int MU_Shell;
static int MU_Mailbox;

static grad_avp_t *auth_request;
static grad_avp_t *getpwnam_request;
static grad_avp_t *getpwuid_request;

int
get_attribute (int *pattr, char *name)
{
  grad_dict_attr_t *attr = grad_attr_name_to_dict (name);
  if (!attr)
    {
      mu_error (_("RADIUS attribute %s not defined"), name);
      return 1;
    }
  *pattr = attr->value;
  return 0;
}

enum parse_state
  {
    state_lhs,
    state_op,
    state_rhs,
    state_delim
  };

static int
cb_request (void *data, mu_config_value_t *val)
{
  grad_avp_t **plist = data;
  size_t i;
  struct mu_wordsplit ws;
  enum parse_state state;
  grad_locus_t loc;
  char *name;
  struct mu_locus locus;
  
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;

  ws.ws_delim = ",";
  if (mu_wordsplit (val->v.string, &ws,
		    MU_WRDSF_DEFFLAGS|MU_WRDSF_DELIM|MU_WRDSF_RETURN_DELIMS))
    {
      mu_error (_("cannot parse input `%s': %s"), val->v.string,
		mu_wordsplit_strerror (&ws));
      return 1;
    }

  if (mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM, 
		       MU_IOCTL_LOGSTREAM_GET_LOCUS,
		       &locus) == 0)
    {
      loc.file = locus.mu_file;
      loc.line = locus.mu_line;
    }
  else
    {
      loc.file = "<unknown>";
      loc.line = 0;
    }

  for (i = 0, state = state_lhs; i < ws.ws_wordc; i++)
    {
      grad_avp_t *pair;

      switch (state)
	{
	case state_lhs:
	  name = ws.ws_wordv[i];
	  state = state_op;
	  break;

	case state_op:
	  //op = ws.ws_wordv[i];
	  state = state_rhs;
	  break;

	case state_rhs:
	  loc.line = i; /* Just to keep track of error location */
	  pair = grad_create_pair (&loc, name, grad_operator_equal,
				   ws.ws_wordv[i]);
	  if (!pair)
	    {
	      mu_error (_("cannot create radius A/V pair `%s'"), name);
	      return 1;
	    }
	  grad_avl_merge (plist, &pair);
	  state = state_delim;
	  break;

	case state_delim:
	  if (strcmp (ws.ws_wordv[i], ","))
	    {
	      mu_error (_("expected `,' but found `%s'"), ws.ws_wordv[i]);
	      return 1;
	    }
	  state = state_lhs;
	}
    }
  mu_wordsplit_free (&ws);
  
  if (state != state_delim && state != state_delim)
    {
      mu_error (_("malformed radius A/V list"));
      return 1;
    }

  return 0;  
}

static int
cb_config_dir (void *data, mu_config_value_t *val)
{
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  grad_config_dir = grad_estrdup (val->v.string);
  return 0;
}

static struct mu_cfg_param mu_radius_param[] = {
  { "auth", mu_cfg_callback, &auth_request, 0, cb_request,
    N_("Radius request for authorization."),
    N_("request: string") },
  { "getpwnam", mu_cfg_callback, &getpwnam_request, 0, cb_request,
    N_("Radius request for getpwnam."),
    N_("request: string") },
  { "getpwuid", mu_cfg_callback, &getpwuid_request, 0, cb_request,
    N_("Radius request for getpwuid."),
    N_("request: string") },
  { "directory", mu_cfg_callback, NULL, 0, cb_config_dir,
    N_("Set radius configuration directory."),
    N_("dir: string") },
  { NULL }
};

struct mu_cli_capa mu_cli_capa_radius = {
  "radius",
  NULL,
  mu_radius_param
};

/* Assume radius support is needed if any of the above requests is
   defined. Actually, all of them should be, but it is the responsibility
   of init to check for consistency of the configuration */

#define NEED_RADIUS_P() \
  (auth_request || getpwnam_request || getpwuid_request)

static void
mu_grad_logger(int level,
	       const grad_request_t *req,
	       const grad_locus_t *loc,
	       const char *func_name, int en,
	       const char *fmt, va_list ap)
{
  static int mlevel[] = {
    MU_DIAG_EMERG,
    MU_DIAG_ALERT,
    MU_DIAG_CRIT,
    MU_DIAG_ERROR,
    MU_DIAG_WARNING,
    MU_DIAG_NOTICE,
    MU_DIAG_INFO,
    MU_DIAG_DEBUG
  };

  char *pfx = NULL;
  if (loc)
    {
      if (func_name)
	mu_asprintf (&pfx, "%s:%lu:%s: %s",
		     loc->file, (unsigned long) loc->line, func_name, fmt);
      else
	mu_asprintf (&pfx, "%s:%lu: %s",
		     loc->file, (unsigned long) loc->line, fmt);
    }
  mu_diag_voutput (mlevel[level & GRAD_LOG_PRIMASK], pfx ? pfx : fmt, ap);
  if (pfx)
    free (pfx);
}

static void
module_init (void *ptr)
{
  if (!NEED_RADIUS_P ())
    return;

  grad_set_logger (mu_grad_logger);

  grad_path_init ();
  srand (time (NULL) + getpid ());

  if (grad_dict_init ())
    {
      mu_error (_("cannot read radius dictionaries"));
      return;
    }

  /* Check whether mailutils attributes are defined */
  if (get_attribute (&MU_User_Name, "MU-User-Name")
      || get_attribute (&MU_UID, "MU-UID")
      || get_attribute (&MU_GID, "MU-GID")
      || get_attribute (&MU_GECOS, "MU-GECOS")
      || get_attribute (&MU_Dir, "MU-Dir")
      || get_attribute (&MU_Shell, "MU-Shell")
      || get_attribute (&MU_Mailbox, "MU-Mailbox"))
    return;

  radius_auth_enabled = 1;
}

static char *
_expand_query (const char *query, const char *ustr, const char *passwd)
{
  struct mu_wordsplit ws;
  const char *env[2 * 2 + 1];
  char *ret;
  
  env[0] = "user";
  env[1] = (char*) ustr;
  env[2] = "passwd";
  env[3] = (char*) passwd;
  env[4] = NULL;

  ws.ws_env = env;
  if (mu_wordsplit (query, &ws,
		    MU_WRDSF_NOSPLIT | MU_WRDSF_NOCMD |
		    MU_WRDSF_ENV | MU_WRDSF_ENV_KV))
    {
      mu_error (_("cannot expand line `%s': %s"), query,
		mu_wordsplit_strerror (&ws));
      return NULL;
    }
  else if (ws.ws_wordc == 0)
    {
      mu_error (_("expanding %s yields empty string"), query);
      mu_wordsplit_free (&ws);
      return NULL;
    }
  
  ret = grad_emalloc (strlen (ws.ws_wordv[0]) + 1);
  strcpy (ret, ws.ws_wordv[0]);
  mu_wordsplit_free (&ws);
  return ret;
}


static grad_avp_t *
create_request (grad_avp_t *template, const char *ustr, const char *passwd)
{
  grad_avp_t *newp, *p;

  newp = grad_avl_dup (template);
  for (p = newp; p; p = p->next)
    {
      if (p->type == GRAD_TYPE_STRING)
	{
	  char *value = _expand_query (p->avp_strvalue, ustr, passwd);
	  grad_free (p->avp_strvalue);
	  p->avp_strvalue = value;
	  p->avp_strlength = strlen (value);
	}
    }
  return newp;
}



grad_request_t *
send_request (grad_avp_t *pairs, int code,
	      const char *user, const char *passwd)
{
  grad_avp_t *plist = create_request (pairs, user, passwd);
  if (plist)
    {
      grad_server_queue_t *queue = grad_client_create_queue (1, 0, 0);
      grad_request_t *reply = grad_client_send (queue,
						GRAD_PORT_AUTH, code,
						plist);
      grad_client_destroy_queue (queue);
      grad_avl_free (plist);
      return reply;
    }
  return NULL;
}

#define DEFAULT_HOME_PREFIX "/home/"
#define DEFAULT_SHELL "/dev/null"

int
decode_reply (grad_request_t *reply, const char *user_name, char *password,
	      struct mu_auth_data **return_data)
{
  grad_avp_t *p;
  int rc;

  uid_t uid = -1;
  gid_t gid = -1;
  char *gecos = "RADIUS User";
  char *dir = NULL;
  char *shell = NULL;
  char *mailbox = NULL;

  p = grad_avl_find (reply->avlist, MU_User_Name);
  if (p)
    user_name = p->avp_strvalue;

  p = grad_avl_find (reply->avlist, MU_UID);
  if (p)
    uid = p->avp_lvalue;
  else
    {
      mu_error (_("radius server did not return UID for `%s'"),  user_name);
      return -1;
    }

  p = grad_avl_find (reply->avlist, MU_GID);
  if (p)
    gid = p->avp_lvalue;
  else
    {
      mu_error (_("radius server did not return GID for `%s'"),  user_name);
      return -1;
    }

  p = grad_avl_find (reply->avlist, MU_GECOS);
  if (p)
    gecos = p->avp_strvalue;

  p = grad_avl_find (reply->avlist, MU_Dir);
  if (p)
    dir = strdup (p->avp_strvalue);
  else /* Try to provide a reasonable default */
    {
      dir = malloc (sizeof DEFAULT_HOME_PREFIX + strlen (user_name));
      if (!dir) /* FIXME: Error code */
	return 1;
      strcat (strcpy (dir, DEFAULT_HOME_PREFIX), user_name);
    }

  p = grad_avl_find (reply->avlist, MU_Shell);
  if (p)
    shell = p->avp_strvalue;
  else
    shell = DEFAULT_SHELL;

  p = grad_avl_find (reply->avlist, MU_Mailbox);
  if (p)
    mailbox = strdup (p->avp_strvalue);
  else
    {
      rc = mu_construct_user_mailbox_url (&mailbox, user_name);
      if (rc)
	return rc;
    }

  rc = mu_auth_data_alloc (return_data,
			   user_name,
			   password,
			   uid,
			   gid,
			   gecos,
			   dir,
			   shell,
			   mailbox,
			   1);

  free (dir);
  free (mailbox);
  return rc;
}

int
mu_radius_authenticate (struct mu_auth_data **return_data MU_ARG_UNUSED,
			const void *key,
			void *func_data MU_ARG_UNUSED, void *call_data)
{
  int rc;
  grad_request_t *reply;
  const struct mu_auth_data *auth_data = key;

  if (!radius_auth_enabled)
    return ENOSYS;

  if (!auth_request)
    {
      mu_error (_("radius request for auth is not specified"));
      return EINVAL;
    }

  reply = send_request (auth_request, RT_ACCESS_REQUEST,
			auth_data->name, (char*) call_data);
  if (!reply)
    return EAGAIN;

  switch (reply->code) {
  case RT_ACCESS_ACCEPT:
    rc = 0;
    break;

  case RT_ACCESS_CHALLENGE:
    /* Should return another code here? */
  default:
    rc = MU_ERR_AUTH_FAILURE;
  }

  grad_request_free (reply);

  return rc;
}

static int
mu_auth_radius_user_by_name (struct mu_auth_data **return_data,
			     const void *key,
			     void *unused_func_data, void *unused_call_data)
{
  int rc = MU_ERR_AUTH_FAILURE;
  grad_request_t *reply;

  if (!radius_auth_enabled)
    return ENOSYS;

  if (!getpwnam_request)
    {
      mu_error (_("radius request for getpwnam is not specified"));
      return MU_ERR_FAILURE;
    }

  reply = send_request (getpwnam_request, RT_ACCESS_REQUEST, key, NULL);
  if (!reply)
    {
      mu_error (_("radius server did not respond"));
      rc = EAGAIN;
    }
  else
    {
      if (reply->code != RT_ACCESS_ACCEPT)
	mu_error (_("%s: server returned %s"),
		  (char*) key,
		  grad_request_code_to_name (reply->code));
      else
	rc = decode_reply (reply, key, "x", return_data);

      grad_request_free (reply);
    }
  return rc;
}

static int
mu_auth_radius_user_by_uid (struct mu_auth_data **return_data,
			    const void *key,
			    void *func_data, void *call_data)
{
  int rc = MU_ERR_AUTH_FAILURE;
  grad_request_t *reply;
  char uidstr[64];

  if (!radius_auth_enabled)
    return ENOSYS;

  if (!key)
    return EINVAL;

  if (!getpwuid_request)
    {
      mu_error (_("radius request for getpwuid is not specified"));
      return MU_ERR_FAILURE;
    }

  snprintf (uidstr, sizeof (uidstr), "%u", *(uid_t*)key);
  reply = send_request (getpwuid_request, RT_ACCESS_REQUEST, uidstr, NULL);
  if (!reply)
    {
      mu_error (_("radius server did not respond"));
      rc = EAGAIN;
    }
  if (reply->code != RT_ACCESS_ACCEPT)
    {
      mu_error (_("uid %s: server returned %s"), uidstr,
		grad_request_code_to_name (reply->code));
    }
  else
    rc = decode_reply (reply, uidstr, "x", return_data);

  grad_request_free (reply);
  return rc;
}

struct mu_auth_module mu_auth_radius_module = {
  .name = "radius",
  .cfg  = mu_radius_param,
  .commit = module_init,
  .handler = {
    [mu_auth_authenticate] = mu_radius_authenticate,
    [mu_auth_getpwnam] = mu_auth_radius_user_by_name,
    [mu_auth_getpwuid] = mu_auth_radius_user_by_uid
  }
};
#else
struct mu_auth_module mu_auth_radius_module = {
  .name = "radius"
};
#endif


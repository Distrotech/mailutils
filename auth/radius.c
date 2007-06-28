/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005, 2007 Free Software Foundation, Inc.

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
#include <mailutils/argp.h>
#include <mailutils/argcv.h>
#include <mailutils/mu_auth.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>

#ifdef ENABLE_RADIUS

#include <radius/radius.h>

#define ARG_AUTH_REQUEST     256
#define ARG_GETPWNAM_REQUEST 257
#define ARG_GETPWUID_REQUEST 258
#define ARG_RADIUS_DIR       259 

static struct argp_option mu_radius_argp_option[] = {
  { "radius-auth-request", ARG_AUTH_REQUEST, N_("REQUEST"), 0,
    N_("Radius request to authenitcate the user"), 0 },
  { "radius-getpwnam-request", ARG_GETPWNAM_REQUEST, N_("REQUEST"), 0,
    N_("Radius request to retrieve a passwd entry based on username"), 0 },
  { "radius-getpwuid-request", ARG_GETPWUID_REQUEST, N_("REQUEST"), 0,
    N_("Radius request to retrieve a passwd entry based on UID"), 0 },
  { "radius-directory", ARG_RADIUS_DIR, N_("DIR"), 0,
    N_("Set path to the radius configuration directory"), 0 },
  { NULL }
};

static char *auth_request_str;
static grad_avp_t *auth_request;

static char *getpwnam_request_str;
static grad_avp_t *getpwnam_request;

static char *getpwuid_request_str;
static grad_avp_t *getpwuid_request;

/* Assume radius support is needed if any of the above requests is
   defined. Actually, all of them should be, but it is the responsibility
   of init to check for consistency of the configuration */

#define NEED_RADIUS_P() \
  (auth_request_str||getpwnam_request_str||getpwuid_request_str)

static int radius_auth_enabled;

static int MU_User_Name;		
static int MU_UID;		
static int MU_GID;		
static int MU_GECOS;	
static int MU_Dir;		
static int MU_Shell;		
static int MU_Mailbox;  

void
get_attribute (int *pattr, char *name, struct argp_state *state)
{
  grad_dict_attr_t *attr = grad_attr_name_to_dict (name);
  if (!attr)
    argp_error (state, _("Radius attribute %s not defined"), name);
  *pattr = attr->value;
}

enum parse_state
  {
    state_lhs, 
    state_op,  
    state_rhs,
    state_delim
  };

int
parse_pairlist (grad_avp_t **plist, const char *input,
		struct argp_state *argp_state)
{
  int rc;
  int i, argc;
  char **argv;
  enum parse_state state;
  grad_locus_t loc;
  char *name;
  char *op; /* FIXME: It is actually ignored. Should it be? */

  if (!input)
    return 1;
  
  if ((rc = mu_argcv_get (input, ",", NULL, &argc, &argv)))
    argp_error (argp_state, _("Cannot parse input `%s': %s"),
		input, mu_strerror (rc));

  loc.file = "<command line>"; /* FIXME */
  loc.line = 0;
  
  for (i = 0, state = state_lhs; i < argc; i++)
    {
      grad_avp_t *pair;
      
      switch (state)
	{
	case state_lhs:
	  name = argv[i];
	  state = state_op;
	  break;
	  
	case state_op:
	  op = argv[i];
	  state = state_rhs;
	  break;
	  
	case state_rhs:
	  loc.line = i; /* Just to keep track of error location */
	  pair = grad_create_pair (&loc, name, grad_operator_equal, argv[i]);
	  if (!pair)
	    argp_error (argp_state, _("cannot create radius A/V pair `%s'"),
			name);
	  grad_avl_merge (plist, &pair);
	  state = state_delim;
	  break;

	case state_delim:
	  if (strcmp (argv[i], ","))
	    argp_error (argp_state, _("expected `,' but found `%s'"), argv[i]);
	  state = state_lhs;
	}
    }

  if (state != state_delim && state != state_delim)
    argp_error (argp_state, _("malformed radius A/V list"));
  
  mu_argcv_free (argc, argv);
  return 0;
}

static void
init (struct argp_state *state)
{
  grad_path_init ();
  srand (time (NULL) + getpid ());
  
  if (grad_dict_init ())
    argp_error (state, _("Cannot read radius dictionaries"));

  /* Check whether mailutils attributes are defined */
  get_attribute (&MU_User_Name, "MU-User-Name", state);
  get_attribute (&MU_UID, "MU-UID", state);
  get_attribute (&MU_GID, "MU-GID", state);
  get_attribute (&MU_GECOS, "MU-GECOS", state);
  get_attribute (&MU_Dir, "MU-Dir", state);
  get_attribute (&MU_Shell, "MU-Shell", state);
  get_attribute (&MU_Mailbox, "MU-Mailbox", state);  
  
  /* Parse saved requests */
  parse_pairlist (&auth_request, auth_request_str, state);
  parse_pairlist (&getpwnam_request, getpwnam_request_str, state);
  parse_pairlist (&getpwuid_request, getpwuid_request_str, state);

  radius_auth_enabled = 1;
}  

static error_t
mu_radius_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_AUTH_REQUEST:
      auth_request_str = arg;
      break;
      
    case ARG_GETPWNAM_REQUEST:
      getpwnam_request_str = arg;
      break;
      
    case ARG_GETPWUID_REQUEST:
      getpwuid_request_str = arg;
      break;
      
    case ARG_RADIUS_DIR:
      grad_config_dir = grad_estrdup (arg);
      break;
      
    case ARGP_KEY_FINI:
      if (NEED_RADIUS_P())
	init (state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}
      
char *
_expand_query (const char *query, const char *ustr, const char *passwd)
{
  char *p, *q, *res;
  int len;

  if (!query)
    return NULL;
  
  /* Compute resulting query length */
  for (len = 0, p = (char *) query; *p; )
    {
      if (*p == '%')
	{
	  if (p[1] == 'u')
	    {
	      len += ustr ? strlen (ustr) : 2;
	      p += 2;
	    }
	  else if (p[1] == 'p')
	    {
	      len += passwd ? strlen (passwd) : 2;
	      p += 2;
	    }
	  else if (p[1] == '%')
	    {
	      len++;
	      p += 2;
	    }
	  else
	    {
	      len++;
	      p++;
	    }
	}
      else
	{
	  len++;
	  p++;
	}
    }

  res = grad_emalloc (len + 1);
  if (!res)
    return res;

  for (p = (char *) query, q = res; *p; )
    {
      if (*p == '%')
	{
	  switch (*++p)
	    {
	    case 'u':
	      if (ustr)
		{
		  strcpy (q, ustr);
		  q += strlen (q);
		}
	      else
		{
		  *q++ = '%';
		  *q++ = 'u';
		}
	      p++;
	      break;

	    case 'p':
	      if (passwd)
		{
		  strcpy (q, passwd);
		  q += strlen (q);
		}
	      else
		{
		  *q++ = '%';
		  *q++ = 'u';
		}
	      p++;
	      break;
	      
	    case '%':
	      *q++ = *p++;
	      break;
	      
	    default:
	      *q++ = *p++;
	    }
	}
      else
	*q++ = *p++;
    }
  *q = 0;
  return res;
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
      mu_error (_("Radius did not return UID for `%s'"),  user_name);
      return -1;
    }
		
  p = grad_avl_find (reply->avlist, MU_GID);
  if (p)
    gid = p->avp_lvalue;
  else
    {
      mu_error (_("Radius did not return GID for `%s'"),  user_name);
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
      mu_error (_("--radius-auth-request is not specified"));
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
      mu_error (_("--radius-getpwnam-request is not specified"));
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
      mu_error (_("--radius-getpwuid-request is not specified"));
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

struct argp mu_radius_argp = {
  mu_radius_argp_option,
  mu_radius_argp_parser,
};

#else
static int
mu_radius_authenticate (struct mu_auth_data **return_data MU_ARG_UNUSED,
			const void *key,
			void *func_data MU_ARG_UNUSED, void *call_data)
{
  return ENOSYS;
}

static int
mu_auth_radius_user_by_name (struct mu_auth_data **return_data MU_ARG_UNUSED,
			     const void *key MU_ARG_UNUSED,
			     void *func_data MU_ARG_UNUSED,
			     void *call_data MU_ARG_UNUSED)
{
  return ENOSYS;
}

static int
mu_auth_radius_user_by_uid (struct mu_auth_data **return_data,
			    const void *key,
			    void *func_data, void *call_data)
{
  return ENOSYS;
}
#endif

struct mu_auth_module mu_auth_radius_module = {
  "radius",
#ifdef ENABLE_RADIUS
  &mu_radius_argp,
#else
  NULL,
#endif
  mu_radius_authenticate,
  NULL,
  mu_auth_radius_user_by_name,
  NULL,
  mu_auth_radius_user_by_uid,
  NULL
};
    

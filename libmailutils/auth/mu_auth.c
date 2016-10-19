/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2004-2007, 2010-2012, 2014-2016 Free Software
   Foundation, Inc.

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
#ifdef HAVE_SHADOW_H
# include <shadow.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_SECURITY_PAM_APPL_H
# include <security/pam_appl.h>
#endif
#ifdef HAVE_CRYPT_H
# include <crypt.h>
#endif

#include <mailutils/wordsplit.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/mailbox.h>
#include <mailutils/mu_auth.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/debug.h>

#define S(s) ((s) ? (s) : "(none)")
#define DEBUG_AUTH(a)                                                         \
     mu_debug (MU_DEBCAT_AUTH, MU_DEBUG_TRACE,                                \
                      ("source=%s, name=%s, passwd=%s, uid=%lu, gid=%lu, "    \
                       "gecos=%s, dir=%s, shell=%s, mailbox=%s, quota=%lu, "  \
                       "change_uid=%d",                                       \
                       S ((a)->source),                                       \
                       S ((a)->name),                                         \
                       S ((a)->passwd),                                       \
                       (unsigned long) (a)->uid,                              \
                       (unsigned long) (a)->gid,                              \
                       S ((a)->gecos),                                        \
                       S ((a)->dir),                                          \
                       S ((a)->shell),                                        \
                       S ((a)->mailbox),                                      \
                       (unsigned long) (a)->quota,                            \
                       (a)->change_uid))
     
/* memory allocation */
int 
mu_auth_data_alloc (struct mu_auth_data **ptr,
                    const char *name,
                    const char *passwd,
                    uid_t uid,
                    gid_t gid,
                    const char *gecos,
                    const char *dir,
                    const char *shell,
                    const char *mailbox,
                    int change_uid)
{
  size_t size;
  char *p;
  char *tmp_mailbox_name = NULL;
  
  if (!name)
    return EINVAL;
  if (!passwd)
    passwd = "x";
  if (!gecos)
    gecos = "";
  if (!dir)
    dir = "/nonexisting";
  if (!shell)
    shell = "/dev/null";
  if (!mailbox)
    {
      int rc = mu_construct_user_mailbox_url (&tmp_mailbox_name, name);
      if (rc)
	return rc;
      mailbox = tmp_mailbox_name;
    }

  size = sizeof (**ptr) +
         strlen (name) + 1 +
         strlen (passwd) + 1 +
         strlen (gecos) + 1 +
         strlen (dir) + 1 +
         strlen (shell) + 1 +
         strlen (mailbox) + 1;
  
  *ptr = calloc (1, size);
  if (!*ptr)
    return ENOMEM;

  p = (char *)(*ptr + 1);
  
#define COPY(f) \
  (*ptr)->f = p; \
  strcpy (p, f); \
  p += strlen (f) + 1
  
  COPY (name);
  COPY (passwd);
  COPY (gecos);
  COPY (dir);
  COPY (shell);
  COPY (mailbox);
  (*ptr)->uid = uid;
  (*ptr)->gid = gid;
  (*ptr)->change_uid = change_uid;

  free (tmp_mailbox_name);
  return 0;
}

void
mu_auth_data_set_quota (struct mu_auth_data *ptr, mu_off_t q)
{
  ptr->flags |= MU_AF_QUOTA;
  ptr->quota = q;
}

void
mu_auth_data_free (struct mu_auth_data *ptr)
{
  free (ptr);
}

void
mu_auth_data_destroy (struct mu_auth_data **pptr)
{
  if (pptr)
    {
      mu_auth_data_free (*pptr);
      *pptr = NULL;
    }
}

/* Generic functions */

static void
append_auth_module (mu_list_t *pflist, struct mu_auth_module *mod)
{
  if (!*pflist && mu_list_create (pflist))
    return;
  mu_list_append (*pflist, mod);
}

int
mu_auth_runlist (mu_list_t flist,
		 enum mu_auth_mode mode,
                 const void *key, void *data,
		 struct mu_auth_data **return_data)
{
  int status = MU_ERR_AUTH_FAILURE;
  int rc;
  mu_iterator_t itr;

  if (mu_list_get_iterator (flist, &itr) == 0)
    {
      struct mu_auth_module *ep;
      
      for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
           mu_iterator_next (itr))
        {
          mu_iterator_current (itr, (void **)&ep);
	  if (!ep->handler[mode])
	    continue;
          mu_debug (MU_DEBCAT_AUTH, MU_DEBUG_TRACE2,
		    ("Trying %s...", ep->name));
          rc = ep->handler[mode] (return_data, key, ep->data[mode], data);
	  mu_debug (MU_DEBCAT_AUTH, MU_DEBUG_TRACE2, 
		    ("%s yields %d=%s", ep->name, rc, mu_strerror (rc)));
          if (rc == 0)
            {
              if (return_data)
                {
                  struct mu_auth_data *auth = *return_data;
                  if (auth->source == NULL)
                    auth->source = ep->name;
                  DEBUG_AUTH (auth);
                }
              status = rc;
              break;
            }
          else if (rc == ENOSYS && status != 0)
	    /* nothing: do not overwrite last meaningful return code */;
	  else if (status != EAGAIN)
            status = rc;
        }

      mu_iterator_destroy (&itr);
    }
  return status;
}

int
mu_auth_nosupport (struct mu_auth_data **return_data MU_ARG_UNUSED,
                   const void *key MU_ARG_UNUSED,
                   void *func_data MU_ARG_UNUSED,
                   void *call_data MU_ARG_UNUSED)
{
  return ENOSYS;
}

/* II. Authorization: retrieving information about user */

static mu_list_t mu_getpw_modules, selected_getpw_modules;

int
mu_get_auth (struct mu_auth_data **auth, enum mu_auth_key_type type,
             const void *key)
{
  if (!mu_getpw_modules)
    mu_auth_begin_setup ();
  switch (type)
    {
    case mu_auth_key_name:
      mu_debug (MU_DEBCAT_AUTH, MU_DEBUG_TRACE1,
                ("Getting auth info for user %s", (char*) key));
      break;

    case mu_auth_key_uid:
      mu_debug (MU_DEBCAT_AUTH, MU_DEBUG_TRACE1, 
                ("Getting auth info for UID %lu",
		 (unsigned long) *(uid_t*) key));
      break;

    default:
      mu_debug (MU_DEBCAT_AUTH, MU_DEBUG_ERROR, 
                ("Unknown mu_auth_key_type: %d", type));
      return EINVAL;
    }
  return mu_auth_runlist (mu_getpw_modules, type, key, NULL, auth);
}

struct mu_auth_data *
mu_get_auth_by_name (const char *username)
{
  struct mu_auth_data *auth = NULL;
  mu_get_auth (&auth, mu_auth_key_name, username);
  return auth;
}

struct mu_auth_data *
mu_get_auth_by_uid (uid_t uid)
{
  struct mu_auth_data *auth = NULL;
  mu_get_auth (&auth, mu_auth_key_uid, &uid);
  return auth;
}

/* III. Authentication: determining the authenticity of a user */

static mu_list_t mu_auth_modules, selected_auth_modules;

int
mu_authenticate (struct mu_auth_data *auth_data, const char *pass)
{
  if (!auth_data)
    return EINVAL;
  mu_debug (MU_DEBCAT_AUTH, MU_DEBUG_TRACE1, 
            ("mu_authenticate, user %s, source %s", 
             auth_data->name, auth_data->source));
  if (!mu_auth_modules)
    mu_auth_begin_setup ();
  return mu_auth_runlist (mu_auth_modules,
			  mu_auth_authenticate,
			  auth_data, pass, NULL);
}


/* ************************************************************************* */

static mu_list_t module_list;

static void
module_list_init (void)
{
  if (!module_list)
    {
      if (mu_list_create (&module_list))
	abort ();
      mu_list_append (module_list, &mu_auth_generic_module); 
      mu_list_append (module_list, &mu_auth_system_module);
    }
}

void
mu_auth_register_module (struct mu_auth_module *mod)
{
  module_list_init ();
  mu_list_append (module_list, mod);  
}

static struct mu_auth_module *
_locate (const char *name)
{
  struct mu_auth_module *rp = NULL;
  mu_iterator_t itr;

  if (mu_list_get_iterator (module_list, &itr) == 0)
    {
      struct mu_auth_module *p;
      
      for (mu_iterator_first (itr); !rp && !mu_iterator_is_done (itr);
           mu_iterator_next (itr))
        {
          mu_iterator_current (itr, (void **)&p);
          if (strcmp (p->name, name) == 0)
            rp = p;
        }

      mu_iterator_destroy (&itr);
    }
  return rp;
}

static void
_add_module_list (const char *modlist, int (*fun)(const char *name))
{
  struct mu_wordsplit ws;
  int i;

  ws.ws_delim = ":";
  if (mu_wordsplit (modlist, &ws, MU_WRDSF_DEFFLAGS|MU_WRDSF_DELIM))
    {
      mu_error (_("cannot split line `%s': %s"), modlist,
		mu_wordsplit_strerror (&ws));
      exit (1);
    }

  for (i = 0; i < ws.ws_wordc; i++)
    {
      if (fun (ws.ws_wordv[i]))
        {
          /* Historically,auth functions used ENOENT. We support this
             return value for backward compatibility. */
          if (errno == ENOENT || errno == MU_ERR_NOENT)
            mu_error (_("no such module: %s"), ws.ws_wordv[i]);
          else
            mu_error (_("failed to add module %s: %s"),
                      ws.ws_wordv[i], strerror (errno));
          exit (1);
        }
    }
  mu_wordsplit_free (&ws);
}


int
mu_authorization_add_module (const char *name)
{
  struct mu_auth_module *mod = _locate (name);
  
  if (!mod)
    {
      errno = MU_ERR_NOENT;
      return 1;
    }
  append_auth_module (&selected_getpw_modules, mod);
  return 0;
}

void
mu_authorization_add_module_list (const char *modlist)
{
  _add_module_list (modlist, mu_authorization_add_module);
}

void
mu_authorization_clear_list ()
{
  mu_list_destroy (&selected_getpw_modules);
}


int
mu_authentication_add_module (const char *name)
{
  struct mu_auth_module *mod = _locate (name);

  if (!mod)
    {
      errno = MU_ERR_NOENT;
      return 1;
    }
  append_auth_module (&selected_auth_modules, mod);
  return 0;
}

void
mu_authentication_add_module_list (const char *modlist)
{
  _add_module_list (modlist, mu_authentication_add_module);
}

void
mu_authentication_clear_list ()
{
  mu_list_destroy (&selected_auth_modules);
}


/* ************************************************************************ */

/* Setup functions. Note that:
   1) If libmuath is not linked in, then "generic" and "system" modules
      are used unconditionally. This provides compatibility with the
      standard getpw.* functions.
   2) --authentication and --authorization modify only temporary lists,
      which get flushed to the main ones when mu_auth_finish_setup() is
      run. Thus, the default "generic:system" remain in force until
      argp_parse() exits. */
   
void
mu_auth_begin_setup (void)
{
  mu_iterator_t itr;

  module_list_init ();

  if (!mu_auth_modules)
    {
      if (mu_list_get_iterator (module_list, &itr) == 0)
        {
          struct mu_auth_module *mod;
          
          for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
               mu_iterator_next (itr))
            {
              mu_iterator_current (itr, (void **)&mod);
	      append_auth_module (&mu_auth_modules, mod);
            }
          mu_iterator_destroy (&itr);
        }
    }

  if (!mu_getpw_modules)
    {
      if (mu_list_get_iterator (module_list, &itr) == 0)
        {
          struct mu_auth_module *mod;
          
          for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
               mu_iterator_next (itr))
            {
              mu_iterator_current (itr, (void **)&mod);
              append_auth_module (&mu_getpw_modules, mod);
            }
          mu_iterator_destroy (&itr);
        }
    }
}

void
mu_auth_finish_setup (void)
{
  mu_list_destroy (&mu_auth_modules);
  mu_auth_modules = selected_auth_modules;
  selected_auth_modules = NULL;

  mu_list_destroy (&mu_getpw_modules);
  mu_getpw_modules = selected_getpw_modules;
  selected_getpw_modules = NULL;
  
  mu_auth_begin_setup ();
}

struct settings
{
  mu_list_t opts;
  mu_list_t commits;
};
  
static int
do_extend (void *item, void *data)
{
  struct mu_auth_module *mod = item;
  struct settings *set = data;
  
  if (mod->opt)
    mu_list_append (set->opts, mod->opt);
  if (mod->commit)
    mu_list_append (set->commits, mod->commit);
  if (mod->parser || mod->cfg)
    mu_config_root_register_section (NULL, mod->name, NULL,
				     mod->parser, mod->cfg);
  return 0;
}

void
mu_auth_extend_settings (mu_list_t opts, mu_list_t commits)
{
  struct settings s;
  s.opts = opts;
  s.commits = commits;
  mu_list_foreach (module_list, do_extend, &s);
}



/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


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

#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/mailbox.h>
#include <mailutils/argp.h>
#include <mailutils/mu_auth.h>
#include <mailutils/error.h>

/*#define DEBUG(c) do { printf c; printf("\n"); } while (0)*/
#define DEBUG(c)

static void mu_auth_begin_setup __P((void));
static void mu_auth_finish_setup __P((void));

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
  size_t size = sizeof (**ptr) +
                strlen (name) + 1 +
                strlen (passwd) + 1 +
                strlen (gecos) + 1 +
                strlen (dir) + 1 +
                strlen (shell) + 1 +
                strlen (mailbox) + 1;
  char *p;
  
  *ptr = malloc (size);
  if (!*ptr)
    return 1;

  p = (char *)(*ptr + 1);
  
#define COPY(f) \
  (*ptr)->##f = p; \
  strcpy (p, ##f); \
  p += strlen (##f) + 1
  
  COPY (name);
  COPY (passwd);
  COPY (gecos);
  COPY (dir);
  COPY (shell);
  COPY (mailbox);
  (*ptr)->uid = uid;
  (*ptr)->gid = gid;
  (*ptr)->change_uid = change_uid;
  return 0;
}

void
mu_auth_data_free (struct mu_auth_data *ptr)
{
  free (ptr);
}

/* Generic functions */

struct auth_stack_entry {
  char *name;        /* for diagnostics purposes */
  mu_auth_fp fun;
  void *func_data;
};

void
mu_insert_stack_entry (list_t *pflist, struct auth_stack_entry *entry)
{
  if (!*pflist && list_create (pflist))
    return;
  list_append (*pflist, entry);
}

int
mu_auth_runlist (list_t flist, void *return_data, void *key, void *data)
{
  int rc = 1;
  iterator_t itr;

  if (iterator_create (&itr, flist) == 0)
    {
      struct auth_stack_entry *ep;
      
      for (iterator_first (itr); rc && !iterator_is_done (itr);
	   iterator_next (itr))
	{
	  iterator_current (itr, (void **)&ep);
	  DEBUG(("trying %s", ep->name));
	  rc = ep->fun (return_data, key, ep->func_data, data);
	  DEBUG(("Result: %d", rc));
	}

      iterator_destroy (&itr);
    }
  return rc;
}

int
mu_auth_nosupport (void *usused_return_data, void *unused_key,
		   void *unused_func_data, void *unused_call_data)
{
  errno = ENOSYS;
  return 1;
}

/* II. Authorization: retrieving information about user */

static list_t mu_auth_by_name_list, _tmp_auth_by_name_list;
static list_t mu_auth_by_uid_list, _tmp_auth_by_uid_list;

struct mu_auth_data *
mu_get_auth_by_name (const char *username)
{
  struct mu_auth_data *auth = NULL;

  DEBUG(("mu_get_auth_by_name"));
  if (!mu_auth_by_name_list)
    mu_auth_begin_setup ();
  mu_auth_runlist (mu_auth_by_name_list, &auth, username, NULL);
  return auth;
}

struct mu_auth_data *
mu_get_auth_by_uid (uid_t uid)
{
  struct mu_auth_data *auth = NULL;

  DEBUG(("mu_get_auth_by_uid"));
  if (!mu_auth_by_uid_list)
    mu_auth_begin_setup ();
  mu_auth_runlist (mu_auth_by_uid_list, &auth, &uid, NULL);
  return auth;
}

/* III. Authentication: determining the authenticity of a user */

static list_t mu_authenticate_list, _tmp_authenticate_list;

int
mu_authenticate (struct mu_auth_data *auth_data, char *pass)
{
  DEBUG(("mu_authenticate"));
  if (!mu_authenticate_list)
    mu_auth_begin_setup ();
  return mu_auth_runlist (mu_authenticate_list, NULL, auth_data, pass);
}

/* Modules & configuration */


#define ARG_AUTHORIZATION 1
#define ARG_AUTHENTICATION 2

static error_t mu_auth_argp_parser __P((int key, char *arg,
					struct argp_state *state));

/* Options used by programs that use extended authentication mechanisms. */
static struct argp_option mu_auth_argp_option[] = {
  { "authentication", ARG_AUTHENTICATION, "MODLIST", 0,
    "Set list of modules to be used for authentication", 0 },
  { "authorization", ARG_AUTHORIZATION, "MODLIST", 0,
    "Set list of modules to be used for authorization", 0 },
  { NULL,      0, NULL, 0, NULL, 0 }
};

struct argp mu_auth_argp = {
  mu_auth_argp_option,
  mu_auth_argp_parser,
};

struct argp_child mu_auth_argp_child = {
  &mu_auth_argp,
  0,
  "Authentication options",
  0
};

static error_t
mu_auth_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARGP_KEY_FINI:
      mu_auth_finish_setup ();
      break;

      /* authentication */
    case ARG_AUTHORIZATION:
      mu_authorization_add_module_list (arg);
      break;
      
    case ARG_AUTHENTICATION:
      mu_authentication_add_module_list (arg);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


struct _module_handler {
  struct auth_stack_entry authenticate;
  struct auth_stack_entry auth_by_name;
  struct auth_stack_entry auth_by_uid;
};

static list_t module_handler_list;

void
mu_auth_register_module (struct mu_auth_module *mod)
{
  struct _module_handler *entry;
  
  if (mod->argp)
    {
      int i;
      struct argp_child *cp;
      
      if (mu_auth_argp.children)
	{
	  struct argp_child *tmp;
	  for (i = 0; mu_auth_argp.children[i].argp; i++)
	    ;
	  tmp = realloc ((void*) mu_auth_argp.children,
			 (i+1) * sizeof(mu_auth_argp.children[0]));
	  mu_auth_argp.children = tmp;
	}
      else
	{
	  mu_auth_argp.children = calloc (2, sizeof(mu_auth_argp.children[0]));
	  i = 0;
	}
	  
      if (!mu_auth_argp.children)
	{
	  mu_error ("not enough memory");
	  exit (1);
	}
      cp = (struct argp_child *) &mu_auth_argp.children[i];
      cp->argp = mod->argp;
      cp->flags = 0;
      cp->header = NULL;
      cp->group = 0; /* FIXME */
      cp++;
      cp->argp = NULL;
    }

  if (!module_handler_list && list_create (&module_handler_list))
    abort ();

  entry = malloc (sizeof (*entry));
  if (!entry)
    {
      mu_error ("not enough memory");
      exit (1);
    }
  entry->authenticate.name = mod->name;
  entry->authenticate.fun  = mod->authenticate;
  entry->authenticate.func_data = mod->authenticate_data;
  entry->auth_by_name.name = mod->name;
  entry->auth_by_name.fun = mod->auth_by_name;
  entry->auth_by_name.func_data = mod->auth_by_name_data;
  entry->auth_by_uid.name = mod->name;
  entry->auth_by_uid.fun = mod->auth_by_uid;
  entry->auth_by_uid.func_data = mod->auth_by_uid_data;
    
  list_append (module_handler_list, entry);
  
}

static struct _module_handler *
_locate (const char *name)
{
  struct _module_handler *rp = NULL;
  iterator_t itr;

  if (iterator_create (&itr, module_handler_list) == 0)
    {
      struct _module_handler *p;
      
      for (iterator_first (itr); !rp && !iterator_is_done (itr);
	   iterator_next (itr))
	{
	  iterator_current (itr, (void **)&p);
	  if (strcmp (p->authenticate.name, name) == 0)
	    rp = p;
	}

      iterator_destroy (&itr);
    }
  return rp;
}

static void
_add_module_list (const char *modlist, int (*fun)(const char *name))
{
  char *sp, *name;
  
  for (name = strtok_r ((char *)modlist, ":", &sp); name;
       name = strtok_r (NULL, ":", &sp))
    {
      if (fun (name))
	{
	  if (errno == ENOENT)
	    mu_error ("no such module: %s", name);
	  else
	    mu_error ("failed to add module %s: %s", strerror (errno));
	  exit (1);
	}
    }
}

int
mu_authorization_add_module (const char *name)
{
  struct _module_handler *mod = _locate (name);
  
  if (!mod)
    {
      errno = ENOENT;
      return 1;
    }
  mu_insert_stack_entry (&_tmp_auth_by_name_list, &mod->auth_by_name);
  mu_insert_stack_entry (&_tmp_auth_by_uid_list, &mod->auth_by_uid);
  return 0;
}

void
mu_authorization_add_module_list (const char *modlist)
{
  _add_module_list (modlist, mu_authorization_add_module);
}

int
mu_authentication_add_module (const char *name)
{
  struct _module_handler *mod = _locate (name);

  if (!mod)
    {
      errno = ENOENT;
      return 1;
    }
  mu_insert_stack_entry (&_tmp_authenticate_list, &mod->authenticate);
  return 0;
}

void
mu_authentication_add_module_list (const char *modlist)
{
  _add_module_list (modlist, mu_authentication_add_module);
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
mu_auth_begin_setup ()
{
  iterator_t itr;

  if (!module_handler_list)
    {
      mu_auth_register_module (&mu_auth_generic_module); 
      mu_auth_register_module (&mu_auth_system_module);
    }
  
  if (!mu_authenticate_list)
    {
      if (iterator_create (&itr, module_handler_list) == 0)
	{
	  struct _module_handler *mod;
	  
	  for (iterator_first (itr); !iterator_is_done (itr);
	       iterator_next (itr))
	    {
	      iterator_current (itr, (void **)&mod);
	      mu_insert_stack_entry (&mu_authenticate_list,
				     &mod->authenticate);
	    }
	  iterator_destroy (&itr);
	}
    }

  if (!mu_auth_by_name_list)
    {
      if (iterator_create (&itr, module_handler_list) == 0)
	{
	  struct _module_handler *mod;
	  
	  for (iterator_first (itr); !iterator_is_done (itr);
	       iterator_next (itr))
	    {
	      iterator_current (itr, (void **)&mod);
	      mu_insert_stack_entry (&mu_auth_by_name_list, &mod->auth_by_name);
	      mu_insert_stack_entry (&mu_auth_by_uid_list, &mod->auth_by_uid);
	    }
	  iterator_destroy (&itr);
	}
    }
}

void
mu_auth_finish_setup ()
{
  list_destroy (&mu_authenticate_list);
  list_destroy (&mu_auth_by_name_list);
  list_destroy (&mu_auth_by_uid_list);
  
  mu_authenticate_list = _tmp_authenticate_list;
  _tmp_authenticate_list = NULL;
  mu_auth_by_name_list = _tmp_auth_by_name_list;
  _tmp_auth_by_name_list = NULL;
  mu_auth_by_uid_list = _tmp_auth_by_uid_list;
  _tmp_auth_by_uid_list = NULL;
  
  mu_auth_begin_setup ();
}

void
mu_auth_init ()
{
  if (mu_register_capa ("auth", &mu_auth_argp_child))
    {
      mu_error ("INTERNAL ERROR: cannot register argp capability auth");
      abort ();
    }
}


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

typedef int (*mu_auth_fp) __P((void *return_data,
			       void *key,
			       void *func_data,
			       void *call_data));

struct mu_auth_data {
  /* These are from struct passwd */
  char    *name;       /* user name */
  char    *passwd;     /* user password */
  uid_t   uid;         /* user id */
  gid_t   gid;         /* group id */
  char    *gecos;      /* real name */
  char    *dir;        /* home directory */
  char    *shell;      /* shell program */
  /* */
  char    *mailbox;
  int     change_uid;
};

struct mu_auth_module {
  char           *name;
  struct argp    *argp;
  mu_auth_fp     authenticate;
  void           *authenticate_data;
  mu_auth_fp     auth_by_name;
  void           *auth_by_name_data;
  mu_auth_fp     auth_by_uid;
  void           *auth_by_uid_data;
};

extern int mu_auth_runlist __P((list_t flist, void *return_data,
				void *key, void *data));
extern struct mu_auth_data *
mu_get_auth_by_name __P ((const char *username));

extern struct mu_auth_data *
mu_get_auth_by_uid __P((uid_t uid));

extern int
mu_authenticate __P((struct mu_auth_data *auth_data, char *pass));

extern int mu_auth_nosupport __P((void *return_data,
				  void *key,
				  void *func_data,
				  void *call_data));


extern void mu_auth_register_module __P((struct mu_auth_module *mod));

extern void mu_authorization_add_module_list __P((const char *modlist));
extern void mu_authentication_add_module_list __P((const char *modlist));

extern void mu_auth_init __P((void));
extern int mu_auth_data_alloc __P((struct mu_auth_data **ptr,
				   const char *name,
				   const char *passwd,
				   uid_t uid,
				   gid_t gid,
				   const char *gecos,
				   const char *dir,
				   const char *shell,
				   const char *mailbox,
				   int change_uid));
extern void mu_auth_data_free __P((struct mu_auth_data *ptr));


extern struct mu_auth_module mu_auth_system_module;
extern struct mu_auth_module mu_auth_generic_module;
extern struct mu_auth_module mu_auth_pam_module;
extern struct mu_auth_module mu_auth_sql_module;
extern struct mu_auth_module mu_auth_virtual_module;

#define MU_AUTH_REGISTER_ALL_MODULES() do {\
  mu_auth_register_module (&mu_auth_generic_module); \
  mu_auth_register_module (&mu_auth_system_module); \
  mu_auth_register_module (&mu_auth_pam_module);\
  mu_auth_register_module (&mu_auth_sql_module);\
  mu_auth_register_module (&mu_auth_virtual_module);\
  mu_auth_init (); } while (0)

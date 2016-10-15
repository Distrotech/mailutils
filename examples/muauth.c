/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2006-2007, 2010-2012, 2014-2016 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <mailutils/mailutils.h>

enum mu_auth_key_type key_type = mu_auth_key_name;
char *password;

static void
use_uid (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  key_type = mu_auth_key_uid;
}

static void
use_name (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  key_type = mu_auth_key_name;
}

static struct mu_option muauth_options[] = {
  { "password", 'p', "STRING", MU_OPTION_DEFAULT,
    "user password",
    mu_c_string, &password },
  { "uid", 'u', NULL, MU_OPTION_DEFAULT,
    "test getpwuid functions",
    mu_c_string, NULL, use_uid },
  { "name", 'n', NULL, MU_OPTION_DEFAULT,
    "test getpwnam functions",
    mu_c_string, NULL, use_name },
  MU_OPTION_END
}, *options[] = { muauth_options, NULL };

static char *capa[] = {
  "auth",
  "debug",
  NULL
};

static struct mu_cli_setup cli = {
  options,
  NULL,
  "muauth -- test mailutils authentication and authorization schemes",
  "key"
};
           
int
main (int argc, char * argv [])
{
  int rc;
  struct mu_auth_data *auth;
  void *key;
  uid_t uid;
  
  MU_AUTH_REGISTER_ALL_MODULES ();

  mu_cli (argc, argv, &cli, capa, NULL, &argc, &argv);

  if (argc == 0)
    {
      mu_error ("not enough arguments, try `%s --help' for more info",
		mu_program_name);
      return 1;
    }

  if (key_type == mu_auth_key_uid)
    {
      uid = strtoul (argv[0], NULL, 0);
      key = &uid;
    }
  else
    key = argv[0];
  
  rc = mu_get_auth (&auth, key_type, key);
  printf ("mu_get_auth => %d, %s\n", rc, mu_strerror (rc));
  if (rc == 0)
    {
      printf ("user name:  %s\n", auth->name);
      printf ("password:   %s\n", auth->passwd);
      printf ("uid:        %lu\n", (unsigned long) auth->uid);
      printf ("gid:        %lu\n", (unsigned long) auth->gid);
      printf ("gecos:      %s\n", auth->gecos);
      printf ("home:       %s\n", auth->dir);
      printf ("shell:      %s\n", auth->shell);
      printf ("mailbox:    %s\n", auth->mailbox);
      printf ("change_uid: %d\n", auth->change_uid);
	
      rc = mu_authenticate (auth, password);
      printf ("mu_authenticate => %d, %s\n", rc, mu_strerror (rc));
      mu_auth_data_free (auth);
    }
  return rc != 0;
}
      
  

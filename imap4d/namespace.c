/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2007, 2008 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "imap4d.h"

/* FIXME: Rewrite using mu_list_t */

/*FIXME: should be global? */
typedef int (*nsfp_t) (void *closure, int ns, char *path, int delim);

struct namespace_t
{
  int subdir_c;
  char **subdir_v;
};

struct namespace_t namespace[NS_MAX];

/* Note: str is not supposed to be NULL */
int
set_namespace (int i, const char *str)
{
  struct namespace_t *ns = namespace + i;
  int argc;
  char **argv;

  mu_argcv_get (str, ":", NULL, &argc, &argv);
  
  ns->subdir_v = mu_realloc (ns->subdir_v,
			    (ns->subdir_c + argc) * sizeof (ns->subdir_v[0]));
  for (i = 0; i < argc; i++)
    ns->subdir_v[ns->subdir_c++] = mu_normalize_path (argv[i], "/");
  /* Free only argv, not its members */
  free (argv);

  return 0;
}

static char *
printable_pathname (char *str)
{
  if (strncmp (str, homedir, strlen (homedir)) == 0)
    {
      str += strlen (homedir);
      if (str[0] == '/')
	str++;
    }
  return str;
}

static void
print_namespace (int n)
{
  int i;

  if (namespace[n].subdir_c == 0)
    {
      util_send ("NIL");
      return;
    }

  util_send ("(");
  for (i = 0; i < namespace[n].subdir_c; i++)
    {
      char *dir = printable_pathname (namespace[n].subdir_v[i]);
      char *suf = (dir[0] && dir[strlen (dir) - 1] != '/') ? "/" : "";
      util_send ("(\"%s%s\" \"/\")", dir, suf);
    }
  util_send (")");
}

static int
namespace_enumerate (int ns, nsfp_t f, void *closure)
{
  int i, rc;

  for (i = 0; i < namespace[ns].subdir_c; i++)
    if ((rc = (*f) (closure, ns, namespace[ns].subdir_v[i], '/')))
      return rc;
  return 0;
}

static int
namespace_enumerate_all (nsfp_t f, void *closure)
{
  return namespace_enumerate (NS_PRIVATE, f, closure)
    || namespace_enumerate (NS_OTHER, f, closure)
    || namespace_enumerate (NS_SHARED, f, closure);
}

/*
5. NAMESPACE Command

   Arguments: none

   Response:  an untagged NAMESPACE response that contains the prefix
                 and hierarchy delimiter to the server's Personal
                 Namespace(s), Other Users' Namespace(s), and Shared
                 Namespace(s) that the server wishes to expose. The
                 response will contain a NIL for any namespace class
                 that is not available. Namespace_Response_Extensions
                 MAY be included in the response.
                 Namespace_Response_Extensions which are not on the IETF
                 standards track, MUST be prefixed with an "X-".
*/

int
imap4d_namespace (struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  if (imap4d_tokbuf_argc (tok) != 2)
    return util_finish (command, RESP_BAD, "Invalid arguments");

  util_send ("* NAMESPACE ");

  print_namespace (NS_PRIVATE);
  util_send (" ");
  print_namespace (NS_OTHER);
  util_send (" ");
  print_namespace (NS_SHARED);
  util_send ("\r\n");

  return util_finish (command, RESP_OK, "Completed");
}


struct namespace_info
{
  char *name;
  int namelen;
  int ns;
  int exact;
};

static int
check_namespace (void *closure, int ns, char *path, int delim)
{
  struct namespace_info *p = (struct namespace_info *) closure;
  int len = strlen (path);
  if ((len == 0 && p->namelen == len)
      || (len > 0 && strncmp (path, p->name, strlen (path)) == 0))
    {
      p->ns = ns;
      p->exact = len == p->namelen;
      return 1;
    }
  return 0;
}

static int
risky_pattern (const char *pattern, int delim)
{
  for (; *pattern && *pattern != delim; pattern++)
    {
      if (*pattern == '%' || *pattern == '*')
	return 1;
    }
  return 0;
}

char *
namespace_checkfullpath (char *name, const char *pattern, const char *delim)
{
  struct namespace_info info;
  char *p, *path = NULL;
  char *scheme = NULL;

  p = strchr (name, ':');
  if (p)
    {
      size_t size = p - name + 1;
      scheme = malloc (size + 1);
      if (!scheme)
	return NULL;
      memcpy (scheme, name, size);
      scheme[size] = 0;
      name = p + 1;
    }

  path = util_getfullpath (name, delim);
  if (!path)
    {
      free (scheme);
      return path;
    }
  info.name = path;
  info.namelen = strlen (path);
  if (!namespace_enumerate_all (check_namespace, &info))
    {
      free (scheme);
      free (path);
      return NULL;
    }

  if (pattern &&
      info.ns == NS_OTHER && info.exact && risky_pattern (pattern, '/'))
    {
      free (scheme);
      free (path);
      return NULL;
    }

  if (scheme)
    {
      char *pathstr = malloc (strlen (scheme) + strlen (path) + 2);
      if (pathstr)
	{
	  strcpy (pathstr, scheme);
	  strcat (pathstr, path);
	}
      free (scheme);
      free (path);
      path = pathstr;
    }
  return path;
}

char *
namespace_getfullpath (char *name, const char *delim)
{
  if (strcasecmp (name, "INBOX") == 0 && auth_data->change_uid)
    name = strdup (auth_data->mailbox);
  else
    name = namespace_checkfullpath (name, NULL, delim);
  return name;
}

int
namespace_init (char *path)
{
  set_namespace (NS_PRIVATE, path);
  return 0;
}

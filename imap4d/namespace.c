/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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

#include "imap4d.h"

/*FIXME: should be global? */
typedef int (*nsfp_t) __P((void *closure, int ns, char *path, int delim));

struct namespace_t
{
  int subdir_c;
  char **subdir_v;
};

struct namespace_t namespace[NS_MAX];

/* Note: str is not supposed to be NULL */
int
set_namespace(int i, char *str)
{
  char *p, *save;
  struct namespace_t ns;

  /* first, estimate the number of items in subdir_v array: */
  ns.subdir_c = 1;
  for (p = strchr(str, ':'); p && *p; p = strchr(p+1, ':'))
    ns.subdir_c++;

  /* Now allocate the memory */
  ns.subdir_v = calloc(ns.subdir_c, sizeof(ns.subdir_v[0]));

  /* Fill in the array */
  if (ns.subdir_c == 1)
    {
      ns.subdir_v[0] = mu_normalize_path(strdup(str), "/");
    }
  else
    {
      ns.subdir_c = 0;
      for (p = strtok_r(str, ":", &save); p; p = strtok_r(NULL, ":", &save))
	{
	  ns.subdir_v[ns.subdir_c++] = mu_normalize_path(strdup(p), "/");
	}
    }

  namespace[i] = ns;

  return 0;
}

static char *
printable_pathname(char *str)
{
  if (strncmp(str, homedir, strlen(homedir)) == 0)
    {
      str += strlen(homedir);
      if (str[0] == '/')
	str++;
    }
  return str;
}

static void
print_namespace(int n)
{
  int i;

  if (namespace[n].subdir_c == 0)
    {
      util_send("NIL");
      return;
    }

  util_send("(");
  for (i = 0; i < namespace[n].subdir_c; i++)
    {
      util_send("(\"%s\" \"/\")", printable_pathname(namespace[n].subdir_v[i]));
    }
  util_send(")");
}

static int
namespace_enumerate(int ns, nsfp_t f, void *closure)
{
  int i, rc;

  for (i = 0; i < namespace[ns].subdir_c; i++)
    if ((rc = (*f)(closure, ns, namespace[ns].subdir_v[i], '/')))
      return rc;
  return 0;
}

static int
namespace_enumerate_all(nsfp_t f, void *closure)
{
  return namespace_enumerate(NS_PRIVATE, f, closure)
    || namespace_enumerate(NS_OTHER, f, closure)
    || namespace_enumerate(NS_SHARED, f, closure);
}

int
imap4d_namespace(struct imap4d_command *command, char *arg)
{
  if (*arg)
    return util_finish (command, RESP_BAD, "Too many arguments");

  util_send("* NAMESPACE ");

  print_namespace(NS_PRIVATE);
  util_send(" ");
  print_namespace(NS_OTHER);
  util_send(" ");
  print_namespace(NS_SHARED);
  util_send("\r\n");

  return util_finish (command, RESP_OK, "Completed");
}


struct namespace_info
{
  char *name;
  int  namelen;
  int ns;
  int exact;
};

static int
check_namespace (void *closure, int ns, char *path, int delim)
{
  struct namespace_info *p = (struct namespace_info *)closure;
  int len = strlen(path);
  if ((len == 0 && p->namelen == len)
      || (len > 0 && strncmp(path, p->name, strlen(path)) == 0))
    {
      p->ns = ns;
      p->exact = len == p->namelen;
      return 1;
    }
  return 0;
}

static int
risky_pattern(const char *pattern, int delim)
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
  char *path = util_getfullpath (name, delim);

  if (!path)
    return path;

  mu_normalize_path(path, "/");
  
  info.name = path;
  info.namelen = strlen(path);
  if (!namespace_enumerate_all(check_namespace, &info))
    {
      free(path);
      return NULL;
    }

  if (pattern &&
      info.ns == NS_OTHER && info.exact && risky_pattern(pattern, '/'))
    {
      free(path);
      return NULL;
    }

  return path;
}

char *
namespace_getfullpath (char *name, const char *delim)
{
  if (strcasecmp (name, "INBOX") == 0 && !mu_virtual_domain)
    {
      struct passwd *pw = mu_getpwuid (getuid ());
      if (pw)
	{
	  name = malloc (strlen (mu_path_maildir) +
			 strlen (pw->pw_name) + 1);
	  if (!name)
	    {
	      syslog (LOG_ERR, "Not enough memory");
	      return NULL;
	    }
	  sprintf (name, "%s%s", mu_path_maildir, pw->pw_name);
	}
      else
	name = strdup ("/dev/null");
    }
  else
    name = namespace_checkfullpath (name, NULL, delim);
  return name;
}


int
namespace_init(char *path)
{
  set_namespace(NS_PRIVATE, path);
  return 0;
}

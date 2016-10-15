/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2016 Free Software Foundation, Inc.

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
#include <mailutils/mailutils.h>
#include <mailutils/opt.h>

char *file_name;
char *opt_value = "initial";
char *find_value;
int jobs = 0;
int x_option;
int a_option;
int d_option;

struct mu_option group_a[] = {
  MU_OPTION_GROUP("Group A"),
  { "file", 'f', "FILE", MU_OPTION_DEFAULT,
    "set file name",
    mu_c_string, &file_name
  },
  { "optional", 'o', "FILE", MU_OPTION_ARG_OPTIONAL,
    "optional argument",
    mu_c_string, &opt_value },
  { NULL, 'x', NULL, MU_OPTION_DEFAULT,
    "short-only option",
    mu_c_incr, &x_option },
  { "all", 'a', NULL, MU_OPTION_DEFAULT,
    "no arguments to this one",
    mu_c_bool, &a_option },
  MU_OPTION_END
};

struct mu_option group_b[] = {
  MU_OPTION_GROUP("Group B"),
  { "debug", 'd', NULL, MU_OPTION_DEFAULT,
    "another option",
    mu_c_incr, &d_option },
  { "verbose", 'v', NULL, MU_OPTION_ALIAS },
  { "find", 'F', "VALUE", MU_OPTION_DEFAULT,
    "find VALUE",
    mu_c_string, &find_value },
  { "jobs", 'j', "N", MU_OPTION_DEFAULT,
    "sets numeric value",
    mu_c_int, &jobs },
  MU_OPTION_END
};

struct mu_option *optv[] = { group_a, group_b, NULL };

static void
version_hook (struct mu_parseopt *po, mu_stream_t str)
{
  mu_stream_printf (str, "version hook called\n");
}

#define S(s) ((s)?(s):"(null)")

struct parseopt_param
{
  char *name;
  int flag;
  mu_c_type_t type;
  size_t off;
  void (*setfn) (struct parseopt_param *param, char const *val, void *target);
};

static void
set_prog_args (struct parseopt_param *param, char const *str, void *target)
{
  char ***args_ptr = target;
  char **args;
  char *p;
  size_t size, i;

  size = 1;
  for (i = 0; str[i]; i++)
    if (str[i] == '\n')
      size++;

  args = mu_calloc (size + 1, sizeof (args[0]));

  i = 0;
  while (1)
    {
      size_t len = strcspn (str, "|");
      p = mu_alloc (len + 1);
      memcpy (p, str, len);
      p[len] = 0;
      args[i++] = p;
      str += len;
      if (str[0])
	++str;
      else
	break;
    }
  args[i] = NULL;
  
  *args_ptr = args;
}
  
static struct parseopt_param parseopt_param[] = {
  { "MU_PARSEOPT_ARGV0", MU_PARSEOPT_ARGV0, mu_c_void },
  { "MU_PARSEOPT_IGNORE_ERRORS", MU_PARSEOPT_IGNORE_ERRORS, mu_c_void },
  { "MU_PARSEOPT_IN_ORDER", MU_PARSEOPT_IN_ORDER, mu_c_void },
  { "MU_PARSEOPT_NO_STDOPT", MU_PARSEOPT_NO_STDOPT, mu_c_void },
  { "MU_PARSEOPT_NO_ERREXIT", MU_PARSEOPT_NO_ERREXIT, mu_c_void },
  { "MU_PARSEOPT_IMMEDIATE", MU_PARSEOPT_IMMEDIATE, mu_c_void },
  { "MU_PARSEOPT_NO_SORT", MU_PARSEOPT_NO_SORT, mu_c_void },

  { "MU_PARSEOPT_PROG_NAME", MU_PARSEOPT_PROG_NAME,
    mu_c_string, mu_offsetof(struct mu_parseopt, po_prog_name) },
  { "MU_PARSEOPT_PROG_DOC", MU_PARSEOPT_PROG_DOC,
    mu_c_string, mu_offsetof(struct mu_parseopt, po_prog_doc) },
  { "MU_PARSEOPT_PROG_ARGS", MU_PARSEOPT_PROG_ARGS,
    mu_c_string, mu_offsetof(struct mu_parseopt, po_prog_args),
    set_prog_args },
  { "MU_PARSEOPT_BUG_ADDRESS", MU_PARSEOPT_BUG_ADDRESS,
    mu_c_string, mu_offsetof(struct mu_parseopt, po_bug_address) },
  { "MU_PARSEOPT_PACKAGE_NAME", MU_PARSEOPT_PACKAGE_NAME,
    mu_c_string, mu_offsetof(struct mu_parseopt, po_package_name) },
  { "MU_PARSEOPT_PACKAGE_URL", MU_PARSEOPT_PACKAGE_URL,
    mu_c_string, mu_offsetof(struct mu_parseopt, po_package_url) },
  { "MU_PARSEOPT_EXTRA_INFO", MU_PARSEOPT_EXTRA_INFO,
    mu_c_string, mu_offsetof(struct mu_parseopt, po_extra_info) },
  { "MU_PARSEOPT_EXIT_ERROR", MU_PARSEOPT_EXIT_ERROR,
    mu_c_int, mu_offsetof(struct mu_parseopt, po_exit_error) },
  { "MU_PARSEOPT_VERSION_HOOK", MU_PARSEOPT_VERSION_HOOK, mu_c_void },
  { NULL }
};

int
main (int argc, char *argv[])
{
  struct mu_parseopt po;
  int rc;
  int i;
  int flags = MU_PARSEOPT_DEFAULT;
  
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);

  if (getenv ("MU_PARSEOPT_DEFAULT"))
    flags = MU_PARSEOPT_DEFAULT;
  else
    {
      struct parseopt_param *param;
      for (param = parseopt_param; param->name; param++)
	{
	  char *val = getenv (param->name);
	  if (val)
	    {
	      flags |= param->flag;
	      if (param->setfn)
		{
		  param->setfn (param, val, ((char*)&po + param->off));
		}
	      else if (param->type != mu_c_void)
		{
		  char *errmsg;
		  int rc = mu_str_to_c (val, param->type,
					((char*)&po + param->off),
					&errmsg);
		  if (rc)
		    {
		      fprintf (stderr, "envvar %s: %s\n",
			       param->name, errmsg ? errmsg : mu_strerror (rc));
		    }
		}
	    }
	}
      if (flags & MU_PARSEOPT_VERSION_HOOK)
	po.po_version_hook = version_hook;
    }
  
  rc = mu_parseopt (&po, argc, argv, optv, flags);
  printf ("rc=%d\n", rc);
  mu_parseopt_apply (&po);

  argc -= po.po_arg_start;
  argv += po.po_arg_start;

  mu_parseopt_free (&po);

  printf ("file_name=%s\n", S(file_name));
  printf ("opt_value=%s\n", S(opt_value));
  printf ("x_option=%d\n", x_option);
  printf ("a_option=%d\n", a_option);
  printf ("find_value=%s\n", S(find_value));
  printf ("d_option=%d\n", d_option);
  printf ("jobs=%d\n", jobs);
  
  printf ("argv:\n");
  for (i = 0; i < argc; i++)
    {
      printf ("%d: %s\n", i, argv[i]);
    }

  return 0;
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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
#include <stdlib.h>
#include <string.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/folder.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>
#include <mailutils/list.h>
#include <mailutils/url.h>
#include <mailutils/util.h>
#include <mailutils/registrar.h>
#include <mailutils/sys/folder.h>
#include <mailutils/sys/registrar.h>

int sort_option;
int prefix_len;

struct command
{
  char *verb;
  int nargs;
  char *args;
  void (*handler) (mu_folder_t folder, char **argv);
};

static int
compare_response (void const *a, void const *b)
{
  struct mu_list_response const *ra = a;
  struct mu_list_response const *rb = b;

  if (ra->level < rb->level)
    return -1;
  if (ra->level > rb->level)
    return 1;
  return strcmp (ra->name, rb->name);
}

static int
_print_list_entry (void *item, void *data)
{
  struct mu_list_response *resp = item;
  int len = data ? *(int*) data : 0;
  mu_printf ("%c%c %c %4d %s\n",
	     (resp->type & MU_FOLDER_ATTRIBUTE_DIRECTORY) ? 'd' : '-',
	     (resp->type & MU_FOLDER_ATTRIBUTE_FILE) ? 'f' : '-',
	     resp->separator ? resp->separator : ' ',
	     resp->level,
	     resp->name + len);
  return 0;
}

static void
com_list (mu_folder_t folder, char **argv)
{
  int rc;
  mu_list_t list;
  
  mu_printf ("listing '%s' '%s'\n", argv[0], argv[1]);
  rc = mu_folder_list (folder, argv[0], argv[1], 0, &list);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_list", argv[0], rc);
  else
    {
      if (sort_option)
	mu_list_sort (list, compare_response);
      mu_list_foreach (list, _print_list_entry, &prefix_len);
      mu_list_destroy (&list);
    }
}

static void
com_lsub (mu_folder_t folder, char **argv)
{
  int rc;
  mu_list_t list;
  
  mu_printf ("listing subscriptions for '%s' '%s'\n", argv[0], argv[1]);
  rc = mu_folder_lsub (folder, argv[0], argv[1], &list);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_lsub", argv[0], rc);
  else
    {
      if (sort_option)
	mu_list_sort (list, compare_response);
      mu_list_foreach (list, _print_list_entry, NULL);
      mu_list_destroy (&list);
    }
}

static void
com_rename (mu_folder_t folder, char **argv)
{
  int rc;

  mu_printf ("renaming %s to %s\n", argv[0], argv[1]);
  rc = mu_folder_rename (folder, argv[0], argv[1]);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_rename", argv[0], rc);
  else
    mu_printf ("rename successful\n");
}

static void
com_subscribe (mu_folder_t folder, char **argv)
{
  int rc;

  mu_printf ("subscribing %s\n", argv[0]);
  rc = mu_folder_subscribe (folder, argv[0]);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_subscribe", argv[0], rc);
  else
    mu_printf ("subscribe successful\n");
}

static void
com_unsubscribe (mu_folder_t folder, char **argv)
{
  int rc;

  mu_printf ("unsubscribing %s\n", argv[0]);
  rc = mu_folder_unsubscribe (folder, argv[0]);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_unsubscribe", argv[0], rc);
  else
    mu_printf ("unsubscribe successful\n");
}

static struct command comtab[] = {
  { "list", 2, "REF MBX", com_list },
  { "lsub", 2, "REF MBX", com_lsub },
  { "rename", 2, "OLD NEW", com_rename },
  { "subscribe", 1, "MBX", com_subscribe },
  { "unsubscribe", 1, "MBX", com_unsubscribe },
  { NULL }
};

static struct command *
find_command (const char *name)
{
  struct command *cp;
  
  for (cp = comtab; cp->verb; cp++)
    if (strcmp (cp->verb, name) == 0)
      return cp;
  return NULL;
}

static void
usage ()
{
  struct command *cp;
  
  mu_printf (
    "usage: %s [-debug=SPEC] -name=URL [-sort] [-glob] OP ARG [ARG...] [OP ARG [ARG...]...]\n",
    mu_program_name);
  mu_printf ("OPerations and corresponding ARGuments are:\n");
  for (cp = comtab; cp->verb; cp++)
    mu_printf (" %s %s\n", cp->verb, cp->args);
}

static int
_always_is_scheme (mu_record_t record, mu_url_t url, int flags)
{
  return 1;
}

static struct _mu_record test_record =
{
  0,
  "file",
  MU_RECORD_LOCAL,
  MU_URL_SCHEME | MU_URL_PATH,
  MU_URL_PATH,
  mu_url_expand_path, /* URL init.  */
  NULL, /* Mailbox init.  */
  NULL, /* Mailer init.  */
  _mu_fsfolder_init, /* Folder init.  */
  NULL, /* No need for an back pointer.  */
  _always_is_scheme, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};

int
main (int argc, char **argv)
{
  int i;
  int rc;
  mu_folder_t folder;
  char *fname = NULL;
  int glob_option = 0;
  
  mu_set_program_name (argv[0]);
  mu_registrar_record (&test_record);

  if (argc == 1)
    {
      usage ();
      exit (0);
    }

  for (i = 1; i < argc; i++)
    {
      if (strncmp (argv[i], "-debug=", 7) == 0)
	mu_debug_parse_spec (argv[i] + 7);
      else if (strncmp (argv[i], "-name=", 6) == 0)
	fname = argv[i] + 6;
      else if (strcmp (argv[i], "-sort") == 0)
	sort_option = 1;
      else if (strcmp (argv[i], "-glob") == 0)
	glob_option = 1;
      else
	break;
    }

  if (!fname)
    {
      mu_error ("name not specified");
      exit (1);
    }
  
  if (fname[0] != '/')
    {
      char *cwd = mu_getcwd ();
      prefix_len = strlen (cwd);
      if (cwd[prefix_len-1] != '/')
	prefix_len++;
      fname = mu_make_file_name (cwd, fname);
      free (cwd);
    }
  
  rc = mu_folder_create (&folder, fname);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_create", fname, rc);
      return 1;
    }
  rc = mu_folder_open (folder, MU_STREAM_READ);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_open", fname, rc);
      return 1;
    }

  if (glob_option)
    mu_folder_set_match (folder, mu_folder_glob_match);
  
  while (i < argc)
    {
      char *comargs[2];
      struct command *cmd;
      
      cmd = find_command (argv[i]);
      if (!cmd)
	{
	  mu_error ("unknown command %s\n", argv[i]);
	  break;
	}

      i++;
      if (i + cmd->nargs > argc)
	{
	  mu_error ("not enough arguments for %s", cmd->verb);
	  break;
	}
      memcpy (comargs, argv + i, cmd->nargs * sizeof (comargs[0]));
      i += cmd->nargs;

      cmd->handler (folder, comargs);
    }

  mu_folder_close (folder);
  mu_folder_destroy (&folder);

  return 0;
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#include <mailutils/mailutils.h>

struct command
{
  char *verb;
  int nargs;
  char *args;
  void (*handler) (mu_folder_t folder, char **argv);
};

static int
_print_list_entry (void *item, void *data)
{
  struct mu_list_response *resp = item;
  mu_printf ("%c%c %c %4d %s\n",
	     (resp->type & MU_FOLDER_ATTRIBUTE_DIRECTORY) ? 'd' : '-',
	     (resp->type & MU_FOLDER_ATTRIBUTE_FILE) ? 'f' : '-',
	     resp->separator ? resp->separator : ' ',
	     resp->level,
	     resp->name);
  return 0;
}

static void
com_list (mu_folder_t folder, char **argv)
{
  int rc;
  mu_list_t list;
  
  mu_printf ("listing %s %s\n", argv[0], argv[1]);
  rc = mu_folder_list (folder, argv[0], argv[1], 0, &list);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_list", argv[0], rc);
  else
    {
      mu_list_foreach (list, _print_list_entry, NULL);
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
      mu_list_foreach (list, _print_list_entry, NULL);
      mu_list_destroy (&list);
    }
}

static void
com_delete (mu_folder_t folder, char **argv)
{
  int rc;
  
  mu_printf ("deleting %s\n", argv[0]);
  rc = mu_folder_delete (folder, argv[0]);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_lsub", argv[0], rc);
  else
    mu_printf ("delete successful\n");
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
  { "delete", 1, "MBX", com_delete },
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
    "usage: %s [debug=SPEC] url=URL OP ARG [ARG...] [OP ARG [ARG...]...]\n",
    mu_program_name);
  mu_printf ("OPerations and corresponding ARGuments are:\n");
  for (cp = comtab; cp->verb; cp++)
    mu_printf (" %s %s\n", cp->verb, cp->args);
}

int
main (int argc, char **argv)
{
  int i;
  int rc;
  mu_folder_t folder;
  char *fname = NULL;
  
  mu_set_program_name (argv[0]);
  mu_registrar_record (mu_imap_record);
  mu_registrar_record (mu_imaps_record);

  if (argc == 1)
    {
      usage ();
      exit (0);
    }

  for (i = 1; i < argc; i++)
    {
      if (strncmp (argv[i], "debug=", 6) == 0)
	mu_debug_parse_spec (argv[i] + 6);
      else if (strncmp (argv[i], "url=", 4) == 0)
	fname = argv[i] + 4;
      else
	break;
    }

  if (!fname)
    {
      mu_error ("URL not specified");
      exit (1);
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

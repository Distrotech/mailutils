/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include <mh.h>

const char *program_version = "mark (" PACKAGE_STRING ")";
/* TRANSLATORS: Please, preserve the vertical tabulation (^K character)
   in this message */
static char doc[] = N_("GNU MH mark\v\
Use -help to obtain the list of traditional MH options.");
static char args_doc[] = "[msgs...]";

/* GNU options */
static struct argp_option options[] = {
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0,
   N_("Specify folder to operate upon")},
  {"sequence", ARG_SEQUENCE, N_("NAME"), 0,
   N_("Specify sequence name to operate upon")},
  {"add", ARG_ADD, NULL, 0,
   N_("Add messages to the sequence")},
  {"delete", ARG_DELETE, NULL, 0,
   N_("Delete messages from the sequence")},
  {"list", ARG_LIST, NULL, 0,
   N_("List the sequences")},
  {"public", ARG_PUBLIC, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Create public sequence")},
  {"nopublic", ARG_NOPUBLIC, NULL, OPTION_HIDDEN, "" },
  {"zero", ARG_ZERO, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Empty the sequence before adding messages")},
  {"nozero", ARG_NOZERO, NULL, OPTION_HIDDEN, "" },
  {"license", ARG_LICENSE, 0,      0,
   N_("Display software license"), -1},
  {NULL}
};

struct mh_option mh_option[] = {
  {"sequence", 1, },
  {"add", 1, }, 
  {"delete", 1, },
  {"list", 1, },
  {"public", 1, MH_OPT_BOOL },
  {"zero", 1, MH_OPT_BOOL },
  { NULL }
};

static int action;  /* Action to perform */
static int seq_flags = 0; /* Create public sequences;
			     Do not zero the sequence before addition */
static list_t seq_list;  /* List of sequence names to operate upon */

static char *mbox_dir;

static void
add_sequence (char *name)
{
  if (!seq_list && mu_list_create (&seq_list))
    {
      mh_error (_("Cannot create sequence list"));
      exit (1);
    }
  mu_list_append (seq_list, name);
}

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      current_folder = arg;
      break;

    case ARG_SEQUENCE:
      add_sequence (arg);
      break;

    case ARG_ADD:
    case ARG_DELETE:
    case ARG_LIST:
      action = key;
      break;
      
    case ARG_PUBLIC:
      if (is_true (arg))
	seq_flags &= ~SEQ_PRIVATE;
      else
	seq_flags |= SEQ_PRIVATE;
      break;
      
    case ARG_NOPUBLIC:
      seq_flags |= SEQ_PRIVATE;
      break;
      
    case ARG_ZERO:
      if (is_true (arg))
	seq_flags |= SEQ_ZERO;
      else
	seq_flags &= ~SEQ_ZERO;
      break;

    case ARG_NOZERO:
      seq_flags &= ~SEQ_ZERO;
      break;
      
    case ARG_LICENSE:
      mh_license (argp_program_version);
      break;

    default:
      return 1;
    }
  return 0;
}

static int
action_add (void *item, void *data)
{
  mh_seq_add ((char *)item, (mh_msgset_t *)data, seq_flags);
  return 0;
}

static int
action_delete (void *item, void *data)
{
  mh_seq_delete ((char *)item, (mh_msgset_t *)data, seq_flags);
  return 0;
}

static int
action_list (void *item, void *data)
{
  char *name = item;
  char *val;
  
  val = mh_seq_read (name, 0);
  if (val)
    printf ("%s: %s\n", name, val);
  else if ((val = mh_seq_read (name, SEQ_PRIVATE)))
    printf ("%s (%s): %s\n", name, _("private"), val);
  return 0;
}

static int
list_private (char *name, char *value, char *data)
{
  int nlen;
  
  if (memcmp (name, "atr-", 4))
    return 0;
  name += 4;

  nlen = strlen (name) - strlen (mbox_dir);
  if (nlen > 0 && strcmp (name + nlen, mbox_dir) == 0)
    {
      name[nlen-1] = 0;
      printf ("%s (%s): %s\n", name, _("private"), value);
    }
  return 0;
}

static int
list_public (char *name, char *value, char *data)
{
  printf ("%s: %s\n", name, value);
  return 0;
}

static void
list_all ()
{
  mh_global_sequences_iterate (list_public, NULL);
  mh_global_context_iterate (list_private, NULL);
}

int
main (int argc, char **argv)
{
  int index;
  mh_msgset_t msgset;
  mailbox_t mbox;
  url_t url;
  
  mu_init_nls ();
  mu_argp_init (program_version, NULL);
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  mbox = mh_open_folder (current_folder, 0);
  mu_mailbox_get_url (mbox, &url);
  mbox_dir = url_to_string (url);
  if (memcmp (mbox_dir, "mh:", 3) == 0)
    mbox_dir += 3;
	
  argc -= index;
  argv += index;
  mh_msgset_parse (mbox, &msgset, argc, argv, "cur");
  mh_msgset_uids (mbox, &msgset);
  
  switch (action)
    {
    case ARG_ADD:
      if (!seq_list)
	{
	  mh_error (_("--add requires at least one --sequence argument"));
	  return 1;
	}
      mu_list_do (seq_list, action_add, (void *) &msgset);
      mh_global_save_state ();
      break;
      
    case ARG_DELETE:
      if (!seq_list)
	{
	  mh_error (_("--delete requires at least one --sequence argument"));
	  return 1;
	}
      mu_list_do (seq_list, action_delete, (void *) &msgset);
      mh_global_save_state ();
      break;
      
    case ARG_LIST:
      if (!seq_list)
	list_all ();
      else
	mu_list_do (seq_list, action_list, NULL);
      break;
    }

  return 0;
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include <mh.h>

const char *argp_program_version = "mark (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH mark\v"
"Use -help to obtain the list of traditional MH options.");
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
static int mode_public = 1; /* Create public sequences */
static int mode_zero = 1;   /* Zero the sequence before addition */
static list_t seq_list;  /* List of sequence names to operate upon */

static char *mbox_dir;

static void
add_sequence (char *name)
{
  if (!seq_list && list_create (&seq_list))
    {
      mh_error (_("can't create sequence list"));
      exit (1);
    }
  list_append (seq_list, name);
}

static int
opt_handler (int key, char *arg, void *unused)
{
  switch (key)
    {
    case '+':
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
      mode_public = is_true (arg);
      break;
      
    case ARG_NOPUBLIC:
      mode_public = 0;
      break;
      
    case ARG_ZERO:
      mode_zero = is_true (arg);
      break;

    case ARG_NOZERO:
      mode_zero = 0;
      break;
      
    default:
      return 1;
    }
  return 0;
}

static char *
private_sequence_name (char *name)
{
  char *p;
  
  asprintf (&p, "atr-%s-%s", name, mbox_dir);
  return p;
}

static char *
read_sequence (char *name, int public)
{
  char *value;

  if (public)
    value = mh_global_sequences_get (name, NULL);
  else
    {
      char *p = private_sequence_name (name);
      value = mh_global_context_get (p, NULL);
      free (p);
    }
  return value;
}

static void
write_sequence (char *name, char *value, int public)
{
  if (public)
    mh_global_sequences_set (name, value);
  else
    {
      char *p = private_sequence_name (name);
      mh_global_context_set (p, value);
      free (p);
    }
}

static void
delete_sequence (char *name, int public)
{
  write_sequence (name, NULL, public);
}

static int
action_add (void *item, void *data)
{
  char *name = item;
  mh_msgset_t *mset = data;
  char *value = read_sequence (name, mode_public);
  char *new_value, *p;
  char buf[64];
  size_t i, len;
  
  delete_sequence (name, !mode_public);

  if (mode_zero)
    value = NULL;
  
  if (value)
    len = strlen (value);
  else
    len = 0;
  len++;
  for (i = 0; i < mset->count; i++)
    {
      snprintf (buf, sizeof buf, "%lu", (unsigned long) mset->list[i]);
      len += strlen (buf) + 1;
    }

  new_value = xmalloc (len + 1);
  if (value)
    strcpy (new_value, value);
  else
    new_value[0] = 0;
  p = new_value + strlen (new_value);
  *p++ = ' ';
  for (i = 0; i < mset->count; i++)
    {
      p += sprintf (p, "%lu", (unsigned long) mset->list[i]);
      *p++ = ' ';
    }
  *p = 0;
  write_sequence (name, new_value, mode_public);
  return 0;
}

static int
cmp_msgnum (const void *a, const void *b)
{
  size_t *as = a;
  size_t *bs = b;

  if (*as < *bs)
    return -1;
  if (*as > *bs)
    return 1;
  return 0;
}

static int
action_delete (void *item, void *data)
{
  char *name = item;
  mh_msgset_t *mset = data;
  char *value = read_sequence (name, mode_public);
  char *p;
  int argc, i;
  char **argv;
  
  if (!value)
    return 0;

  if (argcv_get (value, "", NULL, &argc, &argv))
    return 0;

  for (i = 0; i < argc; i++)
    {
      char *p;
      size_t num = strtoul (argv[i], &p, 10);

      if (*p)
	continue;

      if (bsearch (&num, mset->list, mset->count, sizeof (mset->list[0]),
		   cmp_msgnum))
	{
	  free (argv[i]);
	  argv[i] = NULL;
	}
    }

  p = value;
  for (i = 0; i < argc; i++)
    {
      if (argv[i])
	{
	  strcpy (p, argv[i]);
	  p += strlen (p);
	  *p++ = ' ';
	}
    }
  *p = 0;
  write_sequence (name, value, mode_public);
  argcv_free (argc, argv);
  
  return 0;
}

static int
action_list (void *item, void *data)
{
  char *name = item;
  char *val;
  
  val = read_sequence (name, 1);
  if (val)
    printf ("%s: %s\n", name, val);
  else if ((val = read_sequence (name, 0)))
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
  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  mbox = mh_open_folder (current_folder, 0);
  mailbox_get_url (mbox, &url);
  mbox_dir = url_to_string (url);
  if (memcmp (mbox_dir, "mh:", 3) == 0)
    mbox_dir += 3;
	
  argc -= index;
  argv += index;
  mh_msgset_parse (mbox, &msgset, argc, argv, "cur");

  switch (action)
    {
    case ARG_ADD:
      if (!seq_list)
	{
	  mh_error (_("--add requires at least one --sequence argument"));
	  return 1;
	}
      list_do (seq_list, action_add, (void *) &msgset);
      mh_global_save_state ();
      break;
      
    case ARG_DELETE:
      if (!seq_list)
	{
	  mh_error (_("--delete requires at least one --sequence argument"));
	  return 1;
	}
      list_do (seq_list, action_delete, (void *) &msgset);
      mh_global_save_state ();
      break;
      
    case ARG_LIST:
      if (!seq_list)
	list_all ();
      else
	list_do (seq_list, action_list, NULL);
      break;
    }

  return 0;
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005-2012, 2014-2016 Free Software Foundation,
   Inc.

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

#include <mh.h>

static char prog_doc[] = N_("Manipulate message sequences");
static char args_doc[] = N_("[MSGLIST]");

enum action_type
  {
    action_undef,
    action_list,
    action_add,
    action_delete
  };
static enum action_type action = action_undef;  /* Action to perform */

static int seq_flags = 0; /* Create public sequences;
			     Do not zero the sequence before addition */
static mu_list_t seq_list;  /* List of sequence names to operate upon */

static const char *mbox_dir;
static int public_option = -1;
static int zero_option = -1;

static void
set_action_add (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  action = action_add;
}

static void
set_action_delete (struct mu_parseopt *po, struct mu_option *opt,
		   char const *arg)
{
  action = action_delete;
}

static void
set_action_list (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  action = action_list;
}

static void
add_sequence (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  if (!seq_list && mu_list_create (&seq_list))
    {
      mu_error (_("cannot create sequence list"));
      exit (1);
    }
  mu_list_append (seq_list, mu_strdup (arg));
}

static struct mu_option options[] = {
  { "sequence", 0, N_("NAME"), MU_OPTION_DEFAULT,
    N_("specify sequence name to operate upon"),
    mu_c_string, NULL, add_sequence },
  { "add",      0, NULL, MU_OPTION_DEFAULT,
    N_("add messages to the sequence"),
    mu_c_string, NULL, set_action_add },
  { "delete",   0, NULL, MU_OPTION_DEFAULT,
    N_("delete messages from the sequence"),
    mu_c_string, NULL, set_action_delete },
  { "list",     0, NULL, MU_OPTION_DEFAULT,
    N_("list the sequences"),
    mu_c_string, NULL, set_action_list },
  { "public",   0, NULL, MU_OPTION_DEFAULT,
    N_("create public sequence"),
    mu_c_bool, &public_option },
  { "zero", 0, NULL, MU_OPTION_DEFAULT,
    N_("empty the sequence before adding messages"),
    mu_c_bool, &zero_option },
  MU_OPTION_END
};


struct mark_closure
{
  mu_mailbox_t mbox;
  mu_msgset_t msgset;
};

static int
do_add (void *item, void *data)
{
  struct mark_closure *clos = data;
  mh_seq_add (clos->mbox, (char *)item, clos->msgset, seq_flags);
  return 0;
}

static int
do_delete (void *item, void *data)
{
  struct mark_closure *clos = data;
  mh_seq_delete (clos->mbox, (char *)item, clos->msgset, seq_flags);
  return 0;
}

static int
do_list (void *item, void *data)
{
  struct mark_closure *clos = data;
  char *name = item;
  const char *val;
  
  val = mh_seq_read (clos->mbox, name, 0);
  if (val)
    printf ("%s: %s\n", name, val);
  else if ((val = mh_seq_read (clos->mbox, name, SEQ_PRIVATE)))
    printf ("%s (%s): %s\n", name, _("private"), val);
  return 0;
}

static int
list_private (const char *name, const char *value, void *data)
{
  int nlen;
  
  if (memcmp (name, "atr-", 4))
    return 0;
  name += 4;

  nlen = strlen (name) - strlen (mbox_dir);
  if (nlen > 0 && strcmp (name + nlen, mbox_dir) == 0)
    {
      nlen--;
      printf ("%*.*s (%s): %s\n", nlen, nlen, name, _("private"), value);
    }
  return 0;
}

static int
list_public (const char *name, const char *value, void *data)
{
  printf ("%s: %s\n", name, value);
  return 0;
}

static void
list_all (mu_mailbox_t mbox)
{
  mh_global_sequences_iterate (mbox, list_public, NULL);
  mh_global_context_iterate (list_private, NULL);
}

int
main (int argc, char **argv)
{
  mu_msgset_t msgset;
  mu_mailbox_t mbox;
  mu_url_t url;
  struct mark_closure clos;
  
  mh_getopt (&argc, &argv, options, MH_GETOPT_DEFAULT_FOLDER,
	     args_doc, prog_doc, NULL);
  if (public_option == -1)
    /* use default */;
  else if (public_option)
    seq_flags &= ~SEQ_PRIVATE;
  else
    seq_flags |= SEQ_PRIVATE;

  if (zero_option == -1)
    /* use default */;
  else if (zero_option)
    seq_flags |= SEQ_ZERO;
  else
    seq_flags &= ~SEQ_ZERO;

  if (action == action_undef)
    {
      /* If no explicit action is given, assume -add if a sequence
	 was specified, and -list otherwise. */
      if (mu_list_is_empty (seq_list))
	action = action_list;
      else
	action = action_add;
    }
  
  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_RDWR);
  mu_mailbox_get_url (mbox, &url);
  mbox_dir = mu_url_to_string (url);
  if (memcmp (mbox_dir, "mh:", 3) == 0)
    mbox_dir += 3;
	
  mh_msgset_parse (&msgset, mbox, argc, argv, "cur");
  
  clos.mbox = mbox;
  clos.msgset = msgset;
  //FIXME: msgset operates on UIDs but there's no way to inform it about that.
  switch (action)
    {
    case action_add:
      if (!seq_list)
	{
	  mu_error (_("--add requires at least one --sequence argument"));
	  return 1;
	}
      mu_list_foreach (seq_list, do_add, (void *) &clos);
      mh_global_save_state ();
      break;
      
    case action_delete:
      if (!seq_list)
	{
	  mu_error (_("--delete requires at least one --sequence argument"));
	  return 1;
	}
      mu_list_foreach (seq_list, do_delete, (void *) &clos);
      mh_global_save_state ();
      break;
      
    case action_list:
      if (!seq_list)
	list_all (mbox);
      else
	mu_list_foreach (seq_list, do_list, &clos);
      break;
    default:
      abort ();
    }
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return 0;
}

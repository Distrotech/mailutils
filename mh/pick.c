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

/* MH pick command */

#include <mh.h>
#include <regex.h>
#include <pick.h>
#include <pick-gram.h>

static char prog_doc[] = N_("Search for messages by content");
static char args_doc[] = N_("[--COMPONENT PATTERN]... [MSGLIST]");

static int public_option = 1;
static int zero_option = 0;

static int list = 1;
static int seq_flags = 0; /* Create public sequences;
			     Do not zero the sequence before addition */
static mu_list_t seq_list;  /* List of sequence names to operate upon */

static mu_list_t lexlist;   /* List of input tokens */

static mu_msgset_t picked_message_uids;

static void
add_component_name (struct mu_parseopt *po, struct mu_option *opt,
		    char const *arg)
{
  pick_add_token (&lexlist, T_COMP, arg);
}

static void
add_component_pattern (struct mu_parseopt *po, struct mu_option *opt,
		       char const *arg)
{
  pick_add_token (&lexlist, T_STRING, arg);
}

static void
add_cflags (struct mu_parseopt *po, struct mu_option *opt,
	    char const *arg)
{
  pick_add_token (&lexlist, T_CFLAGS, arg);
}

static void
add_comp_match (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  pick_add_token (&lexlist, T_COMP, opt->opt_default);
  pick_add_token (&lexlist, T_STRING, arg);
}

static void
add_datefield (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  pick_add_token (&lexlist, T_DATEFIELD, arg);
}

static void
add_after (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  pick_add_token (&lexlist, T_AFTER, NULL);
  pick_add_token (&lexlist, T_STRING, arg);
}

static void
add_before (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  pick_add_token (&lexlist, T_BEFORE, NULL);
  pick_add_token (&lexlist, T_STRING, arg);
}

static void
add_and (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  pick_add_token (&lexlist, T_AND, NULL);
}

static void
add_or (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  pick_add_token (&lexlist, T_OR, NULL);  
}

static void
add_not (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  pick_add_token (&lexlist, T_NOT, NULL);
}

static void
add_lbrace (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  pick_add_token (&lexlist, T_LBRACE, NULL);
}

static void
add_rbrace (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  pick_add_token (&lexlist, T_RBRACE, NULL);
}

static void
add_to_sequence (struct mu_parseopt *po, struct mu_option *opt,
		 char const *arg)
{
  if (!seq_list && mu_list_create (&seq_list))
    {
      mu_error (_("cannot create sequence list"));
      exit (1);
    }
  mu_list_append (seq_list, mu_strdup (arg));
  list = 0;
}

static struct mu_option options[] = {
  MU_OPTION_GROUP (N_("Search patterns")),
  { "component", 0, N_("FIELD"), MU_OPTION_DEFAULT,
    N_("search the named header field"),
    mu_c_string, NULL, add_component_name },
  { "pattern",   0, N_("STRING"), MU_OPTION_DEFAULT,
    N_("set pattern to look for"),
    mu_c_string, NULL, add_component_pattern },
  { "search",    0, NULL, MU_OPTION_ALIAS },
  { "cflags",    0,  N_("STRING"), MU_OPTION_DEFAULT,
    N_("flags controlling the type of regular expressions. "
       "STRING must consist of one or more of the following letters: "
       "B=basic, E=extended, I=ignore case, C=case sensitive. "
       "Default is \"EI\". The flags remain in effect until the next "
       "occurrence of --cflags option. The option must occur right "
       "before --pattern or --component option (or its alias)."),
    mu_c_string, NULL, add_cflags },
  { "cc",        0,  N_("STRING"), MU_OPTION_DEFAULT,
    N_("search for Cc matching STRING"),
    mu_c_string, NULL, add_comp_match, "cc" },
  { "date",      0,  N_("STRING"), MU_OPTION_DEFAULT,
    N_("search for Date matching STRING"),
    mu_c_string, NULL, add_comp_match, "date" },
  { "from",      0,  N_("STRING"), MU_OPTION_DEFAULT,
    N_("search for From matching STRING"),
    mu_c_string, NULL, add_comp_match, "from" },
  { "subject",   0,  N_("STRING"), MU_OPTION_DEFAULT,
    N_("search for Subject matching STRING"),
    mu_c_string, NULL, add_comp_match, "subject" },
  { "to",        0,  N_("STRING"), MU_OPTION_DEFAULT,
    N_("search for To matching STRING"),
    mu_c_string, NULL, add_comp_match, "to" },

  MU_OPTION_GROUP (N_("Date constraint operations")),
  { "datefield", 0, N_("STRING"), MU_OPTION_DEFAULT,
    N_("search in the named date header field (default is `Date:')"),
    mu_c_string, NULL, add_datefield },
  { "after",     0, N_("DATE"), MU_OPTION_DEFAULT,
    N_("match messages after the given date"),
    mu_c_string, NULL, add_after },
  { "before",    0, N_("DATE"), MU_OPTION_DEFAULT,
    N_("match messages before the given date"),
    mu_c_string, NULL, add_before },

  MU_OPTION_GROUP (N_("Logical operations and grouping")),
  { "and",       0, NULL, MU_OPTION_DEFAULT,
    N_("logical AND (default)"),
    mu_c_string, NULL, add_and },
  { "or",        0, NULL, MU_OPTION_DEFAULT,
    N_("logical OR"),
    mu_c_string, NULL, add_or },
  { "not",       0, NULL, MU_OPTION_DEFAULT,
    N_("logical NOT"),
    mu_c_string, NULL, add_not },
  { "lbrace",    0, NULL, MU_OPTION_DEFAULT,
    N_("open group"),
    mu_c_string, NULL, add_lbrace },
  { "(",         0, NULL, MU_OPTION_ALIAS },
  { "rbrace",    0, NULL, MU_OPTION_DEFAULT,
    N_("close group"),
    mu_c_string, NULL, add_rbrace },
  { ")",         0, NULL, MU_OPTION_ALIAS },

  MU_OPTION_GROUP (N_("Operations over the selected messages")),
  { "list",      0, NULL, MU_OPTION_DEFAULT,
    N_("list the numbers of the selected messages (default)"),
    mu_c_bool, &list },
  { "sequence",  0, N_("NAME"), MU_OPTION_DEFAULT,
    N_("add matching messages to the given sequence"),
    mu_c_string, NULL, add_to_sequence },
  { "public",    0, NULL,       MU_OPTION_DEFAULT,
    N_("create public sequence"),
    mu_c_bool, &public_option },
  { "zero",      0, NULL,       MU_OPTION_DEFAULT,
    N_("empty the sequence before adding messages"),
    mu_c_bool, &zero_option },

  MU_OPTION_END
  
};

static int
pick_message (size_t num, mu_message_t msg, void *data)
{
  if (pick_eval (msg))
    {
      mh_message_number (msg, &num);
      if (list)
	printf ("%s\n", mu_umaxtostr (0, num));
      if (picked_message_uids)
	mu_msgset_add_range (picked_message_uids, num, num, MU_MSGSET_UID);
    }
  return 0;
}


static int
action_add (void *item, void *data)
{
  mu_mailbox_t mbox = data;
  mh_seq_add (mbox, (char *)item, picked_message_uids, seq_flags);
  return 0;
}

static void
parse_comp_match (int *pargc, char **argv)
{
  int i, j;
  int argc = *pargc;

  for (i = j = 0; i < argc; i++)
    {
      if (strncmp (argv[i], "--", 2) == 0)
	{
	  if (++i < argc)
	    {
	      pick_add_token (&lexlist, T_COMP, argv[i-1] + 2);
	      pick_add_token (&lexlist, T_STRING, argv[i]);
	    }
	  else
	    {
	      mu_error (_("%s: must be followed by a pattern"), argv[i-1]);
	      exit (1);
	    }
	}
      else
	argv[j++] = argv[i];
    }
  argv[j] = NULL;
  *pargc = j;
}

int
main (int argc, char **argv)
{
  int status;
  mu_mailbox_t mbox;
  mu_msgset_t msgset;

  mh_getopt (&argc, &argv, options, MH_GETOPT_DEFAULT_FOLDER,
	     args_doc, prog_doc, NULL);

  parse_comp_match (&argc, argv);
  
  if (pick_parse (lexlist))
    return 1;

  seq_flags = (public_option ? 0 : SEQ_PRIVATE) | (zero_option ? SEQ_ZERO : 0);
  
  mbox = mh_open_folder (mh_current_folder (),
			 seq_list ? MU_STREAM_RDWR : MU_STREAM_READ);

  if (seq_list)
    mu_msgset_create (&picked_message_uids, NULL, MU_MSGSET_UID);
  
  mh_msgset_parse (&msgset, mbox, argc, argv, "all");
  status = mu_msgset_foreach_message (msgset, pick_message, NULL);

  if (picked_message_uids)
    mu_list_foreach (seq_list, action_add, mbox);

  mh_global_save_state ();
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return status;
}
  

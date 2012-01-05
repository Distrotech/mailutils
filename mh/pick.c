/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005-2012 Free Software Foundation, Inc.

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

static char doc[] = N_("GNU MH pick")"\v"
N_("Compatibility syntax for picking a matching component is:\n\
\n\
  --Component pattern\n\
\n\
where Component is the component name, containing at least one capital\n\
letter or followed by a colon, e.g.:\n\
\n\
  --User-Agent Mailutils\n\
  --user-agent: Mailutils\n\
\n\
Use -help to obtain a list of traditional MH options.");
static char args_doc[] = N_("[MSGLIST]");

/* GNU options */
static struct argp_option options[] = {
#define GRID 10
  { "folder",  ARG_FOLDER, N_("FOLDER"), 0,
    N_("specify folder to operate upon"), GRID },
#undef GRID

#define GRID 20
  { N_("Search patterns"), 0,  NULL, OPTION_DOC,  NULL, GRID },
  { "component", ARG_COMPONENT, N_("FIELD"), 0,
    N_("search the named header field"), GRID+1},
  { "pattern", ARG_PATTERN, N_("STRING"), 0,
    N_("set pattern to look for"), GRID+1 },
  { "search",  0, NULL, OPTION_ALIAS, NULL, GRID+1 },
  { "cflags",  ARG_CFLAGS,  N_("STRING"), 0,
    N_("flags controlling the type of regular expressions. STRING must consist of one or more of the following letters: B=basic, E=extended, I=ignore case, C=case sensitive. Default is \"EI\". The flags remain in effect until the next occurrence of --cflags option. The option must occur right before --pattern or --component option (or its alias)."), GRID+1 },
  { "cc",      ARG_CC,      N_("STRING"), 0,
    N_("same as --component cc --pattern STRING"), GRID+1 },
  { "date",    ARG_DATE,    N_("STRING"), 0,
    N_("same as --component date --pattern STRING"), GRID+1 },
  { "from",    ARG_FROM,    N_("STRING"), 0,
    N_("same as --component from --pattern STRING"), GRID+1 },
  { "subject", ARG_SUBJECT, N_("STRING"), 0,
    N_("same as --component subject --pattern STRING"), GRID+1 },
  { "to",      ARG_TO,      N_("STRING"), 0,
    N_("same as --component to --pattern STRING"), GRID+1 },
#undef GRID

#define GRID 30  
  { N_("Date constraint operations"), 0,  NULL, OPTION_DOC, NULL, GRID },
  { "datefield",ARG_DATEFIELD, N_("STRING"), 0,
    N_("search in the named date header field (default is `Date:')"), GRID+1 },
  { "after",    ARG_AFTER,     N_("DATE"), 0,
    N_("match messages after the given date"), GRID+1 },
  { "before",   ARG_BEFORE,    N_("DATE"), 0,
    N_("match messages before the given date"), GRID+1 },
#undef GRID

#define GRID 40
  { N_("Logical operations and grouping"), 0, NULL, OPTION_DOC, NULL, GRID },
  { "and",     ARG_AND,    NULL, 0,
    N_("logical AND (default)"), GRID+1 },
  { "or",      ARG_OR,     NULL, 0,
    N_("logical OR"), GRID+1 },
  { "not",     ARG_NOT,    NULL, 0,
    N_("logical NOT"), GRID+1 },
  { "lbrace",  ARG_LBRACE, NULL, 0,
    N_("open group"), GRID+1 },
  { "(",       0, NULL, OPTION_ALIAS, NULL, GRID+1 },
  { "rbrace",  ARG_RBRACE, NULL, 0,
    N_("close group"), GRID+1},
  { ")",       0, NULL, OPTION_ALIAS, NULL, GRID+1 },
#undef GRID

#define GRID 50
  { N_("Operations over the selected messages"), 0, NULL, OPTION_DOC, NULL,
    GRID },
  { "list",   ARG_LIST,       N_("BOOL"), OPTION_ARG_OPTIONAL,
    N_("list the numbers of the selected messages (default)"), GRID+1 },
  { "nolist", ARG_NOLIST,     NULL, OPTION_HIDDEN, "", GRID+1 },
  { "sequence", ARG_SEQUENCE,  N_("NAME"), 0,
    N_("add matching messages to the given sequence"), GRID+1 },
  { "public", ARG_PUBLIC, N_("BOOL"), OPTION_ARG_OPTIONAL,
    N_("create public sequence"), GRID+1 },
  { "nopublic", ARG_NOPUBLIC, NULL, OPTION_HIDDEN, "", GRID+1 },
  { "zero",     ARG_ZERO,     N_("BOOL"), OPTION_ARG_OPTIONAL,
    N_("empty the sequence before adding messages"), GRID+1 },
  { "nozero", ARG_NOZERO, NULL, OPTION_HIDDEN, "", GRID+1 },
#undef GRID

  { NULL }
  
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "component", MH_OPT_ARG, "field" },
  { "pattern",   MH_OPT_ARG, "pattern" },
  { "search",    MH_OPT_ARG, "pattern" },
  { "cc",        MH_OPT_ARG, "pattern" },
  { "date",      MH_OPT_ARG, "pattern" },
  { "from",      MH_OPT_ARG, "pattern" },
  { "subject",   MH_OPT_ARG, "pattern" },
  { "to",        MH_OPT_ARG, "pattern" },
  { "datefield", MH_OPT_ARG, "field" },
  { "after",     MH_OPT_ARG, "date" },
  { "before",    MH_OPT_ARG, "date" },
  { "and" },
  { "or" }, 
  { "not" },
  { "lbrace" },
  { "rbrace" },

  { "list",      MH_OPT_BOOL },
  { "sequence",  MH_OPT_ARG, "name" },
  { "public",    MH_OPT_BOOL },
  { "zero",      MH_OPT_BOOL },
  { NULL }
};

static int list = 1;
static int seq_flags = 0; /* Create public sequences;
			     Do not zero the sequence before addition */
static mu_list_t seq_list;  /* List of sequence names to operate upon */

static mu_list_t lexlist;   /* List of input tokens */

static mu_msgset_t picked_message_uids;

static void
add_sequence (char *name)
{
  if (!seq_list && mu_list_create (&seq_list))
    {
      mu_error (_("cannot create sequence list"));
      exit (1);
    }
  mu_list_append (seq_list, name);
}

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      mh_set_current_folder (arg);
      break;

    case ARG_SEQUENCE:
      add_sequence (arg);
      list = 0;
      break;

    case ARG_LIST:
      list = is_true (arg);
      break;

    case ARG_NOLIST:
      list = 0;
      break;

    case ARG_COMPONENT:
      pick_add_token (&lexlist, T_COMP, arg);
      break;
      
    case ARG_PATTERN:
      pick_add_token (&lexlist, T_STRING, arg);
      break;
      
    case ARG_CC:
      pick_add_token (&lexlist, T_COMP, "cc");
      pick_add_token (&lexlist, T_STRING, arg);
      break;
      
    case ARG_DATE:           
      pick_add_token (&lexlist, T_COMP, "date");
      pick_add_token (&lexlist, T_STRING, arg);
      break;
      
    case ARG_FROM:           
      pick_add_token (&lexlist, T_COMP, "from");
      pick_add_token (&lexlist, T_STRING, arg);
      break;
      
    case ARG_SUBJECT:        
      pick_add_token (&lexlist, T_COMP, "subject");
      pick_add_token (&lexlist, T_STRING, arg);
      break;
      
    case ARG_TO:
      pick_add_token (&lexlist, T_COMP, "to");
      pick_add_token (&lexlist, T_STRING, arg);
      break;
      
    case ARG_DATEFIELD:
      pick_add_token (&lexlist, T_DATEFIELD, arg);
      break;
      
    case ARG_AFTER:
      pick_add_token (&lexlist, T_AFTER, NULL);
      pick_add_token (&lexlist, T_STRING, arg);
      break;
      
    case ARG_BEFORE:
      pick_add_token (&lexlist, T_BEFORE, NULL);
      pick_add_token (&lexlist, T_STRING, arg);
      break;
	
    case ARG_AND:
      pick_add_token (&lexlist, T_AND, NULL);
      break;
      
    case ARG_OR:
      pick_add_token (&lexlist, T_OR, NULL);
      break;
      
    case ARG_NOT:
      pick_add_token (&lexlist, T_NOT, NULL);
      break;

    case ARG_LBRACE:
      pick_add_token (&lexlist, T_LBRACE, NULL);
      break;
      
    case ARG_RBRACE:
      pick_add_token (&lexlist, T_RBRACE, NULL);
      break;

    case ARG_CFLAGS:
      pick_add_token (&lexlist, T_CFLAGS, arg);
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
	
    default:
      return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

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

void
pick_help_hook (void)
{
  printf ("\n");
  printf (_("To match another component, use:\n\n"));
  printf (_("  --Component pattern\n\n"));
  printf (_("Note, that the component name must either be capitalized,\n"
	    "or followed by a colon.\n"));
  printf ("\n");
}

/* NOTICE: For compatibility with RAND MH we have to support
   the following command line syntax:

       --COMP STRING

   where `COMP' may be any string and which is equivalent to
   `--component FIELD --pattern STRING'. Obviously this is in conflict
   with the usual GNU long options paradigm which requires that any
   unrecognized long option produce an error. The following
   compromise solution is used:

   The arguments `--COMP STRING' is recognized as a component matching
   request if any of the following conditions is met:

   1. The word `COMP' contains at least one capital letter.  E.g.:

       --User-Agent Mailutils
       
   2. The word `COMP' ends with a colon, e.g.:

       --user-agent: Mailutils
   
   3. Standard input is not connected to a terminal.  This is always
      true when pick is invoked from mh-pick.el Emacs module.
*/

int
main (int argc, char **argv)
{
  int status;
  int index;
  mu_mailbox_t mbox;
  mu_msgset_t msgset;
  int interactive = mh_interactive_mode_p ();

  MU_APP_INIT_NLS ();

  for (index = 1; index < argc; index++)
    {
      int colon = 0, cpos;
      if (argv[index][0] == '-' && argv[index][1] == '-' &&
	  !strchr (argv[index], '=') &&
	  (!interactive ||
	   (colon = argv[index][cpos = strlen (argv[index]) - 1] == ':') ||
	   *mu_str_skip_class_comp (argv[index], MU_CTYPE_UPPER)) &&
	  index + 1 < argc)
	{
	  if (colon)
	    {
	      cpos -= 2;
	      mu_asprintf (&argv[index], "--component=%*.*s", cpos, cpos,
			   argv[index] + 2);
	    }
	  else
	    mu_asprintf (&argv[index], "--component=%s", argv[index] + 2);
	  mu_asprintf (&argv[index + 1], "--pattern=%s", argv[index + 1]);
	  index++;
	}
    }
  mh_help_hook = pick_help_hook;
  mh_argp_init ();
  mh_argp_parse (&argc, &argv, 0, options, mh_option,
		 args_doc, doc, opt_handler, NULL, &index);
  if (pick_parse (lexlist))
    return 1;

  mbox = mh_open_folder (mh_current_folder (),
			 seq_list ? MU_STREAM_RDWR : MU_STREAM_READ);

  argc -= index;
  argv += index;

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
  

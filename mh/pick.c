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

/* MH pick command */

#include <mh.h>
#include <regex.h>

const char *argp_program_version = "pick (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH pick\v"
"Options marked with `*' are not yet implemented.\n"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[messages]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0,
   N_("Specify folder to operate upon"), 0},

  {N_("Specifying search patterns:"), 0,  NULL, OPTION_DOC,  NULL, 0},
  {"component", ARG_COMPONENT, N_("FIELD"), 0,
   N_("Search the named header field"), 1},
  {"pattern", ARG_PATTERN, N_("STRING"), 0,
   N_("A pattern to look for"), 1},
  {"search",  0, NULL, OPTION_ALIAS, NULL, 1},
  {"cc",      ARG_CC,      N_("STRING"), 0,
   N_("Same as --component cc --pattern STRING"), 1},
  {"date",    ARG_DATE,    N_("STRING"), 0,
   N_("Same as --component date --pattern STRING"), 1},
  {"from",    ARG_FROM,    N_("STRING"), 0,
   N_("Same as --component from --pattern STRING"), 1},
  {"subject", ARG_SUBJECT, N_("STRING"), 0,
   N_("Same as --component subject --pattern STRING"), 1},
  {"to",      ARG_TO,      N_("STRING"), 0,
   N_("Same as --component to --pattern STRING"), 1},

  {N_("Date constraint operations:"), 0,  NULL, OPTION_DOC, NULL, 1},
  {"datefield",ARG_DATEFIELD, N_("STRING"), 0,
   N_("Search in the named date header field (default is `Date:')"), 2},
  {"after",    ARG_AFTER,     N_("DATE"), 0,
   N_("Match messages after the given date"), 2},
  {"before",   ARG_BEFORE,    N_("DATE"), 0,
   N_("Match messages before the given date"), 2},

  {N_("Logical operations and grouping:"), 0, NULL, OPTION_DOC, NULL, 2},
  {"and",     ARG_AND,    NULL, 0,
   N_("Logical AND (default)"), 3 },
  {"or",      ARG_OR,     NULL, 0,
   N_("Logical OR"), 3 },
  {"not",     ARG_NOT,    NULL, 0,
   N_("Logical NOT"), 3},
  {"lbrace",  ARG_LBRACE, NULL, 0,
   N_("Open group"), 3},
  {"(",       0, NULL, OPTION_ALIAS, NULL, 3},
  {"rbrace",  ARG_RBRACE, NULL, 0,
   N_("Close group"), 3},
  {")",       0, NULL, OPTION_ALIAS, NULL, 3},

  {N_("Operations over the selected messages:"), 0, NULL, OPTION_DOC, NULL, 3},
  {"list",   ARG_LIST,       N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("List the numbers of the selected messages (default)"), 4},
  {"nolist", ARG_NOLIST,     NULL, OPTION_HIDDEN, "", 4 },
  {"sequence", ARG_SEQUENCE,  N_("NAME"), 0,
   N_("Add matching messages to the given sequence"), 4},
  {"public", ARG_PUBLIC, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Create public sequence"), 4},
  {"nopublic", ARG_NOPUBLIC, NULL, OPTION_HIDDEN, "", 4 },
  {"zero",     ARG_ZERO,     N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Empty the sequence before adding messages"), 4},
  {"nozero", ARG_NOZERO, NULL, OPTION_HIDDEN, "", 4 },
  {NULL},
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"component", 1,  0, "field" },
  {"pattern",   1,  0, "pattern" },
  {"search",    1,  0, "pattern" },
  {"cc",        1,  0, "pattern" },
  {"date",      1,  0, "pattern" },
  {"from",      1,  0, "pattern" },
  {"subject",   1,  0, "pattern" },
  {"to",        1,  0, "pattern" },
  {"datefield", 1,  0, "field" },
  {"after",     1,  0, "date" },
  {"before",    1,  0, "date"},
  {"and",       1,  0, NULL },
  {"or",        1,  0, NULL }, 
  {"not",       1,  0, NULL },
  {"lbrace",    1,  0, NULL },
  {"rbrace",    1,  0, NULL },

  {"list",      1,  MH_OPT_BOOL, },
  {"sequence",  1,  0, NULL },
  {"public",    1,  MH_OPT_BOOL },
  {"zero",      1,  MH_OPT_BOOL },
  {NULL}
};

static int list;
static int mode_public = 1;
static int mode_zero = 0;

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
      /*  add_sequence (arg); */
      break;

    case ARG_LIST:
      list = is_true (arg);
      break;

    case ARG_NOLIST:
      list = 0;
      break;

    case ARG_COMPONENT:      
    case ARG_PATTERN:        
    case ARG_CC:              
    case ARG_DATE:           
    case ARG_FROM:           
    case ARG_SUBJECT:        
    case ARG_TO:             
    case ARG_DATEFIELD:
    case ARG_AFTER:
    case ARG_BEFORE:
    case ARG_AND:
    case ARG_OR:
    case ARG_NOT:
    case ARG_LBRACE:
    case ARG_RBRACE:
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

int
main (int argc, char **argv)
{
  int index;

  mu_init_nls ();
  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);
}
  

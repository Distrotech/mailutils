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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* MH send command */

#include <mh.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>

const char *argp_program_version = "send (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH send\v"
"Options marked with `*' are not yet implemented.\n"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("file [file...]");

#define ARG_ALIAS            257
#define ARG_DRAFT            258
#define ARG_DRAFTFOLDER      259
#define ARG_DRAFTMESSAGE     260
#define ARG_NODRAFTFOLDER    261
#define ARG_FILTER           262
#define ARG_NOFILTER         263
#define ARG_FORMAT           264
#define ARG_NOFORMAT         265
#define ARG_FORWARD          266
#define ARG_NOFORWARD        267
#define ARG_MIME             268
#define ARG_NOMIME           269
#define ARG_MSGID            270
#define ARG_NOMSGID          271
#define ARG_PUSH             272
#define ARG_NOPUSH           273
#define ARG_SPLIT            274
#define ARG_VERBOSE          275
#define ARG_NOVERBOSE        276
#define ARG_WATCH            277
#define ARG_NOWATCH          278
#define ARG_WIDTH            279

/* GNU options */
static struct argp_option options[] = {
  {"alias",         ARG_ALIAS,         N_("FILE"), 0,
   N_("* Specify additional alias file") },
  {"draft",         ARG_DRAFT,         NULL, 0,
   N_("Use prepared draft") },
  {"draftfolder",   ARG_DRAFTFOLDER,   N_("FOLDER"), 0,
   N_("* Specify the folder for message drafts") },
  {"draftmessage",  ARG_DRAFTMESSAGE,  N_("MESSAGE"), 0,
   N_("* Invoke the draftmessage facility") },
  {"nodraftfolder", ARG_NODRAFTFOLDER, NULL, 0,
   N_("* Undo the effect of the last --draftfolder option") },
  {"filter",        ARG_FILTER,        N_("FILE"), 0,
  N_("* Set the filter program to preprocess the body of the message") },
  {"nofilter",      ARG_NOFILTER,      NULL, 0,
   N_("* Undo the effect of the last --filter option") },
  {"format",        ARG_FORMAT,        N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Reformat To: and Cc: addresses") },
  {"noformat",      ARG_NOFORMAT,      NULL, OPTION_HIDDEN, "" },
  {"forward",       ARG_FORWARD,       N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* In case of failure forward the draft along with the failure notice to the sender.") },
  {"noforward",     ARG_NOFORWARD,     NULL, OPTION_HIDDEN, "" },
  {"mime",          ARG_MIME,          N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Use MIME encapsulation") },
  {"nomime",        ARG_NOMIME,        NULL, OPTION_HIDDEN, "" },
  {"msgid",         ARG_MSGID,         N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Add Message-ID: field") },
  {"nomsgid",       ARG_NOMSGID,       NULL, OPTION_HIDDEN, ""},
  {"push",          ARG_PUSH,          N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Run in the backround.") },
  {"nopush",        ARG_NOPUSH,        NULL, OPTION_HIDDEN, "" },
  {"split",         ARG_SPLIT,         N_("SECONDS"), 0,
   N_("* Split the draft into several partial messages and send them with SECONDS interval") },
  {"verbose",       ARG_VERBOSE,       N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Print the transcript of interactions with the transport system") },
  {"noverbose",     ARG_NOVERBOSE,     NULL, OPTION_HIDDEN, "" },
  {"watch",         ARG_WATCH,         N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Monitor the delivery of mail") },
  {"nowatch",       ARG_NOWATCH,       NULL, OPTION_HIDDEN, "" },
  {"width",         ARG_WIDTH,         N_("NUMBER"), 0,
   N_("* Make header fields no longer than NUMBER columns") },
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"alias",         1, 0, "aliasfile" },
  {"draft",         5, 0, NULL },
  {"draftfolder",   6, 0, "folder" },
  {"draftmessage",  6, 0, "message"},
  {"nodraftfolder", 3, 0, NULL },
  {"filter",        2, 0, "filterfile"},
  {"nofilter",      3, 0, NULL },
  {"format",        4, MH_OPT_BOOL, NULL},
  {"forward",       4, MH_OPT_BOOL, NULL},
  {"mime",          2, MH_OPT_BOOL, NULL},
  {"msgid",         2, MH_OPT_BOOL, NULL},
  {"push",          1, MH_OPT_BOOL, NULL},
  {"split",         1, 0, "seconds"},
  {"verbose",       1, MH_OPT_BOOL, NULL},
  {"watch",         2, MH_OPT_BOOL, NULL},
  {"width",         2, 0, NULL },
  { 0 }
};

static int use_draft;            /* Use the prepared draft */
static int reformat_recipients;  /* --format option */
static int forward_notice;       /* Forward the failure notice to the sender,
				    --forward flag */
static int mime_encaps;          /* Use MIME encapsulation */
static int append_msgid;         /* Append Message-ID: header */
static int background;           /* Operate in the background */

static int split_message;            /* Split the message */
static unsigned long split_interval; /* Interval in seconds between sending two
					successive partial messages */

static int verbose;              /* Produce verbose diagnostics */
static int watch;                /* Watch the delivery process */
static unsigned width = 76;      /* Maximum width of header fields */

#define WATCH(c) do {\
  if (watch)\
    watch_printf c;\
} while (0)

static int
opt_handler (int key, char *arg, void *unused)
{
  char *p;
  
  switch (key)
    {
    case ARG_ALIAS:
      return 1;
      
    case ARG_DRAFT:
      use_draft = 1;
      break;
	
    case ARG_DRAFTFOLDER:
    case ARG_DRAFTMESSAGE:
    case ARG_NODRAFTFOLDER:
    case ARG_FILTER:
    case ARG_NOFILTER:
      return 1;
      
    case ARG_FORMAT:
      reformat_recipients = is_true(arg);
      break;
      
    case ARG_NOFORMAT:
      reformat_recipients = 0;
      break;
      
    case ARG_FORWARD:
      forward_notice = is_true(arg);
      break;
      
    case ARG_NOFORWARD:
      forward_notice = 0;
      break;
      
    case ARG_MIME:
      mime_encaps = is_true(arg);
      break;
      
    case ARG_NOMIME:
      mime_encaps = 0;
      break;
      
    case ARG_MSGID:
      append_msgid = is_true(arg);
      break;
      
    case ARG_NOMSGID:
      append_msgid = 0;
      break;
      
    case ARG_PUSH:
      background = is_true(arg);
      break;
      
    case ARG_NOPUSH:
      background = 0;
      break;
      
    case ARG_SPLIT:
      split_message = 1;
      split_interval = strtoul(arg, &p, 10);
      if (*p)
	{
	  mh_error (_("Invalid number"));
	  exit (1);
	}
      break;
      
    case ARG_VERBOSE:
      verbose = is_true(arg);
      break;
      
    case ARG_NOVERBOSE:
      verbose = 0;
      break;
      
    case ARG_WATCH:
      watch = is_true(arg);
      break;
      
    case ARG_NOWATCH:
      watch = 0;
      break;
      
    case ARG_WIDTH:
      width = strtoul(arg, &p, 10);
      if (*p)
	{
	  mh_error (_("Invalid number"));
	  exit (1);
	}
      break;
      
    default:
      return 1;
    }
  return 0;
}

static int
watch_printf (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  va_end (ap);
}

static char *
draft_name()
{
  char *draftfolder = mh_global_profile_get ("Draft-Folder",
					     mu_path_folder_dir);
  return mh_expand_name (draftfolder, "draft", 0);
}

static list_t mbox_list;
static mh_context_t *mts_profile;

int
check_file (char *name)
{
  mailbox_t mbox;

  mbox = mh_open_msg_file (name);
  if (!mbox)
    return 1;
  if (!mbox_list && list_create (&mbox_list))
    {
      mh_error (_("can't create mailbox list"));
      return 1;
    }
  
  return list_append (mbox_list, mbox);
}

void
read_mts_profile ()
{
  char *p;

  p = mh_expand_name (MHLIBDIR, "mtstailor", 0);
  if (!p)
    {
      char *home = mu_get_homedir ();
      if (!home)
	abort (); /* shouldn't happen */
      asprintf (&p, "%s/%s", home, ".mtstailor");
      free (home);
    }
  mts_profile = mh_context_create (p, 1);
  mh_context_read (mts_profile);
}


mailer_t
open_mailer ()
{
  char *url = mh_context_get_value (mts_profile,
				    "url",
				    "sendmail:/usr/sbin/sendmail");
  mailer_t mailer;
  int status;
    
  WATCH(("creating mailer %s", url));
  status = mailer_create (&mailer, url);
  if (status)
    {
      mh_error(_("cannot create mailer \"%s\""), url);
      return NULL;
    }

  if (verbose)
    {
      mu_debug_t debug = NULL;
      mailer_get_debug (mailer, &debug);
      mu_debug_set_level (debug, MU_DEBUG_TRACE | MU_DEBUG_PROT);
    }

  WATCH(("opening mailer %s", url));
  status = mailer_open (mailer, MU_STREAM_RDWR);
  if (status)
    {
      mh_error(_("cannot open mailer \"%s\""), url);
      return NULL;
    }
  return mailer;
}
    
int
_action_send (void *item, void *data)
{
  mailbox_t mbox = item;
  message_t msg;
  int rc;
  mailer_t mailer;

  WATCH(("Getting message"));
  if ((rc = mailbox_get_message (mbox, 1, &msg)))
    {
      mh_error(_("cannot get message: %s"), mu_strerror (rc));
      return 1;
    }

  mailer = open_mailer ();
  if (!mailer)
    return 1;

  WATCH(("Sending message"));
  rc = mailer_send_message (mailer, msg, NULL, NULL);
  if (rc)
    {
      mh_error(_("cannot send message: %s"), mu_strerror (rc));
      return 1;
    }

  WATCH(("Destroying the mailer"));
  mailer_close (mailer);
  mailer_destroy (&mailer);
  
  return 0;
}

int
send (int argc, char **argv)
{
  int i, rc;
  
  for (i = 0; i < argc; i++)
    if (check_file (argv[i]))
      return 1;

  read_mts_profile ();
  
  if (background && daemon (0, 0) < 0)
    {
      mh_error(_("cannot switch to background: %s"), mu_strerror (errno));
      return 1;
    }

  rc = list_do (mbox_list, _action_send, NULL);
  return rc;
}
	  
int
main (int argc, char **argv)
{
  int index;
  
  mu_init_nls ();
  
  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  argc -= index;
  argv += index;

  if (argc == 0)
    {
      struct stat st;
      static char *xargv[2];
      xargv[0] = draft_name();

      if (stat (xargv[0], &st))
	{
	  mh_error(_("cannot stat %s: %s"), xargv[0], mu_strerror (errno));
	  return 1;
	}

      if (!use_draft && !mh_usedraft (xargv[0]))
	exit (0);
      xargv[1] = NULL;
      argv = xargv;
      argc = 1;
    }

  return send(argc, argv);  
}

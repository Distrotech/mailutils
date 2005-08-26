/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002,2003,2005 Free Software Foundation, Inc.

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

/* MH repl command */

#include <mh.h>
#include <sys/stat.h>
#include <unistd.h>

const char *program_version = "reply (" PACKAGE_STRING ")";
/* TRANSLATORS: Please, preserve the vertical tabulation (^K character)
   in this message */
static char doc[] = N_("GNU MH repl\v\
Options marked with `*' are not yet implemented.\n\
Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[+folder] [msg]");


/* GNU options */
static struct argp_option options[] = {
  {"annotate", ARG_ANNOTATE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Add Replied: header to the message being replied to")},
  {"build",   ARG_BUILD, 0, 0,
   N_("Build the draft and quit immediately.")},
  {"draftfolder", ARG_DRAFTFOLDER, N_("FOLDER"), 0,
   N_("Specify the folder for message drafts")},
  {"nodraftfolder", ARG_NODRAFTFOLDER, 0, 0,
   N_("Undo the effect of the last --draftfolder option")},
  {"draftmessage" , ARG_DRAFTMESSAGE, N_("MSG"), 0,
   N_("Invoke the draftmessage facility")},
  {"cc", ARG_CC, "{all|to|cc|me}", 0,
   N_("Specify whom to place on the Cc: list of the reply")},
  {"nocc", ARG_NOCC, "{all|to|cc|me}", 0,
   N_("Specify whom to remove from the Cc: list of the reply")},
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0, N_("Specify folder to operate upon")},
  {"group",  ARG_GROUP,  N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Construct a group or followup reply") },
  {"editor", ARG_EDITOR, N_("PROG"), 0, N_("Set the editor program to use")},
  {"noedit", ARG_NOEDIT, 0, 0, N_("Suppress the initial edit")},
  {"fcc",    ARG_FCC, N_("FOLDER"), 0, N_("* Set the folder to receive Fcc's")},
  {"filter", ARG_FILTER, N_("MHL-FILTER"), 0,
   N_("Set the mhl filter to preprocess the body of the message being replied")},
  {"form",   ARG_FORM, N_("FILE"), 0, N_("Read format from given file")},
  {"format", ARG_FORMAT, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Include a copy of the message being replied. The message will be processed using either the default filter \"mhl.reply\", or the filter specified by --filter option") },
  {"inplace", ARG_INPLACE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Annotate the message in place")},
  {"query", ARG_QUERY, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Query for addresses to place in To: and Cc: lists")},
  {"width", ARG_WIDTH, N_("NUMBER"), 0, N_("Set output width")},
  {"whatnowproc", ARG_WHATNOWPROC, N_("PROG"), 0,
   N_("* Set the replacement for whatnow program")},
  {"nowhatnowproc", ARG_NOWHATNOWPROC, NULL, 0,
   N_("* Ignore whatnowproc variable. Use standard `whatnow' shell instead.")},
  {"use", ARG_USE, N_("BOOL"), OPTION_ARG_OPTIONAL, N_("Use draft file preserved after the last session") },
  {"license", ARG_LICENSE, 0,      0,
   N_("Display software license"), -1},
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"annotate",    1, MH_OPT_BOOL },
  {"build",       1, },
  {"cc",          1, MH_OPT_ARG, "all/to/cc/me"},
  {"nocc",        3, MH_OPT_ARG, "all/to/cc/me"},
  {"form",        4, MH_OPT_ARG, "formatfile"},
  {"width",       1, MH_OPT_ARG, "number"},
  {"draftfolder", 6, MH_OPT_ARG, "folder"},
  {"nodraftfolder", 3 },
  {"draftmessage", 6, },
  {"editor",      1, MH_OPT_ARG, "program"},
  {"noedit",      3, },
  {"fcc",         1, MH_OPT_ARG, "folder"},
  {"filter",      2, MH_OPT_ARG, "program"},
  {"format",      2, MH_OPT_BOOL },
  {"group",       1, MH_OPT_BOOL },
  {"inplace",     1, MH_OPT_BOOL },
  {"query",       1, MH_OPT_BOOL },
  {"whatnowproc", 2, MH_OPT_ARG, "program"},
  {"nowhatnowproc", 3 },
  { 0 }
};

static char default_format_str[] =
"%(lit)%(formataddr %<{reply-to}%?{from}%?{sender}%?{return-path}%>)"
"%<(nonnull)%(void(width))%(putaddr To: )\\n%>"
"%(lit)%<(rcpt to)%(formataddr{to})%>%<(rcpt cc)%(formataddr{cc})%>%<(rcpt me)%(formataddr(me))%>"
"%<(nonnull)%(void(width))%(putaddr cc: )\\n%>"
"%<{fcc}Fcc: %{fcc}\\n%>"
"%<{subject}Subject: Re: %(unre{subject})\\n%>"
"%(lit)%(concat(in_reply_to))%<(nonnull)%(void(width))%(printhdr In-reply-to: )\\n%>"
"%(lit)%(concat(references))%<(nonnull)%(void(width))%(printhdr References: )\\n%>"
"X-Mailer: MH \\(%(package_string)\\)\\n" 
"--------\n";

static char *format_str = NULL;
static mh_format_t format;
static int width = 80;

struct mh_whatnow_env wh_env = { 0 };
static int initial_edit = 1;
static mh_msgset_t msgset;
static mailbox_t mbox;
static int build_only = 0; /* --build flag */
static int query_mode = 0; /* --query flag */
static int use_draft = 0;  /* --use flag */
static char *mhl_filter = NULL; /* --filter flag */

static int
decode_cc_flag (const char *opt, const char *arg)
{
  int rc = mh_decode_rcpt_flag (arg);
  if (rc == RCPT_NONE)
    {
      mh_error (_("%s %s is unknown"), opt, arg);
      exit (1);
    }
  return rc;
}

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  char *s;
  
  switch (key)
    {
    case ARG_BUILD:
      build_only = 1;
      break;
      
    case ARG_CC:
      rcpt_mask |= decode_cc_flag ("-cc", arg);
      break;

    case ARG_NOCC:
      rcpt_mask &= ~decode_cc_flag ("-nocc", arg);
      break;
	
    case ARG_DRAFTFOLDER:
      wh_env.draftfolder = arg;
      break;
      
    case ARG_EDITOR:
      wh_env.editor = arg;
      break;
      
    case ARG_FOLDER: 
      current_folder = arg;
      break;

    case ARG_FORM:
      free (format_str);
      format_str = NULL;
      s = mh_expand_name (MHLIBDIR, arg, 0);
      mh_read_formfile (s, &format_str);
      free (s);
      break;

    case ARG_GROUP:
      if (is_true (arg))
	{
	  s = mh_expand_name (MHLIBDIR, "replgroupcomps", 0);
	  mh_read_formfile (s, &format_str);
	  free (s);
	  rcpt_mask |= RCPT_ALL;
	}
      else
	{
	  free (format_str);
	  format_str = NULL;
	}
      break;
	    
    case ARG_DRAFTMESSAGE:
      wh_env.draftmessage = arg;
      break;

    case ARG_USE:
      use_draft = is_true (arg);
      break;
      
    case ARG_WIDTH:
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  argp_error (state, _("Invalid width"));
	  exit (1);
	}
      break;

    case ARG_NODRAFTFOLDER:
      wh_env.draftfolder = NULL;
      break;

    case ARG_NOEDIT:
      initial_edit = 0;
      break;
      
    case ARG_QUERY:
      query_mode = is_true (arg);
      break;
      
    case ARG_FILTER:
      mhl_filter = arg;
      break;

    case ARG_FORMAT:
      if (is_true (arg))
	{
	  if (!mhl_filter)
	    mhl_filter = mh_expand_name (MHLIBDIR, "mhl.repl", 0);
	}
      else
	mhl_filter = NULL;
      break;
      
    case ARG_ANNOTATE:
    case ARG_FCC:
    case ARG_INPLACE:
    case ARG_WHATNOWPROC:
    case ARG_NOWHATNOWPROC:
      argp_error (state, _("Option is not yet implemented"));
      exit (1);
      
    case ARG_LICENSE:
      mh_license (argp_program_version);
      break;

    case ARGP_KEY_FINI:
      if (!format_str)
	format_str = default_format_str;
      break;
      
    default:
      return 1;
    }
  return 0;
}

void
make_draft (mailbox_t mbox, int disp, struct mh_whatnow_env *wh)
{
  int rc;
  message_t msg;
  struct stat st;
  
  /* First check if the draft exists */
  if (!build_only && stat (wh->draftfile, &st) == 0)
    {
      if (use_draft)
	disp = DISP_USE;
      else 
	{
	  printf (ngettext ("Draft \"%s\" exists (%lu byte).\n",
			    "Draft \"%s\" exists (%lu bytes).\n",
			    st.st_size),
		  wh->draftfile, (unsigned long) st.st_size);
	  disp = mh_disposition (wh->draftfile);
	}
    }

  switch (disp)
    {
    case DISP_QUIT:
      exit (0);

    case DISP_USE:
      unlink (wh->file);
      rename (wh->draftfile, wh->file);
      break;
	  
    case DISP_REPLACE:
      unlink (wh->draftfile);
      break;  
    }

  
  rc = mu_mailbox_get_message (mbox, msgset.list[0], &msg);
  if (rc)
    {
      mh_error (_("Cannot read message %lu: %s"),
		(unsigned long) msgset.list[0],
		mu_strerror (rc));
      exit (1);
    }

  if (disp == DISP_REPLACE)
    {
      stream_t str;
      char *buf;
      
      rc = file_stream_create (&str, wh->file,
			       MU_STREAM_WRITE|MU_STREAM_CREAT);
      if (rc)
	{
	  mh_error (_("Cannot create draft file stream %s: %s"),
		    wh->file, mu_strerror (rc));
	  exit (1);
	}

      if ((rc = stream_open (str)))
	{
	  mh_error (_("Cannot open draft file %s: %s"),
		    wh->file, mu_strerror (rc));
	  exit (1);
	}	  

      mh_format (&format, msg, msgset.list[0], width, &buf);
      stream_sequential_write (str, buf, strlen (buf));

      if (mhl_filter)
	{
	  list_t filter = mhl_format_compile (mhl_filter);
	  if (!filter)
	    exit (1);
	  mhl_format_run (filter, width, 0, 0, msg, str);
	  mhl_format_destroy (&filter);
	}

      stream_destroy (&str, stream_get_owner (str));
      free (buf);
    }

  {
    url_t url;
    size_t num;
    char *msgname, *p;
    
    mu_mailbox_get_url (mbox, &url);
    mh_message_number (msg, &num);
    asprintf (&msgname, "%s/%lu", url_to_string (url), (unsigned long) num);
    p = strchr (msgname, ':');
    if (!p)
      wh->msg = msgname;
    else
      {
	wh->msg = strdup (p+1);
	free (msgname);
      }
  }
}

int
main (int argc, char **argv)
{
  int index;

  /* Native Language Support */
  mu_init_nls ();

  mu_argp_init (program_version, NULL);
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);
  if (mh_format_parse (format_str, &format))
    {
      mh_error (_("Bad format string"));
      exit (1);
    }

  if (!wh_env.draftfolder)
    wh_env.draftfolder = mh_global_profile_get ("Draft-Folder",
						mu_folder_directory ());
  
  mbox = mh_open_folder (current_folder, 0);
  mh_msgset_parse (mbox, &msgset, argc - index, argv + index, "cur");
  if (msgset.count != 1)
    {
      mh_error (_("only one message at a time!"));
      return 1;
    }
  
  wh_env.file = mh_expand_name (wh_env.draftfolder, "reply", 0);
  wh_env.draftfile = mh_expand_name (wh_env.draftfolder, "draft", 0);

  make_draft (mbox, DISP_REPLACE, &wh_env);

  /* Exit immediately if --build is given */
  if (build_only)
    return 0;

  return mh_whatnow (&wh_env, initial_edit);
}

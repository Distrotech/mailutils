/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002-2003, 2005-2012 Free Software Foundation, Inc.

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

/* MH repl command */

#include <mh.h>
#include <mh_format.h>
#include <sys/stat.h>
#include <unistd.h>

static char doc[] = N_("GNU MH repl")"\v"
N_("Options marked with `*' are not yet implemented.\n\
Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[+FOLDER] [MESSAGE]");


/* GNU options */
static struct argp_option options[] = {
  {"annotate", ARG_ANNOTATE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("add Replied: header to the message being replied to")},
  {"build",   ARG_BUILD, 0, 0,
   N_("build the draft and quit immediately")},
  {"draftfolder", ARG_DRAFTFOLDER, N_("FOLDER"), 0,
   N_("specify the folder for message drafts")},
  {"nodraftfolder", ARG_NODRAFTFOLDER, 0, 0,
   N_("undo the effect of the last --draftfolder option")},
  {"draftmessage" , ARG_DRAFTMESSAGE, N_("MSG"), 0,
   N_("invoke the draftmessage facility")},
  {"cc", ARG_CC, "{all|to|cc|me}", 0,
   N_("specify whom to place on the Cc: list of the reply")},
  {"nocc", ARG_NOCC, "{all|to|cc|me}", 0,
   N_("specify whom to remove from the Cc: list of the reply")},
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0, N_("specify folder to operate upon")},
  {"group",  ARG_GROUP,  N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("construct a group or followup reply") },
  {"editor", ARG_EDITOR, N_("PROG"), 0, N_("set the editor program to use")},
  {"noedit", ARG_NOEDIT, 0, 0, N_("suppress the initial edit")},
  {"fcc",    ARG_FCC, N_("FOLDER"), 0, N_("set the folder to receive Fcc's")},
  {"filter", ARG_FILTER, N_("MHL-FILTER"), 0,
   N_("set the mhl filter to preprocess the body of the message being replied")},
  {"form",   ARG_FORM, N_("FILE"), 0, N_("read format from given file")},
  {"format", ARG_FORMAT, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("include a copy of the message being replied; the message will be processed using either the default filter \"mhl.reply\", or the filter specified by --filter option") },
  {"noformat", ARG_NOFORMAT, 0,          OPTION_HIDDEN, "" },
  {"inplace", ARG_INPLACE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* annotate the message in place")},
  {"query", ARG_QUERY, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* query for addresses to place in To: and Cc: lists")},
  {"width", ARG_WIDTH, N_("NUMBER"), 0, N_("set output width")},
  {"whatnowproc", ARG_WHATNOWPROC, N_("PROG"), 0,
   N_("set the replacement for whatnow program")},
  {"nowhatnowproc", ARG_NOWHATNOWPROC, NULL, 0,
   N_("ignore whatnowproc variable; use standard `whatnow' shell instead")},
  {"use", ARG_USE, N_("BOOL"), OPTION_ARG_OPTIONAL, N_("use draft file preserved after the last session") },
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "annotate",    MH_OPT_BOOL },
  { "build" },
  { "cc",          MH_OPT_ARG, "all/to/cc/me"},
  { "nocc",        MH_OPT_ARG, "all/to/cc/me"},
  { "form",        MH_OPT_ARG, "formatfile"},
  { "width",       MH_OPT_ARG, "number"},
  { "draftfolder", MH_OPT_ARG, "folder"},
  { "nodraftfolder" },
  { "draftmessage" },
  { "editor", MH_OPT_ARG, "program"},
  { "noedit" },
  { "fcc",         MH_OPT_ARG, "folder"},
  { "filter",      MH_OPT_ARG, "program"},
  { "format",      MH_OPT_BOOL },
  { "group",       MH_OPT_BOOL },
  { "inplace",     MH_OPT_BOOL },
  { "query",       MH_OPT_BOOL },
  { "whatnowproc", MH_OPT_ARG, "program"},
  { "nowhatnowproc" },
  { NULL }
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
static const char *whatnowproc;
static mu_msgset_t msgset;
static mu_mailbox_t mbox;
static int build_only = 0; /* --build flag */
static int query_mode = 0; /* --query flag */
static int use_draft = 0;  /* --use flag */
static char *mhl_filter = NULL; /* --filter flag */
static int annotate;       /* --annotate flag */
static char *draftmessage = "new";
static const char *draftfolder = NULL;
static mu_opool_t fcc_pool;
static int has_fcc;

static int
decode_cc_flag (const char *opt, const char *arg)
{
  int rc = mh_decode_rcpt_flag (arg);
  if (rc == RCPT_NONE)
    {
      mu_error (_("%s %s is unknown"), opt, arg);
      exit (1);
    }
  return rc;
}

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARGP_KEY_INIT:
      draftfolder = mh_global_profile_get ("Draft-Folder", NULL);
      whatnowproc = mh_global_profile_get ("whatnowproc", NULL);
      break;
      
    case ARG_ANNOTATE:
      annotate = is_true (arg);
      break;
      
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
      draftfolder = arg;
      break;
      
    case ARG_EDITOR:
      wh_env.editor = arg;
      break;
      
    case ARG_FOLDER: 
      mh_set_current_folder (arg);
      break;

    case ARG_FORM:
      free (format_str);
      format_str = NULL;
      if (mh_read_formfile (arg, &format_str))
	exit (1);
      break;

    case ARG_GROUP:
      if (is_true (arg))
	{
	  if (mh_read_formfile ("replgroupcomps", &format_str))
	    exit (1);
	  rcpt_mask |= RCPT_ALL;
	}
      else
	{
	  free (format_str);
	  format_str = NULL;
	}
      break;
	    
    case ARG_DRAFTMESSAGE:
      draftmessage = arg;
      break;

    case ARG_USE:
      use_draft = is_true (arg);
      break;
      
    case ARG_WIDTH:
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  argp_error (state, _("invalid width"));
	  exit (1);
	}
      break;

    case ARG_NODRAFTFOLDER:
      draftfolder = NULL;
      break;

    case ARG_NOEDIT:
      initial_edit = 0;
      break;
      
    case ARG_QUERY:
      query_mode = is_true (arg);
      if (query_mode)
	mh_opt_notimpl_warning ("-query");
      break;
      
    case ARG_FILTER:
      mh_find_file (arg, &mhl_filter);
      break;

    case ARG_FORMAT:
      if (is_true (arg))
	mh_find_file ("mhl.repl", &mhl_filter);
      else
	mhl_filter = NULL;
      break;

    case ARG_NOFORMAT:
      mhl_filter = NULL;
      break;
      
    case ARG_FCC:
      if (!has_fcc)
	{
	  mu_opool_create (&fcc_pool, 1);
	  has_fcc = 1;
	}
      else
	mu_opool_append (fcc_pool, ", ", 2);
      mu_opool_appendz (fcc_pool, arg);
      break;
	  
    case ARG_INPLACE:
      mh_opt_notimpl_warning ("-inplace");
      break;
      
    case ARG_WHATNOWPROC:
      whatnowproc = arg;
      break;

    case ARG_NOWHATNOWPROC:
      whatnowproc = NULL;
      break;

    case ARGP_KEY_FINI:
      if (!format_str)
	format_str = default_format_str;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

void
make_draft (mu_mailbox_t mbox, int disp, struct mh_whatnow_env *wh)
{
  int rc;
  mu_message_t msg;
  struct stat st;
  size_t msgno;
  
  /* First check if the draft exists */
  if (!build_only && stat (wh->draftfile, &st) == 0)
    {
      if (use_draft)
	disp = DISP_USE;
      else 
	{
	  printf (ngettext ("Draft \"%s\" exists (%s byte).\n",
			    "Draft \"%s\" exists (%s bytes).\n",
			    (unsigned long) st.st_size),
		  wh->draftfile, mu_umaxtostr (0, st.st_size));
	  disp = mh_disposition (wh->draftfile);
	}
    }

  switch (disp)
    {
    case DISP_QUIT:
      exit (0);

    case DISP_USE:
      break;
	  
    case DISP_REPLACE:
      unlink (wh->draftfile);
      break;  
    }

  msgno = mh_msgset_first (msgset);
  rc = mu_mailbox_get_message (mbox, msgno, &msg);
  if (rc)
    {
      mu_error (_("cannot read message %s: %s"),
		mu_umaxtostr (0, msgno),
		mu_strerror (rc));
      exit (1);
    }
  if (annotate)
    {
      wh->anno_field = "Replied";
      mu_list_create (&wh->anno_list);
      mu_list_append (wh->anno_list, msg);
    }
  
  if (disp == DISP_REPLACE)
    {
      mu_stream_t str;
      char *buf;
      
      rc = mu_file_stream_create (&str, wh->file,
				  MU_STREAM_WRITE|MU_STREAM_CREAT);
      if (rc)
	{
	  mu_error (_("cannot create draft file stream %s: %s"),
		    wh->file, mu_strerror (rc));
	  exit (1);
	}

      if (has_fcc)
	{
	  mu_message_t tmp_msg;
	  mu_header_t hdr;
	  char *text;
	  
	  mu_message_create_copy (&tmp_msg, msg);
	  mu_message_get_header (tmp_msg, &hdr);
	  text = mu_opool_finish (fcc_pool, NULL);
	  mu_header_set_value (hdr, MU_HEADER_FCC, text, 0);
	  mh_format (&format, tmp_msg, msgno, width, &buf);
	  mu_message_destroy (&tmp_msg, NULL);
	}
      else
	mh_format (&format, msg, msgno, width, &buf);
      
      mu_stream_write (str, buf, strlen (buf), NULL);

      if (mhl_filter)
	{
	  mu_list_t filter = mhl_format_compile (mhl_filter);
	  if (!filter)
	    exit (1);
	  mhl_format_run (filter, width, 0, 0, msg, str);
	  mhl_format_destroy (&filter);
	}

      mu_stream_destroy (&str);
      free (buf);
    }

  {
    mu_url_t url;
    size_t num;
    char *msgname, *p;
    
    mu_mailbox_get_url (mbox, &url);
    mh_message_number (msg, &num);
    msgname = mh_safe_make_file_name (mu_url_to_string (url), 
                                      mu_umaxtostr (0, num));
    p = strchr (msgname, ':');
    if (!p)
      wh->msg = msgname;
    else
      {
	wh->msg = mu_strdup (p+1);
	free (msgname);
      }
  }
}

int
main (int argc, char **argv)
{
  int index, rc;

  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mh_argp_init ();

  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);
  if (mh_format_parse (format_str, &format))
    {
      mu_error (_("Bad format string"));
      exit (1);
    }

  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_RDWR);
  mh_msgset_parse (&msgset, mbox, argc - index, argv + index, "cur");
  if (!mh_msgset_single_message (msgset))
    {
      mu_error (_("only one message at a time!"));
      return 1;
    }
  
  if (build_only)
    wh_env.file = mh_expand_name (draftfolder, "reply", 0);
  else if (draftfolder)
    {
      if (mh_draft_message (draftfolder, draftmessage, &wh_env.file))
	return 1;
    }
  else
    wh_env.file = mh_expand_name (draftfolder, "draft", 0);
  wh_env.draftfile = wh_env.file;

  make_draft (mbox, DISP_REPLACE, &wh_env);

  /* Exit immediately if --build is given */
  if (build_only)
    return 0;

  rc = mh_whatnowproc (&wh_env, initial_edit, whatnowproc);
  
  mu_mailbox_sync (mbox);
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return rc;
}

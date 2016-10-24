/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002-2003, 2005-2012, 2014-2016 Free Software
   Foundation, Inc.

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

static char prog_doc[] = N_("Reply to a message");
static char args_doc[] = N_("[MESSAGE]");

static char *format_str = NULL;
static mh_format_t format;
static int width = 80;

struct mh_whatnow_env wh_env = { 0 };
static int initial_edit = 1;
static const char *whatnowproc;
static mu_msgset_t msgset;
static mu_mailbox_t mbox;
static int build_only = 0; /* -build flag */
static int nowhatnowproc = 0; /* -nowhatnowproc */
static int use_draft = 0;  /* -use flag */
static char *mhl_filter = NULL; /* -filter flag */
static int annotate;       /* -annotate flag */
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

static void
set_cc (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  rcpt_mask |= decode_cc_flag ("-cc", arg);
}

static void
clr_cc (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  rcpt_mask &= ~decode_cc_flag ("-nocc", arg);
}

static void
set_fcc (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  if (!has_fcc)
    {
      mu_opool_create (&fcc_pool, 1);
      has_fcc = 1;
    }
  else
    mu_opool_append (fcc_pool, ", ", 2);
  mu_opool_appendz (fcc_pool, arg);
}

static void
set_whatnowproc (struct mu_parseopt *po, struct mu_option *opt,
		 char const *arg)
{
  whatnowproc = mu_strdup (arg);
  nowhatnowproc = 0;
}

static void
set_group (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  if (strcmp (arg, "1") == 0)
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
}

static struct mu_option options[] = {
  { "annotate", 0, NULL, MU_OPTION_DEFAULT,
    N_("add Replied: header to the message being replied to"),
    mu_c_bool, &annotate },
  { "build",    0, NULL, MU_OPTION_DEFAULT,
    N_("build the draft and quit immediately"),
    mu_c_bool, &build_only },
  { "draftfolder", 0, N_("FOLDER"), MU_OPTION_DEFAULT,
    N_("specify the folder for message drafts"),
    mu_c_string, &draftfolder },
  { "nodraftfolder", 0, NULL, MU_OPTION_DEFAULT,
    N_("undo the effect of the last --draftfolder option"),
    mu_c_string, &draftfolder, mh_opt_clear_string },
  { "draftmessage" , 0, N_("MSG"), MU_OPTION_DEFAULT,
    N_("invoke the draftmessage facility"),
    mu_c_string, &draftmessage },
  { "cc", 0, "{all|to|cc|me}", 0,
    N_("specify whom to place on the Cc: list of the reply"),
    mu_c_string, NULL, set_cc },
  { "nocc", 0, "{all|to|cc|me}", 0,
    N_("specify whom to remove from the Cc: list of the reply"),
    mu_c_string, NULL, clr_cc },
  { "group", 0,  NULL, MU_OPTION_DEFAULT,
    N_("construct a group or followup reply"),
    mu_c_string, NULL, set_group },
  { "editor", 0, N_("PROG"), MU_OPTION_DEFAULT,
    N_("set the editor program to use"),
    mu_c_string, &wh_env.editor },
  { "noedit", 0, NULL, MU_OPTION_DEFAULT,
    N_("suppress the initial edit"),
    mu_c_int, &initial_edit, NULL, "0" },
  { "fcc",    0, N_("FOLDER"), MU_OPTION_DEFAULT,
    N_("set the folder to receive Fcc's"),
    mu_c_string, NULL, set_fcc },
  { "filter", 0, N_("MHL-FILTER"), MU_OPTION_DEFAULT,
    N_("set the mhl filter to preprocess the body of the message being replied"),
    mu_c_string, &mhl_filter, mh_opt_find_file },
  { "form",   0, N_("FILE"), MU_OPTION_DEFAULT,
    N_("read format from given file") ,
    mu_c_string, &format_str, mh_opt_read_formfile },
  { "format", 0, NULL, MU_OPTION_DEFAULT,
    N_("include a copy of the message being replied; the message will be processed using either the default filter \"mhl.reply\", or the filter specified by --filter option"),
    mu_c_string, &mhl_filter, mh_opt_find_file, "mhl.repl" },
  { "noformat", 0, NULL,   MU_OPTION_DEFAULT,
    N_("cancels the effect of the recent -format option"),
    mu_c_string, &mhl_filter, mh_opt_clear_string },
  { "inplace",  0, NULL,   MU_OPTION_HIDDEN,
    N_("annotate the message in place"),
    mu_c_string, NULL, mh_opt_notimpl_warning },
  { "query",    0, NULL,   MU_OPTION_HIDDEN,
    N_("query for addresses to place in To: and Cc: lists"),
    mu_c_string, NULL, mh_opt_notimpl_warning },
  { "width",    0, N_("NUMBER"), MU_OPTION_DEFAULT,
    N_("set output width"),
    mu_c_int, &width },
  { "whatnowproc", 0, N_("PROG"), MU_OPTION_DEFAULT,
    N_("set the replacement for whatnow program"),
    mu_c_string, NULL, set_whatnowproc },
  { "nowhatnowproc", 0, NULL, MU_OPTION_DEFAULT,
    N_("don't run whatnowproc"),
    mu_c_int, &nowhatnowproc, NULL, "1" },
  { "use", 0, NULL, MU_OPTION_DEFAULT,
    N_("use draft file preserved after the last session"),
    mu_c_bool, &use_draft },
  
  MU_OPTION_END
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
  int rc;

  draftfolder = mh_global_profile_get ("Draft-Folder", NULL);
  whatnowproc = mh_global_profile_get ("whatnowproc", NULL);

  mh_getopt (&argc, &argv, options, MH_GETOPT_DEFAULT_FOLDER,
	     args_doc, prog_doc, NULL);

  if (!format_str)
    format_str = default_format_str;

  if (mh_format_parse (format_str, &format))
    {
      mu_error (_("Bad format string"));
      exit (1);
    }

  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_RDWR);
  mh_msgset_parse (&msgset, mbox, argc, argv, "cur");
  if (!mh_msgset_single_message (msgset))
    {
      mu_error (_("only one message at a time!"));
      return 1;
    }
  
  if (build_only)
    wh_env.file = mh_expand_name (draftfolder, "reply", NAME_ANY);
  else if (draftfolder)
    {
      if (mh_draft_message (draftfolder, draftmessage, &wh_env.file))
	return 1;
    }
  else
    wh_env.file = mh_expand_name (draftfolder, "draft", NAME_ANY);
  wh_env.draftfile = wh_env.file;

  make_draft (mbox, DISP_REPLACE, &wh_env);

  /* Exit immediately if --build is given */
  if (build_only || nowhatnowproc)
    return 0;

  rc = mh_whatnowproc (&wh_env, initial_edit, whatnowproc);
  
  mu_mailbox_sync (mbox);
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return rc;
}

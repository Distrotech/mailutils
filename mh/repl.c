/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

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

/* MH repl command */

#include <mh.h>
#include <sys/stat.h>
#include <unistd.h>

const char *argp_program_version = "reply (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH repl\v"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[+folder] [msg]");

#define ARG_NOEDIT      1
#define ARG_FCC         2
#define ARG_FILTER      3
#define ARG_INPLACE     4
#define ARG_QUERY       5  
#define ARG_WHATNOWPROC 6
#define ARG_NOWHATNOWPROC 7
#define ARG_NODRAFTFOLDER 8

/* GNU options */
static struct argp_option options[] = {
  {"annotate", 'a', N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Add Replied: header to the message being replied to")},
  {"build", 'b', 0, 0,
   N_("Build the draft and quit immediately.")},
  {"draftfolder", 'd', N_("FOLDER"), 0,
   N_("Specify the folder for message drafts")},
  {"nodraftfolder", ARG_NODRAFTFOLDER, 0, 0,
   N_("Undo the effect of the last --draftfolder option")},
  {"draftmessage" , 'm', N_("MSG"), 0,
   N_("Invoke the draftmessage facility")},
  {"cc",       'c', "{all|to|cc|me}", 0,
   N_("Specify whom to place on the Cc: list of the reply")},
  {"nocc",     'n', "{all|to|cc|me}", 0,
   N_("Specify whom to remove from the Cc: list of the reply")},
  {"folder",  'f', N_("FOLDER"), 0, N_("Specify folder to operate upon")},
  {"editor",  'e', N_("PROG"), 0, N_("Set the editor program to use")},
  {"noedit", ARG_NOEDIT, 0, 0, N_("Suppress the initial edit")},
  {"fcc",    ARG_FCC, N_("FOLDER"), 0, N_("Set the folder to receive Fcc's.")},
  {"filter", ARG_FILTER, N_("PROG"), 0,
   N_("Set the filter program to preprocess the body of the message being replied")},
  {"form",   'F', N_("FILE"), 0, N_("Read format from given file")},
  {"inplace", ARG_INPLACE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Annotate the message in place")},
  {"query", ARG_QUERY, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Query for addresses to place in To: and Cc: lists")},
  {"width", 'w', N_("NUMBER"), 0, N_("Set output width")},
  {"whatnowproc", ARG_WHATNOWPROC, N_("PROG"), 0,
   N_("Set the replacement for whatnow program")},
  {"use", 'u', N_("BOOL"), OPTION_ARG_OPTIONAL, N_("Use draft file preserved after the last session") },
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"annotate", 1, NULL, MH_OPT_BOOL },
  {"build", 1, NULL, },
  {"cc", 1, NULL, MH_OPT_ARG, "all/to/cc/me"},
  {"nocc", 3, NULL, MH_OPT_ARG, "all/to/cc/me"},
  {"form",    4,  NULL, MH_OPT_ARG, "formatfile"},
  {"width",   1,  NULL, MH_OPT_ARG, "number"},
  {"draftfolder", 6, NULL, MH_OPT_ARG, "folder"},
  {"nodraftfolder", 3, NULL },
  {"draftmessage", 6, NULL },
  {"editor", 1, NULL, MH_OPT_ARG, "program"},
  {"noedit", 3, NULL, },
  {"fcc", 1, NULL, MH_OPT_ARG, "folder"},
  {"filter", 2, NULL, MH_OPT_ARG, "program"},
  {"inplace", 1, NULL, MH_OPT_BOOL },
  {"query", 1, NULL, MH_OPT_BOOL },
  {"whatnowproc", 2, NULL, MH_OPT_ARG, "program"},
  {"nowhatnowproc", 3, NULL },
  { 0 }
};

static char *format_str =
"%(lit)%(formataddr %<{reply-to}%?{from}%?{sender}%?{return-path}%>)"
"%<(nonnull)%(void(width))%(putaddr To: )\\n%>"
"%(lit)%<(rcpt to)%(formataddr{to})%>%<(rcpt cc)%(formataddr{cc})%>%<(rcpt me)%(formataddr(me))%>"
"%<(nonnull)%(void(width))%(putaddr cc: )\\n%>"
"%<{fcc}Fcc: %{fcc}\\n%>"
"%<{subject}Subject: Re: %(unre{subject})\\n%>"
"%(lit)%(concat(in_reply_to))%<(nonnull)%(void(width))%(printstr In-reply-to: )\\n%>"
"%(lit)%(concat(references))%<(nonnull)%(void(width))%(printstr References: )\\n%>"
"X-Mailer: MH \\(%(package_string)\\)\\n" 
"--------\n";

static mh_format_t format;
static int width = 80;

struct mh_whatnow_env wh_env = { 0 };
static int initial_edit = 1;
static mh_msgset_t msgset;
static mailbox_t mbox;
static int build_only = 0; /* --build flag */
static int query_mode = 0; /* --query flag */
static int use_draft = 0;  /* --use flag */

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
opt_handler (int key, char *arg, void *unused)
{
  char *s;
  
  switch (key)
    {
    case 'b':
    case ARG_NOWHATNOWPROC:
      build_only = 1;
      break;
      
    case 'c':
      rcpt_mask |= decode_cc_flag ("-cc", arg);
      break;

    case 'n':
      rcpt_mask &= ~decode_cc_flag ("-nocc", arg);
      break;
	
    case 'd':
      wh_env.draftfolder = arg;
      break;
      
    case 'e':
      wh_env.editor = arg;
      break;
      
    case '+':
    case 'f': 
      current_folder = arg;
      break;

    case 'F':
      s = mh_expand_name (MHLIBDIR, arg, 0);
      mh_read_formfile (s, &format_str);
      free (s);
      break;

    case 'm':
      wh_env.draftmessage = arg;
      break;

    case 'u':
      use_draft = is_true (arg);
      break;
      
    case 'w':
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  mh_error (_("Invalid width"));
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
      
    case 'a':
    case ARG_FCC:
    case ARG_FILTER:
    case ARG_INPLACE:
    case ARG_WHATNOWPROC:
      mh_error (_("option is not yet implemented"));
      exit (1);
      
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
  if (stat (wh->draftfile, &st) == 0)
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

  
  rc = mailbox_get_message (mbox, msgset.list[0], &msg);
  if (rc)
    {
      mh_error (_("cannot read message %lu: %s"),
		(unsigned long) msgset.list[0],
		mu_strerror (rc));
      exit (1);
    }

  if (disp == DISP_REPLACE)
    {
      FILE *fp = fopen (wh->file, "w+");
      char buffer[1024];
#define bufsize sizeof(buffer)

      if (!fp)
	{
	  mh_error (_("cannot open draft file %s: %s"),
		    wh->file, strerror (errno));
	  exit (1);
	}
      mh_format (&format, msg, msgset.list[0], buffer, bufsize);
      fprintf (fp, "%s", buffer);
      fclose (fp);
    }

  {
    url_t url;
    size_t num;
    char *msgname, *p;
    
    mailbox_get_url (mbox, &url);
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

  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);
  if (mh_format_parse (format_str, &format))
    {
      mh_error (_("Bad format string"));
      exit (1);
    }

  if (!wh_env.draftfolder)
    wh_env.draftfolder = mh_global_profile_get ("Draft-Folder",
						mu_path_folder_dir);
  
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

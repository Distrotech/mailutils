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

/* MH forw command */

#include <mh.h>

const char *program_version = "forw (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH forw\v"
"Options marked with `*' are not yet implemented.\n"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = "[msgs]";

/* GNU options */
static struct argp_option options[] = {
  {"annotate",      ARG_ANNOTATE,      N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Add Replied: header to the message being replied to")},
  {"build",         ARG_BUILD,         0, 0,
   N_("Build the draft and quit immediately.")},
  {"draftfolder",   ARG_DRAFTFOLDER,   N_("FOLDER"), 0,
   N_("Specify the folder for message drafts")},
  {"nodraftfolder", ARG_NODRAFTFOLDER, 0, 0,
   N_("Undo the effect of the last --draftfolder option")},
  {"draftmessage" , ARG_DRAFTMESSAGE,  N_("MSG"), 0,
   N_("Invoke the draftmessage facility")},
  {"folder",        ARG_FOLDER,        N_("FOLDER"), 0,
   N_("Specify folder to operate upon")},
  {"editor",        ARG_EDITOR,        N_("PROG"), 0,
   N_("Set the editor program to use")},
  {"noedit",        ARG_NOEDIT,        0, 0,
   N_("Suppress the initial edit")},
  {"format",        ARG_FORMAT,        N_("BOOL"), 0, 
   N_("Format messages")},
  {"noformat",      ARG_NOFORMAT,      NULL, 0,
   N_("Undo the effect of the last --format option") },
  {"form",          ARG_FORM,          N_("FILE"), 0,
   N_("Read format from given file")},
  {"filter",        ARG_FILTER,        N_("FILE"), 0,
  N_("Use filter FILE to preprocess the body of the message") },
  {"nofilter",      ARG_NOFILTER,      NULL, 0,
   N_("Undo the effect of the last --filter option") },
  {"inplace",       ARG_INPLACE,       N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Annotate the message in place")},
  {"noinplace",     ARG_NOINPLACE,     0,          OPTION_HIDDEN, "" },
  {"mime",          ARG_MIME,          N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Use MIME encapsulation") },
  {"nomime",        ARG_NOMIME,        NULL, OPTION_HIDDEN, "" },
  {"width", ARG_WIDTH, N_("NUMBER"), 0, N_("Set output width")},
  {"whatnowproc",   ARG_WHATNOWPROC,   N_("PROG"), 0,
   N_("* Set the replacement for whatnow program")},
  {"nowhatnowproc", ARG_NOWHATNOWPROC, NULL, 0,
   N_("* Ignore whatnowproc variable. Use standard `whatnow' shell instead.")},
  {"use",           ARG_USE,           N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Use draft file preserved after the last session") },
  {"nouse",         ARG_NOUSE,         N_("BOOL"), OPTION_HIDDEN, "" },

  {"license", ARG_LICENSE, 0,      0,
   N_("Display software license"), -1},

  {NULL},
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"annotate",      1, MH_OPT_BOOL },
  {"build",         1, },
  {"form",          4, MH_OPT_ARG, "formatfile"},
  {"format",        5,  MH_OPT_ARG, "string"},
  {"draftfolder",   6, MH_OPT_ARG, "folder"},
  {"nodraftfolder", 3 },
  {"draftmessage",  6, },
  {"editor",        1, MH_OPT_ARG, "program"},
  {"noedit",        3, },
  {"filter",        2, MH_OPT_ARG, "program"},
  {"inplace",       1, MH_OPT_BOOL },
  {"whatnowproc",   2, MH_OPT_ARG, "program"},
  {"nowhatnowproc", 3 },
  {"mime",          2, MH_OPT_BOOL, NULL},
  {NULL}
};

enum encap_type {
  encap_clear,
  encap_mhl,
  encap_mime
};

static char *formfile;
struct mh_whatnow_env wh_env = { 0 };
static int initial_edit = 1;
static char *mhl_filter = NULL; /* --filter flag */
static int build_only = 0;      /* --build flag */
static enum encap_type encap = encap_clear; /* controlled by --format, --form
					       and --mime flags */
static int use_draft = 0;       /* --use flag */
static int width = 80;          /* --width flag */

static mh_msgset_t msgset;
static mailbox_t mbox;

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  switch (key)
    {
    case ARG_BUILD:
      build_only = 1;
      break;

    case ARG_DRAFTFOLDER:
      wh_env.draftfolder = arg;
      break;
      
    case ARG_DRAFTMESSAGE:
      wh_env.draftmessage = arg;
      break;

    case ARG_USE:
      use_draft = is_true (arg);
      break;

    case ARG_NOUSE:
      use_draft = 0;
      break;

    case ARG_WIDTH:
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  argp_error (state, _("Invalid width"));
	  exit (1);
	}
      break;

    case ARG_EDITOR:
      wh_env.editor = arg;
      break;
      
    case ARG_FOLDER: 
      current_folder = arg;
      break;

    case ARG_FORM:
      formfile = arg;
      break;

    case ARG_FORMAT:
      if (is_true (arg))
	{
	  encap = encap_mhl;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOFORMAT:
      if (encap == encap_mhl)
	encap = encap_clear;
      break;

    case ARG_FILTER:
      encap = encap_mhl;
      break;
	
    case ARG_MIME:
      if (is_true (arg))
	{
	  encap = encap_mime;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOMIME:
      if (encap == encap_mime)
	encap = encap_clear;
      break;
      
    case ARG_ANNOTATE:
    case ARG_INPLACE:
    case ARG_WHATNOWPROC:
    case ARG_NOWHATNOWPROC:
      argp_error (state, _("option is not yet implemented"));
      exit (1);
      
    case ARG_LICENSE:
      mh_license (argp_program_version);
      break;

    default:
      return 1;
    }
  return 0;
}

struct format_data {
  int num;
  stream_t stream;
  list_t format;
};

static int
msg_copy (message_t msg, stream_t ostream)
{
  stream_t istream;
  int rc;
  size_t n;
  char buf[512];
  
  rc = message_get_stream (msg, &istream);
  if (rc)
    return rc;
  stream_seek (istream, 0, SEEK_SET);
  while (rc == 0
	 && stream_sequential_read (istream, buf, sizeof buf, &n) == 0
	 && n > 0)
    rc = stream_sequential_write (ostream, buf, n);
  return rc;
}

void
format_message (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  struct format_data *fp = data;
  char *s;
  int rc;

  if (fp->num)
    {
      asprintf (&s, "\n------- Message %d\n", fp->num++);
      rc = stream_sequential_write (fp->stream, s, strlen (s));
      free (s);
    }

  if (fp->format)
    rc = mhl_format_run (fp->format, width, 0, 0, msg, fp->stream);
  else
    rc = msg_copy (msg, fp->stream);
}

void
finish_draft ()
{
  int rc;
  stream_t stream;
  list_t format = NULL;
  struct format_data fd;
  char *str;
  
  if (!mhl_filter)
    {
      char *s = mh_expand_name (MHLIBDIR, "mhl.forward", 0);
      if (access (s, R_OK) == 0)
	mhl_filter = "mhl.forward";
      free (s);
    }

  if (mhl_filter)
    {
      char *s = mh_expand_name (MHLIBDIR, mhl_filter, 0);
      format = mhl_format_compile (s);
      if (!format)
	exit (1);
      free (s);
    }
  
  if ((rc = file_stream_create (&stream,
				wh_env.file,
				MU_STREAM_WRITE|MU_STREAM_CREAT)) != 0
      || (rc = stream_open (stream)))
    {
      mh_error (_("cannot open output file \"%s\": %s"),
		wh_env.file, mu_strerror (rc));
      exit (1);
    }

  stream_seek (stream, 0, SEEK_END);

  if (encap == encap_mime)
    {
      url_t url;
      const char *mbox_path;
      char buf[64];
      size_t i;
      
      mailbox_get_url (mbox, &url);
      
      mbox_path = url_to_string (url);
      if (memcmp (mbox_path, "mh:", 3) == 0)
	mbox_path += 3;
      asprintf (&str, "#forw [] +%s", mbox_path);
      rc = stream_sequential_write (stream, str, strlen (str));
      free (str);
      for (i = 0; rc == 0 && i < msgset.count; i++)
	{
          message_t msg;
	  size_t num;
		  
	  mailbox_get_message (mbox, msgset.list[i], &msg);
	  mh_message_number (msg, &num);
	  snprintf (buf, sizeof buf, " %lu", (unsigned long) num);
          rc = stream_sequential_write (stream, buf, strlen (buf));
	}
    }
  else
    {
      str = "\n------- ";
      rc = stream_sequential_write (stream, str, strlen (str));

      if (msgset.count == 1)
	{
	  fd.num = 0;
	  str = _("Forwarded message\n");
	}
      else
	{
	  fd.num = 1;
	  str = _("Forwarded messages\n");
	}
  
      rc = stream_sequential_write (stream, str, strlen (str));
      fd.stream = stream;
      fd.format = format;
      rc = mh_iterate (mbox, &msgset, format_message, &fd);
      
      str = "\n------- ";
      rc = stream_sequential_write (stream, str, strlen (str));
      
      if (msgset.count == 1)
	str = _("End of Forwarded message");
      else
	str = _("End of Forwarded messages");
      
      rc = stream_sequential_write (stream, str, strlen (str));
    }
  
  rc = stream_sequential_write (stream, "\n\n", 2);
  stream_close (stream);
  stream_destroy (&stream, stream_get_owner (stream));
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

  argc -= index;
  argv += index;

  mbox = mh_open_folder (current_folder, 0);
  mh_msgset_parse (mbox, &msgset, argc, argv, "cur");
  
  if (!wh_env.draftfolder)
    wh_env.draftfolder = mh_global_profile_get ("Draft-Folder",
						mu_path_folder_dir);

  wh_env.file = mh_expand_name (wh_env.draftfolder, "forw", 0);
  if (!wh_env.draftfile)
    wh_env.draftfile = mh_expand_name (wh_env.draftfolder, "draft", 0);

  switch (build_only ?
	    DISP_REPLACE : check_draft_disposition (&wh_env, use_draft))
    {
    case DISP_QUIT:
      exit (0);

    case DISP_USE:
      unlink (wh_env.file);
      rename (wh_env.draftfile, wh_env.file);
      break;
	  
    case DISP_REPLACE:
      unlink (wh_env.draftfile);
      mh_comp_draft (formfile, "forwcomps", wh_env.file);
      finish_draft ();
    }
  
  /* Exit immediately if --build is given */
  if (build_only)
    {
      rename (wh_env.file, wh_env.draftfile);
      return 0;
    }
  
  return mh_whatnow (&wh_env, initial_edit);
}

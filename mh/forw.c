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

/* MH forw command */

#include <mh.h>

static char prog_doc[] = N_("Forward messages");
static char args_doc[] = N_("[MSGLIST]");

enum encap_type
  {
    encap_clear,
    encap_mhl,
    encap_mime
  };

char *formfile;
struct mh_whatnow_env wh_env = { 0 };
static int initial_edit = 1;
static const char *whatnowproc;

static char *mhl_filter_file = NULL; /* --filter flag */

static int build_only = 0;      /* --build flag */
static int nowhatnowproc = 0;   /* --nowhatnowproc */
static int annotate = 0;        /* --annotate flag */
static enum encap_type encap = encap_clear; /* controlled by --format, --form
					       and --mime flags */
static int use_draft = 0;       /* --use flag */
static int width = 80;          /* --width flag */
static char *draftmessage = "new";
static const char *draftfolder = NULL;
static char *input_file;        /* input file name (--file option) */

static mu_msgset_t msgset;
static mu_mailbox_t mbox;

static void
set_filter (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  mh_find_file (arg, &mhl_filter_file);
  encap = encap_mhl;
}

static void
clear_filter (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  mhl_filter_file = NULL;
  encap = encap_clear;
}

static void
set_mime (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  if (strcmp (arg, "1") == 0)
    encap = encap_mime;
  else
    encap = encap_clear;
}

static void
set_format (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  if (arg)
    {
      encap = encap_mhl;
      mh_find_file ("mhl.forward", &mhl_filter_file);
    }
  else
    encap = encap_clear;
}

static struct mu_option options[] = {
  { "annotate",      0,      NULL, MU_OPTION_DEFAULT,
    N_("add Forwarded: header to each forwarded message"),
    mu_c_bool, &annotate },
  { "build",         0,      NULL, MU_OPTION_DEFAULT,
    N_("build the draft and quit immediately"),
    mu_c_bool, &build_only },
  { "draftfolder",   0,      N_("FOLDER"), MU_OPTION_DEFAULT,
    N_("specify the folder for message drafts"),
    mu_c_string, &draftfolder },
  { "nodraftfolder", 0, NULL, MU_OPTION_DEFAULT,
    N_("undo the effect of the last --draftfolder option"),
    mu_c_string, &draftfolder, mh_opt_clear_string },
  { "draftmessage" , 0, N_("MSG"), MU_OPTION_DEFAULT,
    N_("invoke the draftmessage facility"),
    mu_c_string, &draftmessage },
  { "editor",        0, N_("PROG"), MU_OPTION_DEFAULT,
    N_("set the editor program to use"),
    mu_c_string, &wh_env.editor },
  { "noedit",        0, NULL, MU_OPTION_DEFAULT,
    N_("suppress the initial edit"),
    mu_c_int, &initial_edit, NULL, "0" },
  { "file",          0, N_("FILE"), MU_OPTION_DEFAULT,
    N_("read message from FILE"),
    mu_c_string, &input_file },
  { "format",        0, NULL, MU_OPTION_DEFAULT, 
    N_("format messages"),
    mu_c_bool, NULL, set_format },
  { "form",          0, N_("FILE"), MU_OPTION_DEFAULT,
    N_("read format from given file"),
    mu_c_string, &formfile, mh_opt_find_file },
  { "filter",        0, N_("FILE"), MU_OPTION_DEFAULT,
    N_("use filter FILE to preprocess the body of the message"),
    mu_c_string, NULL, set_filter },
  { "nofilter",      0, NULL, MU_OPTION_DEFAULT,
   N_("undo the effect of the last --filter option"),
    mu_c_string, NULL, clear_filter },
  { "inplace",       0, NULL, MU_OPTION_HIDDEN,
    N_("annotate the message in place"),
    mu_c_bool, NULL, mh_opt_notimpl_warning },
  { "mime",          0, NULL, MU_OPTION_DEFAULT,
    N_("use MIME encapsulation"),
    mu_c_bool, NULL, set_mime },
  { "width",         0, N_("NUMBER"), MU_OPTION_DEFAULT,
    N_("Set output width"),
    mu_c_int, &width },
  { "whatnowproc",   0, N_("PROG"), MU_OPTION_DEFAULT,
    N_("set the replacement for whatnow program"),
    mu_c_string, &whatnowproc },
  { "nowhatnowproc", 0, NULL, MU_OPTION_DEFAULT,
    N_("don't run whatnowproc"),
    mu_c_int, &nowhatnowproc, NULL, "1" },
  { "use",           0, NULL, MU_OPTION_DEFAULT,
    N_("use draft file preserved after the last session"),
    mu_c_bool, &use_draft },
  MU_OPTION_END
};

struct format_data
{
  int num;
  mu_stream_t stream;
  mu_list_t format;
};

/* State machine according to RFC 934:
   
      S1 ::   CRLF {CRLF} S1
            | "-" {"- -"} S2
            | c {c} S2

      S2 ::   CRLF {CRLF} S1
            | c {c} S2
*/

enum rfc934_state { S1, S2 };

static int
msg_copy (mu_message_t msg, mu_stream_t ostream)
{
  mu_stream_t istream;
  int rc;
  size_t n;
  char buf[512];
  enum rfc934_state state = S1;
  
  rc = mu_message_get_streamref (msg, &istream);
  if (rc)
    return rc;
  while (rc == 0
	 && mu_stream_read (istream, buf, sizeof buf, &n) == 0
	 && n > 0)
    {
      size_t start, i;
	
      for (i = start = 0; i < n; i++)
	switch (state)
	  {
	  case S1:
	    if (buf[i] == '-')
	      {
		rc = mu_stream_write (ostream, buf + start,
				      i - start + 1, NULL);
		if (rc)
		  return rc;
		rc = mu_stream_write (ostream, " -", 2, NULL);
		if (rc)
		  return rc;
		start = i + 1;
		state = S2;
	      }
	    else if (buf[i] != '\n')
	      state = S2;
	    break;
	      
	  case S2:
	    if (buf[i] == '\n')
	      state = S1;
	  }
      if (i > start)
	rc = mu_stream_write (ostream, buf + start, i  - start, NULL);
    }
  mu_stream_destroy (&istream);
  return rc;
}

void
format_message (mu_stream_t outstr, mu_message_t msg, int num,
		mu_list_t format)
{
  int rc = 0;
  
  if (annotate)
    mu_list_append (wh_env.anno_list, msg);
  
  if (num)
    rc = mu_stream_printf (outstr, "\n------- Message %d\n", num);

  if (rc == 0)
    {
      if (format)
	rc = mhl_format_run (format, width, 0, 0, msg, outstr);
      else
	rc = msg_copy (msg, outstr);
    }
  
  if (rc)
    {
      mu_error (_("cannot copy message: %s"), mu_strerror (rc));
      exit (1);
    }
}

int
format_message_itr (size_t num, mu_message_t msg, void *data)
{
  struct format_data *fp = data;

  format_message (fp->stream, msg, fp->num, fp->format);
  if (fp->num)
    fp->num++;
  return 0;
}

static int
_proc_forwards (size_t n, mu_message_t msg, void *call_data)
{
  mu_stream_t stream = call_data;
  size_t num;
	      
  if (annotate)
    mu_list_append (wh_env.anno_list, msg);
  mh_message_number (msg, &num);
  return mu_stream_printf (stream, " %lu", (unsigned long) num);
}

void
finish_draft ()
{
  int rc;
  mu_stream_t stream;
  mu_list_t format = NULL;
  struct format_data fd;
  char *str;
  
  if ((rc = mu_file_stream_create (&stream,
				   wh_env.file,
				   MU_STREAM_WRITE|MU_STREAM_CREAT)))
    {
      mu_error (_("cannot open output file `%s': %s"),
		wh_env.file, mu_strerror (rc));
      exit (1);
    }

  mu_stream_seek (stream, 0, SEEK_END, NULL);

  if (input_file)
    {
      mu_stream_t instr;
      int rc;
  
      if ((rc = mu_file_stream_create (&stream,
				       wh_env.file,
				       MU_STREAM_WRITE|MU_STREAM_CREAT)))
	{
	  mu_error (_("cannot open output file `%s': %s"),
		    wh_env.file, mu_strerror (rc));
	  exit (1);
	}

      mu_stream_seek (stream, 0, SEEK_END, NULL);
      
      rc = mu_file_stream_create (&instr, input_file, MU_STREAM_READ);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_file_stream_create",
			   input_file, rc);
	  exit (1);
	}
      
      rc = mu_stream_copy (stream, instr, 0, NULL);
      mu_stream_unref (instr);
    }
  else
    {
      if (encap == encap_mhl)
	{
	  if (mhl_filter_file)
	    {
	      format = mhl_format_compile (mhl_filter_file);
	      if (!format)
		exit (1);
	    }
	}
      
      if (annotate)
	{
	  wh_env.anno_field = "Forwarded";
	  mu_list_create (&wh_env.anno_list);
	}
      
      if (encap == encap_mime)
	{
	  mu_url_t url;
	  const char *mbox_path;
      
	  mu_mailbox_get_url (mbox, &url);
	  mu_url_sget_path (url, &mbox_path);
	  mu_asprintf (&str, "#forw [] +%s", mbox_path);
	  rc = mu_stream_write (stream, str, strlen (str), NULL);
	  free (str);
	  mu_msgset_foreach_message (msgset, _proc_forwards, stream);
	}
      else
	{
	  int single_message = mh_msgset_single_message (msgset);
	  
	  str = "\n------- ";
	  rc = mu_stream_write (stream, str, strlen (str), NULL);
	  
	  if (single_message)
	    {
	      fd.num = 0;
	      str = (char*) _("Forwarded message\n");
	    }
	  else
	    {
	      fd.num = 1;
	      str = (char*) _("Forwarded messages\n");
	    }
	  
	  rc = mu_stream_write (stream, str, strlen (str), NULL);
	  fd.stream = stream;
	  fd.format = format;
	  rc = mu_msgset_foreach_message (msgset, format_message_itr, &fd);
      
	  str = "\n------- ";
	  rc = mu_stream_write (stream, str, strlen (str), NULL);
	  
	  if (single_message)
	    str = (char*) _("End of Forwarded message");
	  else
	    str = (char*) _("End of Forwarded messages");
	  
	  rc = mu_stream_write (stream, str, strlen (str), NULL);
	}
      
      rc = mu_stream_write (stream, "\n\n", 2, NULL);
    }
  mu_stream_close (stream);
  mu_stream_destroy (&stream);
}

int
main (int argc, char **argv)
{
  int rc;

  draftfolder = mh_global_profile_get ("Draft-Folder", NULL);
  whatnowproc = mh_global_profile_get ("whatnowproc", NULL);
  mh_getopt (&argc, &argv, options, MH_GETOPT_DEFAULT_FOLDER,
	     args_doc, prog_doc, NULL);
  if (!formfile)
    mh_find_file ("forwcomps", &formfile);
  
  if (input_file)
    {
      if (encap == encap_mime)
	{
	  mu_error (_("--build disables --mime"));
	  encap = encap_clear;
	}
      if (argc)
	{
	  mu_error (_("can't mix files and folders/msgs"));
	  exit (1);
	}
    }
  else
    {
      mbox = mh_open_folder (mh_current_folder (), MU_STREAM_RDWR);
      mh_msgset_parse (&msgset, mbox, argc, argv, "cur");
    }
  
  if (build_only || !draftfolder)
    wh_env.file = mh_expand_name (NULL, "draft", NAME_ANY);
  else if (draftfolder)
    {
      if (mh_draft_message (draftfolder, draftmessage, &wh_env.file))
	return 1;
    }
  wh_env.draftfile = wh_env.file;

  switch (build_only ?
	  DISP_REPLACE : check_draft_disposition (&wh_env, use_draft))
    {
    case DISP_QUIT:
      exit (0);
      
    case DISP_USE:
      break;
      
    case DISP_REPLACE:
      unlink (wh_env.draftfile);
      mh_comp_draft (formfile, wh_env.file);
      finish_draft ();
    }
  
  /* Exit immediately if --build is given */
  if (build_only || nowhatnowproc)
    {
      if (strcmp (wh_env.file, wh_env.draftfile))
	rename (wh_env.file, wh_env.draftfile);
      return 0;
    }
  
  rc = mh_whatnowproc (&wh_env, initial_edit, whatnowproc);

  mu_mailbox_sync (mbox);
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return rc;
}

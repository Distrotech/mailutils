/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2005, 2007-2012, 2014-2016 Free Software
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

/* MH show command */

#include <mh.h>

static char prog_doc[] = N_("GNU MH show");
static char args_doc[] = N_("[+FOLDER] [MSGLIST]");

int use_draft;
int use_showproc = 1;
int header_option = 1;
char *file;

const char *showproc;
char **showargv;
size_t showargc;
size_t showargmax;

const char *mhnproc;

static void
addarg (const char *arg)
{
  if (showargc == showargmax)
    showargv = mu_2nrealloc (showargv, &showargmax, sizeof showargv[0]);
  showargv[showargc++] = (char*) arg;
}

static void
insarg (char *arg)
{
  addarg (arg);
  if (showargc > 2)
    {
      char *p = showargv[showargc-1];
      memmove (showargv + 2, showargv + 1,
	       sizeof (showargv[0]) * (showargc - 2));
      showargv[1] = p;
    }
}

static void
set_draft (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  if (use_draft || file)
    {
      mu_parseopt_error (po, _("only one file at a time!"));
      exit (po->po_exit_error);
    }
  use_draft = 1;
}

static void
set_file (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  if (use_draft || file)
    {
      mu_parseopt_error (po, _("only one file at a time!"));
      exit (po->po_exit_error);
    }
  file = mh_expand_name (NULL, arg, NAME_FILE);
}

static void
add_show_arg (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  char *name = mu_alloc (strlen (opt->opt_long) + 2);
  name[0] = '-';
  strcpy (name + 1, opt->opt_long);
  addarg (name);
  if (arg)
    addarg (arg);
}

static struct mu_option options[] = {
  { "draft",        0,     NULL, MU_OPTION_DEFAULT,
    N_("show the draft file"),
    mu_c_string, NULL, set_draft },
  { "file",         0,     N_("FILE"), MU_OPTION_DEFAULT,
    N_("show this file"),
    mu_c_string, NULL, set_file },
  { "header",       0,     NULL, MU_OPTION_DEFAULT,
    N_("display a description of the message before the message itself"),
    mu_c_bool, &header_option },
  { "showproc",     0,     N_("PROGRAM"), MU_OPTION_DEFAULT,
    N_("use PROGRAM to show messages"),
    mu_c_string, &showproc },
  { "noshowproc",   0,  NULL, MU_OPTION_DEFAULT,
    N_("disable the use of the \"showproc:\" profile component"),
    mu_c_int, &use_showproc, NULL, "0" },
  { "form",         0,  N_("FILE"),  MU_OPTION_DEFAULT,
    N_("read format from given file"),
    mu_c_string, NULL, add_show_arg },
  { "moreproc",     0, N_("PROG"),   MU_OPTION_DEFAULT,
    N_("use this PROG as pager"),
    mu_c_string, NULL, add_show_arg },
  { "nomoreproc",   0, NULL,         MU_OPTION_DEFAULT,
    N_("disable the use of moreproc"),
    mu_c_string, NULL, add_show_arg },
  { "length",       0, N_("NUMBER"), MU_OPTION_DEFAULT,
    N_("set output screen length"),
    mu_c_string, NULL, add_show_arg },
  { "width",        0, N_("NUMBER"), MU_OPTION_DEFAULT,
    N_("set output width"),
    mu_c_string, NULL, add_show_arg },
  
  MU_OPTION_END
};

static int
resolve_mime (size_t num, mu_message_t msg, void *data)
{
  int *ismime = data;
  
  mu_message_is_multipart (msg, ismime);
  return *ismime;
}

static int
resolve_msg (size_t num, mu_message_t msg, void *data)
{
  char *path = data;
  
  mh_message_number (msg, &num);
  addarg (mh_safe_make_file_name (path, mu_umaxtostr (0, num)));

  return 0;
}

static int
adduid (size_t num, void *data)
{
  addarg (mu_umaxtostr (0, num));
  return 0;
}

static int
printheader (size_t num, void *data)
{
  mu_printf ("(Message %s:%lu)\n", (char*) data, (unsigned long)num);
  return 1;
}

static void
checkfile (char *file)
{
  int ismime = 0;

  if (!showproc)
    {
      mu_message_t msg = mh_file_to_message (NULL, file);
      mu_message_is_multipart (msg, &ismime);
      mu_message_unref (msg);
    }
  if (ismime)
    {
      showproc = mhnproc;
      insarg ("-show");
      addarg ("-file");
    }	      
  addarg (file);
}  

int
main (int argc, char **argv)
{
  mu_mailbox_t mbox;
  mu_msgset_t msgset;
  const char *p;
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  showargmax = 2;
  showargc = 1;
  showargv = mu_calloc (showargmax, sizeof showargv[0]);

  mh_getopt (&argc, &argv, options, MH_GETOPT_DEFAULT_FOLDER,
	     args_doc, prog_doc, NULL);

  
  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_RDWR);

  if (use_draft || file)
    {
      if (argc)
	{
	  mu_error (_("only one file at a time!"));
	  exit (1);
	}
    }
  else
    mh_msgset_parse (&msgset, mbox, argc, argv, "cur");

  /* 1. If !use_showproc set showproc to /bin/cat and go to X.
     2. If -file or -draft is given
     2.a.  If showproc is set, go to 2.c
     2.b.  If the file is a multipart one, set showproc to mhn with -file
           and -show options and go to X
     2.c.  Add file to the argument list and go to X.
     3. Scan all messages to see if there are MIME ones. If so, set
        showproc to mhn with the -show option, transfer message UIDs
	to showproc arguments, and go to X
     4. Otherwise set showproc from the showproc variable in .mh_profile
     X. If showproc is not set, set it to PAGER ("/usr/bin/more" by default).
     Y. Execute showproc with the collected argument list.
  */

  if (!use_showproc)
    showproc = "/bin/cat";
  else
    showproc = mh_global_profile_get ("showproc", NULL);

  mhnproc = mh_global_profile_get ("mhnproc", "mhn");
  
  if (use_draft || file)
    {
      if (file)
	checkfile (file);
      else
	{
	  const char *dfolder = mh_global_profile_get ("Draft-Folder", NULL);

	  if (dfolder)
	    {
	      int ismime = 0;
	      
	      mu_mailbox_close (mbox);
	      mu_mailbox_destroy (&mbox);
	      mbox = mh_open_folder (dfolder, MU_STREAM_RDWR);
	      mh_msgset_parse (&msgset, mbox, 0, NULL, "cur");

	      mu_msgset_foreach_message (msgset, resolve_mime, &ismime);
	      if (ismime)
		{
		  showproc = mhnproc;
		  insarg ("-show");
		  addarg ("-file");
		}	      
	      mu_msgset_foreach_message (msgset, resolve_msg,
					 (void *) dfolder);
	    }
	  else
	    checkfile (mh_expand_name (mu_folder_directory (),
				       "draft", NAME_ANY));
	}
    }
  else
    {
      mu_url_t url;
      size_t n;
      int ismime = 0;
      const char *path;
      
      mu_mailbox_get_url (mbox, &url);
      mu_url_sget_path (url, &path);

      if (!showproc)
	{
	  mu_msgset_foreach_message (msgset, resolve_mime, &ismime);
	  if (ismime)
	    {
	      showproc = mhnproc;
	      insarg ("-show");
	    }
	}
      
      if (ismime)
	mu_msgset_foreach_msguid (msgset, adduid, NULL);
      else
	mu_msgset_foreach_message (msgset, resolve_msg, (void*) path);

      p = mh_global_profile_get ("Unseen-Sequence", NULL);
      if (p)
	{
	  mh_seq_delete (mbox, p, msgset, 0);
	  mh_global_save_state ();
	}
      
      if (header_option && mu_msgset_count (msgset, &n) == 0 && n == 1)
	mu_msgset_foreach_msguid (msgset, printheader, (void*) path);
    }

  mu_stream_flush (mu_strout);
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  
  if (!showproc)
    {
      showproc = getenv ("PAGER");
      if (!showproc)
	showproc = "/usr/bin/more";
    }

  addarg (NULL);
  p = strrchr (showproc, '/');
  showargv[0] = (char*) (p ? p + 1 : showproc);
  execvp (showproc, showargv);
  mu_error (_("unable to exec %s: %s"), showargv[0], mu_strerror (errno));
  return 1;
}
  
  

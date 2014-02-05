/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2005, 2007-2012, 2014 Free Software Foundation,
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

/* MH show command */

#include <mh.h>

static char doc[] = N_("GNU MH show")"\v"
N_("Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[+FOLDER] [MSGLIST]");

#define ARG_PASS ARG_MAX

static struct argp_option options[] = {
  {"draft",         ARG_DRAFT,          NULL, 0,
   N_("show the draft file") },
  {"file",          ARG_FILE,     N_("FILE"), 0,
   N_("show this file") },
  {"header",        ARG_HEADER,   N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("display a description of the message before the message itself") },
  {"noheader",      ARG_HEADER,   NULL, OPTION_HIDDEN, "" },
  {"showproc",      ARG_SHOWPROC, N_("PROGRAM"), 0,
   N_("use PROGRAM to show messages")},
  {"noshowproc",    ARG_NOSHOWPROC, NULL, 0,
   N_("disable the use of the \"showproc:\" profile component") },

  {"form",          ARG_FORM,     N_("FILE"),   0,
   N_("read format from given file")},
  {"moreproc",      ARG_MOREPROC, N_("PROG"),   0,
   N_("use this PROG as pager"), },
  {"nomoreproc",    ARG_NOMOREPROC,     NULL,         0,
   N_("disable the use of moreproc") },
  {"length",        ARG_LENGTH,   N_("NUMBER"), 0,
   N_("set output screen length")},
  {"width",         ARG_WIDTH,    N_("NUMBER"), 0,
   N_("set output width")},
  
  {NULL}
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "draft" },
  { "file", MH_OPT_ARG, "file" },
  { "header", MH_OPT_BOOL },
  { "showproc", MH_OPT_ARG, "program" },
  { "noshowproc" },
  { "form",       MH_OPT_ARG, "formatfile"},
  { "width",      MH_OPT_ARG, "number"},
  { "length",     MH_OPT_ARG, "number"},
  { "moreproc",   MH_OPT_ARG, "program"},
  { "nomoreproc" },
  
  { NULL }
};

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

static const char *
findopt (int key)
{
  struct argp_option *p;

  for (p = options; p->name; p++)
    if (p->key == key)
      return p->name;
  abort ();
}

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      mh_set_current_folder (arg);
      break;

    case ARG_DRAFT:
      if (use_draft || file)
	argp_error (state, _("only one file at a time!"));
      use_draft = 1;
      break;

    case ARG_FILE:
      if (use_draft || file)
	argp_error (state, _("only one file at a time!"));
      file = mh_expand_name (NULL, arg, NAME_FILE);
      break;
      
    case ARG_HEADER:
      header_option = is_true (arg);
      break;

    case ARG_NOHEADER:
      header_option = 0;
      break;

    case ARG_SHOWPROC:
      showproc = arg;
      break;

    case ARG_NOSHOWPROC:
      use_showproc = 0;
      break;
      
    case ARG_NOMOREPROC:
    case ARG_FORM:
    case ARG_MOREPROC:
    case ARG_LENGTH:
    case ARG_WIDTH:
      addarg (findopt (key));
      if (arg)
	addarg (arg);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

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
  int index = 0;
  mu_mailbox_t mbox;
  mu_msgset_t msgset;
  const char *p;
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  showargmax = 2;
  showargc = 1;
  showargv = mu_calloc (showargmax, sizeof showargv[0]);
  
  mh_argp_init ();
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);
  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_RDWR);

  argc -= index;
  argv += index;
  
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
  
  

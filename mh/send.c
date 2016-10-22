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

/* MH send command */

#include <mh.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <pwd.h>

static char prog_doc[] = N_("GNU MH send");
static char args_doc[] = N_("FILE [FILE...]");

static const char *draftfolder;  /* Use this draft folder */
static int use_draftfolder = 1;
static int use_draft;

static int append_msgid;         /* Append Message-ID: header */
static int background;           /* Operate in the background */

static int split_message;            /* Split the message */
static unsigned long split_interval; /* Interval in seconds between sending two
					successive partial messages */
static size_t split_size = 76*632;   /* Size of split parts */
static int verbose;              /* Produce verbose diagnostics */
static int watch;                /* Watch the delivery process */

static int keep_files;           /* Keep draft files */

#define DEFAULT_X_MAILER "MH (" PACKAGE_STRING ")"

#define WATCH(c) do {\
  if (watch)\
    watch_printf c;\
} while (0)

static int add_file (char const *name);
static void mesg_list_fixup (void);

static void
add_alias (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  mh_alias_read (arg, 1);
}

static void
set_draftfolder (struct mu_parseopt *po, struct mu_option *opt,
		 char const *arg)
{
  draftfolder = mu_strdup (arg);
  use_draftfolder = 1;
}

static void
add_draftmessage (struct mu_parseopt *po, struct mu_option *opt,
		  char const *arg)
{
  add_file (arg);
}

static void
set_split_opt (struct mu_parseopt *po, struct mu_option *opt,
	       char const *arg)
{
  char *errmsg;
  int rc = mu_str_to_c (arg, opt->opt_type, opt->opt_ptr, &errmsg);
  if (rc)
    {
      char const *errtext;
      if (errmsg)
	errtext = errmsg;
      else
	errtext = mu_strerror (rc);

      mu_parseopt_error (po, "%s%s: %s", po->po_long_opt_start,
			 opt->opt_long, errtext);
      free (errmsg);

      if (!(po->po_flags & MU_PARSEOPT_NO_ERREXIT))
	exit (po->po_exit_error);
    }
  split_message = 1;
}

static struct mu_option options[] = {
  { "alias",        0,    N_("FILE"), MU_OPTION_DEFAULT,
    N_("specify additional alias file"),
    mu_c_string, NULL, add_alias },
  { "draft",        0,    NULL, MU_OPTION_DEFAULT,
    N_("use prepared draft"),
    mu_c_bool, &use_draft },
  { "draftfolder",  0,   N_("FOLDER"), MU_OPTION_DEFAULT,
    N_("specify the folder for message drafts"),
    mu_c_string, NULL, set_draftfolder },
  { "draftmessage", 0,   N_("MSG"), MU_OPTION_DEFAULT,
    N_("use MSG from the draftfolder as a draft"),
    mu_c_string, NULL, add_draftmessage },
  { "nodraftfolder", 0, NULL, MU_OPTION_DEFAULT,
    N_("undo the effect of the last --draftfolder option"),
    mu_c_int, &use_draftfolder, NULL, "1" },
  { "filter",        0, N_("FILE"), MU_OPTION_HIDDEN,
    N_("use filter FILE to preprocess the body of the message"),
    mu_c_string, NULL, mh_opt_notimpl },
  { "nofilter",      0, NULL, MU_OPTION_HIDDEN,
    N_("undo the effect of the last --filter option"),
    mu_c_string, NULL, mh_opt_notimpl },
  { "format",        0, NULL, MU_OPTION_HIDDEN,
    N_("reformat To: and Cc: addresses"),
    mu_c_string, NULL, mh_opt_notimpl_warning },
  { "noformat",      0, NULL, MU_OPTION_HIDDEN },
  { "forward",       0, NULL, MU_OPTION_HIDDEN,
    N_("in case of failure forward the draft along with the failure notice to the sender"),
    mu_c_string, NULL, mh_opt_notimpl_warning },
  { "noforward",     0, NULL, MU_OPTION_HIDDEN, "" },
  { "mime",          0, NULL, MU_OPTION_HIDDEN,
    N_("use MIME encapsulation"),
    mu_c_bool, NULL, mh_opt_notimpl_warning },
  { "msgid",         0, NULL, MU_OPTION_DEFAULT,
    N_("add Message-ID: field"),
    mu_c_bool, &append_msgid },
  { "push",          0, NULL, MU_OPTION_DEFAULT,
    N_("run in the background"),
    mu_c_bool, &background },
  { "preserve",      0, NULL, MU_OPTION_DEFAULT,
    N_("keep draft files"),
    mu_c_bool, &keep_files },
  { "keep",          0, NULL, MU_OPTION_ALIAS },
  { "split",         0, N_("SECONDS"), MU_OPTION_DEFAULT,
   N_("split the draft into several partial messages and send them with SECONDS interval"),
    mu_c_ulong, &split_interval, set_split_opt },
  { "chunksize",     0, N_("NUMBER"), MU_OPTION_DEFAULT,
    N_("set the size of chunk for --split (in bytes)"),
    mu_c_size, &split_size },
  { "verbose",       0, NULL, MU_OPTION_DEFAULT,
    N_("print the transcript of interactions with the transport system"),
    mu_c_bool, &verbose },
  { "watch",         0, NULL, MU_OPTION_DEFAULT,
    N_("monitor the delivery of mail"),
    mu_c_bool, &watch },
  { "width",         0, N_("NUMBER"), MU_OPTION_HIDDEN,
    N_("make header fields no longer than NUMBER columns"),
    mu_c_string, NULL, mh_opt_notimpl_warning },

  MU_OPTION_END
};

static void
watch_printf (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  va_end (ap);
}

struct list_elt           /* Element of the send list */
{
  const char *file_name;  /* Duplicated in msg stream, but there's no way
			     to get it from there */
  mu_message_t msg;       /* Corresponding message */
};

static mu_list_t mesg_list;
static mu_property_t mts_profile;

int
add_file (char const *name)
{
  struct list_elt *elt;

  if (!mesg_list && mu_list_create (&mesg_list))
    {
      mu_error (_("cannot create message list"));
      return 1;
    }
  
  elt = mu_alloc (sizeof *elt);
  elt->file_name = name;
  elt->msg = NULL;
  return mu_list_append (mesg_list, elt);
}

int
checkdraft (const char *name)
{
  struct stat st;
  
  if (stat (name, &st))
    {
      mu_error (_("unable to stat draft file %s: %s"), name,
		mu_strerror (errno));
      return 1;
    }
  return 0;
}

int
elt_fixup (void *item, void *data)
{
  struct list_elt *elt = item;
  
  elt->file_name = mh_expand_name (draftfolder, elt->file_name, NAME_ANY);
  if (checkdraft (elt->file_name))
    exit (1);
  elt->msg = mh_file_to_message (NULL, elt->file_name);
  if (!elt->msg)
    return 1;

  return 0;
}

void
mesg_list_fixup ()
{
  if (mesg_list && mu_list_foreach (mesg_list, elt_fixup, NULL))
    exit (1);
}  

void
read_mts_profile ()
{
  char *name;
  const char *p;
  char *hostname = NULL;
  int rc;

  name = getenv ("MTSTAILOR");
  if (name)
    mts_profile = mh_read_property_file (name, 1);
  else
    {
      mu_property_t local_profile;

      name = mh_expand_name (MHLIBDIR, "mtstailor", NAME_ANY);
      mts_profile = mh_read_property_file (name, 1);

      name = mu_tilde_expansion ("~/.mtstailor", MU_HIERARCHY_DELIMITER, NULL);
      local_profile = mh_read_property_file (name, 1);

      mh_property_merge (mts_profile, local_profile);
      mu_property_destroy (&local_profile);
    }

  rc = mu_property_aget_value (mts_profile, "localname", &hostname);
  if (rc == MU_ERR_NOENT)
    {
      rc = mu_get_host_name (&hostname);
      if (rc)
	{
	  mu_error (_("cannot get system host name: %s"), mu_strerror (rc));
	  exit (1);
	}
    }
  else if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_profile_aget_value", "localname", rc);
      exit (1);
    }

  rc = mu_property_sget_value (mts_profile, "localdomain", &p);
  if (rc == 0)
    {
      hostname = mu_realloc (hostname, strlen (hostname) + 1 + strlen (p) + 1);
      strcat (hostname, ".");
      strcat (hostname, p);
    }
  else if (rc != MU_ERR_NOENT)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_profile_sget_value",
		       "localdomain", rc);
      exit (1);
    }

  rc = mu_set_user_email_domain (hostname);
  free (hostname);
  if (rc)
    {
      mu_error (_("cannot set user mail domain: %s"), mu_strerror (rc));
      exit (1);
    }
  
  rc = mu_property_sget_value (mts_profile, "username", &p);
  if (rc == 0)
    {
      size_t len;
      const char *domain;
      char *newemail;
      int rc;
      
      rc = mu_get_user_email_domain (&domain);
      if (rc)
	{
	  mu_error (_("cannot get user email: %s"), mu_strerror (rc));
	  exit (1);
	}
      len = strlen (p) + 1 + strlen (domain) + 1;
      newemail = mu_alloc (len);
      strcpy (newemail, p);
      strcat (newemail, "@");
      strcat (newemail, domain);

      rc = mu_set_user_email (newemail);
      if (rc)
	{
	  mu_error (_("cannot set user email (%s): %s"),
		    newemail, mu_strerror (rc));
	  exit (1);
	}
      
      free (newemail);
    }
  else if (rc != MU_ERR_NOENT)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_profile_sget_value",
		       "username", rc);
      exit (1);
    }
}


mu_mailer_t
open_mailer ()
{
  const char *url = mu_mhprop_get_value (mts_profile,
					 "url",
					 "sendmail:/usr/sbin/sendmail");
  mu_mailer_t mailer;
  int status;
    
  WATCH ((_("Creating mailer %s"), url));
  status = mu_mailer_create (&mailer, url);
  if (status)
    {
      mu_error (_("cannot create mailer `%s'"), url);
      return NULL;
    }

  if (verbose)
    {
      mu_debug_set_category_level (MU_DEBCAT_MAILER,
                                   MU_DEBUG_LEVEL_UPTO (MU_DEBUG_PROT));
    }

  WATCH ((_("Opening mailer %s"), url));
  status = mu_mailer_open (mailer, MU_STREAM_RDWR);
  if (status)
    {
      mu_error (_("cannot open mailer `%s'"), url);
      return NULL;
    }
  return mailer;
}

static void
create_message_id (mu_header_t hdr)
{
  char *p = mh_create_message_id (0);
  mu_header_set_value (hdr, MU_HEADER_MESSAGE_ID, p, 1);
  free (p);
}

static const char *
get_sender_personal ()
{
  const char *s = mh_global_profile_get ("signature", getenv ("SIGNATURE"));
  if (!s)
    {
      struct passwd *pw = getpwuid (getuid ());
      if (pw && pw->pw_gecos[0])
	{
	  char *p = strchr (pw->pw_gecos, ',');
	  if (p)
	    *p = 0;
	  s = pw->pw_gecos;
	}
    }
  return s;
}

static void
set_address_header (mu_header_t hdr, char *name, mu_address_t addr)
{
  const char *value;
  if (mu_address_sget_printable (addr, &value) == 0)
    mu_header_set_value (hdr, name, value, 1);
  /* FIXME: Error reporting */
}

void
expand_aliases (mu_message_t msg)
{
  mu_header_t hdr;
  mu_address_t addr_to = NULL,
               addr_cc = NULL,
               addr_bcc = NULL;

  mh_expand_aliases (msg, &addr_to, &addr_cc, &addr_bcc);

  mu_message_get_header (msg, &hdr);
  if (addr_to)
    {
      set_address_header (hdr, MU_HEADER_TO, addr_to);
      mu_address_destroy (&addr_to);
    }

  if (addr_cc)
    {
      set_address_header (hdr, MU_HEADER_CC, addr_cc);
      mu_address_destroy (&addr_cc);
    }

  if (addr_bcc)
    {
      set_address_header (hdr, MU_HEADER_BCC, addr_bcc);
      mu_address_destroy (&addr_bcc);
    }
}

void
fix_fcc (mu_message_t msg)
{
  mu_header_t hdr;
  char *fcc;
  
  mu_message_get_header (msg, &hdr);
  if (mu_header_aget_value (hdr, MU_HEADER_FCC, &fcc) == 0)
    {
      struct mu_wordsplit ws;
      int need_fixup = 0;
      size_t fixup_len = 0;

      ws.ws_delim = ",";
      if (mu_wordsplit (fcc, &ws,
			MU_WRDSF_DEFFLAGS | MU_WRDSF_DELIM | MU_WRDSF_WS))
	{
	  mu_error (_("cannot split line `%s': %s"), fcc,
		    mu_wordsplit_strerror (&ws));
	}
      else
	{
	  size_t i;
	  
	  for (i = 0; i < ws.ws_wordc; i += 2)
	    {
	      if (strchr ("+%~/=", ws.ws_wordv[i][0]) == NULL)
		{
		  need_fixup++;
		  fixup_len ++;
		}
	      fixup_len += strlen (ws.ws_wordv[i]);
	    }

	  if (need_fixup)
	    {
	      char *p;
	      
	      /* the new fcc string contains: folder names - fixup_len
		 characters, ws.ws_wordc - 1 comma-space pairs and a
		 terminating nul */
	      fcc = realloc (fcc, fixup_len + ws.ws_wordc - 1 + 1);
	      for (i = 0, p = fcc; i < ws.ws_wordc; i++)
		{
		  if (strchr ("+%~/=", ws.ws_wordv[i][0]) == NULL)
		    *p++ = '+';
		  strcpy (p, ws.ws_wordv[i]);
		  p += strlen (p);
		  *p++ = ',';
		  *p++ = ' ';
		}
	      *p = 0;
	    }
	}

      mu_wordsplit_free (&ws);

      if (need_fixup)
	{
	  mu_header_set_value (hdr, MU_HEADER_FCC, fcc, 1);
	  WATCH ((_("Fixed fcc: %s"), fcc));
	}
      free (fcc);
    }	  
}

/* Convert MH-style DCC headers to normal BCC.
   FIXME: Normally we should iterate through the headers to catch
   multiple Dcc occurrences (the same holds true for Fcc as well),
   however at the time of this writing we have mu_header_get_field_value,
   but we miss mu_header_set_field_value. */
void
fix_dcc (mu_message_t msg)
{
  mu_header_t hdr;
  char *dcc;
  
  mu_message_get_header (msg, &hdr);
  if (mu_header_aget_value (hdr, MU_HEADER_DCC, &dcc) == 0)
    {
      char *bcc = NULL;
      
      mu_header_set_value (hdr, MU_HEADER_DCC, NULL, 1);
      mu_header_aget_value (hdr, MU_HEADER_BCC, &bcc);
      if (bcc)
	{
	  char *newbcc = realloc (bcc, strlen (bcc) + 1 + strlen (dcc) + 1);
	  if (!newbcc)
	    {
	      mu_error (_("not enough memory"));
	      free (dcc);
	      free (bcc);
	      return;
	    }
	  bcc = newbcc;
	  strcat (bcc, ",");
	  strcat (bcc, dcc);
	  free (dcc);
	}
      else
	bcc = dcc;

      WATCH ((_("Fixed bcc: %s"), bcc));
      mu_header_set_value (hdr, MU_HEADER_BCC, bcc, 1);
      free (bcc);
    }
}

void
backup_file (const char *file_name)
{
  char *new_name = mu_alloc (strlen (file_name) + 2);
  char *p = strrchr (file_name, '/');
  if (p)
    {
      size_t len = p - file_name + 1;
      memcpy (new_name, file_name, len);
      new_name[len++] = ',';
      strcpy (new_name + len, p + 1);
    }
  else
    {
      new_name[0] = ',';
      strcpy (new_name + 1, file_name);
    }
  WATCH ((_("Renaming %s to %s"), file_name, new_name));

  if (unlink (new_name) && errno != ENOENT)
    mu_diag_funcall (MU_DIAG_ERROR, "unlink", new_name, errno);
  else if (rename (file_name, new_name))
    mu_error (_("cannot rename `%s' to `%s': %s"),
	      file_name, new_name, mu_strerror (errno));
  free (new_name);
}

int
_action_send (void *item, void *data)
{
  struct list_elt *elt = item;
  mu_message_t msg = elt->msg;
  int rc;
  mu_mailer_t mailer;
  mu_header_t hdr;
  size_t n;

  WATCH ((_("Getting message %s"), elt->file_name));

  if (mu_message_get_header (msg, &hdr) == 0)
    {
      char date[80];
      time_t t = time (NULL);
      struct tm *tm = localtime (&t);
      
      mu_strftime (date, sizeof date, "%a, %d %b %Y %H:%M:%S %z", tm);
      mu_header_set_value (hdr, MU_HEADER_DATE, date, 1);

      if (mu_header_get_value (hdr, MU_HEADER_FROM, NULL, 0, &n))
	{
	  char *from;
	  char *email = mu_get_user_email (NULL);
	  const char *pers = get_sender_personal ();
	  if (pers)
	    {
	      mu_asprintf (&from, "\"%s\" <%s>", pers, email);
	      free (email);
	    }
	  else
	    from = email;

	  mu_header_set_value (hdr, MU_HEADER_FROM, from, 1);
	  free (from);
	}
	  
      if (append_msgid
	  && mu_header_get_value (hdr, MU_HEADER_MESSAGE_ID, NULL, 0, &n))
	create_message_id (hdr);

      if (mu_header_get_value (hdr, MU_HEADER_X_MAILER, NULL, 0, &n))
	{
	  const char *p = mu_mhprop_get_value (mts_profile,
					       "x-mailer", "yes");

	  if (!strcmp (p, "yes"))
	    mu_header_set_value (hdr, MU_HEADER_X_MAILER,
				 DEFAULT_X_MAILER, 0);
	  else if (strcmp (p, "no"))
	    mu_header_set_value (hdr, MU_HEADER_X_MAILER, p, 0);
	}
    }
  
  expand_aliases (msg);
  fix_fcc (msg);
  fix_dcc (msg);
  
  mailer = open_mailer ();
  if (!mailer)
    return 1;

  WATCH ((_("Sending message %s"), elt->file_name));
  if (split_message)
    {
      struct timeval delay;
      delay.tv_sec = split_interval;
      delay.tv_usec = 0;
      rc = mu_mailer_send_fragments (mailer, msg,
				     split_size, &delay,
				     NULL, NULL);
    }
  else
    rc = mu_mailer_send_message (mailer, msg, NULL, NULL);
  if (rc)
    {
      mu_error(_("cannot send message: %s"), mu_strerror (rc));
      return 1;
    }

  WATCH ((_("Destroying the mailer")));
  mu_mailer_close (mailer);
  mu_mailer_destroy (&mailer);

  if (!keep_files)
    backup_file (elt->file_name);
  
  return 0;
}

static int
_add_to_mesg_list (size_t num, mu_message_t msg, void *data)
{
  char const *path = data;
  struct list_elt *elt;
  size_t uid;
  int rc;
  char *file_name;
  
  if (!mesg_list && mu_list_create (&mesg_list))
    {
      mu_error (_("cannot create message list"));
      return 1;
    }

  mu_message_get_uid (msg, &uid);
  file_name = mu_make_file_name (path, mu_umaxtostr (0, uid));
  if (!use_draft)
    {
      if (!mh_usedraft (file_name))
	exit (0);
    }
  
  elt = mu_alloc (sizeof *elt);
  elt->msg = msg;
  elt->file_name = file_name;
  rc = mu_list_append (mesg_list, elt);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_append", NULL, rc);
      exit (1);
    }
  return 0;
}

static void
addfolder (const char *folder, int argc, char **argv)
{
  mu_url_t url;
  const char *path;
  mu_msgset_t msgset;
  
  mu_mailbox_t mbox = mh_open_folder (folder, MU_STREAM_READ);
  if (!mbox)
    {
      mu_error (_("cannot open folder %s: %s"), folder,
		mu_strerror (errno));
      exit (1);
    }

  mh_msgset_parse (&msgset, mbox, argc, argv, "cur");
  if (!use_draft)
    {
      size_t count = 0;
      mu_msgset_count (msgset, &count);
      if (count > 1)
	use_draft = 1;
    }

  mu_mailbox_get_url (mbox, &url);
  mu_url_sget_path (url, &path);
  mu_msgset_foreach_message (msgset, _add_to_mesg_list, (void*)path);
      
  mu_msgset_free (msgset);
}

/* Usage cases:
 *
 * 1. send
 *   a) If Draft-Folder is set: ask whether to use "cur" message from that
 *      folder as a draft;
 *   b) If Draft-Folder is not set: ask whether to use $(Path)/draft;
 * 2. send -draft
 *   Use $(Path)/draft
 * 3. send MSG
 *   Use $(Path)/MSG
 * 4. send -draftmessage MSG
 *   Same as (3)
 * 5. send -draftfolder DIR
 *   Use "cur" from that folder
 * 6. send -draftfolder DIR MSG
 *   Use MSG from folder DIR
 * 7. send -draftfolder DIR -draftmessage MSG
 *   Same as 6.
 */

int
main (int argc, char **argv)
{
  mu_mailbox_t mbox = NULL;
  char *p;
  int rc;
  
  MU_APP_INIT_NLS ();
  
  mh_getopt (&argc, &argv, options, 0, args_doc, prog_doc, NULL);

  mh_read_aliases ();
  /* Process the mtstailor file */
  read_mts_profile ();
 
  if (!draftfolder)
    {
      if (mu_list_is_empty (mesg_list) && argc == 0)
	{
	  char *dfolder =
	    (!use_draft && use_draftfolder)
	                ? (char*) mh_global_profile_get ("Draft-Folder", NULL)
	                : NULL;

	  if (dfolder)
	    addfolder (dfolder, 0, NULL);
	  else
	    {
	      char *df = mh_expand_name (mu_folder_directory (), "draft", 
	                                 NAME_ANY);
	      if (checkdraft (df))
		exit (1);
	      if (!use_draft && !mh_usedraft (df))
		exit (0);
	      add_file (df);
	      mesg_list_fixup ();
	    }
	}
      else
	{
	  while (argc--)
	    add_file (*argv++);
	  mesg_list_fixup ();
	}
    }
  else
    {
      /* -draftfolder is supplied */
      draftfolder = mh_expand_name (mu_folder_directory (),
				    draftfolder, NAME_ANY);
      use_draft = 1;
      mesg_list_fixup ();
      if (mu_list_is_empty (mesg_list) || argc != 0)
	addfolder (draftfolder, argc, argv);
    }
  
  /* Detach from the console if required */
  if (background && daemon (0, 0) < 0)
    {
      mu_error (_("cannot switch to background: %s"), mu_strerror (errno));
      return 1;
    }

  /* Prepend url specifier to the folder dir. We won't need this
     when the default format becomes configurable */
  mu_asprintf (&p, "mh:%s", mu_folder_directory ());
  mu_set_folder_directory (p);
  free (p);
  
  /* Finally, do the work */
  rc = mu_list_foreach (mesg_list, _action_send, NULL);

  mu_mailbox_destroy (&mbox);
  return !!rc;
}


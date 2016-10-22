/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2013-2016 Free Software Foundation, Inc.

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

#include "mh.h"
#include "mailutils/kwd.h"
#include "mailutils/folder.h"
#include "mailutils/auth.h"
#include "mailutils/datetime.h"
#include <pwd.h>

static char prog_doc[] = N_("Check for messages");
static char args_doc[] = N_("USER [USER...]");

int date_option = 1;
int apop_option;
char *remote_host;
char *remote_user;

#define NOTIFY_MAIL   0x1
#define NOTIFY_NOMAIL 0x2
#define NOTIFY_ALL    (NOTIFY_MAIL|NOTIFY_NOMAIL)

int notify = NOTIFY_ALL;

static struct mu_kwd notifytab[] = {
  { "mail", NOTIFY_MAIL },
  { "nomail", NOTIFY_NOMAIL },
  { "all", NOTIFY_ALL },
  { NULL }
};

static void
set_notify (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  int n;
  if (mu_kwd_xlat_name (notifytab, arg, &n))
    {
      mu_parseopt_error (po, "unknown notify argument: %s", arg);
      exit (po->po_exit_error);
    }
  notify |= n;
}

static void
clr_notify (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  int n;
  if (mu_kwd_xlat_name (notifytab, arg, &n))
    {
      mu_parseopt_error (po, "unknown notify argument: %s", arg);
      exit (po->po_exit_error);
    }
  notify &= ~n;
}
  
static struct mu_option options[] = {
  { "date",        0,    NULL, MU_OPTION_DEFAULT,
    N_("print out the last date mail was read"),
    mu_c_bool, &date_option },
  { "notify",      0, "all|mail|nomail", MU_OPTION_DEFAULT,
    N_("produce a message upon these events"),
    mu_c_string, NULL, set_notify },
  { "nonotify",    0, "all|mail|nomail", MU_OPTION_DEFAULT,
    N_("disable notification"),
    mu_c_string, NULL, clr_notify },
  { "host",        0,  N_("URL"), MU_OPTION_DEFAULT,
    N_("check mail on this host or URL"),
    mu_c_string, &remote_host },
  { "user",        0,  N_("NAME"), MU_OPTION_DEFAULT,
    N_("set user name for remote mailbox access"),
    mu_c_string, &remote_user },
  { "apop",        0,  NULL, MU_OPTION_DEFAULT,
    N_("enable APOP"),
    mu_c_bool, &apop_option },
  
  MU_OPTION_END
};

static char *
attach_auth_ticket (mu_mailbox_t mbox)
{
  char *filename = NULL;
  mu_folder_t folder = NULL;
  mu_authority_t auth = NULL;

  if (mu_mailbox_get_folder (mbox, &folder) == 0
      && mu_folder_get_authority (folder, &auth) == 0
      && auth)
    {
      mu_wicket_t wicket;
      int rc;
  
      filename = mu_tilde_expansion (mu_ticket_file, MU_HIERARCHY_DELIMITER,
				     NULL);
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
		("Reading user ticket file %s", filename));
      
      if ((rc = mu_file_wicket_create (&wicket, filename)) == 0)
	{
	  mu_ticket_t ticket;
      
	  if ((rc = mu_wicket_get_ticket (wicket, NULL, &ticket)) == 0)
	    {
	      rc = mu_authority_set_ticket (auth, ticket);
	      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
			("Retrieved and set ticket: %d", rc));
	    }
	  else
	    mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		      ("Error retrieving ticket: %s\n",
		       mu_strerror (rc)));
	  mu_wicket_destroy (&wicket);
	}
      else
        mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		  ("Error creating wicket: %s\n", mu_strerror (rc)));
      if (rc)
	{
	  free (filename);
	  filename = NULL;
	}
    }
  return filename;
}

int
checkmail (const char *username, int personal)
{
  int rc;
  mu_url_t url;
  mu_mailbox_t mbox;
  size_t recent = 0, count = 0;
  int status = 1;
  int have_user;
  
  if (remote_host)
    {
      static mu_url_t pop_hint;

      if (!pop_hint)
	{
	  rc = mu_url_create (&pop_hint, "pop://");
	  if (rc)
	    {
	      mu_error ("cannot create URL hint: %s", mu_strerror (rc));
	      exit (2);
	    }
	}
      
      rc = mu_url_create_hint (&url, remote_host, MU_URL_PARSE_DEFAULT,
			       pop_hint);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_url_create_hint", remote_host,
			   rc);
	  exit (2);
	}
      
      if (apop_option)
	{
	  rc = mu_url_set_auth (url, "+APOP");
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_url_set_auth", "+APOP", rc);
	      exit (2);
	    }
	}
    }
  else
    {
      char *s;
      rc = mu_construct_user_mailbox_url (&s, username);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_construct_user_mailbox_url",
			   username, rc);
	  exit (2);
	}
      
      rc = mu_url_create (&url, s);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_url_create", s, rc);
	  exit (2);
	}
      free (s);
    }
  
  rc = mu_mailbox_create_from_url (&mbox, url);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_create_from_url",
		       mu_url_to_string (url), rc);
      exit (2);
    }

  if (personal)
    {
      char *filename = attach_auth_ticket (mbox);

      have_user = mu_url_has_flag (url, MU_URL_USER);
      if (!have_user)
	{
	  mu_url_t tickurl;

	  if (mu_wicket_file_match_url (filename, url, MU_URL_PARSE_ALL,
					&tickurl) == 0)
	    {
	      have_user = mu_url_has_flag (tickurl, MU_URL_USER);
	      mu_url_destroy (&tickurl);
	    }
	}
      free (filename);
    }
  else
    have_user = 0;

  if (!have_user)
    {
      mu_url_t turl;

      mu_mailbox_get_url (mbox, &turl);
      rc = mu_url_set_user (turl, username);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_url_set_user", username, rc);
	  exit (2);
	}
    }
  
  rc = mu_mailbox_open (mbox, MU_STREAM_READ);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_open",
		       mu_url_to_string (url), rc);
      exit (2);
    }
  
  rc = mu_mailbox_messages_count (mbox, &count);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_messages_count",
		     mu_url_to_string (url), rc);
  if (count)
    {
      status = 0;

      if (notify & NOTIFY_MAIL)
	{
	  mu_off_t mbsiz = 0;
	  int newmail = 0;
	  
	  rc = mu_mailbox_message_unseen (mbox, &recent);
	  switch (rc)
	    {
	    case 0:
	      newmail = 1;
	      break;
	      
	    case MU_ERR_NOENT:
	      newmail = 0;
	      break;

	    default:
	      if (rc != ENOSYS && rc != MU_ERR_INFO_UNAVAILABLE)
		mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_messages_unseen",
				 mu_url_to_string (url), rc);
	      rc = mu_mailbox_messages_recent (mbox, &recent);
	      if (rc == 0)
		newmail = recent > 0;
	    }

	  if (rc)
	    {
	      if (rc != ENOSYS && rc != MU_ERR_INFO_UNAVAILABLE)
		mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_messages_recent",
				 mu_url_to_string (url), rc);

	      mu_mailbox_get_size (mbox, &mbsiz);
	      if (personal)
		mu_printf (ngettext ("You have %lu message",
				     "You have %lu messages",
				     count),
			   (unsigned long) count);
	      else
		mu_printf (ngettext ("%s has %lu message",
				     "%s has %lu messages",
				     count),
			   username, (unsigned long) count);
	      mu_printf (ngettext (" (%lu byte)",
				   " (%lu bytes)",
				   mbsiz), (unsigned long) mbsiz);
	    }
	  else
	    {
	      if (personal)
		mu_printf (newmail ? _("You have new mail waiting") :
			             _("You have old mail waiting"));
	      else
		mu_printf (newmail ? _("%s has new mail waiting") :
			             _("%s has old mail waiting"), username);
	    }

	  if (date_option)
	    {
	      time_t t;

	      if (mu_mailbox_access_time (mbox, &t) == 0)
		mu_c_streamftime (mu_strout,
				  _("; last read on %a, %d %b %Y %H:%M:%S %z"),
				  localtime (&t), NULL);
	    }
	      
	  mu_printf ("\n");
	}
    }
  else
    {
      status = 1;

      if (notify & NOTIFY_NOMAIL)
	{
	  if (personal)
	    mu_printf (_("You don't have any mail waiting\n"));
	  else
	    mu_printf (_("%s doesn't have any mail waiting\n"),
		       username);
	}
    }
  mu_mailbox_destroy (&mbox);
  
  return status;
}

int
main (int argc, char **argv)
{
  int rc = 0;
  
  mh_getopt (&argc, &argv, options, 0, args_doc, prog_doc, NULL);  

  if (argc == 0)
    {
      if (remote_user)
	rc |= checkmail (remote_user, 1);
      else
	{
	  struct passwd *pw = getpwuid (getuid ());
	  if (!pw)
	    {
	      mu_error (_("cannot determine my username"));
	      return 2;
	    }
	  rc |= checkmail (pw->pw_name, 1);
	}
    }
  else if (remote_user)
    {
      mu_error (_("no arguments are allowed when the -user option is given"));
      return 2;
    }
  else
    while (argc--)
      rc |= checkmail (*argv++, 0);

  return rc;
}
  



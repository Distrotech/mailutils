/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005, 2006, 2007, 2008 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <mailutils/mailutils.h>
#include <mailutils/tls.h>
#include <mu_asprintf.h>
#include "mailutils/libargp.h"

const char *program_version = "movemail (" PACKAGE_STRING ")";
static char doc[] = N_("GNU movemail");
static char args_doc[] = N_("inbox-url destfile [POP-password]");

#define OPT_EMACS 256

static struct argp_option options[] = {
  { "preserve", 'p', NULL, 0, N_("Preserve the source mailbox"), 0 },
  { "keep-messages", 0, NULL, OPTION_ALIAS, NULL },
  { "reverse",  'r', NULL, 0, N_("Reverse the sorting order"), 0 },
  { "emacs", OPT_EMACS, NULL, 0,
    N_("Output information used by Emacs rmail interface"), 0 },
  { NULL,      0, NULL, 0, NULL, 0 }
};

static int reverse_order;
static int preserve_mail; 
static int emacs_mode;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  static struct mu_argp_node_list lst;

  switch (key)
    {
    case 'r':
      mu_argp_node_list_new (&lst, "reverse", "yes");
      break;

    case 'p':
      mu_argp_node_list_new (&lst, "preserve", arg);
      break;

    case OPT_EMACS:
      mu_argp_node_list_new (&lst, "emacs", arg);
      break;
      
    case ARGP_KEY_INIT:
      mu_argp_node_list_init (&lst);
      break;
      
    case ARGP_KEY_FINI:
      mu_argp_node_list_finish (&lst, NULL, NULL);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  args_doc,
  doc,
  NULL,
  NULL, NULL
};


struct mu_cfg_param movemail_cfg_param[] = {
  { "preserve", mu_cfg_bool, &preserve_mail, 0, NULL,
    N_("Do not remove messages from the source mailbox.") },
  { "reverse",  mu_cfg_bool, &reverse_order, 0, NULL,
    N_("Reverse message sorting order.") },
  { "emacs", mu_cfg_bool, &emacs_mode, 0, NULL,
    N_("Output information used by Emacs rmail interface.") },
  { NULL }
};


static const char *movemail_capa[] = {
  "common",
  "debug",
  "license",
  "locking",
  "mailbox",
  NULL 
};

void
die (mu_mailbox_t mbox, const char *msg, int status)
{
  mu_url_t url = NULL;
  
  mu_mailbox_get_url (mbox, &url);
  if (emacs_mode)
    mu_error (_("%s:mailbox `%s': %s: %s"),
	      mu_errname (status),
	      mu_url_to_string (url),
	      msg,
	      mu_strerror (status));
  else
    mu_error (_("mailbox `%s': %s: %s"),
	      mu_url_to_string (url), msg, mu_strerror (status));
  exit (1);
}

void
lock_mailbox (mu_mailbox_t mbox)
{
  mu_locker_t lock;
  int status;
  
  status = mu_mailbox_get_locker (mbox, &lock);
  if (status)
    die (mbox, _("Cannot retrieve locker"), status);
      
  if (!lock)
    /* Remote mailboxes have no lockers */
    return;

  status = mu_locker_lock (lock);

  if (status)
    die (mbox, _("Cannot lock"), status);
}


/* A password ticket: returns the cleantext password. */
void
password_destroy (mu_ticket_t t)
{
  char *p = mu_ticket_get_owner (t);
  free (p);
}

int
password_pop (mu_ticket_t t, mu_url_t u, const char *challenge, char **ppwd)
{
  char *p = mu_ticket_get_owner (t);
  *ppwd = strdup (p);
  return 0;
}

void
attach_passwd_ticket (mu_mailbox_t mbx, char *passwd)
{
  mu_folder_t folder = NULL;
  mu_authority_t auth = NULL;
  char *p = strdup (passwd);
  mu_ticket_t t;
  int rc;
  
  mu_ticket_create (&t, p);
  mu_ticket_set_destroy (t, password_destroy, p);
  mu_ticket_set_pop (t, password_pop, p);

  if ((rc = mu_mailbox_get_folder (mbx, &folder)))
    die (mbx, _("mu_mailbox_get_folder failed"), rc);

  if ((rc = mu_folder_get_authority (folder, &auth)))
    die (mbx, _("mu_folder_get_authority failed"), rc);

  if (auth && (rc = mu_authority_set_ticket (auth, t)))
    die (mbx, _("mu_authority_set_ticket failed"), rc);
}


/* Create and open a mailbox associated with the given URL,
   flags and (optionally) password */
void
open_mailbox (mu_mailbox_t *mbx, char *name, int flags, char *passwd)
{
  int status = mu_mailbox_create_default (mbx, name);

  if (status)
    {
      if (name)
	mu_error (_("Could not create mailbox `%s': %s"),
		  name,
		  mu_strerror (status));
      else
	mu_error (_("Could not create default mailbox: %s"),
		  mu_strerror (status));
      exit (1);
    }

  if (passwd)
    attach_passwd_ticket (*mbx, passwd);
  status = mu_mailbox_open (*mbx, flags);
  if (status)
    die (*mbx, _("Cannot open"), status);
  lock_mailbox (*mbx);
}

int
move_message (mu_mailbox_t src, mu_mailbox_t dst, size_t msgno)
{
  int rc;
  mu_message_t msg;

  if ((rc = mu_mailbox_get_message (src, msgno, &msg)) != 0)
    {
      fprintf (stderr, _("Cannot read message %lu: %s\n"),
	       (unsigned long) msgno, mu_strerror (rc));
      return rc;
    }
  if ((rc = mu_mailbox_append_message (dst, msg)) != 0)
    {
      fprintf (stderr, _("Cannot append message %lu: %s\n"),
	       (unsigned long) msgno, mu_strerror (rc));
      return rc;
    }
  if (!preserve_mail)
    {
      mu_attribute_t attr;
      mu_message_get_attribute (msg, &attr);
      mu_attribute_set_deleted (attr);
    }
  return rc;
}

/* Open source mailbox using compatibility syntax. Source_name is
   of the form:

     po:USERNAME[:POP-SERVER]

   if POP-SERVER part is omitted, the MAILHOST environment variable
   will be consulted. */
void
compatibility_mode (mu_mailbox_t *mbx, char *source_name, char *password,
		    int flags)
{
  char *tmp;
  char *user_name = strtok (source_name+3, ":");
  char *host = strtok (NULL, ":");
  if (!host)
    host = getenv ("MAILHOST");
  if (!host)
    {
      mu_error (_("Hostname of the POP3 server is unknown"));
      exit (1);
    }
  asprintf (&tmp, "pop://%s@%s", user_name, host);
  open_mailbox (mbx, tmp, flags, password);
  free (tmp);
}

static mu_mailbox_t source, dest;

static void
close_mailboxes (void)
{
  mu_mailbox_close (dest);
  mu_mailbox_close (source);
}  

int
main (int argc, char **argv)
{
  int index;
  size_t i, total;
  int rc = 0;
  char *source_name, *dest_name;
  int flags;
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();
  /* Register the desired mailbox formats.  */
  mu_register_all_mbox_formats ();

  /* argument parsing */
  
#ifdef WITH_TLS
  mu_gocs_register ("tls", mu_tls_module_init);
#endif
  mu_argp_init (program_version, NULL);
  if (mu_app_init (&argp, movemail_capa, movemail_cfg_param, 
		   argc, argv, 0, NULL, NULL))
    exit (1);

  argc -= index;
  argv += index;

  if (argc < 2 || argc > 3)
    {
      mu_error (_("Wrong number of arguments"));
      return 1;
    }

  atexit (close_mailboxes);
  
  source_name = argv[0];
  dest_name = argv[1];

  flags = preserve_mail ? MU_STREAM_READ : MU_STREAM_RDWR;
  
  if (strncmp (source_name, "po:", 3) == 0)
    compatibility_mode (&source, source_name, argv[2], flags);
  else
    open_mailbox (&source, source_name, flags, argv[2]);
  
  open_mailbox (&dest, dest_name, MU_STREAM_RDWR | MU_STREAM_CREAT, NULL);
  
  mu_mailbox_messages_count (source, &total);
  if (reverse_order)
    {
      for (i = total; rc == 0 && i > 0; i--)
	rc = move_message (source, dest, i);
    }
  else
    {
      for (i = 1; rc == 0 && i <= total; i++)
	rc = move_message (source, dest, i);
    }
  
  if (rc)
    return rc;
  
  rc = mu_mailbox_close (dest);
  mu_mailbox_destroy (&dest);
  if (rc)
    mu_error (_("Cannot close destination mailbox: %s"), mu_strerror (rc));
  else
    mu_mailbox_flush (source, 1);

  mu_mailbox_close (source);
  mu_mailbox_destroy (&source);

  return rc;
}

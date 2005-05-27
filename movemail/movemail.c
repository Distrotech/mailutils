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

const char *program_version = "movemail (" PACKAGE_STRING ")";
static char doc[] = N_("GNU movemail");
static char args_doc[] = N_("inbox-url destfile [POP-password]");

#define OPT_EMACS 256

static struct argp_option options[] = {
  { "preserve", 'p', NULL, 0, N_("Preserve the source mailbox"), 0 },
  { "keep-messages", 0, NULL, OPTION_ALIAS, NULL },
  { "reverse",  'r', NULL, 0, N_("Reverse the sorting order"), 0 },
  { "emacs", OPT_EMACS, NULL, 0, N_("Output information used by Emacs rmail interface"), 0 },
  { NULL,      0, NULL, 0, NULL, 0 }
};

static int reverse_order;
static int preserve_mail; 
static int emacs_mode;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'r':
      reverse_order++;
      break;

    case 'p':
      preserve_mail++;
      break;

    case OPT_EMACS:
      emacs_mode++;
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

static const char *mail_capa[] = {
	"common",
	"license",
	"mailbox",
#ifdef WITH_TLS
	"tls",
#endif
	 NULL 
};

int
movemail_error_printer (const char *fmt, va_list ap)
{
  int n;
  
  n = fprintf (stderr, "%s: ", program_invocation_short_name);
  n += vfprintf (stderr, fmt, ap);
  fputc ('\n', stderr);
  return n + 1;
}

void
die (mailbox_t mbox, char *msg, int status)
{
  url_t url = NULL;
  
  mailbox_get_url (mbox, &url);
  if (emacs_mode)
    mu_error (_("%s:mailbox `%s': %s: %s"),
	      mu_errname (status),
	      url_to_string (url),
	      msg,
	      mu_strerror (status));
  else
    mu_error (_("mailbox `%s': %s: %s"),
	      url_to_string (url), msg, mu_strerror (status));
  exit (1);
}

void
lock_mailbox (mailbox_t mbox)
{
  locker_t lock;
  int status;
  
  status = mailbox_get_locker (mbox, &lock);
  if (status)
    die (mbox, _("Cannot retrieve locker"), status);
      
  if (!lock)
    /* Remote mailboxes have no lockers */
    return;

  status = locker_lock (lock);

  if (status)
    die (mbox, _("Cannot lock"), status);
}


/* A password ticket: returns the cleantext password. */
void
password_destroy (ticket_t t)
{
  char *p = ticket_get_owner (t);
  free (p);
}

int
password_pop (ticket_t t, url_t u, const char *challenge, char **ppwd)
{
  char *p = ticket_get_owner (t);
  *ppwd = strdup (p);
  return 0;
}

void
attach_passwd_ticket (mailbox_t mbx, char *passwd)
{
  folder_t folder = NULL;
  authority_t auth = NULL;
  char *p = strdup (passwd);
  ticket_t t;
  int rc;
  
  ticket_create (&t, p);
  ticket_set_destroy (t, password_destroy, p);
  ticket_set_pop (t, password_pop, p);

  if ((rc = mailbox_get_folder (mbx, &folder)))
    die (mbx, _("mailbox_get_folder failed"), rc);

  if ((rc = folder_get_authority (folder, &auth)))
    die (mbx, _("folder_get_authority failed"), rc);

  if (auth && (rc = authority_set_ticket (auth, t)))
    die (mbx, _("authority_set_ticket failed"), rc);
}


/* Create and open a mailbox associated with the given URL,
   flags and (optionally) password */
void
open_mailbox (mailbox_t *mbx, char *name, int flags, char *passwd)
{
  int status = mailbox_create_default (mbx, name);

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
  status = mailbox_open (*mbx, flags);
  if (status)
    die (*mbx, _("Cannot open"), status);
  lock_mailbox (*mbx);
}

int
move_message (mailbox_t src, mailbox_t dst, size_t msgno)
{
  int rc;
  message_t msg;

  if ((rc = mailbox_get_message (src, msgno, &msg)) != 0)
    {
      fprintf (stderr, _("Cannot read message %lu: %s\n"),
	       (unsigned long) msgno, mu_strerror (rc));
      return rc;
    }
  if ((rc = mailbox_append_message (dst, msg)) != 0)
    {
      fprintf (stderr, _("Cannot append message %lu: %s\n"),
	       (unsigned long) msgno, mu_strerror (rc));
      return rc;
    }
  if (!preserve_mail)
    {
      attribute_t attr;
      message_get_attribute (msg, &attr);
      attribute_set_deleted (attr);
    }
  return rc;
}

/* Open source mailbox using compatibility syntax. Source_name is
   of the form:

     po:USERNAME[:POP-SERVER]

   if POP-SERVER part is omitted, the MAILHOST environment variable
   will be consulted. */
void
compatibility_mode (mailbox_t *mbx, char *source_name, char *password,
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

int
main (int argc, char **argv)
{
  int index;
  mailbox_t source, dest;
  size_t i, total;
  int rc = 0;
  char *source_name, *dest_name;
  int flags;
  
  /* Native Language Support */
  mu_init_nls ();
  /* Register the desired mailbox formats.  */
  mu_register_all_mbox_formats ();

  /* argument parsing */

  mu_error_set_print (movemail_error_printer);
  
  mu_argp_init (program_version, NULL);
#ifdef WITH_TLS
  mu_tls_init_client_argp ();
#endif
  mu_argp_parse (&argp, &argc, &argv, 0, mail_capa, &index, NULL);

  argc -= index;
  argv += index;

  if (argc < 2 || argc > 3)
    {
      mu_error (_("Wrong number of arguments"));
      return 1;
    }

  source_name = argv[0];
  dest_name = argv[1];

  flags = preserve_mail ? MU_STREAM_READ : MU_STREAM_RDWR;
  
  if (strncmp (source_name, "po:", 3) == 0)
    compatibility_mode (&source, source_name, argv[2], flags);
  else
    open_mailbox (&source, source_name, flags, argv[2]);
  
  open_mailbox (&dest, dest_name, MU_STREAM_RDWR | MU_STREAM_CREAT, NULL);
  
  mailbox_messages_count (source, &total);
  if (reverse_order)
    {
      for (i = total; rc == 0 && i > 0; i--)
	move_message (source, dest, i);
    }
  else
    {
      for (i = 1; rc == 0 && i <= total; i++)
	move_message (source, dest, i);
    }
  if (rc)
    return rc;
  mailbox_flush (source, 1);
  mailbox_close (source);
  mailbox_destroy (&source);
  mailbox_close (dest);
  mailbox_destroy (&dest);
  return 0;
}

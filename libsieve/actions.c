/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>  
#include <sieve.h>

int
sieve_action_stop (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_sieve_log_action (mach, "STOP", NULL);
  mach->pc = 0;
  return 0;
}

int
sieve_action_keep (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_sieve_log_action (mach, "KEEP", NULL);
  if (mu_sieve_is_dry_run (mach))
    return 0;
  sieve_mark_deleted (mach->msg, 0);
  return 0;
}

int
sieve_action_discard (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_sieve_log_action (mach, "DISCARD", _("marking as deleted"));
  if (mu_sieve_is_dry_run (mach))
    return 0;
  sieve_mark_deleted (mach->msg, 1);
  return 0;
}

int
sieve_action_fileinto (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  int rc;
  mu_sieve_value_t *val = mu_sieve_value_get (args, 0);
  if (!val)
    {
      mu_sieve_error (mach, _("cannot get filename!"));
      mu_sieve_abort (mach);
    }
  mu_sieve_log_action (mach, "FILEINTO", _("delivering into %s"), val->v.string);
  if (mu_sieve_is_dry_run (mach))
    return 0;

  rc = mu_message_save_to_mailbox (mach->msg, mach->ticket, mach->debug,
				val->v.string);
  if (rc)
    mu_sieve_error (mach, _("cannot save to mailbox: %s"),
		 mu_strerror (rc));
  else
    sieve_mark_deleted (mach->msg, 1);    
  
  return rc;
}

int
mu_stream_printf (mu_stream_t stream, size_t *off, const char *fmt, ...)
{
  va_list ap;
  char *buf = NULL;
  size_t size, bytes;
  int rc;
  
  va_start (ap, fmt);
  vasprintf (&buf, fmt, ap);
  va_end (ap);
  size = strlen (buf);
  rc = mu_stream_write (stream, buf, size, *off, &bytes);
  if (rc)
    return rc;
  *off += bytes;
  if (bytes != size)
    return EIO;
  return 0;
}

int
mu_sieve_get_message_sender (mu_message_t msg, char **ptext)
{
  int rc;
  mu_envelope_t envelope;
  char *text;
  size_t size;
  
  rc = mu_message_get_envelope (msg, &envelope);
  if (rc)
    return rc;
  
  rc = mu_envelope_sender (envelope, NULL, 0, &size);
  if (rc == 0)
    {
      if (!(text = malloc (size + 1)))
	return ENOMEM;
      mu_envelope_sender (envelope, text, size + 1, NULL);
    }
  else
    {
      mu_header_t hdr = NULL;
      mu_message_get_header (msg, &hdr);
      if (rc = mu_header_aget_value (hdr, MU_HEADER_SENDER, &text))
	rc = mu_header_aget_value (hdr, MU_HEADER_FROM, &text);
    }

  if (rc == 0)
    *ptext = text;
  return rc;
}

static int
build_mime (mu_mime_t *pmime, mu_message_t msg, const char *text)
{
  mu_mime_t mime = NULL;
  char datestr[80];
  
  mu_mime_create (&mime, NULL, 0);
  {
    mu_message_t newmsg;
    mu_stream_t stream;
    time_t t;
    struct tm *tm;
    char *sender;
    size_t off = 0;
    mu_body_t body;
      
    mu_message_create (&newmsg, NULL);
    mu_message_get_body (newmsg, &body);
    mu_body_get_stream (body, &stream);

    time (&t);
    tm = localtime (&t);
    strftime (datestr, sizeof datestr, "%a, %b %d %H:%M:%S %Y %Z", tm);

    mu_sieve_get_message_sender (msg, &sender);

    mu_stream_printf (stream, &off,
		 "\nThe original message was received at %s from %s.\n",
		 datestr, sender);
    free (sender);
    mu_stream_printf (stream, &off,
		   "Message was refused by recipient's mail filtering program.\n");
    mu_stream_printf (stream, &off, "Reason given was as follows:\n\n");
    mu_stream_printf (stream, &off, "%s", text);
    mu_stream_close (stream);
    mu_mime_add_part (mime, newmsg);
    mu_message_unref (newmsg);
  }
  
  /*  message/delivery-status */
  {
    mu_message_t newmsg;
    mu_stream_t stream;
    mu_header_t hdr;
    size_t off = 0;
    mu_body_t body;
    char *email;
    
    mu_message_create (&newmsg, NULL);
    mu_message_get_header (newmsg, &hdr); 
    mu_header_set_value (hdr, "Content-Type", "message/delivery-status", 1);
    mu_message_get_body (newmsg, &body);
    mu_body_get_stream (body, &stream);
    mu_stream_printf (stream, &off, "Reporting-UA: sieve; %s\n", PACKAGE_STRING);
    mu_stream_printf (stream, &off, "Arrival-Date: %s\n", datestr);
    email = mu_get_user_email (NULL);
    mu_stream_printf (stream, &off, "Final-Recipient: RFC822; %s\n",
		   email ? email : "unknown");
    free (email);
    mu_stream_printf (stream, &off, "Action: deleted\n");
    mu_stream_printf (stream, &off, 
		 "Disposition: automatic-action/MDN-sent-automatically;deleted\n");
    mu_stream_printf (stream, &off, "Last-Attempt-Date: %s\n", datestr);
    mu_stream_close (stream);
    mu_mime_add_part(mime, newmsg);
    mu_message_unref (newmsg);
  }
  
  /* Quote original message */
  {
    mu_message_t newmsg;
    mu_stream_t istream, ostream;
    mu_header_t hdr;
    size_t ioff = 0, ooff = 0, n;
    char buffer[512];
    mu_body_t body;
    
    mu_message_create (&newmsg, NULL);
    mu_message_get_header (newmsg, &hdr); 
    mu_header_set_value (hdr, "Content-Type", "message/rfc822", 1);
    mu_message_get_body (newmsg, &body);
    mu_body_get_stream (body, &ostream);
    mu_message_get_stream (msg, &istream);
  
    while (mu_stream_read (istream, buffer, sizeof buffer - 1, ioff, &n) == 0
	   && n != 0)
      {
	size_t sz;
	mu_stream_write (ostream, buffer, n, ooff, &sz);
	if (sz != n)
	  return EIO;
	ooff += n;
	ioff += n;
      }
    mu_stream_close (ostream);
    mu_mime_add_part (mime, newmsg);
    mu_message_unref (newmsg);
  }

  *pmime = mime;
  
  return 0;
}

int
sieve_action_reject (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_mime_t mime = NULL;
  mu_mailer_t mailer = mu_sieve_get_mailer (mach);
  int rc;
  mu_message_t newmsg;
  char *addrtext;
  mu_address_t from, to;
  
  mu_sieve_value_t *val = mu_sieve_value_get (args, 0);
  if (!val)
    {
      mu_sieve_error (mach, _("reject: cannot get text!"));
      mu_sieve_abort (mach);
    }
  mu_sieve_log_action (mach, "REJECT", NULL);  
  if (mu_sieve_is_dry_run (mach))
    return 0;

  rc = build_mime (&mime, mach->msg, val->v.string);

  mu_mime_get_message (mime, &newmsg);

  mu_sieve_get_message_sender (mach->msg, &addrtext);
  rc = mu_address_create (&to, addrtext);
  if (rc)
    {
      mu_sieve_error (mach,
		   _("%d: cannot create recipient address <%s>: %s"),
		   mu_sieve_get_message_num (mach),
		   addrtext, mu_strerror (rc));
      free (addrtext);
      goto end;
    }
  free (addrtext);
  
  rc = mu_address_create (&from, mu_sieve_get_daemon_email (mach));
  if (rc)
    {
      mu_sieve_error (mach,
		   _("%d: cannot create sender address <%s>: %s"),
		   mu_sieve_get_message_num (mach),
		   mu_sieve_get_daemon_email (mach),
		   mu_strerror (rc));
      goto end;
    }
  
  rc = mu_mailer_open (mailer, 0);
  if (rc)
    {
      mu_url_t url = NULL;
      mu_mailer_get_url (mailer, &url);
	
      mu_sieve_error (mach,
		   _("%d: cannot open mailer %s: %s"),
		   mu_sieve_get_message_num (mach),
		   mu_url_to_string (url),
		   mu_strerror (rc));
      goto end;
    }

  rc = mu_mailer_send_message (mailer, newmsg, from, to);
  mu_mailer_close (mailer);

 end:
  sieve_mark_deleted (mach->msg, rc == 0);    
  mu_mime_destroy (&mime);
  mu_address_destroy (&from);
  mu_address_destroy (&to);
  
  return rc;
}

/* rfc3028 says: 
   "Implementations SHOULD take measures to implement loop control,"
    We do this by appending an "X-Loop-Prevention" header to each message
    being redirected. If one of the "X-Loop-Prevention" headers of the message
    contains our email address, we assume it is a loop and bail out. */

static int
check_redirect_loop (mu_message_t msg)
{
  mu_header_t hdr = NULL;
  size_t i, num = 0;
  char buf[512];
  int loop = 0;
  char *email = mu_get_user_email (NULL);
  
  mu_message_get_header (msg, &hdr);
  mu_header_get_field_count (hdr, &num);
  for (i = 1; !loop && i <= num; i++)
    {
      mu_header_get_field_name (hdr, i, buf, sizeof buf, NULL);
      if (strcasecmp (buf, "X-Loop-Prevention") == 0)
	{
	  size_t j, cnt = 0;
	  mu_address_t addr;
	  
	  mu_header_get_field_value (hdr, i, buf, sizeof buf, NULL);
	  if (mu_address_create (&addr, buf))
	    continue;

	  mu_address_get_count (addr, &cnt);
	  for (j = 1; !loop && j <= cnt; j++)
	    {
	      mu_address_get_email (addr, j, buf, sizeof buf, NULL);
	      if (strcasecmp (buf, email) == 0)
		loop = 1;
	    }
	  mu_address_destroy (&addr);
	}
    }
  free (email);
  return loop;
}

int
sieve_action_redirect (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_message_t msg, newmsg = NULL;
  mu_address_t addr = NULL, from = NULL;
  mu_header_t hdr = NULL;
  int rc;
  char *fromaddr, *p;
  mu_mailer_t mailer = mu_sieve_get_mailer (mach);
  
  mu_sieve_value_t *val = mu_sieve_value_get (args, 0);
  if (!val)
    {
      mu_sieve_error (mach, _("cannot get address!"));
      mu_sieve_abort (mach);
    }

  rc = mu_address_create (&addr, val->v.string);
  if (rc)
    {
      mu_sieve_error (mach,
		   _("%d: parsing recipient address `%s' failed: %s"),
		   mu_sieve_get_message_num (mach),
		   val->v.string, mu_strerror (rc));
      return 1;
    }
  
  mu_sieve_log_action (mach, "REDIRECT", _("to %s"), val->v.string);
  if (mu_sieve_is_dry_run (mach))
    return 0;

  msg = mu_sieve_get_message (mach);
  if (check_redirect_loop (msg))
    {
      mu_sieve_error (mach, _("%d: Redirection loop detected"),
		   mu_sieve_get_message_num (mach));
      goto end;
    }

  rc = mu_sieve_get_message_sender (msg, &fromaddr);
  if (rc)
    {
      mu_sieve_error (mach,
		   _("%d: cannot get envelope sender: %s"),
		   mu_sieve_get_message_num (mach), mu_strerror (rc));
      goto end;
    }

  rc = mu_address_create (&from, fromaddr);
  if (rc)
    {
      mu_sieve_error (mach,
		   "redirect",
		   _("%d: cannot create sender address <%s>: %s"),
		   mu_sieve_get_message_num (mach),
		   fromaddr, mu_strerror (rc));
      free (fromaddr);
      goto end;
    }

  free (fromaddr);

  rc = mu_message_create_copy (&newmsg, msg);
  if (rc)
    {
      mu_sieve_error (mach,
		   _("%d: cannot copy message: %s"),
		   mu_sieve_get_message_num (mach),
		   mu_strerror (rc));
      goto end;
    }
  
  mu_message_get_header (newmsg, &hdr);
  p = mu_get_user_email (NULL);
  if (p)
    {
      mu_header_set_value (hdr, "X-Loop-Prevention", p, 0);
      free (p);
    }
  else
    {
      mu_sieve_error (mach, _("%d: cannot get my email address"),
		   mu_sieve_get_message_num (mach));
      goto end;
    }
  
  rc = mu_mailer_open (mailer, 0);
  if (rc)
    {
      mu_url_t url = NULL;
      mu_mailer_get_url (mailer, &url);
	
      mu_sieve_error (mach,
		   _("%d: cannot open mailer %s: %s"),
		   mu_sieve_get_message_num (mach),
		   mu_url_to_string (url),
		   mu_strerror (rc));
      goto end;
    }
  
  rc = mu_mailer_send_message (mailer, newmsg, from, addr);
  mu_mailer_close (mailer);
  
 end:
  sieve_mark_deleted (mach->msg, rc == 0);    
  mu_message_destroy (&newmsg, NULL);
  mu_address_destroy (&from);
  mu_address_destroy (&addr);
  
  return rc;
}

mu_sieve_data_type fileinto_args[] = {
  SVT_STRING,
  SVT_VOID
};

void
sieve_register_standard_actions (mu_sieve_machine_t mach)
{
  mu_sieve_register_action (mach, "stop", sieve_action_stop, NULL, NULL, 1);
  mu_sieve_register_action (mach, "keep", sieve_action_keep, NULL, NULL, 1);
  mu_sieve_register_action (mach, "discard", sieve_action_discard, NULL, NULL, 1);
  mu_sieve_register_action (mach, "fileinto", sieve_action_fileinto,
			 fileinto_args, NULL, 0);
  mu_sieve_register_action (mach, "reject", sieve_action_reject, fileinto_args,
			 NULL, 0);
  mu_sieve_register_action (mach, "redirect", sieve_action_redirect, 
			 fileinto_args, NULL, 0);
}


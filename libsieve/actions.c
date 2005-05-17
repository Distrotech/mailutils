/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
sieve_action_stop (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_log_action (mach, "STOP", NULL);
  mach->pc = 0;
  return 0;
}

int
sieve_action_keep (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_log_action (mach, "KEEP", NULL);
  if (sieve_is_dry_run (mach))
    return 0;
  sieve_mark_deleted (mach->msg, 0);
  return 0;
}

int
sieve_action_discard (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_log_action (mach, "DISCARD", _("marking as deleted"));
  if (sieve_is_dry_run (mach))
    return 0;
  sieve_mark_deleted (mach->msg, 1);
  return 0;
}

int
sieve_action_fileinto (sieve_machine_t mach, list_t args, list_t tags)
{
  int rc;
  sieve_value_t *val = sieve_value_get (args, 0);
  if (!val)
    {
      sieve_error (mach, _("cannot get filename!"));
      sieve_abort (mach);
    }
  sieve_log_action (mach, "FILEINTO", _("delivering into %s"), val->v.string);
  if (sieve_is_dry_run (mach))
    return 0;

  rc = message_save_to_mailbox (mach->msg, mach->ticket, mach->mu_debug,
				val->v.string);
  if (rc)
    sieve_error (mach, _("cannot save to mailbox: %s"),
		 mu_strerror (rc));
  else
    sieve_mark_deleted (mach->msg, 1);    
  
  return rc;
}

int
stream_printf (stream_t stream, size_t *off, const char *fmt, ...)
{
  va_list ap;
  char *buf = NULL;
  size_t size, bytes;
  int rc;
  
  va_start (ap, fmt);
  vasprintf (&buf, fmt, ap);
  va_end (ap);
  size = strlen (buf);
  rc = stream_write (stream, buf, size, *off, &bytes);
  if (rc)
    return rc;
  *off += bytes;
  if (bytes != size)
    return EIO;
  return 0;
}

int
sieve_get_message_sender (message_t msg, char **ptext)
{
  int rc;
  envelope_t envelope;
  char *text;
  size_t size;
  
  rc = message_get_envelope (msg, &envelope);
  if (rc)
    return rc;
  
  rc = envelope_sender (envelope, NULL, 0, &size);
  if (rc)
    return rc;

  if (!(text = malloc (size + 1)))
    return ENOMEM;
  envelope_sender (envelope, text, size + 1, NULL);
  *ptext = text;
  return 0;
}

static int
build_mime (mime_t *pmime, message_t msg, const char *text)
{
  mime_t mime = NULL;
  char datestr[80];
  
  mime_create (&mime, NULL, 0);
  {
    message_t newmsg;
    stream_t stream;
    time_t t;
    struct tm *tm;
    char *sender;
    size_t off = 0;
    body_t body;
      
    message_create (&newmsg, NULL);
    message_get_body (newmsg, &body);
    body_get_stream (body, &stream);

    time (&t);
    tm = localtime (&t);
    strftime (datestr, sizeof datestr, "%a, %b %d %H:%M:%S %Y %Z", tm);

    sieve_get_message_sender (msg, &sender);

    stream_printf (stream, &off,
		 "\nThe original message was received at %s from %s.\n",
		 datestr, sender);
    free (sender);
    stream_printf (stream, &off,
		   "Message was refused by recipient's mail filtering program.\n");
    stream_printf (stream, &off, "Reason given was as follows:\n\n");
    stream_printf (stream, &off, "%s", text);
    stream_close (stream);
    mime_add_part (mime, newmsg);
    message_unref (newmsg);
  }
  
  /*  message/delivery-status */
  {
    message_t newmsg;
    stream_t stream;
    header_t hdr;
    size_t off = 0;
    body_t body;
    char *email;
    
    message_create (&newmsg, NULL);
    message_get_header (newmsg, &hdr); 
    header_set_value (hdr, "Content-Type", "message/delivery-status", 1);
    message_get_body (newmsg, &body);
    body_get_stream (body, &stream);
    stream_printf (stream, &off, "Reporting-UA: sieve; %s\n", PACKAGE_STRING);
    stream_printf (stream, &off, "Arrival-Date: %s\n", datestr);
    email = mu_get_user_email (NULL);
    stream_printf (stream, &off, "Final-Recipient: RFC822; %s\n",
		   email ? email : "unknown");
    free (email);
    stream_printf (stream, &off, "Action: deleted\n");
    stream_printf (stream, &off, 
		 "Disposition: automatic-action/MDN-sent-automatically;deleted\n");
    stream_printf (stream, &off, "Last-Attempt-Date: %s\n", datestr);
    stream_close (stream);
    mime_add_part(mime, newmsg);
    message_unref (newmsg);
  }
  
  /* Quote original message */
  {
    message_t newmsg;
    stream_t istream, ostream;
    header_t hdr;
    size_t ioff = 0, ooff = 0, n;
    char buffer[512];
    body_t body;
    
    message_create (&newmsg, NULL);
    message_get_header (newmsg, &hdr); 
    header_set_value (hdr, "Content-Type", "message/rfc822", 1);
    message_get_body (newmsg, &body);
    body_get_stream (body, &ostream);
    message_get_stream (msg, &istream);
  
    while (stream_read (istream, buffer, sizeof buffer - 1, ioff, &n) == 0
	   && n != 0)
      {
	size_t sz;
	stream_write (ostream, buffer, n, ooff, &sz);
	if (sz != n)
	  return EIO;
	ooff += n;
	ioff += n;
      }
    stream_close (ostream);
    mime_add_part (mime, newmsg);
    message_unref (newmsg);
  }

  *pmime = mime;
  
  return 0;
}

int
sieve_action_reject (sieve_machine_t mach, list_t args, list_t tags)
{
  mime_t mime = NULL;
  mailer_t mailer = sieve_get_mailer (mach);
  int rc;
  message_t newmsg;
  char *addrtext;
  address_t from, to;
  
  sieve_value_t *val = sieve_value_get (args, 0);
  if (!val)
    {
      sieve_error (mach, _("reject: cannot get text!"));
      sieve_abort (mach);
    }
  sieve_log_action (mach, "REJECT", NULL);  
  if (sieve_is_dry_run (mach))
    return 0;

  rc = build_mime (&mime, mach->msg, val->v.string);

  mime_get_message (mime, &newmsg);

  sieve_get_message_sender (mach->msg, &addrtext);
  rc = address_create (&to, addrtext);
  if (rc)
    {
      sieve_error (mach,
		   _("%d: cannot create recipient address <%s>: %s"),
		   sieve_get_message_num (mach),
		   addrtext, mu_strerror (rc));
      free (addrtext);
      goto end;
    }
  free (addrtext);
  
  rc = address_create (&from, sieve_get_daemon_email (mach));
  if (rc)
    {
      sieve_error (mach,
		   _("%d: cannot create sender address <%s>: %s"),
		   sieve_get_message_num (mach),
		   sieve_get_daemon_email (mach),
		   mu_strerror (rc));
      goto end;
    }
  
  rc = mailer_open (mailer, 0);
  if (rc)
    {
      url_t url = NULL;
      mailer_get_url (mailer, &url);
	
      sieve_error (mach,
		   _("%d: cannot open mailer %s: %s"),
		   sieve_get_message_num (mach),
		   url_to_string (url),
		   mu_strerror (rc));
      goto end;
    }

  rc = mailer_send_message (mailer, newmsg, from, to);
  mailer_close (mailer);

 end:
  sieve_mark_deleted (mach->msg, rc == 0);    
  mime_destroy (&mime);
  address_destroy (&from);
  address_destroy (&to);
  
  return rc;
}

/* rfc3028 says: 
   "Implementations SHOULD take measures to implement loop control,"
    We do this by appending an "X-Loop-Prevention" header to each message
    being redirected. If one of the "X-Loop-Prevention" headers of the message
    contains our email address, we assume it is a loop and bail out. */

static int
check_redirect_loop (message_t msg)
{
  header_t hdr = NULL;
  size_t i, num = 0;
  char buf[512];
  int loop = 0;
  char *email = mu_get_user_email (NULL);
  
  message_get_header (msg, &hdr);
  header_get_field_count (hdr, &num);
  for (i = 1; !loop && i <= num; i++)
    {
      header_get_field_name (hdr, i, buf, sizeof buf, NULL);
      if (strcasecmp (buf, "X-Loop-Prevention") == 0)
	{
	  size_t j, cnt = 0;
	  address_t addr;
	  
	  header_get_field_value (hdr, i, buf, sizeof buf, NULL);
	  if (address_create (&addr, buf))
	    continue;

	  address_get_count (addr, &cnt);
	  for (j = 1; !loop && j <= cnt; j++)
	    {
	      address_get_email (addr, j, buf, sizeof buf, NULL);
	      if (strcasecmp (buf, email) == 0)
		loop = 1;
	    }
	  address_destroy (&addr);
	}
    }
  free (email);
  return loop;
}

int
sieve_action_redirect (sieve_machine_t mach, list_t args, list_t tags)
{
  message_t msg, newmsg = NULL;
  address_t addr = NULL, from = NULL;
  header_t hdr = NULL;
  int rc;
  char *fromaddr, *p;
  mailer_t mailer = sieve_get_mailer (mach);
  
  sieve_value_t *val = sieve_value_get (args, 0);
  if (!val)
    {
      sieve_error (mach, _("cannot get address!"));
      sieve_abort (mach);
    }

  rc = address_create (&addr, val->v.string);
  if (rc)
    {
      sieve_error (mach,
		   _("%d: parsing recipient address `%s' failed: %s"),
		   sieve_get_message_num (mach),
		   val->v.string, mu_strerror (rc));
      return 1;
    }
  
  sieve_log_action (mach, "REDIRECT", _("to %s"), val->v.string);
  if (sieve_is_dry_run (mach))
    return 0;

  msg = sieve_get_message (mach);
  if (check_redirect_loop (msg))
    {
      sieve_error (mach, _("%d: Redirection loop detected"),
		   sieve_get_message_num (mach));
      goto end;
    }

  rc = sieve_get_message_sender (msg, &fromaddr);
  if (rc)
    {
      sieve_error (mach,
		   _("%d: cannot get envelope sender: %s"),
		   sieve_get_message_num (mach), mu_strerror (rc));
      goto end;
    }

  rc = address_create (&from, fromaddr);
  if (rc)
    {
      sieve_error (mach,
		   "redirect",
		   _("%d: cannot create sender address <%s>: %s"),
		   sieve_get_message_num (mach),
		   fromaddr, mu_strerror (rc));
      free (fromaddr);
      goto end;
    }

  free (fromaddr);

  rc = message_create_copy (&newmsg, msg);
  if (rc)
    {
      sieve_error (mach,
		   _("%d: cannot copy message: %s"),
		   sieve_get_message_num (mach),
		   mu_strerror (rc));
      goto end;
    }
  
  message_get_header (newmsg, &hdr);
  p = mu_get_user_email (NULL);
  if (p)
    {
      header_set_value (hdr, "X-Loop-Prevention", p, 0);
      free (p);
    }
  else
    {
      sieve_error (mach, _("%d: cannot get my email address"),
		   sieve_get_message_num (mach));
      goto end;
    }
  
  rc = mailer_open (mailer, 0);
  if (rc)
    {
      url_t url = NULL;
      mailer_get_url (mailer, &url);
	
      sieve_error (mach,
		   _("%d: cannot open mailer %s: %s"),
		   sieve_get_message_num (mach),
		   url_to_string (url),
		   mu_strerror (rc));
      goto end;
    }
  
  rc = mailer_send_message (mailer, newmsg, from, addr);
  mailer_close (mailer);
  
 end:
  sieve_mark_deleted (mach->msg, rc == 0);    
  message_destroy (&newmsg, NULL);
  address_destroy (&from);
  address_destroy (&addr);
  
  return rc;
}

sieve_data_type fileinto_args[] = {
  SVT_STRING,
  SVT_VOID
};

void
sieve_register_standard_actions (sieve_machine_t mach)
{
  sieve_register_action (mach, "stop", sieve_action_stop, NULL, NULL, 1);
  sieve_register_action (mach, "keep", sieve_action_keep, NULL, NULL, 1);
  sieve_register_action (mach, "discard", sieve_action_discard, NULL, NULL, 1);
  sieve_register_action (mach, "fileinto", sieve_action_fileinto,
			 fileinto_args, NULL, 0);
  sieve_register_action (mach, "reject", sieve_action_reject, fileinto_args,
			 NULL, 0);
  sieve_register_action (mach, "redirect", sieve_action_redirect, 
			 fileinto_args, NULL, 0);
}


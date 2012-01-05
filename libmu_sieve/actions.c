/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>  
#include <sieve-priv.h>

static int
sieve_mark_deleted (mu_message_t msg, int deleted)
{
  mu_attribute_t attr = 0;
  int rc;

  rc = mu_message_get_attribute (msg, &attr);

  if (!rc)
    {
      if (deleted)
	rc = mu_attribute_set_deleted (attr);
      else
	rc = mu_attribute_unset_deleted (attr);
    }

  return rc;
}


static int
sieve_action_stop (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_sieve_log_action (mach, "STOP", NULL);
  mach->pc = 0;
  return 0;
}

static int
sieve_action_keep (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_sieve_log_action (mach, "KEEP", NULL);
  if (mu_sieve_is_dry_run (mach))
    return 0;
  sieve_mark_deleted (mach->msg, 0);
  return 0;
}

static int
sieve_action_discard (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_sieve_log_action (mach, "DISCARD", _("marking as deleted"));
  if (mu_sieve_is_dry_run (mach))
    return 0;
  sieve_mark_deleted (mach->msg, 1);
  return 0;
}

static int
sieve_action_fileinto (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  int rc;
  int mbflags = 0;
  mu_sieve_value_t *opt;
  mu_sieve_value_t *val = mu_sieve_value_get (args, 0);
  if (!val)
    {
      mu_sieve_error (mach, _("cannot get filename!"));
      mu_sieve_abort (mach);
    }

  if (mu_sieve_tag_lookup (tags, "permissions", &opt))
    {
      const char *p;
      
      if (mu_parse_stream_perm_string (&mbflags, opt->v.string, &p))
	{
	  /* Should not happen, but anyway... */
	  mu_sieve_error (mach, _("invalid permissions (near %s)"), p);
	  return 1;
	}
    }
  
  mu_sieve_log_action (mach, "FILEINTO",
		       _("delivering into %s"), val->v.string);
  if (mu_sieve_is_dry_run (mach))
    return 0;

  rc = mu_message_save_to_mailbox (mach->msg, val->v.string, mbflags);
  if (rc)
    mu_sieve_error (mach, _("cannot save to mailbox: %s"),
		    mu_strerror (rc));
  else
    sieve_mark_deleted (mach->msg, 1);    
  
  return rc;
}

int
mu_sieve_get_message_sender (mu_message_t msg, char **ptext)
{
  int rc;
  mu_envelope_t envelope;
  
  rc = mu_message_get_envelope (msg, &envelope);
  if (rc)
    return rc;
  
  rc = mu_envelope_aget_sender (envelope, ptext);
  if (rc)
    {
      mu_header_t hdr = NULL;
      mu_message_get_header (msg, &hdr);
      if ((rc = mu_header_aget_value (hdr, MU_HEADER_SENDER, ptext)))
	rc = mu_header_aget_value (hdr, MU_HEADER_FROM, ptext);
    }
  return rc;
}

static void
mime_create_reason (mu_mime_t mime, mu_message_t msg, const char *text)
{
  mu_message_t newmsg;
  mu_stream_t stream;
  time_t t;
  struct tm *tm;
  char *sender;
  mu_body_t body;
  mu_header_t hdr;
  static char *content_header =
    "Content-Type: text/plain;charset=" MU_SIEVE_CHARSET "\n"
    "Content-Transfer-Encoding: 8bit\n";
 
  mu_message_create (&newmsg, NULL);
  mu_message_get_body (newmsg, &body);
  mu_body_get_streamref (body, &stream);

  time (&t);
  tm = localtime (&t);

  mu_sieve_get_message_sender (msg, &sender);

  mu_c_streamftime (stream,
		    "The original message was received at "
		    "%a, %b %d %H:%M:%S %Y %Z", tm, NULL);
  
  mu_stream_printf (stream, " from %s.\n", sender);
  free (sender);
  mu_stream_printf (stream,
		    "Message was refused by recipient's mail filtering program.\n");
  mu_stream_printf (stream, "Reason given was as follows:\n\n");
  mu_stream_printf (stream, "%s", text);
  mu_stream_close (stream);
  mu_stream_destroy (&stream);
  
  mu_header_create (&hdr, content_header, strlen (content_header));
  mu_message_set_header (newmsg, hdr, NULL);
  mu_mime_add_part (mime, newmsg);
  mu_message_unref (newmsg);
}

static void
mime_create_ds (mu_mime_t mime, mu_message_t orig)
{
  mu_message_t newmsg;
  mu_stream_t stream;
  mu_header_t hdr;
  mu_body_t body;
  char *email;
  struct tm tm, *tmp;
  struct mu_timezone tz;
  mu_envelope_t env;
  const char *p;
  time_t t = time (NULL);
  
  mu_message_create (&newmsg, NULL);
  mu_message_get_header (newmsg, &hdr); 
  mu_header_set_value (hdr, "Content-Type", "message/delivery-status", 1);
  mu_message_get_body (newmsg, &body);
  mu_body_get_streamref (body, &stream);
  mu_stream_printf (stream, "Reporting-UA: sieve; %s\n", PACKAGE_STRING);

  mu_message_get_envelope (orig, &env);
  if (mu_envelope_sget_date (env, &p) == 0
      && mu_scan_datetime (p, MU_DATETIME_FROM, &tm, &tz, NULL) == 0)
    {
      tmp = &tm;
    }
  else
    {
      tmp = localtime (&t);
      mu_datetime_tz_local (&tz);
    }
      
  mu_c_streamftime (stream, "Arrival-Date: %a, %b %d %H:%M:%S %Y %Z%n",
		    tmp, &tz);

  email = mu_get_user_email (NULL);
  mu_stream_printf (stream, "Final-Recipient: RFC822; %s\n",
		    email ? email : "unknown");
  free (email);
  mu_stream_printf (stream, "Action: deleted\n");
  mu_stream_printf (stream,  
		    "Disposition: automatic-action/MDN-sent-automatically;deleted\n");

  tmp = localtime (&t);
  mu_datetime_tz_local (&tz);
  mu_c_streamftime (stream, "Last-Attempt-Date: %a, %b %d %H:%M:%S %Y %Z%n",
		    tmp, &tz);

  mu_stream_close (stream);
  mu_stream_destroy (&stream);
  
  mu_mime_add_part (mime, newmsg);
  mu_message_unref (newmsg);
}


/* Quote original message */
static int
mime_create_quote (mu_mime_t mime, mu_message_t msg)
{
  mu_message_t newmsg;
  mu_stream_t istream, ostream;
  mu_header_t hdr;
  int rc;
  mu_body_t body;
  
  mu_message_create (&newmsg, NULL);
  mu_message_get_header (newmsg, &hdr); 
  mu_header_set_value (hdr, "Content-Type", "message/rfc822", 1);
  mu_message_get_body (newmsg, &body);
  mu_body_get_streamref (body, &ostream);
  mu_message_get_streamref (msg, &istream);

  rc = mu_stream_copy (ostream, istream, 0, NULL);

  mu_stream_destroy (&istream);
  mu_stream_close (ostream);
  mu_stream_destroy (&ostream);
  
  mu_mime_add_part (mime, newmsg);
  mu_message_unref (newmsg);
  return rc;
}
  
static int
build_mime (mu_mime_t *pmime, mu_message_t msg, const char *text)
{
  mu_mime_t mime = NULL;
  int status;
  
  mu_mime_create (&mime, NULL, 0);
  mime_create_reason (mime, msg, text);
  mime_create_ds (mime, msg);
  status = mime_create_quote (mime, msg);
  if (status)
    {
      mu_mime_destroy (&mime);
      return status;
    }
  
  *pmime = mime;
  
  return 0;
}

static int
sieve_action_reject (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_mime_t mime = NULL;
  mu_mailer_t mailer = mu_sieve_get_mailer (mach);
  int rc;
  mu_message_t newmsg;
  char *addrtext;
  mu_address_t from, to;
  mu_header_t hdr;
  
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
  mu_message_unref (newmsg);
  mu_sieve_get_message_sender (mach->msg, &addrtext);
  mu_message_get_header (newmsg, &hdr);
  mu_header_prepend (hdr, MU_HEADER_TO, addrtext);

  rc = mu_address_create (&to, addrtext);
  if (rc)
    {
      mu_sieve_error (mach,
		      _("%lu: cannot create recipient address <%s>: %s"),
		      (unsigned long) mu_sieve_get_message_num (mach),
		      addrtext, mu_strerror (rc));
      free (addrtext);
      goto end;
    }
  free (addrtext);
  
  rc = mu_address_create (&from, mu_sieve_get_daemon_email (mach));
  if (rc)
    {
      mu_sieve_error (mach,
		      _("%lu: cannot create sender address <%s>: %s"),
		      (unsigned long) mu_sieve_get_message_num (mach),
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
		      _("%lu: cannot open mailer %s: %s"),
		      (unsigned long) mu_sieve_get_message_num (mach),
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
      if (mu_c_strcasecmp (buf, "X-Loop-Prevention") == 0)
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
	      if (mu_c_strcasecmp (buf, email) == 0)
		loop = 1;
	    }
	  mu_address_destroy (&addr);
	}
    }
  free (email);
  return loop;
}

static int
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
		      _("%lu: parsing recipient address `%s' failed: %s"),
		      (unsigned long) mu_sieve_get_message_num (mach),
		      val->v.string, mu_strerror (rc));
      return 1;
    }
  
  mu_sieve_log_action (mach, "REDIRECT", _("to %s"), val->v.string);
  if (mu_sieve_is_dry_run (mach))
    return 0;

  msg = mu_sieve_get_message (mach);
  if (check_redirect_loop (msg))
    {
      mu_sieve_error (mach, _("%lu: redirection loop detected"),
		      (unsigned long) mu_sieve_get_message_num (mach));
      goto end;
    }

  rc = mu_sieve_get_message_sender (msg, &fromaddr);
  if (rc)
    {
      mu_sieve_error (mach,
		      _("%lu: cannot get envelope sender: %s"),
		      (unsigned long) mu_sieve_get_message_num (mach),
		      mu_strerror (rc));
      goto end;
    }

  rc = mu_address_create (&from, fromaddr);
  if (rc)
    {
      mu_sieve_error (mach,
		      _("%lu: cannot create sender address <%s>: %s"),
		      (unsigned long) mu_sieve_get_message_num (mach),
		      fromaddr, mu_strerror (rc));
      free (fromaddr);
      goto end;
    }

  free (fromaddr);

  rc = mu_message_create_copy (&newmsg, msg);
  if (rc)
    {
      mu_sieve_error (mach,
		      _("%lu: cannot copy message: %s"),
		      (unsigned long) mu_sieve_get_message_num (mach),
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
      mu_sieve_error (mach, _("%lu: cannot get my email address"),
		      (unsigned long) mu_sieve_get_message_num (mach));
      goto end;
    }
  
  rc = mu_mailer_open (mailer, 0);
  if (rc)
    {
      mu_url_t url = NULL;
      mu_mailer_get_url (mailer, &url);
	
      mu_sieve_error (mach,
		      _("%lu: cannot open mailer %s: %s"),
		      (unsigned long) mu_sieve_get_message_num (mach),
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

static int
perms_tag_checker (const char *name, mu_list_t tags, mu_list_t args)
{
  mu_iterator_t itr;
  int err = 0;
  
  if (!tags || mu_list_get_iterator (tags, &itr))
    return 0;
  for (mu_iterator_first (itr); !err && !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      int flag;
      const char *p;
      mu_sieve_runtime_tag_t *t;
      mu_iterator_current (itr, (void **)&t);
      if (strcmp (t->tag, "permissions") == 0)
	{
	  if (mu_parse_stream_perm_string (&flag, t->arg->v.string, &p))
	    {
	      mu_sv_compile_error (&mu_sieve_locus, 
				   _("invalid permissions (near %s)"), p);
	      err = 1;
	    }
	}
    }
  mu_iterator_destroy (&itr);
  return err;
}

static mu_sieve_tag_def_t perms_tags[] = {
  { "permissions", SVT_STRING },
  { NULL }
};
  
static mu_sieve_tag_group_t fileinto_tag_groups[] = {
  { perms_tags, perms_tag_checker },
  { NULL }
};
  
void
mu_sv_register_standard_actions (mu_sieve_machine_t mach)
{
  mu_sieve_register_action (mach, "stop", sieve_action_stop, NULL, NULL, 1);
  mu_sieve_register_action (mach, "keep", sieve_action_keep, NULL, NULL, 1);
  mu_sieve_register_action (mach, "discard", sieve_action_discard,
			    NULL, NULL, 1);
  mu_sieve_register_action (mach, "fileinto", sieve_action_fileinto,
			    fileinto_args, fileinto_tag_groups, 0);
  mu_sieve_register_action (mach, "reject", sieve_action_reject, fileinto_args,
			    NULL, 0);
  mu_sieve_register_action (mach, "redirect", sieve_action_redirect, 
			    fileinto_args, NULL, 0);
}


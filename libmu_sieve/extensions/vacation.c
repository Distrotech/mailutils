/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005, 2007, 2009-2012, 2014-2016 Free Software
   Foundation, Inc.

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

/* Syntax: vacation [:days <ndays: number>]
                    [:subject <subject: string>]
		    [:aliases <address-list: list>]
		    [:noreply <noreply-address-list: list>]
		    [:reply_regex <expr: string>]
		    [:reply_prefix <prefix: string>]
		    [:sender <email: string>]
		    [:database <path: string>]
		    [:rfc2822]
		    [:file]
		    [:mime]
		    [:always_reply]
		    [:return_address <email: string>]
		    [:header <header-list: list>]
		    <reply: string>
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <regex.h>
#include <mailutils/sieve.h>
#include <mailutils/mu_auth.h>

/* Build a mime response message from original message MSG. TEXT
   is the message text.
*/
static int
build_mime (mu_sieve_machine_t mach, mu_list_t tags, mu_mime_t *pmime,
	    mu_message_t msg, const char *text)
{
  mu_mime_t mime = NULL;
  mu_message_t newmsg;
  mu_stream_t stream, input;
  mu_header_t hdr;
  mu_body_t body;
  const char *header =
    "Content-Type: text/plain;charset=" MU_SIEVE_CHARSET "\n"
    "Content-Transfer-Encoding: 8bit\n\n";
  int rc;
  
  mu_mime_create (&mime, NULL, 0);
  mu_message_create (&newmsg, NULL);
  mu_message_get_body (newmsg, &body);

  if ((rc = mu_static_memory_stream_create (&input, text, strlen (text))))
    {
      mu_sieve_error (mach,
		      _("cannot create temporary stream: %s"),
		      mu_strerror (rc));
      mu_mime_destroy (&mime);
      mu_message_destroy (&newmsg, NULL);
      return 1;
    }

  if (mu_sieve_tag_lookup (mach, tags, "mime", SVT_VOID, NULL))
    {
      mu_stream_t fstr;
      rc = mu_filter_create (&fstr, input, "base64",
			     MU_FILTER_ENCODE, 
			     MU_STREAM_READ);
      mu_stream_unref (input); 
      if (rc == 0) 
	{
	  header = "Content-Type: text/plain;charset=" MU_SIEVE_CHARSET "\n"
	           "Content-Transfer-Encoding: base64\n\n";
	  input = fstr;
	}
    }

  rc = mu_body_get_streamref (body, &stream);
  if (rc)
    {
      mu_sieve_error (mach,
		      _("cannot get input body stream: %s"),
		      mu_strerror (rc));
      mu_mime_destroy (&mime);
      mu_message_destroy (&newmsg, NULL);
      mu_stream_destroy (&input);
      return 1;
    }

  rc = mu_stream_copy (stream, input, 0, NULL);
  mu_stream_destroy (&input);
  mu_stream_destroy (&stream);
  
  if (rc)
    {
      mu_sieve_error (mach,
		      _("stream copy failed: %s"),
		      mu_strerror (rc));
      mu_mime_destroy (&mime);
      mu_message_destroy (&newmsg, NULL);
      return 1;
    }

  mu_header_create (&hdr, header, strlen (header));
  mu_message_set_header (newmsg, hdr, NULL);

  mu_mime_add_part (mime, newmsg);
  mu_message_unref (newmsg);

  *pmime = mime;

  return 0;
}



/* Produce diagnostic output. */
static int
diag (mu_sieve_machine_t mach)
{
  mu_sieve_log_action (mach, "VACATION", NULL);
  return mu_sieve_is_dry_run (mach);
}


struct addr_data
{
  mu_address_t addr;
  char *my_address;
};

static int
_compare (void *item, void *data)
{
  struct addr_data *ad = data;
  int rc = mu_address_contains_email (ad->addr, item);
  if (rc)
    ad->my_address = item;
  return rc;
}

/* Check whether an alias from ADDRESSES is part of To: or Cc: headers
   of the originating mail. Return non-zero if so and store a pointer
   to the matching address in *MY_ADDRESS. */
static int
match_addresses (mu_header_t hdr, char *email, mu_sieve_value_t *addresses,
		 char const **my_address)
{
  int match = 0;
  const char *str;
  struct addr_data ad;

  ad.my_address = NULL;
  if (mu_header_sget_value (hdr, MU_HEADER_TO, &str) == 0)
    {
      if (!mu_address_create (&ad.addr, str))
	{
	  if (_compare (email, &ad))
	    match = 1;
	  else if (addresses)
	    match += mu_sieve_vlist_do (addresses, _compare, &ad);
	  mu_address_destroy (&ad.addr);
	}
    }

  if (!match && mu_header_sget_value (hdr, MU_HEADER_CC, &str) == 0)
    {
      if (!mu_address_create (&ad.addr, str))
	{
	  if (_compare (email, &ad))
	    match = 1;
	  else if (addresses)
	    match += mu_sieve_vlist_do (addresses, _compare, &ad);
	  mu_address_destroy (&ad.addr);
	}
    }
  *my_address = ad.my_address;
  return match;
}


struct regex_data
{
  mu_sieve_machine_t mach;
  char *email;
};

static int
regex_comparator (void *item, void *data)
{
  regex_t preg;
  int rc;
  struct regex_data *d = data;
  
  if (regcomp (&preg, item,
	       REG_EXTENDED | REG_NOSUB | REG_NEWLINE | REG_ICASE))
    {
      mu_sieve_error (d->mach,
		      _("%lu: cannot compile regular expression \"%s\""),
		      (unsigned long) mu_sieve_get_message_num (d->mach),
		      (char*) item);
      return 0;
    }
  rc = regexec (&preg, d->email, 0, NULL, 0) == 0;
  regfree (&preg);
  return rc;
}

/* Decide whether EMAIL address should not be responded to.
 */
static int
noreply_address_p (mu_sieve_machine_t mach, mu_list_t tags, char *email)
{
  int i, rc = 0;
  mu_sieve_value_t *arg;
  struct regex_data rd;
  static char *noreply_sender[] = {
    ".*-REQUEST@.*",
    ".*-RELAY@.*",
    ".*-OWNER@.*",
    "^OWNER-.*",
    "^postmaster@.*",
    "^UUCP@.*",
    "^MAILER@.*",
    "^MAILER-DAEMON@.*",
    NULL
  };

  rd.mach = mach;
  rd.email = email;
  for (i = 0; rc == 0 && noreply_sender[i]; i++)
    rc = regex_comparator (noreply_sender[i], &rd);

  if (!rc && mu_sieve_tag_lookup_untyped (mach, tags, "noreply", &arg))
    rc = mu_sieve_vlist_do (arg, regex_comparator, &rd);
  
  return rc;
}


/* Return T if letter precedence is 'bulk' or 'junk' */
static int
bulk_precedence_p (mu_header_t hdr)
{
  int rc = 0;
  const char *str;
  if (mu_header_sget_value (hdr, MU_HEADER_PRECEDENCE, &str) == 0)
    {
      rc = mu_c_strcasecmp (str, "bulk") == 0
	   || mu_c_strcasecmp (str, "junk") == 0;
    }
  return rc;
}

#define	DAYS_DEFAULT	7
#define	DAYS_MAX	60

static int
test_and_update_prop (mu_property_t prop, const char *from,
		      time_t now, unsigned int days,
		      mu_sieve_machine_t mach)
{
  const char *result;
  char *timebuf;
  time_t last;
  
  int rc = mu_property_sget_value (prop, from, &result);
  switch (rc)
    {
    case MU_ERR_NOENT:
      break;
      
    case 0:
      if (days == 0)
	return 1;
      last = (time_t) strtoul (result, NULL, 0);
      if (last + (24 * 60 * 60 * days) > now)
	return 1;
      break;
      
    default:
      mu_sieve_error (mach, "%lu: mu_property_sget_value: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      mu_strerror (rc));
      return -1;
    }

  rc = mu_asprintf (&timebuf, "%lu", (unsigned long) now);
  if (rc)
    {
      mu_sieve_error (mach, "%lu: mu_asprintf: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      mu_strerror (rc));
      return -1;
    } 
     
  rc = mu_property_set_value (prop, from, timebuf, 1);
  free (timebuf);
  if (rc)
    {
      mu_sieve_error (mach, "%lu: mu_property_set_value: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      mu_strerror (rc));
      return -1;
    }
  
  rc = mu_property_save (prop);
  if (rc)
    {
      mu_sieve_error (mach, "%lu: mu_property_save: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      mu_strerror (rc));
      return -1;
    }
  return 0;
}

/* Check and update the vacation database. Return 0 if the mail should
   be answered, 0 if it should not, and throw exception if an error
   occurs. */
static int
check_db (mu_sieve_machine_t mach, mu_list_t tags, char *from)
{
  mu_property_t prop;
  char *file;
  unsigned int days;
  int rc;
  mu_stream_t str;
  mu_locker_t locker;
  const char *dbfile = "~/.vacation";
  size_t n;
  
  if (mu_sieve_tag_lookup (mach, tags, "days", SVT_NUMBER, &n))
    {
      days = n;
      if (days > DAYS_MAX)
	days = DAYS_MAX;
    }
  else
    days = DAYS_DEFAULT;

  mu_sieve_tag_lookup (mach, tags, "database", SVT_STRING, &dbfile);
  
  file = mu_tilde_expansion (dbfile, MU_HIERARCHY_DELIMITER, NULL);
  if (!file)
    {
      mu_sieve_error (mach, _("%lu: cannot build db file name"),
		      (unsigned long) mu_sieve_get_message_num (mach));
      mu_sieve_abort (mach);
    }

  rc = mu_locker_create (&locker, file, 0);
  if (rc)
    {
      mu_sieve_error (mach, _("%lu: cannot lock %s: %s"),
		      (unsigned long) mu_sieve_get_message_num (mach),
		      file,
		      mu_strerror (rc));
      free (file);
      mu_sieve_abort (mach);
    }

  rc = mu_file_stream_create (&str, file, MU_STREAM_RDWR|MU_STREAM_CREAT);
  if (rc)
    {
      mu_sieve_error (mach, "%lu: mu_file_stream_create(%s): %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      file,
		      mu_strerror (rc));
      mu_locker_destroy (&locker);
      free (file);
      mu_sieve_abort (mach);
    }
  
  free (file);

  rc = mu_property_create_init (&prop, mu_assoc_property_init, str);
  if (rc)
    {
      mu_sieve_error (mach, "%lu: mu_property_create_init: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      mu_strerror (rc));
      mu_locker_destroy (&locker);
      mu_sieve_abort (mach);
    }

  rc = mu_locker_lock (locker);
  if (rc)
    {
      mu_sieve_error (mach, "%lu: cannot lock vacation database: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      mu_strerror (rc));
      mu_property_destroy (&prop);
      mu_sieve_abort (mach);
    }

  rc = test_and_update_prop (prop, from, time (NULL), days, mach);
  mu_property_destroy (&prop);
  mu_locker_unlock (locker);
  mu_locker_destroy (&locker);
  if (rc == -1)
    mu_sieve_abort (mach);

  return rc;
}

/* Add a reply prefix to the subject. *PSUBJECT points to the
   original subject, which must be allocated using malloc. Before
   returning its value is freed and replaced with the new one.
   Default reply prefix is "Re: ", unless overridden by
   "reply_prefix" tag.
 */
static void
re_subject (mu_sieve_machine_t mach, mu_list_t tags, char **psubject)
{
  char *subject;
  char *prefix = "Re";
  
  mu_sieve_tag_lookup (mach, tags, "reply_prefix", SVT_STRING, &prefix);

  subject = malloc (strlen (*psubject) + strlen (prefix) + 3);
  if (!subject)
    {
      mu_sieve_error (mach,
		      _("%lu: not enough memory"),
		      (unsigned long) mu_sieve_get_message_num (mach));
      return;
    }
  
  strcpy (subject, prefix);
  strcat (subject, ": ");
  strcat (subject, *psubject);
  free (*psubject);
  *psubject = subject;
}

/* Process reply subject header.

   If :subject is set, its value is used.
   Otherwise, if original subject matches reply_regex, it is used verbatim.
   Otherwise, reply_prefix is prepended to it. */

static void
vacation_subject (mu_sieve_machine_t mach, mu_list_t tags,
		  mu_message_t msg, mu_header_t newhdr)
{
  char *value;
  char *subject;
  int subject_allocated = 0;
  mu_header_t hdr;
  
  if (mu_sieve_tag_lookup (mach, tags, "subject", SVT_STRING, &subject))
    /* nothing */;
  else if (mu_message_get_header (msg, &hdr) == 0
	   && mu_header_aget_value_unfold (hdr, MU_HEADER_SUBJECT,
					   &value) == 0)
    {
      char *p;

      int rc = mu_rfc2047_decode (MU_SIEVE_CHARSET, value, &p);

      subject_allocated = 1;
      if (rc)
	{
	  subject = value;
	  value = NULL;
	}
      else
	{
	  subject = p;
	}

      if (mu_sieve_tag_lookup (mach, tags, "reply_regex", SVT_STRING, &p))
	{
	  char *err = NULL;
	  
	  rc = mu_unre_set_regex (p, 0, &err);
	  if (rc)
	    {
	      mu_sieve_error (mach,
			      _("%lu: cannot compile reply prefix regexp: %s: %s"),
			      (unsigned long) mu_sieve_get_message_num (mach),
			      mu_strerror (rc),
			      err ? err : "");
	    }
	}
	  
      if (mu_unre_subject (subject, NULL))
	re_subject (mach, tags, &subject);
      
      free (value);
    }
  else
    subject = "Re: Your mail";
    
  if (mu_rfc2047_encode (MU_SIEVE_CHARSET, "quoted-printable",
			 subject, &value))
    mu_header_set_value (newhdr, MU_HEADER_SUBJECT, subject, 0);
  else
    {
      mu_header_set_value (newhdr, MU_HEADER_SUBJECT, value, 0);
      free (value);
    }

  if (subject_allocated)
    free (subject);
}

static int
header_split (const char *str, char **hname, char **hval)
{
  char *p, *q, *fn, *fv;
  size_t n;
  
  q = strchr (str, ':');
  if (!q)
    return MU_ERR_FORMAT;

  for (p = q; p > str && mu_isspace (p[-1]); --p)
    ;
  if (p == str)
    return MU_ERR_FORMAT;
  
  n = p - str;
  fn = malloc (n + 1);
  if (!fn)
    return ENOMEM;

  memcpy (fn, str, n);
  fn[n] = 0;

  for (++q; *q && mu_isspace (*q); ++q)
    ;

  fv = strdup (q);
  if (!fv)
    {
      free (fn);
      return ENOMEM;
    }

  *hname = fn;
  *hval = fv;

  return 0;
}

struct header_closure
{
  mu_sieve_machine_t mach;  
  mu_header_t hdr;
};

static int
add_header (void *item, void *data)
{
  char const *str = item;
  struct header_closure *hc = data;
  char *fn, *fv;
  int rc;

  rc = header_split (str, &fn, &fv);
  if (rc)
    {
      mu_sieve_error (hc->mach,
		      _("%lu: can't add header \"%s\": %s"),
		      (unsigned long) mu_sieve_get_message_num (hc->mach),
		      str, mu_strerror (rc));
      return 0;
    }

  rc = mu_header_append (hc->hdr, fn, fv);
  free (fn);
  free (fv);
  
  if (rc)
    mu_sieve_error (hc->mach,
		    _("%lu: can't add header \"%s\": %s"),
		    (unsigned long) mu_sieve_get_message_num (hc->mach),
		    str, mu_strerror (rc));
  return 0;
}

/* Generate and send the reply message */
static int
vacation_reply (mu_sieve_machine_t mach, mu_list_t tags, mu_message_t msg,
		char const *text, char const *to, char const *from)
{
  mu_mime_t mime = NULL;
  mu_message_t newmsg;
  mu_header_t newhdr;
  mu_address_t to_addr = NULL, from_addr = NULL;
  char *value;
  mu_mailer_t mailer;
  int rc;
  mu_sieve_value_t *val;

  if (mu_sieve_tag_lookup (mach, tags, "file", SVT_VOID, NULL))
    {
      mu_stream_t instr;
      
      rc = mu_mapfile_stream_create (&instr, text, MU_STREAM_READ);
      if (rc)
	{
	  mu_sieve_error (mach,
			  _("%lu: cannot open message file %s: %s"),
			  (unsigned long) mu_sieve_get_message_num (mach),
			  text,
			  mu_strerror (rc));
	  return -1;
	}
      
      if (mu_sieve_tag_lookup (mach, tags, "rfc2822", SVT_VOID, NULL))
	{
	  rc = mu_stream_to_message (instr, &newmsg);
	  mu_stream_unref (instr);
	  if (rc)
	    {
	      mu_sieve_error (mach,
			      _("%lu: cannot read message from file %s: %s"),
			      (unsigned long) mu_sieve_get_message_num (mach),
			      text,
			      mu_strerror (rc));
	      return -1;
	    }
	}
      else
	{
	  mu_stream_t text_stream;
	  mu_transport_t trans[2];

	  rc = mu_memory_stream_create (&text_stream, MU_STREAM_RDWR);
	  if (rc)
	    {
	      mu_stream_unref (instr);
	      mu_sieve_error (mach,
			      _("%lu: cannot create memory stream: %s"),
			      (unsigned long) mu_sieve_get_message_num (mach),
			      mu_strerror (rc));
	      return -1;
	    }

	  rc = mu_stream_copy (text_stream, instr, 0, NULL);
	  mu_stream_unref (instr);
	  if (rc == 0)
	    rc = mu_stream_write (text_stream, "", 1, NULL);
	  if (rc)
	    {
	      mu_sieve_error (mach,
			      _("%lu: failed reading from %s: %s"),
			      (unsigned long) mu_sieve_get_message_num (mach),
			      text,
			      mu_strerror (rc));
	      return -1;
	    }

	  rc = mu_stream_ioctl (text_stream, MU_IOCTL_TRANSPORT,
				MU_IOCTL_OP_GET, trans);
	  if (rc)
	    {
	      mu_stream_unref (text_stream);
	      mu_sieve_error (mach,
			      "%lu: mu_stream_ioctl: %s",
			      (unsigned long) mu_sieve_get_message_num (mach),
			      mu_strerror (rc));
	      return -1;
	    }

	  if (build_mime (mach, tags, &mime, msg, (char const *) trans[0]))
	    {
	      mu_stream_unref (text_stream);
	      return -1;
	    }
	  mu_mime_get_message (mime, &newmsg);
	  mu_message_unref (newmsg);
	  mu_stream_unref (text_stream);
	}
    }
  else
    {
      if (build_mime (mach, tags, &mime, msg, text))
	return -1;
      mu_mime_get_message (mime, &newmsg);
      mu_message_unref (newmsg);
    }
  
  mu_message_get_header (newmsg, &newhdr);
  
  rc = mu_address_create (&to_addr, to);
  if (rc)
    {
      mu_sieve_error (mach,
		      _("%lu: cannot create recipient address <%s>: %s"),
		      (unsigned long) mu_sieve_get_message_num (mach),
		      from, mu_strerror (rc));
    }
  else
    {
      mu_header_set_value (newhdr, MU_HEADER_TO, to, 0);

      if (mu_sieve_tag_lookup_untyped (mach, tags, "header", &val))
	{
	  struct header_closure hc;
	  hc.mach = mach;
	  hc.hdr = newhdr;
	  mu_sieve_vlist_do (val, add_header, &hc);
	}
      
      vacation_subject (mach, tags, msg, newhdr);
      
      if (from)
        {
          if (mu_address_create (&from_addr, from))
	    from_addr = NULL;
        }
      else
        {
          from_addr = NULL;
        }
      
      if (mu_rfc2822_in_reply_to (msg, &value) == 0)
        {
          mu_header_set_value (newhdr, MU_HEADER_IN_REPLY_TO, value, 1);
          free (value);
        }
      
      if (mu_rfc2822_references (msg, &value) == 0)
        {
          mu_header_set_value (newhdr, MU_HEADER_REFERENCES, value, 1);
          free (value);
        }
      
      mailer = mu_sieve_get_mailer (mach);
      if (mailer)
	{
	  rc = mu_mailer_send_message (mailer, newmsg, from_addr, to_addr);
	}
      else
	rc = MU_ERR_FAILURE;
    }
  mu_address_destroy (&to_addr);
  mu_address_destroy (&from_addr);
  mu_mime_destroy (&mime);
  return rc;
}

int
sieve_action_vacation (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  int rc;
  char *text, *from = NULL;
  char const *return_address;
  mu_message_t msg;
  mu_header_t hdr;
  char *my_address;
  
  if (diag (mach))
    return 0;
  
  mu_sieve_value_get (mach, args, 0, SVT_STRING, &text);

  msg = mu_sieve_get_message (mach);
  mu_message_get_header (msg, &hdr);

  if (mu_sieve_tag_lookup (mach, tags, "sender", SVT_STRING, &from))
    {
      /* Debugging hook: :sender sets fake reply address */
      from = strdup (from);
      if (!from)
        {
          mu_sieve_error (mach, "%lu: %s",
                          (unsigned long) mu_sieve_get_message_num (mach),
                          mu_strerror (ENOMEM));
          mu_sieve_abort (mach);
        }
    }
  else if ((rc = mu_sieve_get_message_sender (msg, &from)) != 0)
    {
      mu_sieve_error (mach,
		      _("%lu: cannot get sender address: %s"),
		      (unsigned long) mu_sieve_get_message_num (mach),
		      mu_strerror (rc));
      mu_sieve_abort (mach);
    }

  my_address = mu_get_user_email (NULL);

  if (mu_sieve_tag_lookup (mach, tags, "always_reply", SVT_VOID, NULL))
    return_address = my_address;
  else
    {
      mu_sieve_value_t *val = NULL;
      mu_sieve_tag_lookup_untyped (mach, tags, "aliases", &val);
      if (match_addresses (hdr, my_address, val, &return_address) == 0)
	{
	  free (my_address);
	  return 0;
	}
    }

  if (noreply_address_p (mach, tags, from)
      || bulk_precedence_p (hdr)
      || check_db (mach, tags, from))
    {
      free (from);
      free (my_address);
      return 0;
    }

  mu_sieve_tag_lookup (mach, tags, "return_address", SVT_STRING,
		       &return_address);

  rc = vacation_reply (mach, tags, msg, text, from, return_address);
  free (from);
  free (my_address);
  if (rc == -1)
    mu_sieve_abort (mach);
  return rc;
}

/* Tagged arguments: */
static mu_sieve_tag_def_t vacation_tags[] = {
  {"days", SVT_NUMBER},
  {"subject", SVT_STRING},
  {"aliases", SVT_STRING_LIST},
  {"noreply", SVT_STRING_LIST},
  {"reply_regex", SVT_STRING},
  {"reply_prefix", SVT_STRING},
  {"sender", SVT_STRING},
  {"database", SVT_STRING},
  {"mime", SVT_VOID},
  {"file", SVT_VOID},
  {"always_reply", SVT_VOID},
  {"return_address", SVT_STRING},
  {"header", SVT_STRING_LIST},
  {"rfc2822", SVT_VOID},
  {NULL}
};

static mu_sieve_tag_group_t vacation_tag_groups[] = {
  {vacation_tags, NULL},
  {NULL}
};

/* Required arguments: */
static mu_sieve_data_type vacation_args[] = {
  SVT_STRING,			/* message text */
  SVT_VOID
};

int SIEVE_EXPORT (vacation, init) (mu_sieve_machine_t mach)
{
  return mu_sieve_register_action (mach, "vacation", sieve_action_vacation,
				vacation_args, vacation_tag_groups, 1);
}

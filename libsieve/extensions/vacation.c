/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

/* Syntax: vacation [:days <ndays: number]
                    [:subject <subject: string>]
		    [:aliases <address-list: list>]
		    [:addresses <noreply-address-list: list>]
		    [:reply_regex <expr: string>]
		    [:reply_prefix <prefix: string>]
		    <reply text: string>
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
#ifdef USE_DBM
# include <mu_dbm.h>
#endif
#include <mailutils/libsieve.h>
#include <mailutils/mu_auth.h>

/* Build a mime response message from original message MSG. TEXT
   is the message text.
   FIXME: This is for future use, when I add :mime tag
*/
static int
build_mime (mime_t *pmime, message_t msg, const char *text)
{
  mime_t mime = NULL;
  message_t newmsg;
  stream_t stream;
  size_t off = 0;
  header_t hdr;
  body_t body;
  /* FIXME: charset */
  char *header = "Content-Type: text/plain;charset=iso-8859-1\n"
    "Content-Transfer-Encoding: 8bit\n\n";

  mime_create (&mime, NULL, 0);

  message_create (&newmsg, NULL);
  message_get_body (newmsg, &body);
  header_create (&hdr, header, strlen (header), newmsg);
  message_set_header (newmsg, hdr, NULL);
  body_get_stream (body, &stream);

  stream_printf (stream, &off, "%s", text);
  stream_close (stream);
  mime_add_part (mime, newmsg);
  message_unref (newmsg);

  *pmime = mime;

  return 0;
}



/* Produce diagnostic output. */
static int
diag (sieve_machine_t mach)
{
  if (sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
    {
      sieve_locus_t locus;
      sieve_get_locus (mach, &locus);
      sieve_debug (mach, "%s:%lu: VACATION\n",
		   locus.source_file,
		   (unsigned long) locus.source_line);
    }

  sieve_log_action (mach, "VACATION", NULL);
  return sieve_is_dry_run (mach);
}


struct addr_data {
  address_t addr;
  char *my_address;
};

static int
_compare (void *item, void *data)
{
  struct addr_data *ad = data;
  int rc = address_contains_email (ad->addr, item);
  if (rc)
    ad->my_address = item;
  return rc;
}

/* Check whether an alias from ADDRESSES is part of To: or Cc: headers
   of the originating mail. Return non-zero if so and store a pointer
   to the matching address to *MY_ADDRESS. */
static int
match_addresses (header_t hdr, sieve_value_t *addresses, char **my_address)
{
  int match = 0;
  char *str;
  struct addr_data ad;

  ad.my_address = NULL;
  if (header_aget_value (hdr, MU_HEADER_TO, &str) == 0)
    {
      if (!address_create (&ad.addr, str))
	{
	  match += sieve_vlist_do (addresses, _compare, &ad);
	  address_destroy (&ad.addr);
	}
      free (str);
    }

  if (!match && header_aget_value (hdr, MU_HEADER_CC, &str) == 0)
    {
      if (!address_create (&ad.addr, str))
	{
	  match += sieve_vlist_do (addresses, _compare, &ad);
	  address_destroy (&ad.addr);
	}
      free (str);
    }
  *my_address = ad.my_address;
  return match;
}


struct regex_data {
  sieve_machine_t mach;
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
      sieve_error (d->mach,
		   ("%d: cannot compile regular expression \"%s\"\n"),
		   sieve_get_message_num (d->mach),
		   item);
      return 0;
    }
  rc = regexec (&preg, d->email, 0, NULL, 0) == 0;
  regfree (&preg);
  return rc;
}

/* Decide whether EMAIL address should not be responded to.
 */
static int
noreply_address_p (sieve_machine_t mach, list_t tags, char *email)
{
  int i, rc = 0;
  sieve_value_t *arg;
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

  if (!rc && sieve_tag_lookup (tags, "addresses", &arg))
    rc = sieve_vlist_do (arg, regex_comparator, &rd);
  
  return rc;
}


/* Return T if letter precedence is 'bulk' or 'junk' */
static int
bulk_precedence_p (header_t hdr)
{
  int rc = 0;
  char *str;
  if (header_aget_value (hdr, MU_HEADER_PRECEDENCE, &str) == 0)
    {
      rc = strcasecmp (str, "bulk") == 0
	   || strcasecmp (str, "junk") == 0;
      free (str);
    }
  return rc;
}

#define	DAYS_MIN	1
#define	DAYS_DEFAULT	7
#define	DAYS_MAX	60

/* Check and updated vacation database. Return 0 if the mail should
   be answered. */
static int
check_db (sieve_machine_t mach, list_t tags, char *from)
{
#ifdef USE_DBM
  DBM_FILE db;
  DBM_DATUM key, value;
  time_t now;
  char buffer[64];
  char *file, *home;
  sieve_value_t *arg;
  unsigned int days;
  int rc;
  
  if (sieve_tag_lookup (tags, "days", &arg))
    {
      days = arg->v.number;
      if (days < DAYS_MIN)
	days = DAYS_MIN;
      else if (days > DAYS_MAX)
	days = DAYS_MAX;
    }
  else
    days = DAYS_DEFAULT;

  home = mu_get_homedir ();

  if (asprintf (&file, "%s/.vacation", (home ? home : ".")) == -1)
    {
      sieve_error (mach, _("%d: cannot build db file name\n"),
		   sieve_get_message_num (mach));
      free (home);
      sieve_abort (mach);
    }
  free (home);
  
  rc = mu_dbm_open (file, &db, MU_STREAM_RDWR, 0600);
  if (rc)
    {
      sieve_error (mach,
		   _("%d: cannot open \"%s\": %s"),
		   sieve_get_message_num (mach), file,
		   mu_strerror (rc));
      free (file);
      sieve_abort (mach);
    }
  free (file);

  time (&now);

  MU_DATUM_SIZE (key) = strlen (from);
  MU_DATUM_PTR (key) = from;

  rc = mu_dbm_fetch (db, key, &value);
  if (!rc)
    {
      time_t last;
      
      strncpy(buffer, MU_DATUM_PTR (value), MU_DATUM_SIZE (value));
      buffer[MU_DATUM_SIZE (value)] = 0;

      last = (time_t) strtoul (buffer, NULL, 0);


      if (last + (24 * 60 * 60 * days) > now)
	{
	  mu_dbm_close (db);
	  return 1;
	}
    }

  MU_DATUM_SIZE (value) = snprintf (buffer, sizeof buffer, "%ld", now);
  MU_DATUM_PTR (value) = buffer;
  MU_DATUM_SIZE (key) = strlen (from);
  MU_DATUM_PTR (key) = from;

  mu_dbm_insert (db, key, value, 1);
  mu_dbm_close (db);
  return 0;
#else
  sieve_error (mach,
	       /* TRANSLATORS: 'vacation' and ':days' are Sieve keywords.
		  Do not translate them! */
	       _("%d: vacation compiled without DBM support. Ignoring :days tag"),
	       sieve_get_message_num (mach));
  return 0;
#endif
}

/* Add a reply prefix to the subject. *PSUBJECT points to the
   original subject, which must be allocated using malloc. Before
   returning its value is freed and replaced with the new one.
   Default reply prefix is "Re: ", unless overridden by
   "reply_prefix" tag.
 */
static void
re_subject (sieve_machine_t mach, list_t tags, char **psubject)
{
  char *subject;
  sieve_value_t *arg;
  char *prefix = "Re";
  
  if (sieve_tag_lookup (tags, "reply_prefix", &arg))
    {
      prefix = arg->v.string;
    }

  subject = malloc (strlen (*psubject) + strlen (prefix) + 3);
  if (!subject)
    {
      sieve_error (mach,
		   ("%d: not enough memory"),
		   sieve_get_message_num (mach));
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
vacation_subject (sieve_machine_t mach, list_t tags,
		  message_t msg, header_t newhdr)
{
  sieve_value_t *arg;
  char *value;
  char *subject;
  int subject_allocated;
  header_t hdr;
  
  if (sieve_tag_lookup (tags, "subject", &arg))
    {
      subject =  arg->v.string;
      subject_allocated = 0;
    }
  else if (message_get_header (msg, &hdr) == 0
	   && header_aget_value_unfold (hdr, MU_HEADER_SUBJECT, &value) == 0)
    {
      char *p;
      
      int rc = rfc2047_decode ("iso-8859-1", value, &p);

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

      if (sieve_tag_lookup (tags, "reply_regex", &arg))
	{
	  char *err = NULL;
	  
	  rc = munre_set_regex (arg->v.string, 0, &err);
	  if (rc)
	    {
	      sieve_error (mach,
			   ("%d: vacation - cannot compile reply prefix regexp: %s: %s"),
			   sieve_get_message_num (mach),
			   mu_strerror (rc),
			   err ? err : "");
	    }
	}
	  
      if (munre_subject (subject, NULL))
	re_subject (mach, tags, &subject);
      
      free (value);
    }

  if (rfc2047_encode ("iso-8859-1", "quoted-printable",
		      subject, &value))
    header_set_value (newhdr, MU_HEADER_SUBJECT, subject, 0);
  else
    {
      header_set_value (newhdr, MU_HEADER_SUBJECT, value, 0);
      free (value);
    }

  if (subject_allocated)
    free (subject);
}

/* Generate and send the reply message */
static int
vacation_reply (sieve_machine_t mach, list_t tags, message_t msg,
		char *text, char *to, char *from)
{
  mime_t mime = NULL;
  message_t newmsg;
  header_t newhdr;
  address_t to_addr = NULL, from_addr = NULL;
  char *value;
  mailer_t mailer;
  int rc;
  
  build_mime (&mime, msg, text);
  mime_get_message (mime, &newmsg);
  message_get_header (newmsg, &newhdr);

  rc = address_create (&to_addr, to);
  if (rc)
    {
      sieve_error (mach,
		   _("%d: cannot create recipient address <%s>: %s\n"),
		   sieve_get_message_num (mach), from, mu_strerror (rc));
      return -1;
    }

  header_set_value (newhdr, MU_HEADER_TO, to, 0);
  
  vacation_subject (mach, tags, msg, newhdr);
    
  if (from)
    {
      if (address_create (&from_addr, from))
	from_addr = NULL;
    }
  else
    {
      from_addr = NULL;
    }
  
  mu_rfc2822_in_reply_to (msg, &value);
  header_set_value (newhdr, MU_HEADER_IN_REPLY_TO, value, 1);
  free (value);
  
  mu_rfc2822_references (msg, &value);
  header_set_value (newhdr, MU_HEADER_REFERENCES, value, 1);
  free (value);
  
  mailer = sieve_get_mailer (mach);
  rc = mailer_open (mailer, 0);
  if (rc)
    {
      url_t url = NULL;
      mailer_get_url (mailer, &url);
      
      sieve_error (mach,
		   _("%d: cannot open mailer %s: %s\n"),
		   sieve_get_message_num (mach),
		   url_to_string (url), mu_strerror (rc));
      return -1;
    }

  rc = mailer_send_message (mailer, newmsg, from_addr, to_addr);
  mailer_close (mailer);
  return rc;
}

int
sieve_action_vacation (sieve_machine_t mach, list_t args, list_t tags)
{
  int rc;
  char *text, *from;
  sieve_value_t *val;
  message_t msg;
  header_t hdr;
  char *my_address = sieve_get_daemon_email (mach);
  
  if (diag (mach))
    return 0;
  
  val = sieve_value_get (args, 0);
  if (!val)
    {
      sieve_error (mach, _("cannot get text!"));
      sieve_abort (mach);
    }
  else
    text = val->v.string;

  msg = sieve_get_message (mach);
  message_get_header (msg, &hdr);

  if (sieve_tag_lookup (tags, "sender", &val))
    {
      /* Debugging hook: :sender sets fake reply address */
      from = strdup (val->v.string);
    }
  else if (sieve_get_message_sender (msg, &from))
    {
      sieve_error (mach,
		   _("%d: cannot get sender address\n"),
		   sieve_get_message_num (mach));
      sieve_abort (mach);
    }

  if (sieve_tag_lookup (tags, "aliases", &val)
      && match_addresses (hdr, val, &my_address) == 0)
    return 0;

  if (noreply_address_p (mach, tags, from)
      || bulk_precedence_p (hdr)
      || check_db (mach, tags, from))
    {
      free (from);
      return 0;
    }

  rc = vacation_reply (mach, tags, msg, text, from, my_address);
  free (from);
  if (rc == -1)
    sieve_abort (mach);
  return rc;
}

/* Tagged arguments: */
static sieve_tag_def_t vacation_tags[] = {
  {"days", SVT_NUMBER},
  {"subject", SVT_STRING},
  {"aliases", SVT_STRING_LIST},
  {"addresses", SVT_STRING_LIST},
  {"reply_regex", SVT_STRING},
  {"reply_prefix", SVT_STRING},
  {NULL}
};

static sieve_tag_group_t vacation_tag_groups[] = {
  {vacation_tags, NULL},
  {NULL}
};

/* Required arguments: */
static sieve_data_type vacation_args[] = {
  SVT_STRING,			/* message text */
  SVT_VOID
};

int SIEVE_EXPORT (vacation, init) (sieve_machine_t mach)
{
  return sieve_register_action (mach, "vacation", sieve_action_vacation,
				vacation_args, vacation_tag_groups, 1);
}

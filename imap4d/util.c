/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2012 Free Software Foundation, Inc.

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

#include "imap4d.h"

/* Get the absolute path.  */
/* NOTE: Path is allocated and must be free()d by the caller.  */
char *
util_getfullpath (const char *name)
{
  char *exp = mu_tilde_expansion (name, MU_HIERARCHY_DELIMITER,
				  imap4d_homedir);
  if (*exp != MU_HIERARCHY_DELIMITER)
    {
      char *p, *s =
	mu_alloc (strlen (imap4d_homedir) + 1 + strlen (exp) + 1);
      p = mu_stpcpy (s, imap4d_homedir);
      *p++ = MU_HIERARCHY_DELIMITER;
      strcpy (p, exp);
      free (exp);
      exp = s;
    }
  return mu_normalize_path (exp);
}

int
util_do_command (struct imap4d_session *session, imap4d_tokbuf_t tok)
{
  char *tag, *cmd;
  struct imap4d_command *command;
  static struct imap4d_command nullcommand;
  int argc = imap4d_tokbuf_argc (tok);
  
  if (argc == 0)
    {
      nullcommand.name = "";
      nullcommand.tag = (char *) "*";
      return io_completion_response (&nullcommand, RESP_BAD, "Null command");
    }
  else if (argc == 1)
    {
      nullcommand.name = "";
      nullcommand.tag = imap4d_tokbuf_getarg (tok, 0);
      return io_completion_response (&nullcommand, RESP_BAD, "Missing command");
    }

  tag = imap4d_tokbuf_getarg (tok, 0);
  cmd = imap4d_tokbuf_getarg (tok, 1);
  
  command = util_getcommand (cmd, imap4d_command_table);
  if (command == NULL)
    {
      nullcommand.name = "";
      nullcommand.tag = tag;
      return io_completion_response (&nullcommand, RESP_BAD, "Invalid command");
    }

  command->tag = tag;

  if (command->states && (command->states & state) == 0)
    return io_completion_response (command, RESP_BAD, "Wrong state");

  return command->func (session, command, tok);
}

struct imap4d_command *
util_getcommand (char *cmd, struct imap4d_command command_table[])
{
  size_t i, len = strlen (cmd);

  for (i = 0; command_table[i].name != 0; i++)
    {
      if (strlen (command_table[i].name) == len &&
	  !mu_c_strcasecmp (command_table[i].name, cmd))
	return &command_table[i];
    }
  return NULL;
}

static void
adjust_tm (struct tm *tm, struct mu_timezone *tz,
	   enum datetime_parse_mode flag)
{
  switch (flag)
    {
    case datetime_default:
      break;
    case datetime_date_only:
      tm->tm_sec = 0;
      tm->tm_min = 0;
      tm->tm_hour = 0;
#if HAVE_STRUCT_TM_TM_ISDST
      tm->tm_isdst = 0;
#endif
#if HAVE_STRUCT_TM_TM_GMTOFF
      tm->tm_gmtoff = 0;
#endif
      tz->utc_offset = 0;
      tz->tz_name = NULL;
      break;

    case datetime_time_only:
      tm->tm_mon = 0;
      tm->tm_year = 0;
      tm->tm_yday = 0;
      tm->tm_wday = 0;
      tm->tm_mday = 0;
      break;
    }
}
  
int
util_parse_internal_date (char *date, time_t *timep,
			  enum datetime_parse_mode flag)
{
  struct tm tm;
  struct mu_timezone tz;
  time_t time;

  if (mu_scan_datetime (date, MU_DATETIME_INTERNALDATE, &tm, &tz, NULL))
    return 1;

  adjust_tm (&tm, &tz, flag);
  
  time = mu_datetime_to_utc (&tm, &tz);
  if (time == (time_t) - 1)
    return 2;

  *timep = time;
  return 0;
}

int
util_parse_822_date (const char *date, time_t *timep,
		     enum datetime_parse_mode flag)
{
  struct tm tm;
  struct mu_timezone tz;
  const char *p = date;

  if (mu_parse822_date_time (&p, date + strlen (date), &tm, &tz) == 0)
    {
      adjust_tm (&tm, &tz, flag);
      *timep = mu_datetime_to_utc (&tm, &tz);
      return 0;
    }
  return 1;
}

int
util_parse_ctime_date (const char *date, time_t *timep,
		       enum datetime_parse_mode flag)
{
  struct tm tm;
  struct mu_timezone tz;

  if (mu_scan_datetime (date, MU_DATETIME_FROM, &tm, &tz, NULL) == 0)
    {
      adjust_tm (&tm, &tz, flag);
      *timep = mu_datetime_to_utc (&tm, &tz);
      return 0;
    }
  return 1;
}

/* Return the first ocurrence of NEEDLE in HAYSTACK. Case insensitive
   comparison */
char *
util_strcasestr (const char *haystack, const char *needle)
{
  return mu_c_strcasestr (haystack, needle);
}

void
util_print_flags (mu_attribute_t attr)
{
  int flags = 0;

  mu_attribute_get_flags (attr, &flags);
  mu_imap_format_flags (iostream, flags, 1);
}

int
util_attribute_matches_flag (mu_attribute_t attr, const char *item)
{
  int flags = 0, mask = 0;

  mu_attribute_get_flags (attr, &flags);
  mu_imap_flag_to_attribute (item, &mask);
  if (mask == MU_ATTRIBUTE_RECENT)
    return MU_ATTRIBUTE_IS_UNSEEN (flags);

  return flags & mask;
}

char *
util_localname ()
{
  static char *localname;

  if (!localname)
    {
      char *name;
      int name_len = 256;
      int status = 1;
      struct hostent *hp;

      name = malloc (name_len);
      while (name
	     && (status = gethostname (name, name_len)) == 0
	     && !memchr (name, 0, name_len))
	{
	  name_len *= 2;
	  name = realloc (name, name_len);
	}
      if (status || name == NULL)
	{
	  mu_diag_output (MU_DIAG_CRIT, _("cannot find out my own hostname"));
	  exit (EX_OSERR);
	}

      hp = gethostbyname (name);
      if (hp)
	{
	  struct in_addr inaddr;
	  inaddr.s_addr = *(unsigned int *) hp->h_addr;
	  hp = gethostbyaddr ((const char *) &inaddr,
			      sizeof (struct in_addr), AF_INET);
	  if (hp)
	    {
	      free (name);
	      name = mu_strdup ((char *) hp->h_name);
	    }
	}
      localname = name;
    }
  return localname;
}

/* Return the uindvalidity of a mailbox.
   When a mailbox is selected, whose first message does not keep X-UIDVALIDITY
   value, the uidvalidity is computed basing on the return of time(). Now,
   if we call "EXAMINE mailbox" or "STATUS mailbox (UIDVALIDITY)" the same
   mailbox is opened second time and the uidvalidity recalculated. Thus each
   subsequent call to EXAMINE or STATUS upon an already selected mailbox
   will return a different uidvalidity value. To avoid this, util_uidvalidity()
   first sees if it is asked to operate upon an already opened mailbox
   and if so, returns the previously computed value. */
int
util_uidvalidity (mu_mailbox_t smbox, unsigned long *uidvp)
{
  mu_url_t mbox_url = NULL;
  mu_url_t smbox_url = NULL;

  mu_mailbox_get_url (mbox, &mbox_url);
  mu_mailbox_get_url (smbox, &smbox_url);
  if (strcmp (mu_url_to_string (mbox_url), mu_url_to_string (smbox_url)) == 0)
    smbox = mbox;
  return mu_mailbox_uidvalidity (smbox, uidvp);
}

static mu_list_t atexit_list;

void
util_atexit (void (*fp) (void))
{
  if (!atexit_list)
    mu_list_create (&atexit_list);
  mu_list_append (atexit_list, (void*)fp);
}

static int
atexit_run (void *item, void *data)
{
  ((void (*) (void)) item) ();
  return 0;
}

void
util_bye ()
{
  mu_stream_close (iostream);
  mu_stream_destroy (&iostream);
  mu_list_foreach (atexit_list, atexit_run, 0);
}

void
util_chdir (const char *dir)
{
  int rc = chdir (dir);
  if (rc)
    mu_error ("Cannot change to home directory `%s': %s",
	      dir, mu_strerror (errno));
}

int
is_atom (const char *s)
{
  if (strpbrk (s, "(){ \t%*\"\\"))
    return 0;
  for (; *s; s++)
    {
      if (mu_iscntrl (*s))
	return 0;
    }
  return 1;
}
     
int
set_xscript_level (int xlev)
{
  if (imap4d_transcript)
    {
      if (xlev != MU_XSCRIPT_NORMAL)
	{
	  if (mu_debug_level_p (MU_DEBCAT_REMOTE, 
	                        xlev == MU_XSCRIPT_SECURE ?
				  MU_DEBUG_TRACE6 : MU_DEBUG_TRACE7))
	    return MU_XSCRIPT_NORMAL;
	}

      if (mu_stream_ioctl (iostream, MU_IOCTL_XSCRIPTSTREAM,
                           MU_IOCTL_XSCRIPTSTREAM_LEVEL, &xlev) == 0)
	return xlev;
    }
  return 0;
}

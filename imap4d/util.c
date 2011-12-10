/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
   2009, 2010, 2011 Free Software Foundation, Inc.

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

/* NOTE: Allocates Memory.  */
/* Expand: ~ --> /home/user, and ~guest --> /home/guest.  */
char *
util_tilde_expansion (const char *ref, const char *delim)
{
  return mu_tilde_expansion (ref, delim, imap4d_homedir);
}

/* Get the absolute path.  */
/* NOTE: Path is allocated and must be free()d by the caller.  */
char *
util_getfullpath (const char *name, const char *delim)
{
  char *p = util_tilde_expansion (name, delim);
  if (*p != delim[0])
    {
      char *s =
	calloc (strlen (imap4d_homedir) + strlen (delim) + strlen (p) + 1, 1);
      sprintf (s, "%s%s%s", imap4d_homedir, delim, p);
      free (p);
      p = s;
    }
  return mu_normalize_path (p);
}

/* A message set is defined as:

   set ::= sequence_num / (sequence_num ":" sequence_num) / (set "," set)
   sequence_num    ::= nz_number / "*"
   ;; * is the largest number in use.  For message
   ;; sequence numbers, it is the number of messages
   ;; in the mailbox.  For unique identifiers, it is
   ;; the unique identifier of the last message in
   ;; the mailbox.
   nz_number       ::= digit_nz *digit

   Non-existing sequence numbers are ignored, except when they form part
   of a message range (sequence_num ":" sequence_num), in which case they
   are treated as minimal or maximal sequence numbers (uids) available in
   the mailbox depending on whether they appear to the left or to the right
   of ":".
*/

/* Message set is a list of non-overlapping msgranges, in ascending
   order.  No matter what command flavor is used, msg_beg and msg_end are
   treated as sequence numbers (not UIDs). */
struct msgrange
{
  size_t msg_beg;           /* beginning message number */
  size_t msg_end;           /* ending message number */
};

/* Comparator function used to sort the message set list. */
static int
compare_msgrange (const void *a, const void *b)
{
  struct msgrange const *sa = a;
  struct msgrange const *sb = b;
  
  if (sa->msg_beg < sb->msg_beg)
    return -1;
  if (sa->msg_beg > sb->msg_beg)
    return 1;
  if (sa->msg_end < sb->msg_end)
    return -1;
  if (sa->msg_end > sb->msg_end)
    return 1;
  return 0;
}

/* Comparator function used to locate a message in the list.  Second argument
   (b) is a pointer to the message number. */
static int
compare_msgnum (const void *a, const void *b)
{
  struct msgrange const *range = a;
  size_t msgno = *(size_t*)b;

  if (range->msg_beg <= msgno && msgno <= range->msg_end)
    return 0;
  return 1;
}

/* This structure keeps parser state while parsing message set. */
struct parse_msgnum_env
{
  char *s;               /* Current position in string */
  size_t minval;         /* Min. sequence number or UID */
  size_t maxval;         /* Max. sequence number or UID */
  mu_list_t list;        /* List being built. */
  int isuid:1;           /* True, if parsing an UID command. */
  mu_mailbox_t mailbox;  /* Reference mailbox (can be NULL). */
};

/* Get a single message number/UID from env->s and store it into *PN.
   Return 0 on success and error code on error.

   Advance env->s to the point past the parsed message number.
 */
static int
get_msgnum (struct parse_msgnum_env *env, size_t *pn)
{
  size_t msgnum;
  char *p;
  
  errno = 0;
  msgnum = strtoul (env->s, &p, 10);
  if (msgnum == ULONG_MAX && errno == ERANGE)
    return MU_ERR_PARSE;
  env->s = p;
  if (msgnum > env->maxval)
    msgnum = env->maxval;
  *pn = msgnum;
  return 0;
}

/* Parse a single message range (A:B). Treat '*' as the largest number/UID
   in use. */
static int
parse_msgrange (struct parse_msgnum_env *env)
{
  int rc;
  struct msgrange msgrange, *mp;
  
  if (*env->s == '*')
    {
      msgrange.msg_beg = env->maxval;
      env->s++;
    }
  else if ((rc = get_msgnum (env, &msgrange.msg_beg)))
    return rc;

  if (*env->s == ':')
    {
      if (*++env->s == '*')
	{
	  msgrange.msg_end = env->maxval;
	  ++env->s;
	}
      else if (*env->s == 0)
	return MU_ERR_PARSE;
      else if ((rc = get_msgnum (env, &msgrange.msg_end)))
	return rc;
    }
  else
    msgrange.msg_end = msgrange.msg_beg;

  if (msgrange.msg_end < msgrange.msg_beg)
    {
      size_t tmp = msgrange.msg_end;
      msgrange.msg_end = msgrange.msg_beg;
      msgrange.msg_beg = tmp;
    }

  if (env->isuid && env->mailbox)
    {
      int rc;

      rc = mu_mailbox_translate (env->mailbox,
				 MU_MAILBOX_UID_TO_MSGNO,
				 msgrange.msg_beg, &msgrange.msg_beg);
      if (rc == MU_ERR_NOENT)
	msgrange.msg_beg = env->minval;
      else if (rc)
	return rc;
      
      rc = mu_mailbox_translate (env->mailbox,
				 MU_MAILBOX_UID_TO_MSGNO,
				 msgrange.msg_end, &msgrange.msg_end);
      if (rc == MU_ERR_NOENT)
	msgrange.msg_end = env->maxval;
      else if (rc)
	return rc;
    }      

  mp = malloc (sizeof (*mp));
  if (!mp)
    return ENOMEM;

  *mp = msgrange;

  rc = mu_list_append (env->list, mp);
  if (rc)
    free (mp);
  return rc;
}

/* Parse message set specification S. ISUID indicates if the set is supposed
   to contain UIDs (they will be translated to message sequence numbers).
   MBX is a reference mailbox or NULL.

   On success, return 0 and place the resulting set into *PLIST. On error,
   return error code and point END to the position in the input string where
   the parsing failed. */
int
util_parse_msgset (char *s, int isuid, mu_mailbox_t mbx,
		   mu_list_t *plist, char **end)
{
  int rc;
  struct parse_msgnum_env env;
  size_t n;
  
  if (!s)
    return EINVAL;
  if (!*s)
    return MU_ERR_PARSE;

  memset (&env, 0, sizeof (env));
  env.s = s;
  if (mbox)
    {
      size_t lastmsgno;      /* Max. sequence number. */

      rc = mu_mailbox_messages_count (mbx, &lastmsgno);
      if (rc == 0)
	{
	  if (isuid)
	    {
	      rc = mu_mailbox_translate (mbox, MU_MAILBOX_MSGNO_TO_UID,
					 lastmsgno, &env.maxval);
	      if (rc == 0)
		rc = mu_mailbox_translate (mbox, MU_MAILBOX_MSGNO_TO_UID,
					   1, &env.minval);
	    }
	  else
	    env.maxval = lastmsgno;
	}
      if (rc)
	return rc;
    }
  rc = mu_list_create (&env.list);
  if (rc)
    return rc;
  mu_list_set_destroy_item (env.list, mu_list_free_item);
  
  env.isuid = isuid;
  env.mailbox = mbx;
  
  while ((rc = parse_msgrange (&env)) == 0 && *env.s)
    {
      if (*env.s != ',' || *++env.s == 0)
	{
	  rc = MU_ERR_PARSE;
	  break;
	}
    }

  if (end)
    *end = env.s;
  if (rc)
    {
      mu_list_destroy (&env.list);
      return rc;
    }

  mu_list_count (env.list, &n);
  if (n > 1)
    {
      mu_iterator_t itr;
      struct msgrange *last = NULL;

      /* Sort the list and coalesce overlapping message ranges. */
      mu_list_sort (env.list, compare_msgrange);

      mu_list_get_iterator (env.list, &itr);
      for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  struct msgrange *msgrange;

	  mu_iterator_current (itr, (void**)&msgrange);
	  if (last)
	    {
	      if (last->msg_beg <= msgrange->msg_beg &&
		  msgrange->msg_beg <= last->msg_end)
		{
		  if (msgrange->msg_end > last->msg_end)
		    last->msg_end = msgrange->msg_end;
		  mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
		  continue;
		}
		  
	    }
	  last = msgrange;
	}
      mu_iterator_destroy (&itr);
    }

  if (rc == 0)
    {
      mu_list_set_comparator (env.list, compare_msgnum);
      *plist = env.list;
    }
  else
    mu_list_destroy (&env.list);
  return rc;
}

struct action_closure
{
  imap4d_message_action_t action;
  void *data;
};

static int
procrange (void *item, void *data)
{
  struct msgrange *mp = item;
  struct action_closure *clos = data;
  size_t i;

  for (i = mp->msg_beg; i <= mp->msg_end; i++)
    {
      int rc = clos->action (i, clos->data);
      if (rc)
	return rc;
    }
  return 0;
}

/* Apply ACTION to each message number from LIST. */
int
util_foreach_message (mu_list_t list, imap4d_message_action_t action,
		      void *data)
{
  struct action_closure clos;
  clos.action = action;
  clos.data = data;
  return mu_list_foreach (list, procrange, &clos);
}

int
util_do_command (imap4d_tokbuf_t tok)
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

  return command->func (command, tok);
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

struct
{
  char *name;
  int flag;
}
_imap4d_attrlist[] =
{
  { "\\Answered", MU_ATTRIBUTE_ANSWERED },
  { "\\Flagged", MU_ATTRIBUTE_FLAGGED },
  { "\\Deleted", MU_ATTRIBUTE_DELETED },
  { "\\Draft", MU_ATTRIBUTE_DRAFT },
  { "\\Seen", MU_ATTRIBUTE_SEEN|MU_ATTRIBUTE_READ },
};

#define NATTR sizeof(_imap4d_attrlist)/sizeof(_imap4d_attrlist[0])

int _imap4d_nattr = NATTR;

int
util_attribute_to_type (const char *item, int *type)
{
  int i;

  if (mu_c_strcasecmp (item, "\\Recent") == 0)
    {
      *type = MU_ATTRIBUTE_RECENT;
      return 0;
    }
  
  for (i = 0; i < _imap4d_nattr; i++)
    if (mu_c_strcasecmp (item, _imap4d_attrlist[i].name) == 0)
      {
	*type = _imap4d_attrlist[i].flag;
	return 0;
      }
  return 1;
}

int
util_format_attribute_flags (mu_stream_t str, int flags)
{
  int i;
  int delim = 0;
  
  for (i = 0; i < _imap4d_nattr; i++)
    if (flags & _imap4d_attrlist[i].flag)
      {
	if (delim)
	  mu_stream_printf (str, " ");
	mu_stream_printf (str, "%s", _imap4d_attrlist[i].name);
	delim = 1;
      }

  if (MU_ATTRIBUTE_IS_UNSEEN (flags))
    {
      if (delim)
	mu_stream_printf (str, " ");
      mu_stream_printf (str, "\\Recent");
    }
  
  return 0;
}

void
util_print_flags (mu_attribute_t attr)
{
  int flags = 0;

  mu_attribute_get_flags (attr, &flags);
  util_format_attribute_flags (iostream, flags);
}

int
util_attribute_matches_flag (mu_attribute_t attr, const char *item)
{
  int flags = 0, mask = 0;

  mu_attribute_get_flags (attr, &flags);
  util_attribute_to_type (item, &mask);
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
	      name = strdup ((char *) hp->h_name);
	    }
	}
      localname = name;
    }
  return localname;
}

/* Match STRING against the IMAP4 wildcard pattern PATTERN. */

#define WILD_FALSE 0
#define WILD_TRUE  1
#define WILD_ABORT 2

int
_wild_match (const char *expr, const char *name, char delim)
{
  while (expr && *expr)
    {
      if (*name == 0 && *expr != '*')
	return WILD_ABORT;
      switch (*expr)
	{
	case '*':
	  while (*++expr == '*')
	    ;
	  if (*expr == 0)
	    return WILD_TRUE;
	  while (*name)
	    {
	      int res = _wild_match (expr, name++, delim);
	      if (res != WILD_FALSE)
		return res;
	    }
	  return WILD_ABORT;

	case '%':
	  while (*++expr == '%')
	    ;
	  if (*expr == 0)
	    return strchr (name, delim) ? WILD_FALSE : WILD_TRUE;
	  while (*name && *name != delim)
	    {
	      int res = _wild_match (expr, name++, delim);
	      if (res != WILD_FALSE)
		return res;
	    }
	  return _wild_match (expr, name, delim);
	  
	default:
	  if (*expr != *name)
	    return WILD_FALSE;
	  expr++;
	  name++;
	}
    }
  return *name == 0;
}

int
util_wcard_match (const char *name, const char *expr, const char *delim)
{
  return _wild_match (expr, name, delim[0]) != WILD_TRUE;
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

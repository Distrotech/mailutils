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

static int add2set (size_t **, int *, unsigned long);

/* NOTE: Allocates Memory.  */
/* Expand: ~ --> /home/user and to ~guest --> /home/guest.  */
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

static int
comp_int (const void *a, const void *b)
{
  return *(int *) a - *(int *) b;
}

/* Parse the message set specification from S. Store message numbers
   in SET, store number of element in the SET into the memory pointed to
   by N.

   A message set is defined as:

   set ::= sequence_num / (sequence_num ":" sequence_num) / (set "," set)
   sequence_num    ::= nz_number / "*"
   ;; * is the largest number in use.  For message
   ;; sequence numbers, it is the number of messages
   ;; in the mailbox.  For unique identifiers, it is
   ;; the unique identifier of the last message in
   ;; the mailbox.
   nz_number       ::= digit_nz *digit

   FIXME: The message sets like <,,,> or <:12> or <20:10> are not considered
   an error */
int
util_msgset (char *s, size_t ** set, int *n, int isuid)
{
  unsigned long val = 0;
  unsigned long low = 0;
  int done = 0;
  int status = 0;
  size_t max = 0;
  size_t *tmp;
  int i, j;
  unsigned long invalid_uid = 0; /* For UID mode only: have we 
				    encountered an uid > max uid? */
  
  status = mu_mailbox_messages_count (mbox, &max);
  if (status != 0)
    return status;
  /* If it is a uid sequence, override max with the UID.  */
  if (isuid)
    {
      mu_message_t msg = NULL;
      mu_mailbox_get_message (mbox, max, &msg);
      mu_message_get_uid (msg, &max);
    }

  *n = 0;
  *set = NULL;
  while (*s)
    {
      switch (*s)
	{
	  /* isdigit */
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  {
	    errno = 0;
	    val = strtoul (s, &s, 10);
	    if (val == ULONG_MAX && errno == ERANGE)
	      {
		if (*set)
		  free (*set);
		*set = NULL;
		*n = 0;
		return EINVAL;
	      }
	    else if (val > max)
	      {
		if (isuid)
		  {
		    invalid_uid = 1;
		    continue;
		  }
		if (*set)
		  free (*set);
		*set = NULL;
		*n = 0;
		return EINVAL;
	      }
	    
	    if (low)
	      {
		/* Reverse it. */
		if (low > val)
		  {
		    long tmp = low;
		    tmp -= 2;
		    if (tmp < 0 || val == 0)
		      {
			free (*set);
			*set = NULL;
			*n = 0;
			return EINVAL;
		      }
		    low = val;
		    val = tmp;
		  }
		for (; low && low <= val; low++)
		  {
		    status = add2set (set, n, low);
		    if (status != 0)
		      return status;
		  }
		low = 0;
	      }
	    else
	      {
		status = add2set (set, n, val);
		if (status != 0)
		  return status;
	      }
	    break;
	  }

	  /* A pair of numbers separated by a ':' character indicates a
	     contiguous set of mesages ranging from the first number to the
	     second:
	     3:5  --> 3 4 5
	   */
	case ':':
	  low = val + 1;
	  s++;
	  break;

	  /* As a convenience. '*' is provided to refer to the highest
	     message number int the mailbox:
	     5:*  --> 5 6 7 8
	   */
	case '*':
	  {
	    val = max;
	    s++;
	    status = add2set (set, n, val);
	    if (status != 0)
	      return status;
	  }
	  break;

	  /* IMAP also allows a set of noncontiguous numbers to be specified
	     with the ',' character:
	     1,3,5,7  --> 1 3 5 7
	   */
	case ',':
	  s++;
	  break;

	default:
	  done = 1;
	  if (*set)
	    free (*set);
	  *set = NULL;
	  *n = 0;
	  return EINVAL;

	}			/* switch */

      if (done)
	break;
    }				/* while */

  if (*n == 0)
    return 0;
  
  /* For message sets in form X:Y where Y is a not-existing UID greater
     than max UID, replace Y with the max UID in the mailbox */
  if (*n == 1 && invalid_uid)
    {
      val = max;
      status = add2set (set, n, val);
      if (status != 0)
	return status;
    }
  
  if (low)
    {
      /* Reverse it. */
      if (low > val)
	{
	  long tmp = low;
	  tmp -= 2;
	  if (tmp < 0 || val == 0)
	    {
	      free (*set);
	      *set = NULL;
	      *n = 0;
	      return EINVAL;
	    }
	  low = val;
	  val = tmp;
	}
      for (; low && low <= val; low++)
	{
	  status = add2set (set, n, low);
	  if (status != 0)
	    return status;
	}
    }

  /* Sort the resulting message set */
  qsort (*set, *n, sizeof (**set), comp_int);

  /* Remove duplicates. tmp serves to avoid extra dereferences */
  tmp = *set;
  for (i = 0, j = 1; i < *n; i++)
    if (tmp[j - 1] != tmp[i])
      tmp[j++] = tmp[i];
  *n = j;
  return 0;
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

static int
add2set (size_t ** set, int *n, unsigned long val)
{
  size_t *tmp;
  tmp = realloc (*set, (*n + 1) * sizeof (**set));
  if (tmp == NULL)
    {
      if (*set)
	free (*set);
      *n = 0;
      return ENOMEM;
    }
  *set = tmp;
  (*set)[*n] = val;
  (*n)++;
  return 0;
}

int
util_parse_internal_date (char *date, time_t * timep)
{
  struct tm tm;
  mu_timezone tz;
  time_t time;
  char **datep = &date;

  if (mu_parse_imap_date_time ((const char **) datep, &tm, &tz))
    return 1;

  time = mu_tm2time (&tm, &tz);
  if (time == (time_t) - 1)
    return 2;

  *timep = time;
  return 0;
}

int
util_parse_822_date (const char *date, time_t * timep)
{
  struct tm tm;
  mu_timezone tz;
  const char *p = date;

  if (mu_parse822_date_time (&p, date + strlen (date), &tm, &tz) == 0)
    {
      *timep = mu_tm2time (&tm, &tz);
      return 0;
    }
  return 1;
}

int
util_parse_ctime_date (const char *date, time_t * timep)
{
  struct tm tm;
  mu_timezone tz;

  if (mu_parse_ctime_date_time (&date, &tm, &tz) == 0)
    {
      *timep = mu_tm2time (&tm, &tz);
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
  mu_list_do (atexit_list, atexit_run, 0);
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

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/mailbox.h>
#include <mailutils/msgset.h>
#include <mailutils/sys/msgset.h>

/* This structure keeps parser state while parsing message set. */
struct parse_msgnum_env
{
  const char *s;         /* Current position in string */
  size_t minval;         /* Min. sequence number or UID */
  size_t maxval;         /* Max. sequence number or UID */
  mu_msgset_t msgset;    /* Message set being built. */
  int mode;              /* Operation mode (num/uid) */
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
  if (env->msgset->mbox && env->maxval && msgnum > env->maxval)
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
  struct mu_msgrange msgrange;
  
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

  if (msgrange.msg_end && msgrange.msg_end < msgrange.msg_beg)
    {
      size_t tmp = msgrange.msg_end;
      msgrange.msg_end = msgrange.msg_beg;
      msgrange.msg_beg = tmp;
    }

  return mu_msgset_add_range (env->msgset, msgrange.msg_beg, msgrange.msg_end,
			      env->mode);
}

/* Parse IMAP-style message set specification S.

   On success, return 0 and populate MSET with the resulting message ranges.
   On error, return error code and point END to the position in the input
   string where parsing has failed. */
int
mu_msgset_parse_imap (mu_msgset_t mset, int mode, const char *s, char **end)
{
  int rc;
  struct parse_msgnum_env env;
  
  if (!s || !mset)
    return EINVAL;
  if (end)
    *end = (char*) s;
  if (!*s)
    return MU_ERR_PARSE;

  memset (&env, 0, sizeof (env));
  env.s = s;
  env.msgset = mset;
  env.minval = 1;
  env.mode = mode;
  
  if (mset->mbox)
    {
      size_t lastmsgno;      /* Max. sequence number. */

      rc = mu_mailbox_messages_count (mset->mbox, &lastmsgno);
      if (rc == 0)
	{
	  if (mode == MU_MSGSET_UID)
	    {
	      rc = mu_mailbox_translate (mset->mbox, MU_MAILBOX_MSGNO_TO_UID,
					 lastmsgno, &env.maxval);
	      if (rc == 0)
		rc = mu_mailbox_translate (mset->mbox, MU_MAILBOX_MSGNO_TO_UID,
					   1, &env.minval);
	    }
	  else
	    env.maxval = lastmsgno;
	}
      if (rc)
	return rc;
    }
  
  while ((rc = parse_msgrange (&env)) == 0 && *env.s)
    {
      if (*env.s != ',' || *++env.s == 0)
	{
	  rc = MU_ERR_PARSE;
	  break;
	}
    }

  if (end)
    *end = (char*) env.s;
  return rc;
}

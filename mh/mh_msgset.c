/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* MH message sets. */

#include <mh.h>

static void
_expand (size_t *msgcnt, size_t **msglist, size_t inc)
{
  if (!inc)
    return;

  *msgcnt += inc;
  *msglist = realloc (*msglist, (*msgcnt)*sizeof(**msglist));
  if (!*msglist)
    {
      mh_error ("not enough memory");
      abort ();
    }
}

static void
msgset_abort (const char *arg)
{
  mh_error ("bad message list `%s'", arg);
  exit (1);
}

static int
msgset_first (mailbox_t mbox, size_t *pnum)
{
  *pnum = 1;
  return 0;
}

static int
msgset_last (mailbox_t mbox, size_t *pnum)
{
  int rc;
  size_t count = 0;

  rc = mailbox_messages_count (mbox, &count);
  if (rc)
    {
      mh_error ("can't get last message: %s", mu_errstring (rc));
      exit (1);
    }
  *pnum = count;
  return 0;
}

static int
msgset_cur (mailbox_t mbox, size_t *pnum)
{
  size_t i, count = 0;
  static int cached_n = 0;

  if (cached_n)
    {
      *pnum = cached_n;
      return 0;
    }
  
  mailbox_messages_count (mbox, &count);
  for (i = 1; i <= count; i++)
    {
      message_t msg = NULL;
      size_t uid = 0;
      
      mailbox_get_message (mbox, i, &msg);
      mh_message_number (msg, &uid);
      if (uid == current_message)
	{
	  *pnum = cached_n = i;
	  return 0;
	}
    }
  mh_error ("no cur message");
  exit (1);
}

static int
msgset_prev (mailbox_t mbox, size_t *pnum)
{
  size_t cur_n = 0;
  msgset_cur (mbox, &cur_n);
  if (cur_n < 1)
    {
      mh_error ("mo prev message");
      exit (1);
    }
  *pnum = cur_n - 1;
  return 0;
}

static int
msgset_next (mailbox_t mbox, size_t *pnum)
{
  size_t cur_n = 0, total = 0;
  msgset_cur (mbox, &cur_n);
  mailbox_messages_count (mbox, &total);
  if (cur_n + 1 > total)
    {
      mh_error ("mo next message");
      exit (1);
    }
  *pnum = cur_n + 1;
  return 0;
}

static struct msgset_keyword {
  char *name;
  int (*handler) __P((mailbox_t mbox, size_t *pnum));
} keywords[] = {
  { "first", msgset_first },
  { "last", msgset_last },
  { "prev", msgset_prev },
  { "next", msgset_next },
  { "cur", msgset_cur },
  { NULL },
};

static char *
msgset_preproc_part (mailbox_t mbox, char *arg, char **rest)
{
  struct msgset_keyword *p;

  for (p = keywords; p->name; p++)
    if (strncmp (arg, p->name, strlen (p->name)) == 0)
      {
	int rc;
	size_t uid, num;
	char *ret = NULL;
	message_t msg;
	
	if (p->handler (mbox, &num))
	  msgset_abort (arg);
	rc = mailbox_get_message (mbox, num, &msg);
	if (rc)
	  {
	    mh_error ("can't get message %d: %s", num, mu_errstring (rc));
	    exit (1);
	  }
	message_get_uid (msg, &uid);
	asprintf (&ret, "%lu", (unsigned long) uid);
	*rest = arg + strlen (p->name);
	return ret;
      }
  *rest = arg + strlen (arg);
  return strdup (arg);
}

static char *
msgset_preproc (mailbox_t mbox, char *arg)
{
  char *buf, *tail;
  
  if (strcmp (arg, "all") == 0)
    {
      /* Special case */
      arg = "first-last";
    }

  buf = msgset_preproc_part (mbox, arg, &tail);
  if (tail[0] == '-')
    {
      char *rest = msgset_preproc_part (mbox, tail+1, &tail);
      char *p = NULL;
      asprintf (&p, "%s-%s", buf, rest);
      free (rest);
      free (buf);
      buf = p;
    }
  
  if (tail[0])
    {
      char *p = NULL;
      asprintf (&p, "%s%s", buf, tail);
      free (buf);
      buf = p;
    }
  return buf;
}

static int
comp_mesg (const void *a, const void *b)
{
  if (*(size_t*)a > *(size_t*)b)
    return 1;
  else if (*(size_t*)a < *(size_t*)b)
    return -1;
  return 0;
}

int
mh_msgset_parse (mailbox_t mbox, mh_msgset_t *msgset, int argc, char **argv)
{
  size_t msgcnt;
  size_t *msglist;
  char *xargv[2];
  size_t i, msgno;
  
  if (argc == 0)
    {
      argc = 1;
      argv = xargv;
      argv[0] = "cur";
      argv[1] = NULL;
    }
  
  msgcnt = argc;
  msglist = calloc (msgcnt, sizeof(*msglist));
  for (i = 0, msgno = 0; i < argc; i++)
    {
      char *p = NULL;
      size_t start, end;
      size_t msg_first, n;
      long num;
      char *arg = msgset_preproc (mbox, argv[i]);
      start = strtoul (arg, &p, 0);
      switch (*p)
	{
	case 0:
	  n = mh_get_message (mbox, start, NULL);
	  if (!n)
	    {
	      mh_error ("message %d does not exist", start);
	      exit (1);
	    }
	  msglist[msgno++] = n;
	  break;
	  
	case '-':
	  end = strtoul (p+1, &p, 0);
	  if (*p || end == start)
	    msgset_abort (argv[i]);
	  if (end < start)
	    {
	      size_t t = start;
	      start = end;
	      end = t;
	    }
	  _expand (&msgcnt, &msglist, end - start);
	  msg_first  = msgno;
	  for (; start <= end; start++)
	    {
	      n = mh_get_message (mbox, start, NULL);
	      if (n)
		msglist[msgno++] = n;
	    }
	  if (msgno == msg_first)
	    {
	      mh_error ("no messages in range %s", argv[i]);
	      exit (1);
	    }
	  break;
	  
	case ':':
	  num = strtoul (p+1, &p, 0);
	  if (*p)
	    msgset_abort (argv[i]);
	  end = start + num;
	  if (end < start)
	    {
	      size_t t = start;
	      start = end + 1;
	      end = t;
	    }
	  else
	    end--;
	  _expand (&msgcnt, &msglist, end - start);
	  msg_first  = msgno;
	  for (; start <= end; start++)
	    {
	      n = mh_get_message (mbox, start, NULL);
	      if (n)
		msglist[msgno++] = n;
	    }
	  if (msgno == msg_first)
	    {
	      mh_error ("no messages in range %s", argv[i]);
	      exit (1);
	    }
	  break;
	  
	default:
	  msgset_abort (argv[i]);
	}
      free (arg);
    }

  msgcnt = msgno;
  
  /* Sort the resulting message set */
  qsort (msglist, msgcnt, sizeof (*msglist), comp_mesg);

  /* Remove duplicates. */
  for (i = 0, msgno = 1; i < msgcnt; i++)
    if (msglist[msgno-1] != msglist[i])
      msglist[msgno++] = msglist[i];
  msgcnt = msgno;
  
  msgset->count = msgcnt;
  msgset->list = msglist;
  return 0;
}

int
mh_msgset_member (mh_msgset_t *msgset, size_t num)
{
  size_t i;

  for (i = 0; i < msgset->count; i++)
    if (msgset->list[i] == num)
      return i + 1;
  return 0;
}

/* Auxiliary function. Performs binary search for a message with the
   given sequence number */
static size_t
mh_search_message (mailbox_t mbox, size_t start, size_t stop,
		   size_t seqno, message_t *mesg)
{
  message_t mid_msg = NULL;
  size_t num = 0, middle;

  middle = (start + stop) / 2;
  if (mailbox_get_message (mbox, middle, &mid_msg)
      || mh_message_number (mid_msg, &num))
    return 0;

  if (num == seqno)
    {
      if (mesg)
	*mesg = mid_msg;
      return middle;
    }
      
  if (start >= stop)
    return 0;

  if (num > seqno)
    return mh_search_message (mbox, start, middle-1, seqno, mesg);
  else /*if (num < seqno)*/
    return mh_search_message (mbox, middle+1, stop, seqno, mesg);
}

/* Retrieve the message with the given sequence number.
   Returns ordinal number of the message in the mailbox if found,
   zero otherwise. The retrieved message is stored in the location
   pointed to by mesg, unless it is NULL. */
   
size_t
mh_get_message (mailbox_t mbox, size_t seqno, message_t *mesg)
{
  size_t num, count;
  message_t msg;

  if (mailbox_get_message (mbox, 1, &msg)
      || mh_message_number (msg, &num))
    return 0;
  if (seqno < num)
    return 0;
  else if (seqno == num)
    {
      if (mesg)
	*mesg = msg;
      return 1;
    }

  if (mailbox_messages_count (mbox, &count)
      || mailbox_get_message (mbox, count, &msg)
      || mh_message_number (msg, &num))
    return 0;
  if (seqno > num)
    return 0;
  else if (seqno == num)
    {
      if (mesg)
	*mesg = msg;
      return count;
    }

  return mh_search_message (mbox, 1, count, seqno, mesg);
}


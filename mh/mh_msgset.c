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
msgset_preproc (mailbox_t mbox, char *arg)
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
	asprintf (&ret, "%lu%s", (unsigned long) uid, arg + strlen (p->name));
	return ret;
      }
  return strdup (arg);
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
      long num;
      char *arg = msgset_preproc (mbox, argv[i]);
      start = strtoul (arg, &p, 0);
      switch (*p)
	{
	case 0:
	  msglist[msgno++] = start;
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
	  for (; start <= end; start++)
	    msglist[msgno++] = start;
	  break;
	  
	case ':':
	  num = strtoul (p+1, &p, 0);
	  if (*p)
	    msgset_abort (argv[i]);
	  end = start + num;
	  if (end < start)
	    {
	      size_t t = start;
	      start = end;
	      end = t;
	    }
	  _expand (&msgcnt, &msglist, end - start);
	  for (; start <= end; start++)
	    msglist[msgno++] = start;
	  break;
	  
	default:
	  msgset_abort (argv[i]);
	}
      free (arg);
    }

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

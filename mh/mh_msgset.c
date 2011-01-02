/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005, 2006, 2007, 2009, 2010, 2011
   Free Software Foundation, Inc.

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

/* MH message sets. */

#include <mh.h>

void
mh_msgset_init (mh_msgset_t *msgset)
{
  memset (msgset, 0, sizeof (*msgset));
}

void
mh_msgset_expand (mh_msgset_t *msgset, size_t count)
{
  size_t rest = msgset->size - msgset->count;

  if (rest < count)
    {
      msgset->size += count;
      msgset->list = xrealloc (msgset->list,
			       msgset->size * sizeof (msgset->list[0]));
    }
}

void
mh_msgset_add (mh_msgset_t *msgset, size_t n)
{
  mh_msgset_expand (msgset, 1);
  msgset->list[msgset->count++] = n;
}

static int
comp_mesg (const void *a, const void *b)
{
  size_t an = *(size_t*)a;
  size_t bn = *(size_t*)b;
  if (an > bn)
    return 1;
  else if (an < bn)
    return -1;
  return 0;
}

void
mh_msgset_optimize (mh_msgset_t *msgset)
{
  size_t i, msgno;
  size_t msgcnt = msgset->count;
  size_t *msglist = msgset->list;
      
  /* Sort the resulting message set */
  qsort (msglist, msgcnt, sizeof (*msgset->list), comp_mesg);

  /* Remove duplicates. */
  for (i = 0, msgno = 1; i < msgset->count; i++)
    if (msglist[msgno-1] != msglist[i])
      msglist[msgno++] = msglist[i];
  msgset->count = msgno;
}

/* Check if message with ordinal number `num' is contained in the
   message set. */
int
mh_msgset_member (mh_msgset_t *msgset, size_t num)
{
  size_t i;

  for (i = 0; i < msgset->count; i++)
    if (msgset->list[i] == num)
      return i + 1;
  return 0;
}

/* Reverse the order of messages in the message set */
void
mh_msgset_reverse (mh_msgset_t *msgset)
{
  int head, tail;

  for (head = 0, tail = msgset->count-1; head < tail; head++, tail--)
    {
      size_t val = msgset->list[head];
      msgset->list[head] = msgset->list[tail];
      msgset->list[tail] = val;
    }
}

/* Set the current message to that contained at position `index'
   in the given message set */
void
mh_msgset_current (mu_mailbox_t mbox, mh_msgset_t *msgset, int index)
{
  mu_message_t msg = NULL;
  int rc;
  size_t cur;
  
  rc = mu_mailbox_get_message (mbox, msgset->list[index], &msg);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_get_message", NULL, rc);
      exit (1);
    }
  mh_message_number (msg, &cur);
  mh_mailbox_set_cur (mbox, cur);
}

/* Free memory allocated for the message set. Note, that the msgset
   itself is supposed to reside in the statically allocated memory and
   therefore is not freed */
void
mh_msgset_free (mh_msgset_t *msgset)
{
  if (msgset->count)
    free (msgset->list);
}

/* Negate the message set: on return `msgset' consists of the messages
   _not contained_ in the input message set. Any memory associated with
   the input message set is freed */
void
mh_msgset_negate (mu_mailbox_t mbox, mh_msgset_t *msgset)
{
  size_t i, total = 0, msgno;
  size_t *list;

  mu_mailbox_messages_count (mbox, &total);
  list = calloc (total, sizeof (list[0]));
  if (!list)
    mh_err_memory (1);
  for (i = 1, msgno = 0; i <= total; i++)
    {
      if (!mh_msgset_member (msgset, i))
	list[msgno++] = i;
    }

  list = realloc (list, sizeof (list[0]) * msgno);
  if (!list)
    {
      mu_error (_("not enough memory"));
      abort ();
    }
  mh_msgset_free (msgset);
  msgset->count = msgno;
  msgset->list = list;
}

void
mh_msgset_uids (mu_mailbox_t mbox, mh_msgset_t *msgset)
{
  size_t i;

  if (msgset->flags & MH_MSGSET_UID)
    return;
  for (i = 0; i < msgset->count; i++)
    {
      mu_message_t msg;
      mu_mailbox_get_message (mbox, msgset->list[i], &msg);
      mh_message_number (msg, &msgset->list[i]);
    }
  msgset->flags |= MH_MSGSET_UID;
}


struct msgset_parser
{
  mu_mailbox_t mbox;
  mh_msgset_t *msgset;
  char *curp;
  int argc;
  char **argv;

  int sign;
  size_t number;
  int validuid;
};

static void
msgset_parser_init (struct msgset_parser *parser, mu_mailbox_t mbox,
		    mh_msgset_t *msgset, int argc, char **argv)
{
  parser->mbox = mbox;
  parser->msgset = msgset;
  parser->argc = argc;
  parser->argv = argv;
  parser->curp = "";

  parser->sign = 0;
  parser->number = 0;
}

static void
msgset_abort (const char *arg)
{
  mu_error (_("bad message list `%s'"), arg);
  exit (1);
}

static void
emptyrange_abort (const char *range)
{
  mu_error (_("no messages in range %s"), range);
  exit (1);
}

/* Advance parser to the next argument */
static int
nextarg (struct msgset_parser *parser)
{
  if (parser->argc == 0)
    return 0;
  parser->argc--;
  parser->curp = *parser->argv++;
  return 1;
}

static void msgset_parser_run (struct msgset_parser *parser);

static int
_expand_sequence (struct msgset_parser *parser, char *term)
{
  struct mu_wordsplit ws;
  const char *listp;
  int negate = 0;

  listp = mh_global_sequences_get (parser->mbox, term, NULL);
  if (!listp)
    {
      int len;
      const char *neg = mh_global_profile_get ("Sequence-Negation", NULL);
      if (!neg)
	return 1;
      len = strlen (neg);
      if (strncmp (term, neg, len))
	return 1;
      negate = 1;
      listp = mh_global_sequences_get (parser->mbox, term + len, NULL);
      if (!listp)
	return 1;
    }

  if (mu_wordsplit (listp, &ws, MU_WRDSF_DEFFLAGS))
    {
      mu_error (_("cannot split line `%s': %s"), listp,
		mu_wordsplit_strerror (&ws));
      exit (1);
    }
  else
    {
      struct msgset_parser clone;
      
      msgset_parser_init (&clone, parser->mbox,  parser->msgset,
			  ws.ws_wordc, ws.ws_wordv);
      msgset_parser_run (&clone);
      mu_wordsplit_free (&ws);
    }
  
  if (negate)
    mh_msgset_negate (parser->mbox, parser->msgset);
  return 0;
}

static int
parse_count (struct msgset_parser *parser)
{
  char *endp;
  if (!*parser->curp && nextarg (parser) == 0)
    return 0;
  if (*parser->curp == '-')
    {
      parser->sign = 1;
      parser->curp++;
    }
  else if (*parser->curp == '+')
    {
      parser->sign = 0;
      parser->curp++;
    }
  parser->number = strtoul (parser->curp, &endp, 10);
  if (*endp)
    msgset_abort (parser->curp);
  parser->curp = endp;
  return 1;
}


static int
msgset_first (mu_mailbox_t mbox, size_t *pnum)
{
  *pnum = 1;
  return 0;
}

static int
msgset_last (mu_mailbox_t mbox, size_t *pnum)
{
  int rc;

  rc = mu_mailbox_messages_count (mbox, pnum);
  if (rc)
    {
      mu_error (_("cannot get last message: %s"), mu_strerror (rc));
      exit (1);
    }
  return 0;
}

static int
msgset_cur (mu_mailbox_t mbox, size_t *pnum)
{
  size_t num;
  mh_mailbox_get_cur (mbox, &num);
  return mu_mailbox_translate (mbox, MU_MAILBOX_UID_TO_MSGNO, num, pnum);
}

static int
msgset_prev (mu_mailbox_t mbox, size_t *pnum)
{
  size_t cur_n = 0;
  msgset_cur (mbox, &cur_n);
  if (cur_n < 1)
    {
      mu_error (_("no prev message"));
      exit (1);
    }
  *pnum = cur_n - 1;
  return 0;
}

static int
msgset_next (mu_mailbox_t mbox, size_t *pnum)
{
  size_t cur_n = 0, total = 0;
  msgset_cur (mbox, &cur_n);
  mu_mailbox_messages_count (mbox, &total);
  if (cur_n + 1 > total)
    {
      mu_error (_("no next message"));
      exit (1);
    }
  *pnum = cur_n + 1;
  return 0;
}

struct msgset_keyword
{
  char *name;
  size_t len;
  int (*handler) (mu_mailbox_t mbox, size_t *pnum);
  int sign;
};

static struct msgset_keyword keywords[] = {
#define S(s) #s, sizeof (#s) - 1
  { S(first), msgset_first, 0 },
  { S(last), msgset_last, 1 },
  { S(prev), msgset_prev, 1 },
  { S(next), msgset_next, 0 },
  { S(cur), msgset_cur, 0 },
  { NULL }
};

#define PARSE_EOF  0
#define PARSE_MORE 1
#define PARSE_SUCCESS 2

/* term : NUMBER
        | "first"
	| "last"
	| "cur"
	| "prev"
	| "next"
	;
*/
static int
parse_term (struct msgset_parser *parser, int seq)
{
  size_t tlen;
  char *term;
  
  if (!*parser->curp && nextarg (parser) == 0)
    return PARSE_EOF;

  term = parser->curp;
  parser->curp = mu_str_skip_class (term, MU_CTYPE_ALPHA|MU_CTYPE_DIGIT);
  tlen = parser->curp - term;
  if (mu_isalpha (*term))
    {
      struct msgset_keyword *p;

      for (p = keywords; p->name; p++)
	if (tlen == p->len && memcmp (p->name, term, tlen) == 0)
	  {
	    if (p->handler (parser->mbox, &parser->number))
	      msgset_abort (term);
	    parser->sign = p->sign;
	    parser->validuid = 1;
	    return PARSE_MORE;
	  }

      if (*parser->curp == 0 && seq)
	{
	  /* See if it is a user-defined sequence */
	  if (_expand_sequence (parser, term) == 0)
	    return PARSE_SUCCESS;
	}
      msgset_abort (term);
    }
  else if (mu_isdigit (*term))
    {
      char *endp;
      size_t num = strtoul (term, &endp, 10);
      if (endp != parser->curp)
	msgset_abort (term);
      
      if (mu_mailbox_translate (parser->mbox, MU_MAILBOX_UID_TO_MSGNO,
				num, &parser->number))
	{
	  parser->validuid = 0;
	  parser->number = num;
	}
      else
	parser->validuid = 1;
      parser->sign = 0;
    }
  else
    msgset_abort (term);
  return PARSE_MORE;
}

static void
add_messages (struct msgset_parser *parser, size_t start, size_t count,
	      int sign)
{
  size_t i;

  if (start == 0)
    start = 1;
  mh_msgset_expand (parser->msgset, count);
  if (sign)
    {
      if (count > start)
	count = start;
      for (i = 0; i < count; i++, start--)
	parser->msgset->list[parser->msgset->count++] = start;
    }
  else
    {
      size_t total;
  
      mu_mailbox_messages_count (parser->mbox, &total);
      if (start + count > total)
	count = total - start + 1;
      for (i = 0; i < count; i++, start++)
	parser->msgset->list[parser->msgset->count++] = start;
    }
  if (count == 0)
    emptyrange_abort (parser->argv[-1]);
}

static void
add_message_range (struct msgset_parser *parser, size_t start, size_t end)
{
  if (end == start)
    emptyrange_abort (parser->argv[-1]);

  if (end < start)
    {
      size_t t = start;
      start = end;
      end = t;
    }
  mh_msgset_expand (parser->msgset, end - start + 1);

  for (; start <= end; start++)
    parser->msgset->list[parser->msgset->count++] = start;
}

/* range: term '-' term
        | term ':' count
	;
   count: NUMBER
        | '+' NUMBER
	| '-' NUMBER
	;
*/	
static int
parse_range (struct msgset_parser *parser)
{
  size_t start;

  switch (parse_term (parser, 1))
    {
    case PARSE_EOF:
      return 0;

    case PARSE_SUCCESS:
      return 1;

    case PARSE_MORE:
      break;
    }
      
  start = parser->number;

  if (*parser->curp == ':')
    {
      int validuid = parser->validuid;
      parser->curp++;
      if (parse_count (parser) == 0)
	return 0;
      if (!validuid)
	{
	  size_t total, lastuid;
	  msgset_last (parser->mbox, &total);
	  mu_mailbox_translate (parser->mbox, MU_MAILBOX_MSGNO_TO_UID,
				total, &lastuid);
	  if (start > lastuid)
	    {
	      if (!parser->sign)
		emptyrange_abort (parser->argv[-1]);
	      start = total;
	    }
	  else
	    {
	      if (parser->sign)
		emptyrange_abort (parser->argv[-1]);
	      start = 1;
	    }
	}
      add_messages (parser, start, parser->number, parser->sign);
      return 1;
    }
  else if (*parser->curp == '-')
    {
      size_t lastuid = 0;
      int validuid = parser->validuid;
      
      parser->curp++;
      if (parse_term (parser, 0) == PARSE_EOF)
	return 0;
      if (!parser->validuid)
	{
	  size_t total;

	  msgset_last (parser->mbox, &total);
	  mu_mailbox_translate (parser->mbox, MU_MAILBOX_MSGNO_TO_UID,
				total, &lastuid);

	  if (parser->number > lastuid)
	    parser->number = total;
	  else if (!validuid)
	    emptyrange_abort (parser->argv[-1]);
	}
      if (!validuid)
	{
	  if (!lastuid)
	    {
	      size_t total;
	      msgset_last (parser->mbox, &total);
	      mu_mailbox_translate (parser->mbox, MU_MAILBOX_MSGNO_TO_UID,
				    total, &lastuid);
	    }
	  if (start > lastuid && !parser->validuid)
	    emptyrange_abort (parser->argv[-1]);
	  start = 1;
	}
      add_message_range (parser, start, parser->number);
    }
  else if (!parser->validuid)
    {
      mu_error (_("message %s does not exist"), parser->argv[-1]);
      exit (1);
    }
  else
    mh_msgset_add (parser->msgset, start);
  return 1;
}
  
  
/* Parse a message specification. The composed msgset is
   not sorted nor optimised */
static void
msgset_parser_run (struct msgset_parser *parser)
{
  while (parse_range (parser))
    ;
}

/* Parse a message specification from (argc;argv). Returned msgset is
   sorted and optimised (i.e. it does not contain duplicate message
   numbers) */
void
mh_msgset_parse (mu_mailbox_t mbox, mh_msgset_t *msgset, 
		 int argc, char **argv, char *def)
{
  struct msgset_parser parser;
  char *xargv[2];
  
  if (argc == 0)
    {
      argc = 1;
      argv = xargv;
      argv[0] = def ? def : "cur";
      argv[1] = NULL;
    }

  if (argc == 1 &&
      (strcmp (argv[0], "all") == 0 || strcmp (argv[0], ".") == 0))
    {
      argc = 1;
      argv = xargv;
      argv[0] = "first-last";
      argv[1] = NULL;
    }
  
  mh_msgset_init (msgset);
  msgset_parser_init (&parser, mbox, msgset, argc, argv);
  msgset_parser_run (&parser);

  mh_msgset_optimize (msgset);
}


/* Retrieve the message with the given sequence number.
   Returns ordinal number of the message in the mailbox if found,
   zero otherwise. The retrieved message is stored in the location
   pointed to by mesg, unless it is NULL. */
   
size_t
mh_get_message (mu_mailbox_t mbox, size_t seqno, mu_message_t *mesg)
{
  int rc;
  size_t num;

  if (mu_mailbox_translate (mbox, MU_MAILBOX_UID_TO_MSGNO, seqno, &num))
    return 0;
  if (mesg)
    {
      rc = mu_mailbox_get_message (mbox, num, mesg);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_get_message", NULL, rc);
	  exit (1);
	}
    }
  return num;
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

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
#include <mailutils/sys/msgset.h>

size_t
mh_msgset_first (mu_msgset_t msgset)
{
  mu_list_t list;
  struct mu_msgrange *r;
  int rc;
  
  rc = mu_msgset_get_list (msgset, &list);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_get_list", NULL, rc);
      exit (1);
    }
  rc = mu_list_head (list, (void**)&r);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_get", NULL, rc);
      exit (1);
    }
  return r->msg_beg;
}

size_t
mh_msgset_first_uid (mu_msgset_t msgset)
{
  int rc;
  size_t cur;

  cur = mh_msgset_first (msgset);
  rc = mu_mailbox_translate (msgset->mbox, MU_MAILBOX_MSGNO_TO_UID, cur, &cur);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_translate", NULL, rc);
      exit (1);
    }
  return cur;
}

/* Set the current message to that contained at position 0
   in the given message set.
   FIXME: mbox is superfluous
*/
void
mh_msgset_first_current (mu_mailbox_t mbox, mu_msgset_t msgset)
{
  mh_mailbox_set_cur (mbox, mh_msgset_first_uid (msgset));
}

int
mh_msgset_single_message (mu_msgset_t msgset)
{
  int rc;
  mu_list_t list;
  struct mu_msgrange *r;
  size_t count;
  
  rc = mu_msgset_get_list (msgset, &list);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_get_list", NULL, rc);
      exit (1);
    }
  rc = mu_list_count (list, &count);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_count", NULL, rc);
      exit (1);
    }
  if (count != 1)
    return 0;
  rc = mu_list_head (list, (void**)&r);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_get", NULL, rc);
      exit (1);
    }
  return r->msg_beg == r->msg_end;
}


struct msgset_parser
{
  mu_msgset_t msgset;
  char *curp;
  int argc;
  char **argv;

  int sign;
  size_t number;
  int validuid;
};

static void
msgset_parser_init (struct msgset_parser *parser, mu_mailbox_t mbox,
		    int argc, char **argv)
{
  int rc;
  
  rc = mu_msgset_create (&parser->msgset, mbox, MU_MSGSET_NUM);//FIXME: flags?
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_create", NULL, rc);
      exit (1);
    }
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

  listp = mh_global_sequences_get (parser->msgset->mbox, term, NULL);
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
      listp = mh_global_sequences_get (parser->msgset->mbox, term + len, NULL);
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
      int rc;
      struct msgset_parser clone;

      msgset_parser_init (&clone, parser->msgset->mbox,
			  ws.ws_wordc, ws.ws_wordv);
      msgset_parser_run (&clone);
      mu_wordsplit_free (&ws);
      if (negate)
	{
	  mu_msgset_t negset;
	  
	  rc = mu_msgset_negate (clone.msgset, &negset);
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_negate", NULL, rc);
	      exit (1);
	    }
	  mu_msgset_free (clone.msgset);
	  clone.msgset = negset;
	}
      rc = mu_msgset_add (parser->msgset, clone.msgset);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_add", NULL, rc);
	  exit (1);
	}
      mu_msgset_free (clone.msgset);
    }
  
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
	    size_t num;
	    
	    if (p->handler (parser->msgset->mbox, &num))
	      msgset_abort (term);
	    parser->number = num;
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
      
      if (mu_mailbox_translate (parser->msgset->mbox,
				MU_MAILBOX_UID_TO_MSGNO,
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
  int rc;

  if (start == 0)
    start = 1;
  if (sign)
    {
      if (count > start)
	count = start;
      rc = mu_msgset_add_range (parser->msgset, start, start - count + 1,
				MU_MSGSET_NUM);
    }
  else
    {
      size_t total;
  
      mu_mailbox_messages_count (parser->msgset->mbox, &total);
      if (start + count > total)
	{
	  count = total - start + 1;
	  if (count == 0)
	    emptyrange_abort (parser->argv[-1]);
	}
      rc = mu_msgset_add_range (parser->msgset, start, start + count - 1,
				MU_MSGSET_NUM);
    }

  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_add_range", NULL, rc);
      exit (1);
    }
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
	  msgset_last (parser->msgset->mbox, &total);
	  mu_mailbox_translate (parser->msgset->mbox,
				MU_MAILBOX_MSGNO_TO_UID,
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
      int rc;
      
      parser->curp++;
      if (parse_term (parser, 0) == PARSE_EOF)
	return 0;
      if (!parser->validuid)
	{
	  size_t total;

	  msgset_last (parser->msgset->mbox, &total);
	  mu_mailbox_translate (parser->msgset->mbox, MU_MAILBOX_MSGNO_TO_UID,
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
	      msgset_last (parser->msgset->mbox, &total);
	      mu_mailbox_translate (parser->msgset->mbox,
				    MU_MAILBOX_MSGNO_TO_UID,
				    total, &lastuid);
	    }
	  if (start > lastuid && !parser->validuid)
	    emptyrange_abort (parser->argv[-1]);
	  start = 1;
	}
      rc = mu_msgset_add_range (parser->msgset, start, parser->number,
				MU_MSGSET_NUM);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_add_range", NULL, rc);
	  exit (1);
	}
    }
  else if (!parser->validuid)
    {
      mu_error (_("message %s does not exist"), parser->argv[-1]);
      exit (1);
    }
  else
    mu_msgset_add_range (parser->msgset, start, start, MU_MSGSET_NUM);
  return 1;
}
  
  
/* Parse a message specification saved in a configured PARSER. */
static void
msgset_parser_run (struct msgset_parser *parser)
{
  while (parse_range (parser))
    ;
}

/* Parse a message specification from (argc;argv).  */
void
mh_msgset_parse (mu_msgset_t *msgset, mu_mailbox_t mbox, 
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
  
  msgset_parser_init (&parser, mbox, argc, argv);
  msgset_parser_run (&parser);
  *msgset = parser.msgset;
}

void
mh_msgset_parse_string (mu_msgset_t *msgset, mu_mailbox_t mbox, 
			const char *string, char *def)
{
  struct mu_wordsplit ws;
  
  if (mu_wordsplit (string, &ws, MU_WRDSF_DEFFLAGS))
    {
      mu_error (_("cannot split line `%s': %s"), string,
		mu_wordsplit_strerror (&ws));
      exit (1);
    }
  mh_msgset_parse (msgset, mbox, ws.ws_wordc, ws.ws_wordv, def);
  mu_wordsplit_free (&ws);
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

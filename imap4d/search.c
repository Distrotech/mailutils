/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2005, 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "imap4d.h"

/*
 * This will be a royal pain in the arse to implement
 * Alain: True, but the new lib mailbox should coming handy with
 * some sort of query interface.
 * Sergey: It was, indeed.
 */

/* Implementation details:

   The searching criteria are parsed and compiled into the array of
   instructions. Each item of the array is either an instruction (inst_t
   type) or the argument to previous instruction. The code array is
   terminated by NULL. The result of `executing' the code array is
   0 or 1 depending on whether current message meets the search
   conditions.

   Note/FIXME: Implementing boolean shortcuts would speed up the long
   search queries. */

struct parsebuf;
typedef void (*inst_t) (struct parsebuf *pb);

static void cond_and (struct parsebuf *pb);
static void cond_or (struct parsebuf *pb);
static void cond_not (struct parsebuf *pb);

static void cond_all (struct parsebuf *pb);                      
static void cond_msgset (struct parsebuf *pb);                      
static void cond_bcc (struct parsebuf *pb);                      
static void cond_before (struct parsebuf *pb);                   
static void cond_body (struct parsebuf *pb);                     
static void cond_cc (struct parsebuf *pb);                       
static void cond_from (struct parsebuf *pb);                     
static void cond_header (struct parsebuf *pb);                   
static void cond_keyword (struct parsebuf *pb);                  
static void cond_larger (struct parsebuf *pb);                   
static void cond_on (struct parsebuf *pb);                       
static void cond_sentbefore (struct parsebuf *pb);               
static void cond_senton (struct parsebuf *pb);                   
static void cond_sentsince (struct parsebuf *pb);                
static void cond_since (struct parsebuf *pb);                    
static void cond_smaller (struct parsebuf *pb);                  
static void cond_subject (struct parsebuf *pb);                  
static void cond_text (struct parsebuf *pb);                     
static void cond_to (struct parsebuf *pb);                       
static void cond_uid (struct parsebuf *pb);                      

/* A basic condition structure */
struct cond
{
  char *name;          /* Condition name */
  char *argtypes;      /* String of argument types or NULL if takes no args */
  inst_t inst;         /* Corresponding instruction */
};

/* Types are: s -- string
              n -- number
	      d -- date
	      m -- message set
*/

/* List of basic conditions. <message set> is handled separately */
struct cond condlist[] =
{
  { "ALL",        NULL,   cond_all },
  { "BCC",        "s",  cond_bcc },
  { "BEFORE",     "d",  cond_before },
  { "BODY",       "s",  cond_body },
  { "CC",         "s",  cond_cc },
  { "FROM",       "s",  cond_from },
  { "HEADER",     "ss", cond_header },
  { "KEYWORD",    "s",  cond_keyword },
  { "LARGER",     "n",  cond_larger },
  { "ON",         "d",  cond_on },
  { "SENTBEFORE", "d",  cond_sentbefore },
  { "SENTON",     "d",  cond_senton },
  { "SENTSINCE",  "d",  cond_sentsince },
  { "SINCE",      "d",  cond_since },
  { "SMALLER",    "n",  cond_smaller },
  { "SUBJECT",    "s",  cond_subject },
  { "TEXT",       "s",  cond_text },
  { "TO",         "s",  cond_to },
  { "UID",        "m",  cond_uid },
  { NULL }
};

/* Other search keys described by rfc2060 are implemented on top of these
   basic conditions. Condition equivalence structure defines the equivalent
   condition in terms of basic ones. (Kind of macro substitution) */

struct cond_equiv
{
  char *name;           /* RFC2060 search key name */
  char *equiv;          /* Equivalent query in terms of basic conds */
};

struct cond_equiv equiv_list[] =
{
  { "ANSWERED",   "KEYWORD \\Answered" },
  { "DELETED",    "KEYWORD \\Deleted" },
  { "DRAFT",      "KEYWORD \\Draft" },
  { "FLAGGED",    "KEYWORD \\Flagged" },
  { "NEW",        "(RECENT UNSEEN)" },
  { "OLD",        "NOT RECENT" },
  { "RECENT",     "KEYWORD \\Recent" },
  { "SEEN",       "KEYWORD \\Seen" },
  { "UNANSWERED", "NOT KEYWORD \\Answered" },
  { "UNDELETED",  "NOT KEYWORD \\Deleted" },
  { "UNDRAFT",    "NOT KEYWORD \\Draft" },
  { "UNFLAGGED",  "NOT KEYWORD \\Flagged" },
  { "UNKEYWORD",  "NOT KEYWORD" },
  { "UNSEEN",     "NOT KEYWORD \\Seen" },
  { NULL }
};

/* A memory allocation chain used to keep track of objects allocated during
   the recursive-descent parsing. */
struct mem_chain {
  struct mem_chain *next;
  void *mem;
};

/* Code and stack sizes for execution of compiled search statement */
#define CODESIZE 64
#define CODEINCR 16
#define STACKSIZE 64
#define STACKINCR 16

/* Maximum length of a token. Tokens longer than that are accepted, provided
   that they are enclosed in doublequotes */
#define MAXTOKEN 64 

/* Parse buffer structure */
struct parsebuf
{
  char *token;                  /* Current token. Either points to tokbuf
				   or is allocated within `alloc' chain */
  char tokbuf[MAXTOKEN+1];      /* Token buffer for short tokens */

  char *arg;                    /* Rest of command line to be parsed */
  int isuid;                    /* UIDs instead of msgnos are required */ 
  char *err_mesg;               /* Error message if a parse error occured */
  struct mem_chain *alloc;      /* Chain of objects allocated during parsing */
  
  int codesize;                 /* Current size of allocated code */
  inst_t *code;                 /* Code buffer */
  int pc;                       /* Program counter. On parse time points
				   to the next free slot in `code' array.
				   On execution time, points to the next
				   instruction to be executed */

  int stacksize;                /* Current size of allocated stack */
  int *stack;                   /* Stack buffer. */
  int tos;                      /* Top of stack */

                                /* Execution time only: */
  size_t msgno;                 /* Number of current message */
  mu_message_t msg;                /* Current message */ 
};

static void put_code (struct parsebuf *pb, inst_t inst);
static void parse_free_mem (struct parsebuf *pb);
static void *parse_regmem (struct parsebuf *pb, void *mem);
static char *parse_strdup (struct parsebuf *pb, char *s);
static void *parse_alloc (struct parsebuf *pb, size_t size);
static int parse_search_key_list (struct parsebuf *pb);
static int parse_search_key (struct parsebuf *pb);
static int parse_gettoken (struct parsebuf *pb, int req);
static int search_run (struct parsebuf *pb);
static void do_search (struct parsebuf *pb);

int
imap4d_search (struct imap4d_command *command, char *arg)
{
  int rc;
  char buffer[64];
  
  rc = imap4d_search0 (arg, 0, buffer, sizeof buffer);
  return util_finish (command, rc, "%s", buffer);
}
  
int
imap4d_search0 (char *arg, int isuid, char *replybuf, size_t replysize)
{
  struct parsebuf parsebuf;
  
  memset (&parsebuf, 0, sizeof(parsebuf));
  parsebuf.arg = arg;
  parsebuf.err_mesg = NULL;
  parsebuf.alloc = NULL;
  parsebuf.isuid = isuid;

  if (!parse_gettoken (&parsebuf, 0))
    {
      snprintf (replybuf, replysize, "Too few args");
      return RESP_BAD;
    }
  
  if (strcasecmp (parsebuf.token, "CHARSET") == 0)
    {
      if (!parse_gettoken (&parsebuf, 0))
	{
	  snprintf (replybuf, replysize, "Too few args");
	  return RESP_BAD;
	}

      /* Currently only ASCII is supported */
      if (strcasecmp (parsebuf.token, "US-ASCII"))
	{
	  snprintf (replybuf, replysize, "Charset not supported");
	  return RESP_NO;
	}

      if (!parse_gettoken (&parsebuf, 0))
	{
	  snprintf (replybuf, replysize, "Too few args");
	  return RESP_BAD;
	}

    }

  /* Compile the expression */
  if (parse_search_key_list (&parsebuf))
    {
      parse_free_mem (&parsebuf);
      snprintf (replybuf, replysize, "%s (near %s)",
			  parsebuf.err_mesg,
			  *parsebuf.arg ? parsebuf.arg : "end");
      return RESP_BAD;
    }

  if (parsebuf.token[0] != 0)
    {
      parse_free_mem (&parsebuf);
      snprintf (replybuf, replysize, "Junk at the end of statement");
      return RESP_BAD;
    }
  
  put_code (&parsebuf, NULL);

  /* Execute compiled expression */
  do_search (&parsebuf);
  
  parse_free_mem (&parsebuf);
  
  snprintf (replybuf, replysize, "Completed");
  return RESP_OK;
}

/* For each message from the mailbox execute the query from `pb' and
   output the message number if the query returned 1 */
void
do_search (struct parsebuf *pb)
{
  size_t count = 0;
  
  mu_mailbox_messages_count (mbox, &count);

  util_send ("* SEARCH");
  for (pb->msgno = 1; pb->msgno <= count; pb->msgno++)
    {
      if (mu_mailbox_get_message (mbox, pb->msgno, &pb->msg) == 0
	  && search_run (pb))
	{
	  if (pb->isuid)
	    {
	      size_t uid;
	      mu_message_get_uid (pb->msg, &uid);
	      util_send (" %s", mu_umaxtostr (0, uid));
	    }
	  else
	    util_send (" %s", mu_umaxtostr (0, pb->msgno));
	}
    }
  util_send ("\r\n");
}

/* Parse buffer functions */

int
parse_gettoken (struct parsebuf *pb, int req)
{
  int rc;
  char *s;
  
  pb->token = pb->tokbuf;
  while (*pb->arg && *pb->arg == ' ')
    pb->arg++;
  switch (*pb->arg)
    {
    case '(':
    case ')':
      pb->token[0] = *pb->arg++;
      pb->token[1] = 0;
      rc = 1;
      break;
    case '"':
      s = ++pb->arg;
      while (*pb->arg && *pb->arg != '"')
	pb->arg++;
      rc = pb->arg - s;
      if (*pb->arg)
	pb->arg++;

      if (rc >= sizeof(pb->tokbuf))
	pb->token = parse_alloc (pb, rc+1);
      memcpy (pb->token, s, rc);
      pb->token[rc] = 0;
      break;
      
    default:
      rc = util_token (pb->token, sizeof(pb->tokbuf), &pb->arg);
      break;
    }
  if (req && rc == 0)
    pb->err_mesg = "Unexpected end of statement";
  return rc;
}

/* Memory handling */

/* Free all memory allocated for parsebuf structure */
void
parse_free_mem (struct parsebuf *pb)
{
  struct mem_chain *alloc, *next;
  alloc = pb->alloc;
  while (alloc)
    {
      next = alloc->next;
      free (alloc->mem);
      free (alloc);
      alloc = next;
    }
  if (pb->code)
    free (pb->code);
  if (pb->stack)
    free (pb->stack);
}

/* Register a memory pointer mem with the parsebuf */
void *
parse_regmem (struct parsebuf *pb, void *mem)
{
  struct mem_chain *mp;

  mp = malloc (sizeof(*mp));
  if (!mp)
    imap4d_bye (ERR_NO_MEM);
  mp->next = pb->alloc;
  pb->alloc = mp;
  mp->mem = mem;
  return mem;
}

/* Allocate `size' bytes of memory within parsebuf structure */
void *
parse_alloc (struct parsebuf *pb, size_t size)
{
  void *p = malloc (size);
  if (!p)
    imap4d_bye (ERR_NO_MEM);
  return parse_regmem (pb, p);
}

/* Create a copy of the string. */ 
char *
parse_strdup (struct parsebuf *pb, char *s)
{
  s = strdup (s);
  if (!s)
    imap4d_bye (ERR_NO_MEM);
  return parse_regmem (pb, s);
}

/* A recursive-descent parser for the following grammar:
   search_key_list : search_key
                   | search_key_list search_key
                   ;

   search_key      : simple_key
                   | NOT simple_key
                   | OR simple_key simple_key
                   | '(' search_key_list ')'
		   ;
*/

int parse_simple_key (struct parsebuf *pb);
int parse_equiv_key (struct parsebuf *pb);

int
parse_search_key_list (struct parsebuf *pb)
{
  int count = 0;
  while (pb->token[0] && pb->token[0] != ')')
    {
      if (parse_search_key (pb))
	return 1;
      if (count++)
	put_code (pb, cond_and);
    }
  return 0;
}

int
parse_search_key (struct parsebuf *pb)
{
  if (strcmp (pb->token, "(") == 0)
    {
      if (parse_gettoken (pb, 1) == 0
	  || parse_search_key_list (pb))
	return 1;
      if (strcmp (pb->token, ")"))
	{
	  pb->err_mesg = "Unbalanced parenthesis";
	  return 1;
	}
      parse_gettoken (pb, 0);
      return 0;
    }
  else if (strcasecmp (pb->token, "NOT") == 0)
    {
      if (parse_gettoken (pb, 1) == 0
	  || parse_search_key (pb))
	return 1;
      put_code (pb, cond_not);
      return 0;
    }
  else if (strcasecmp (pb->token, "OR") == 0)
    {
      if (parse_gettoken (pb, 1) == 0
	  || parse_search_key (pb)
	  || parse_search_key (pb))
	return 1;
      put_code (pb, cond_or);
      return 0;
    }
  else
    return parse_equiv_key (pb);
}

int
parse_equiv_key (struct parsebuf *pb)
{
  struct cond_equiv *condp;
  char *arg;
  char *save_arg;
  
  for (condp = equiv_list; condp->name && strcasecmp (condp->name, pb->token);
       condp++)
    ;

  if (!condp->name)
    return parse_simple_key (pb);

  save_arg = pb->arg;
  arg = parse_strdup (pb, condp->equiv);
  pb->arg = arg;

  parse_gettoken (pb, 0);

  if (parse_search_key_list (pb))
    {
      /* shouldn't happen */
      syslog(LOG_CRIT, _("%s:%d: INTERNAL ERROR (please report)"),
	     __FILE__, __LINE__);
      abort (); 
    }
  
  pb->arg = save_arg;
  parse_gettoken (pb, 0);
  return 0;
}

int
parse_simple_key (struct parsebuf *pb)
{
  struct cond *condp;
  time_t time;
  size_t *set = NULL;
  int n = 0;
  
  for (condp = condlist; condp->name && strcasecmp (condp->name, pb->token);
       condp++)
    ;

  if (!condp->name)
    {
      if (util_msgset (pb->token, &set, &n, 0) == 0) 
	{
	  put_code (pb, cond_msgset);
	  put_code (pb, (inst_t) n);
	  put_code (pb, (inst_t) parse_regmem (pb, set));
	  parse_gettoken (pb, 0);
	  return 0;
	}
      else
	{
	  pb->err_mesg = "Unknown search criterion";
	  return 1;
	}
    }

  put_code (pb, condp->inst);
  parse_gettoken (pb, 0);
  if (condp->argtypes)
    {
      char *t = condp->argtypes;
      char *s;
      int n;
      size_t *set;
      
      for (; *t; t++, parse_gettoken (pb, 0))
	{
	  if (!pb->token[0])
	    {
	      pb->err_mesg = "Not enough arguments for criterion";
	      return 1;
	    }
	  
	  switch (*t)
	    {
	    case 's': /* string */
	      put_code (pb, (inst_t) parse_strdup (pb, pb->token));
	      break;
	    case 'n': /* number */
	      n = strtoul (pb->token, &s, 10);
	      if (*s)
		{
		  pb->err_mesg = "Invalid number";
		  return 1;
		}
	      put_code (pb, (inst_t) n);
	      break;
	    case 'd': /* date */
	      if (util_parse_internal_date (pb->token, &time))
		{
		  pb->err_mesg = "Bad date format";
		  return 1;
		}
	      put_code (pb, (inst_t) time);
	      break;
	    case 'm': /* message set */
	      if (util_msgset (pb->token, &set, &n, 1)) /*FIXME: isuid?*/
		{
		  pb->err_mesg = "Bogus number set";
		  return 1;
		}
	      put_code (pb, (inst_t) n);
	      put_code (pb, (inst_t) parse_regmem (pb, set));
	      break;
	    default:
	      syslog(LOG_CRIT, _("%s:%d: INTERNAL ERROR (please report)"),
		     __FILE__, __LINE__);
	      abort (); /* should never happen */
	    }
	}  
    }
  return 0;
}

/* Code generator */

void
put_code (struct parsebuf *pb, inst_t inst)
{
  if (pb->codesize == 0)
    {
      pb->codesize = CODESIZE;
      pb->code = calloc (CODESIZE, sizeof (pb->code[0]));
      if (!pb->code)
	imap4d_bye (ERR_NO_MEM);
      pb->pc = 0;
    }
  else if (pb->pc >= pb->codesize)
    {
      inst_t *new_code;

      pb->codesize += CODEINCR;
      new_code = realloc (pb->code, pb->codesize*sizeof(pb->code[0]));
      if (!new_code)
	imap4d_bye (ERR_NO_MEM);
      pb->code = new_code;
    }
  pb->code[pb->pc++] = inst;
}

/* The machine */
static void *
_search_arg (struct parsebuf *pb)
{
  return (void*)pb->code[pb->pc++];
}

static void
_search_push (struct parsebuf *pb, int val)
{
  if (pb->tos == pb->stacksize)
    {
      if (pb->stacksize == 0)
	{
	  pb->stacksize = STACKSIZE;
	  pb->stack = calloc (STACKSIZE, sizeof(pb->stack[0]));
	}
      else
	{
	  pb->stacksize += STACKINCR;
	  pb->stack = realloc (pb->stack, pb->stacksize*sizeof(pb->stack[0]));
	}
      if (!pb->stack)
	imap4d_bye (ERR_NO_MEM);
    }
  pb->stack[pb->tos++] = val;
}

static int
_search_pop (struct parsebuf *pb)
{
  if (pb->tos == 0)
    {
      syslog(LOG_CRIT, _("%s:%d: INTERNAL ERROR (please report)"), __FILE__, __LINE__);
      abort (); /* shouldn't happen */
    }
  return pb->stack[--pb->tos];
}

/* Executes a query from parsebuf */
int
search_run (struct parsebuf *pb)
{
  pb->pc = 0;
  while (pb->code[pb->pc] != NULL)
    (*pb->code[pb->pc++]) (pb);
  return _search_pop (pb);
}

/* Helper functions for evaluationg conditions */

/* Scan the header of a message for the occurence of field named `name'.
   Return true if any of the occurences contained substring `value' */
static int
_scan_header (struct parsebuf *pb, char *name, char *value)
{
  char buffer[512];
  mu_header_t header = NULL;
  
  mu_message_get_header (pb->msg, &header);
  if (!mu_header_get_value (header, name, buffer, sizeof(buffer), NULL))
    {
      return util_strcasestr (buffer, value) != NULL;
    }
  return 0;
}

/* Get the value of Date: field and convert it to timestamp */
static int
_header_date (struct parsebuf *pb, time_t *timep)
{
  char buffer[512];
  mu_header_t header = NULL;
  
  mu_message_get_header (pb->msg, &header);
  if (!mu_header_get_value (header, "Date", buffer, sizeof(buffer), NULL)
      && util_parse_822_date (buffer, timep))
    return 0;
  return 1;
}

/* Scan all header fields for the occurence of a substring `text' */
static int
_scan_header_all (struct parsebuf *pb, char *text)
{
  char buffer[512];
  mu_header_t header = NULL;
  size_t fcount = 0;
  int i, rc;

  mu_message_get_header (pb->msg, &header);
  mu_header_get_field_count (header, &fcount);
  for (i = rc = 0; i < fcount; i++)
    {
      if (mu_header_get_field_value (header, i, buffer, sizeof(buffer), NULL))
	rc = util_strcasestr (buffer, text) != NULL;
    }
  return rc;
}

/* Scan body of the message for the occurence of a substring */
static int
_scan_body (struct parsebuf *pb, char *text)
{
  mu_body_t body = NULL;
  mu_stream_t stream = NULL;
  size_t size = 0, lines = 0;
  char buffer[128];
  size_t n = 0;
  off_t offset = 0;
  int rc;
  
  mu_message_get_body (pb->msg, &body);
  mu_body_size (body, &size);
  mu_body_lines (body, &lines);
  mu_body_get_stream (body, &stream);
  rc = 0;
  while (rc == 0
	 && mu_stream_read (stream, buffer, sizeof(buffer)-1, offset, &n) == 0
	 && n > 0)
    {
      buffer[n] = 0;
      offset += n;
      rc = util_strcasestr (buffer, text) != NULL;
    }
  return rc;
}

/* Basic instructions */
void
cond_and (struct parsebuf *pb)
{
  int n1, n2;

  n1 = _search_pop (pb);
  n2 = _search_pop (pb);
  _search_push (pb, n1 && n2);
}                      

void
cond_or (struct parsebuf *pb)
{
  int n1, n2;

  n1 = _search_pop (pb);
  n2 = _search_pop (pb);
  _search_push (pb, n1 || n2);
}                      

void
cond_not (struct parsebuf *pb)
{
  _search_push (pb, !_search_pop (pb));
}

void
cond_all (struct parsebuf *pb)
{
  _search_push (pb, 1);
}                      

void
cond_msgset (struct parsebuf *pb)
{
  int  n = (int)_search_arg (pb);
  size_t *set = (size_t*)_search_arg (pb);
  int i, rc;
  
  for (i = rc = 0; rc == 0 && i < n; i++)
    rc = set[i] == pb->msgno;
      
  _search_push (pb, rc);
}

void
cond_bcc (struct parsebuf *pb)
{
  _search_push (pb, _scan_header (pb, "bcc", _search_arg (pb)));
}                      

void
cond_before (struct parsebuf *pb)
{
  time_t t = (time_t)_search_arg (pb);
  time_t mesg_time = 0;
  const char *date;
  mu_envelope_t env;
  
  mu_message_get_envelope (pb->msg, &env);
  if (mu_envelope_sget_date (env, &date) == 0)
    util_parse_ctime_date (date, &mesg_time);
  _search_push (pb, mesg_time < t);
}                   

void
cond_body (struct parsebuf *pb)
{
  _search_push (pb, _scan_body (pb, _search_arg (pb)));
}                     

void
cond_cc (struct parsebuf *pb)
{
  _search_push (pb, _scan_header (pb, "cc", _search_arg (pb)));
}                       

void
cond_from (struct parsebuf *pb)
{
  char *s = _search_arg (pb);
  mu_envelope_t env;
  const char *from;
  int rc = 0;
  
  mu_message_get_envelope (pb->msg, &env);
  if (mu_envelope_sget_sender (env, &from) == 0)
    rc = util_strcasestr (from, s) != NULL;
  _search_push (pb, _scan_header (pb, "from", s));
}                     

void
cond_header (struct parsebuf *pb)
{
  char *name = _search_arg (pb);
  char *value = _search_arg (pb);
  
  _search_push (pb, _scan_header (pb, name, value));
}                   

void
cond_keyword (struct parsebuf *pb)
{
  char *s = _search_arg (pb);
  mu_attribute_t attr = NULL;
  
  mu_message_get_attribute (pb->msg, &attr);
  _search_push (pb, util_attribute_matches_flag (attr, s));
}                  

void
cond_larger (struct parsebuf *pb)
{
  int n = (int) _search_arg (pb);
  size_t size = 0;
  
  mu_message_size (pb->msg, &size);
  _search_push (pb, size > n);
}                   

void
cond_on (struct parsebuf *pb)
{
  time_t t = (time_t)_search_arg (pb);
  time_t mesg_time = 0;
  const char *date;
  mu_envelope_t env;
  
  mu_message_get_envelope (pb->msg, &env);
  if (mu_envelope_sget_date (env, &date) == 0)
    util_parse_ctime_date (date, &mesg_time);
  _search_push (pb, t <= mesg_time && mesg_time <= t + 86400);
}                       

void
cond_sentbefore (struct parsebuf *pb)
{
  time_t t = (time_t)_search_arg (pb);
  time_t mesg_time = 0;

  _header_date (pb, &mesg_time);
  _search_push (pb, mesg_time < t);
}               

void
cond_senton (struct parsebuf *pb)
{
  time_t t = (time_t)_search_arg (pb);
  time_t mesg_time = 0;

  _header_date (pb, &mesg_time);
  _search_push (pb, t <= mesg_time && mesg_time <= t + 86400);
}                   

void
cond_sentsince (struct parsebuf *pb)
{
  time_t t = (time_t)_search_arg (pb);
  time_t mesg_time = 0;

  _header_date (pb, &mesg_time);
  _search_push (pb, mesg_time >= t);
}                

void
cond_since (struct parsebuf *pb)
{
  time_t t = (time_t)_search_arg (pb);
  time_t mesg_time = 0;
  const char *date;
  mu_envelope_t env;
  
  mu_message_get_envelope (pb->msg, &env);
  if (mu_envelope_sget_date (env, &date) == 0)
    util_parse_ctime_date (date, &mesg_time);
  _search_push (pb, mesg_time >= t);
}                    

void
cond_smaller (struct parsebuf *pb)
{
  int n = (int) _search_arg (pb);
  size_t size = 0;
  
  mu_message_size (pb->msg, &size);
  _search_push (pb, size < n);
}                  

void
cond_subject (struct parsebuf *pb)
{
  _search_push (pb, _scan_header (pb, "subject", _search_arg (pb)));
}                  

void
cond_text (struct parsebuf *pb)
{
  char *s = _search_arg (pb);
  _search_push (pb, _scan_header_all (pb, s) || _scan_body (pb, s));
}                     

void
cond_to (struct parsebuf *pb)
{
  _search_push (pb, _scan_header (pb, "to", _search_arg (pb)));
}                       

void
cond_uid (struct parsebuf *pb)
{
  int  n = (int)_search_arg (pb);
  size_t *set = (size_t*)_search_arg (pb);
  size_t uid = 0;
  int i, rc;
  
  mu_message_get_uid (pb->msg, &uid);
  for (i = rc = 0; rc == 0 && i < n; i++)
    rc = set[i] == uid;
      
  _search_push (pb, rc);
}                      


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2002, 2005, 2007-2012 Free Software
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

#include "imap4d.h"

/*
 * This will be a royal pain in the arse to implement
 * Alain: True, but the new lib mailbox should coming handy with
 * some sort of query interface.
 * Sergey: It was, indeed.
 */

/* Implementation details:

   The searching criteria are parsed and a parse tree is created. Each
   node is of type search_node (see below) and contains either data
   (struct value) or an instruction, which evaluates to a boolean value.

   The function search_run recursively evaluates the tree and returns a
   boolean number, 0 or 1 depending on whether the current message meets
   the search conditions. */

struct parsebuf;

enum value_type
  {
    value_undefined,
    value_number,
    value_string,
    value_date,
    value_msgset
  };

enum node_type
  {
    node_call,
    node_and,
    node_or,
    node_not,
    node_value
  };

struct value
{
  enum value_type type;
  union
  {
    char *string;
    mu_off_t number;
    time_t date;
    mu_msgset_t msgset;
  } v;
};

#define MAX_NODE_ARGS 2

struct search_node;

typedef void (*instr_fn) (struct parsebuf *, struct search_node *,
			  struct value *, struct value *);

struct search_node
{
  enum node_type type;
  union
  {
    struct key_node
    {
      char *keyword;
      instr_fn fun;
      int narg;
      struct search_node *arg[MAX_NODE_ARGS];
    } key;
    struct search_node *arg[2]; /* Binary operation */
    struct value value;
  } v;
};

static void cond_msgset (struct parsebuf *, struct search_node *,
			 struct value *, struct value *);       
static void cond_bcc (struct parsebuf *, struct search_node *,
		      struct value *, struct value *);
static void cond_before (struct parsebuf *, struct search_node *,
			 struct value *, struct value *);     
static void cond_body (struct parsebuf *, struct search_node *,
		       struct value *, struct value *);       
static void cond_cc (struct parsebuf *, struct search_node *,
		     struct value *, struct value *);        
static void cond_from (struct parsebuf *, struct search_node *,
		       struct value *, struct value *);     
static void cond_header (struct parsebuf *, struct search_node *,
			 struct value *, struct value *);      
static void cond_keyword (struct parsebuf *, struct search_node *,
			  struct value *, struct value *);   
static void cond_larger (struct parsebuf *, struct search_node *,
			 struct value *, struct value *);   
static void cond_on (struct parsebuf *, struct search_node *,
		     struct value *, struct value *);         
static void cond_sentbefore (struct parsebuf *, struct search_node *,
			     struct value *, struct value *); 
static void cond_senton (struct parsebuf *, struct search_node *,
			 struct value *, struct value *);     
static void cond_sentsince (struct parsebuf *, struct search_node *,
			    struct value *, struct value *);
static void cond_since (struct parsebuf *, struct search_node *,
			struct value *, struct value *);      
static void cond_smaller (struct parsebuf *, struct search_node *,
			  struct value *, struct value *);    
static void cond_subject (struct parsebuf *, struct search_node *,
			  struct value *, struct value *);  
static void cond_text (struct parsebuf *, struct search_node *,
		       struct value *, struct value *);       
static void cond_to (struct parsebuf *, struct search_node *,
		     struct value *, struct value *);      
static void cond_uid (struct parsebuf *, struct search_node *,
		      struct value *, struct value *);   

/* A basic condition structure */
struct cond
{
  char *name;          /* Condition name */
  char *argtypes;      /* String of argument types or NULL if it takes no
			  args */
  instr_fn inst;       /* Corresponding instruction function */
};

/* Types are: s -- string
              n -- number
	      d -- date
	      m -- message set
*/

/* List of basic conditions. "ALL" and <message set> is handled separately */
struct cond condlist[] =
{
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
  { "UID",        "u",  cond_uid },
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
   the recursive-descend parsing. */
struct mem_chain
{
  struct mem_chain *next;
  void *mem;
  void (*free_fun) (void *);
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
  imap4d_tokbuf_t tok;          /* Token buffer */   
  int arg;                      /* Argument number */
  char *token;                  /* Current token */
  int isuid;                    /* UIDs instead of msgnos are required */ 
  char *err_mesg;               /* Error message if a parse error occured */
  struct mem_chain *alloc;      /* Chain of objects allocated during parsing */
  
  struct search_node *tree;     /* Parse tree */
  
                                /* Execution time only: */
  size_t msgno;                 /* Number of current message */
  mu_message_t msg;             /* Current message */ 
};

static void parse_free_mem (struct parsebuf *pb);
static void *parse_regmem (struct parsebuf *pb, void *mem, void (*f)(void*));
static char *parse_strdup (struct parsebuf *pb, char *s);
static void *parse_alloc (struct parsebuf *pb, size_t size);
static struct search_node *parse_search_key_list (struct parsebuf *pb);
static struct search_node *parse_search_key (struct parsebuf *pb);
static int parse_gettoken (struct parsebuf *pb, int req);
static int search_run (struct parsebuf *pb);
static void do_search (struct parsebuf *pb);

/*
6.4.4.  SEARCH Command

   Arguments:  OPTIONAL [CHARSET] specification
               searching criteria (one or more)

   Responses:  REQUIRED untagged response: SEARCH

   Result:     OK - search completed
               NO - search error: can't search that [CHARSET] or
                    criteria
               BAD - command unknown or arguments invalid
*/

int
imap4d_search (struct imap4d_session *session,
               struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  int rc;
  char *err_text= "";
  
  rc = imap4d_search0 (tok, 0, &err_text);
  return io_completion_response (command, rc, "%s", err_text);
}
  
int
imap4d_search0 (imap4d_tokbuf_t tok, int isuid, char **err_text)
{
  struct parsebuf parsebuf;
  
  memset (&parsebuf, 0, sizeof(parsebuf));
  parsebuf.tok = tok;
  parsebuf.arg = IMAP4_ARG_1 + !!isuid;
  parsebuf.err_mesg = NULL;
  parsebuf.alloc = NULL;
  parsebuf.isuid = isuid;

  if (!parse_gettoken (&parsebuf, 0))
    {
      *err_text = "Too few args";
      return RESP_BAD;
    }
  
  if (mu_c_strcasecmp (parsebuf.token, "CHARSET") == 0)
    {
      if (!parse_gettoken (&parsebuf, 0))
	{
	  *err_text = "Too few args";
	  return RESP_BAD;
	}

      /* Currently only ASCII is supported */
      if (mu_c_strcasecmp (parsebuf.token, "US-ASCII"))
	{
	  *err_text = "Charset not supported";
	  return RESP_NO;
	}

      if (!parse_gettoken (&parsebuf, 0))
	{
	  *err_text = "Too few args";
	  return RESP_BAD;
	}

    }

  /* Compile the expression */
  parsebuf.tree = parse_search_key_list (&parsebuf);
  if (!parsebuf.tree)
    {
      parse_free_mem (&parsebuf);
      *err_text = "Parse error";
      return RESP_BAD;
    }

  if (parsebuf.token)
    {
      parse_free_mem (&parsebuf);
      *err_text = "Junk at the end of statement";
      return RESP_BAD;
    }
  
  /* Execute compiled expression */
  do_search (&parsebuf);
  
  parse_free_mem (&parsebuf);
  
  *err_text = "Completed";
  return RESP_OK;
}

/* For each message from the mailbox execute the query from `pb' and
   output the message number if the query returned 1 */
void
do_search (struct parsebuf *pb)
{
  size_t count = 0;
  
  mu_mailbox_messages_count (mbox, &count);

  io_sendf ("* SEARCH");
  for (pb->msgno = 1; pb->msgno <= count; pb->msgno++)
    {
      if (mu_mailbox_get_message (mbox, pb->msgno, &pb->msg) == 0
	  && search_run (pb))
	{
	  if (pb->isuid)
	    {
	      size_t uid;
	      mu_message_get_uid (pb->msg, &uid);
	      io_sendf (" %s", mu_umaxtostr (0, uid));
	    }
	  else
	    io_sendf (" %s", mu_umaxtostr (0, pb->msgno));
	}
    }
  io_sendf ("\n");
}

/* Parse buffer functions */

int
parse_gettoken (struct parsebuf *pb, int req)
{
  if (req && pb->arg >= imap4d_tokbuf_argc (pb->tok))
    {
      pb->err_mesg = "Unexpected end of statement";
      return 0;
    }
  pb->token = imap4d_tokbuf_getarg (pb->tok, pb->arg++);
  return 1;
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
      if (alloc->free_fun)
	alloc->free_fun (alloc->mem);
      else
	free (alloc->mem);
      free (alloc);
      alloc = next;
    }
}

/* Register a memory pointer mem with the parsebuf */
void *
parse_regmem (struct parsebuf *pb, void *mem, void (*f) (void*))
{
  struct mem_chain *mp;

  mp = mu_alloc (sizeof(*mp));
  mp->next = pb->alloc;
  pb->alloc = mp;
  mp->mem = mem;
  mp->free_fun = f;
  return mem;
}

/* Allocate `size' bytes of memory within parsebuf structure */
void *
parse_alloc (struct parsebuf *pb, size_t size)
{
  void *p = mu_alloc (size);
  return parse_regmem (pb, p, NULL);
}

/* Create a copy of the string. */ 
char *
parse_strdup (struct parsebuf *pb, char *s)
{
  s = mu_strdup (s);
  if (!s)
    imap4d_bye (ERR_NO_MEM);
  return parse_regmem (pb, s, NULL);
}

static void
free_msgset (void *ptr)
{
  mu_msgset_free (ptr);
}

mu_msgset_t
parse_msgset_create (struct parsebuf *pb, mu_mailbox_t mbox, int flags)
{
  mu_msgset_t msgset;

  if (mu_msgset_create (&msgset, mbox, flags))
    imap4d_bye (ERR_NO_MEM);
  return parse_regmem (pb, msgset, free_msgset);
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

struct search_node *parse_simple_key (struct parsebuf *pb);
struct search_node *parse_equiv_key (struct parsebuf *pb);

struct search_node *
parse_search_key_list (struct parsebuf *pb)
{
  struct search_node *leftarg = NULL;

  while (pb->token && pb->token[0] != ')')
    {
      struct search_node *rightarg = parse_search_key (pb);
      if (!rightarg)
	return NULL;
      if (!leftarg)
	leftarg = rightarg;
      else
	{
	  struct search_node *node = parse_alloc (pb, sizeof *node);
	  node->type = node_and;
	  node->v.arg[0] = leftarg;
	  node->v.arg[1] = rightarg;
	  leftarg = node;
	}
    }
  return leftarg;
}

struct search_node *
parse_search_key (struct parsebuf *pb)
{
  struct search_node *node;
  
  if (strcmp (pb->token, "(") == 0)
    {
      if (parse_gettoken (pb, 1) == 0)
	return NULL;
      
      node = parse_search_key_list (pb);
      if (!node)
	return NULL;
      
      if (strcmp (pb->token, ")"))
	{
	  pb->err_mesg = "Unbalanced parenthesis";
	  return NULL;
	}
      parse_gettoken (pb, 0);
      return node;
    }
  else if (mu_c_strcasecmp (pb->token, "ALL") == 0)
    {
      node = parse_alloc (pb, sizeof *node);
      node->type = node_value;
      node->v.value.type = value_number;
      node->v.value.v.number = 1;

      parse_gettoken (pb, 0);
      return node;
    }
  else if (mu_c_strcasecmp (pb->token, "NOT") == 0)
    {
      struct search_node *np;

      if (parse_gettoken (pb, 1) == 0)
	 return NULL;

      np = parse_search_key (pb);
      if (!np)
	 return NULL;

      node = parse_alloc (pb, sizeof *node);
      node->type = node_not;
      node->v.arg[0] = np;

      return node;
    }
  else if (mu_c_strcasecmp (pb->token, "OR") == 0)
    {
      struct search_node *leftarg, *rightarg;

      if (parse_gettoken (pb, 1) == 0)
	 return NULL;

      if ((leftarg = parse_search_key (pb)) == NULL
	   || (rightarg = parse_search_key (pb)) == NULL)
	 return NULL;

      node = parse_alloc (pb, sizeof *node);
      node->type = node_or;
      node->v.arg[0] = leftarg;
      node->v.arg[1] = rightarg;

      return node;
    }
  else
    return parse_equiv_key (pb);
}

struct search_node *
parse_equiv_key (struct parsebuf *pb)
{
  struct search_node *node;
  struct cond_equiv *condp;
  int save_arg;
  imap4d_tokbuf_t save_tok;

  for (condp = equiv_list; condp->name && mu_c_strcasecmp (condp->name, pb->token);
	condp++)
    ;

  if (!condp->name)
    return parse_simple_key (pb);

  save_arg = pb->arg;
  save_tok = pb->tok;
  pb->tok = imap4d_tokbuf_from_string (condp->equiv);
  pb->arg = 0;

  parse_gettoken (pb, 0);

  node = parse_search_key_list (pb);
  if (!node)
    {
      /* shouldn't happen? */
      mu_diag_output (MU_DIAG_CRIT, _("%s:%d: INTERNAL ERROR (please report)"),
		       __FILE__, __LINE__);
      abort (); 
    }
  imap4d_tokbuf_destroy (&pb->tok);

  pb->arg = save_arg;
  pb->tok = save_tok;
  parse_gettoken (pb, 0);
  return node;
}

struct search_node *
parse_simple_key (struct parsebuf *pb)
{
  struct search_node *node;
  struct cond *condp;
  time_t time;

  for (condp = condlist; condp->name && mu_c_strcasecmp (condp->name, pb->token);
       condp++)
    ;
  
  if (!condp->name)
    {
      mu_msgset_t msgset = parse_msgset_create (pb, mbox, MU_MSGSET_NUM);
      
      if (mu_msgset_parse_imap (msgset,
				pb->isuid ? MU_MSGSET_UID : MU_MSGSET_NUM,
				pb->token, NULL) == 0) 
	{
	  struct search_node *np = parse_alloc (pb, sizeof *np);
	  np->type = node_value;
	  np->v.value.type = value_msgset;
	  np->v.value.v.msgset = msgset;
	  
	  node = parse_alloc (pb, sizeof *node);
	  node->type = node_call;
	  node->v.key.keyword = "msgset";
	  node->v.key.narg = 1;
	  node->v.key.arg[0] = np;
	  node->v.key.fun = cond_msgset;
	  
	  parse_gettoken (pb, 0);
	  
	  return node;
	}
      else
	{
	  pb->err_mesg = "Unknown search criterion";
	  return NULL;
	}
    }
  
  node = parse_alloc (pb, sizeof *node);
  node->type = node_call;
  node->v.key.keyword = condp->name;
  node->v.key.fun = condp->inst;
  node->v.key.narg = 0;
  
  parse_gettoken (pb, 0);
  if (condp->argtypes)
    {
      char *t = condp->argtypes;
      char *s;
      mu_off_t number;
      struct search_node *arg;
      
      for (; *t; t++, parse_gettoken (pb, 0))
	{
	  if (node->v.key.narg >= MAX_NODE_ARGS)
	    {
	      pb->err_mesg = "INTERNAL ERROR: too many arguments";
	      return NULL;
	    }
	  
	  if (!pb->token)
	    {
	      pb->err_mesg = "Not enough arguments for criterion";
	      return NULL;
	    }
	  
	  arg = parse_alloc (pb, sizeof *arg);
	  arg->type = node_value;
	  switch (*t)
	    {
	    case 's': /* string */
	      arg->v.value.type = value_string;
	      arg->v.value.v.string = parse_strdup (pb, pb->token);
	      break;
	      
	    case 'n': /* number */
	      number = strtoul (pb->token, &s, 10);
	      if (*s)
		{
		  pb->err_mesg = "Invalid number";
		  return NULL;
		}
	      arg->v.value.type = value_number;
	      arg->v.value.v.number = number;
	      break;
	      
	    case 'd': /* date */
	      if (util_parse_internal_date (pb->token, &time,
					    datetime_date_only))
		{
		  pb->err_mesg = "Bad date format";
		  return NULL;
		}
	      arg->v.value.type = value_date;
	      arg->v.value.v.date = time;
	      break;
	      
	    case 'u': /* UID message set */
	      arg->v.value.v.msgset = parse_msgset_create (pb, NULL,
							   MU_MSGSET_NUM);
	      if (mu_msgset_parse_imap (arg->v.value.v.msgset, MU_MSGSET_UID,
					pb->token, NULL)) 
		{
		  mu_msgset_free (arg->v.value.v.msgset);
		  pb->err_mesg = "Bogus number set";
		  return NULL;
		}
	      arg->v.value.type = value_msgset;
	      break;
	      
	    default:
	      mu_diag_output (MU_DIAG_CRIT,
			      _("%s:%d: INTERNAL ERROR (please report)"),
			      __FILE__, __LINE__);
	      abort (); /* should never happen */
	    }
	  node->v.key.arg[node->v.key.narg++] = arg;
	}  
    }
  return node;
}

/* Executes a query from parsebuf */
void
evaluate_node (struct search_node *node, struct parsebuf *pb,
	       struct value *val)
{
  int i;
  struct value argval[MAX_NODE_ARGS];
  
  switch (node->type)
    {
    case node_call:
      for (i = 0; i < node->v.key.narg; i++)
	{
	  /* FIXME: if (i >= MAX_NODE_ARGS) */
	  evaluate_node (node->v.key.arg[i], pb, &argval[i]);
	  /* FIXME: node types? */
	}
      
      node->v.key.fun (pb, node, argval, val);
      break;

    case node_and:
      val->type = value_number;
      evaluate_node (node->v.arg[0], pb, &argval[0]);
      if (argval[0].v.number == 0)
	val->v.number = 0;
      else
	{
	  evaluate_node (node->v.arg[1], pb, &argval[1]);
	  val->v.number = argval[1].v.number;
	}
      break;
      
    case node_or:
      val->type = value_number;
      evaluate_node (node->v.arg[0], pb, &argval[0]);
      if (argval[0].v.number)
	val->v.number = 1;
      else
	{
	  evaluate_node (node->v.arg[1], pb, &argval[1]);
	  val->v.number = argval[1].v.number;
	}
      break;

    case node_not:
      evaluate_node (node->v.arg[0], pb, &argval[0]);
      val->type = value_number;
      val->v.number = !argval[0].v.number;
      break;
      
    case node_value:
      *val = node->v.value;
      break;
    }
}

int
search_run (struct parsebuf *pb)
{
  struct value value;

  value.type = value_undefined;
  evaluate_node (pb->tree, pb, &value);
  if (value.type != value_number)
    {
      mu_diag_output (MU_DIAG_CRIT, _("%s:%d: INTERNAL ERROR (please report)"),
	     __FILE__, __LINE__);
      abort (); /* should never happen */
    }
  return value.v.number != 0;
}

/* Helper functions for evaluationg conditions */

/* Scan the header of a message for the occurence of field named `name'.
   Return true if any of the occurences contained substring `value' */
static int
_scan_header (struct parsebuf *pb, char *name, char *value)
{
  const char *hval;
  mu_header_t header = NULL;
  
  mu_message_get_header (pb->msg, &header);
  if (mu_header_sget_value (header, name, &hval) == 0)
    {
      return util_strcasestr (hval, value) != NULL;
    }
  return 0;
}

/* Get the value of Date: field and convert it to timestamp */
static int
_header_date (struct parsebuf *pb, time_t *timep)
{
  const char *hval;
  mu_header_t header = NULL;
  
  mu_message_get_header (pb->msg, &header);
  if (mu_header_sget_value (header, "Date", &hval) == 0
      && util_parse_822_date (hval, timep, datetime_date_only))
    return 0;
  return 1;
}

/* Scan all header fields for the occurence of a substring `text' */
static int
_scan_header_all (struct parsebuf *pb, char *text)
{
  const char *hval;
  mu_header_t header = NULL;
  size_t fcount = 0;
  int i, rc;

  mu_message_get_header (pb->msg, &header);
  mu_header_get_field_count (header, &fcount);
  for (i = rc = 0; i < fcount; i++)
    {
      if (mu_header_sget_field_value (header, i, &hval) == 0)
	rc = util_strcasestr (hval, text) != NULL;
    }
  return rc;
}

/* Scan body of the message for the occurrence of a substring */
/* FIXME: The algorithm below is broken */
static int
_scan_body (struct parsebuf *pb, char *text)
{
  mu_body_t body = NULL;
  mu_stream_t stream = NULL;
  size_t size = 0, lines = 0;
  char buffer[128];
  size_t n = 0;
  int rc;
  
  mu_message_get_body (pb->msg, &body);
  mu_body_size (body, &size);
  mu_body_lines (body, &lines);
  mu_body_get_streamref (body, &stream);
  rc = 0;
  while (rc == 0
	 && mu_stream_read (stream, buffer, sizeof(buffer)-1, &n) == 0
	 && n > 0)
    {
      buffer[n] = 0;
      rc = util_strcasestr (buffer, text) != NULL;
    }
  mu_stream_destroy (&stream);
  return rc;
}

/* Basic instructions */

static void
cond_msgset (struct parsebuf *pb, struct search_node *node, struct value *arg,
	     struct value *retval)
{
  int rc = mu_msgset_locate (arg[0].v.msgset, pb->msgno, NULL);
  retval->type = value_number;
  retval->v.number = rc == 0;
}

static void
cond_bcc (struct parsebuf *pb, struct search_node *node, struct value *arg,
	  struct value *retval)
{
  retval->type = value_number;
  retval->v.number = _scan_header (pb, MU_HEADER_BCC, arg[0].v.string);
}                      

static void
cond_before (struct parsebuf *pb, struct search_node *node, struct value *arg,
	     struct value *retval)
{
  time_t t = arg[0].v.date;
  time_t mesg_time;
  const char *date;
  mu_envelope_t env;
  
  mu_message_get_envelope (pb->msg, &env);
  retval->type = value_number;
  if (mu_envelope_sget_date (env, &date))
    retval->v.number = 0;
  else
    {
      util_parse_ctime_date (date, &mesg_time, datetime_date_only);
      retval->v.number = mesg_time < t;
    }
}                   

static void
cond_body (struct parsebuf *pb, struct search_node *node, struct value *arg,
	   struct value *retval)
{
  retval->type = value_number;
  retval->v.number = _scan_body (pb, arg[0].v.string);
}                     

static void
cond_cc (struct parsebuf *pb, struct search_node *node, struct value *arg,
	 struct value *retval)
{
  retval->type = value_number;
  retval->v.number = _scan_header (pb, MU_HEADER_CC, arg[0].v.string);
}                       

static void
cond_from (struct parsebuf *pb, struct search_node *node, struct value *arg,
	   struct value *retval)
{
  char *s = arg[0].v.string;
  mu_envelope_t env;
  const char *from;
  int rc = 0;
  
  mu_message_get_envelope (pb->msg, &env);
  if (mu_envelope_sget_sender (env, &from) == 0)
    rc = util_strcasestr (from, s) != NULL;
  
  retval->type = value_number;
  retval->v.number = rc || _scan_header (pb, MU_HEADER_FROM, s);
}                     

static void
cond_header (struct parsebuf *pb, struct search_node *node, struct value *arg,
	     struct value *retval)
{
  char *name = arg[0].v.string;
  char *value = arg[1].v.string;

  retval->type = value_number;
  retval->v.number = _scan_header (pb, name, value);
}                   

static void
cond_keyword (struct parsebuf *pb, struct search_node *node, struct value *arg,
	     struct value *retval)
{
  char *s = arg[0].v.string;
  mu_attribute_t attr = NULL;
  
  mu_message_get_attribute (pb->msg, &attr);
  retval->type = value_number;
  retval->v.number = util_attribute_matches_flag (attr, s);
}                  

static void
cond_larger (struct parsebuf *pb, struct search_node *node, struct value *arg,
	     struct value *retval)
{
  size_t size = 0;
  
  mu_message_size (pb->msg, &size);
  retval->type = value_number;
  retval->v.number = size > arg[0].v.number;
}                   

static void
cond_on (struct parsebuf *pb, struct search_node *node, struct value *arg,
	 struct value *retval)
{
  time_t t = arg[0].v.date;
  time_t mesg_time;
  const char *date;
  mu_envelope_t env;
  
  mu_message_get_envelope (pb->msg, &env);
  retval->type = value_number;
  if (mu_envelope_sget_date (env, &date))
    retval->v.number = 0;
  else
    {
      util_parse_ctime_date (date, &mesg_time, datetime_date_only);
      retval->v.number = t <= mesg_time && mesg_time <= t + 86400;
    }
}                       

static void
cond_sentbefore (struct parsebuf *pb, struct search_node *node,
		 struct value *arg,
		 struct value *retval)
{
  time_t t = arg[0].v.date;
  time_t mesg_time = 0;

  _header_date (pb, &mesg_time);
  retval->type = value_number;
  retval->v.number = mesg_time < t;
}               

static void
cond_senton (struct parsebuf *pb, struct search_node *node, struct value *arg,
	     struct value *retval)
{
  time_t t = arg[0].v.date;
  time_t mesg_time = 0;

  _header_date (pb, &mesg_time);
  retval->type = value_number;
  retval->v.number = t <= mesg_time && mesg_time <= t + 86400;
}                   

static void
cond_sentsince (struct parsebuf *pb, struct search_node *node,
		struct value *arg,
		struct value *retval)
{
  time_t t = arg[0].v.date;
  time_t mesg_time = 0;

  _header_date (pb, &mesg_time);
  retval->type = value_number;
  retval->v.number = mesg_time >= t;
}                

static void
cond_since (struct parsebuf *pb, struct search_node *node, struct value *arg,
	    struct value *retval)
{
  time_t t = arg[0].v.date;
  time_t mesg_time;
  const char *date;
  mu_envelope_t env;
  
  mu_message_get_envelope (pb->msg, &env);
  retval->type = value_number;
  if (mu_envelope_sget_date (env, &date))
    retval->v.number = 0;
  else
    {
      util_parse_ctime_date (date, &mesg_time, datetime_date_only);
      retval->v.number = mesg_time >= t;
    }
}                    

static void
cond_smaller (struct parsebuf *pb, struct search_node *node, struct value *arg,
	      struct value *retval)
{
  size_t size = 0;
  
  mu_message_size (pb->msg, &size);
  retval->type = value_number;
  retval->v.number = size < arg[0].v.number;
}                  

static void
cond_subject (struct parsebuf *pb, struct search_node *node, struct value *arg,
	      struct value *retval)
{
  retval->type = value_number;
  retval->v.number = _scan_header (pb, MU_HEADER_SUBJECT, arg[0].v.string);
}                  

static void
cond_text (struct parsebuf *pb, struct search_node *node, struct value *arg,
	   struct value *retval)
{
  char *s = arg[0].v.string;
  retval->type = value_number;
  retval->v.number = _scan_header_all (pb, s) || _scan_body (pb, s);
}                     

static void
cond_to (struct parsebuf *pb, struct search_node *node, struct value *arg,
	 struct value *retval)
{
  retval->type = value_number;
  retval->v.number = _scan_header (pb, MU_HEADER_TO, arg[0].v.string);
}                       

static void
cond_uid (struct parsebuf *pb, struct search_node *node, struct value *arg,
	  struct value *retval)
{
  int rc;
  size_t uid = 0;
  
  mu_message_get_uid (pb->msg, &uid);
  rc = mu_msgset_locate (arg[0].v.msgset, pb->msgno, NULL);
  retval->type = value_number;
  retval->v.number = rc == 0;
}                      


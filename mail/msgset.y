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

%{
#include <stdio.h>
#include <stdlib.h>

#include <xalloc.h>  
#include <mail.h>

static msgset_t *msgset_select (int (*sel)(), void *closure, int rev,
				int max_matches);
static int select_subject (message_t msg, void *closure);
static int select_type (message_t msg, void *closure);
static int select_sender (message_t msg, void *closure);
static int select_deleted (message_t msg, void *closure);
 
static msgset_t *result;
%}

%union {
  char *string;
  int number;
  int type;
  msgset_t *mset;
}

%token <type> TYPE
%token <string> IDENT REGEXP
%token <number> NUMBER
%type <mset> msgset msgspec msg rangeset range partno number
%%

input    : /* empty */
           {
	     result = msgset_make_1 (cursor);
	   }
         | '.'
           {
	     result = msgset_make_1 (cursor);
	   }
         | msgset
           {
	     result = $1;
	   }
         | '^'
           {
	     result = msgset_select (select_deleted, NULL, 0, 1);
	   }
         | '$'
           {
	     result = msgset_select (select_deleted, NULL, 1, 1);
	   }
         | '*'
           {
	     result = msgset_range (1, total);
	   }
         | '-'
           {
	     result = msgset_select (select_deleted, NULL, 1, 1);
	   }
         | '+'
           {
	     result = msgset_select (select_deleted, NULL, 0, 1);
	   }
         ;

msgset   : msgspec
         | msgset ',' msgspec
           {
	     $$ = msgset_append ($1, $3);
	   }
         | msgset msgspec
           {
	     $$ = msgset_append ($1, $2);
	   }
         ;

msgspec  : msg
         | msg '[' rangeset ']'
           {
	     $$ = msgset_expand ($1, $3);
	     msgset_free ($1);
	     msgset_free ($3);
	   }
         | range
         ;

msg      : REGEXP /* /.../ */
           {
	     util_strupper ($1);
	     $$ = msgset_select (select_subject, &$1, 0, 0);
	     free ($1);
	   }
         | TYPE  /* :n, :d, etc */
           {
	     if (strchr ("dnoru", $1) == NULL)
	       {
		 yyerror ("unknown message type");
		 YYERROR;
	       }
	     $$ = msgset_select (select_type, &$1, 0, 0);
	   }
         | IDENT /* Sender name */
           {
	     $$ = msgset_select (select_sender, &$1, 0, 0);
	     free ($1);
	   }
         ;

rangeset : range
         | rangeset ',' range
           {
	     $$ = msgset_append ($1, $3);
	   }
         | rangeset range
           {
	     $$ = msgset_append ($1, $2);
	   }
         ;

range    : number
         | NUMBER '-' number
           {
	     if ($3->npart == 1)
	       {
		 $$ = msgset_range ($1, $3->msg_part[0]);
	       }
	     else
	       {
		 msgset_t *mp;
		 $$ = msgset_range ($1, $3->msg_part[0]-1);
		 if (!$$)
		   YYERROR;
		 msgset_append ($$, $3);
	       }
	   }
         | NUMBER '-' '*'
           {
	     $$ = msgset_range ($1, total);
	   }
         ;

number   : partno
         | partno '[' rangeset ']'
           {
	     $$ = msgset_expand ($1, $3);
	     msgset_free ($1);
	     msgset_free ($3);
	   }
         ;

partno   : NUMBER
           {
	     $$ = msgset_make_1 ($1);
	   }
         | '(' rangeset ')'
           {
	     $$ = $2;
	   }
         ;
%%

static int xargc;
static char **xargv;
static int cur_ind;
static char *cur_p;

int
yyerror (char *s)
{
  fprintf (stderr, "%s: ", xargv[0]);
  fprintf (stderr, "%s", s);
  if (!cur_p)
    fprintf (stderr, " near end");
  else if (*cur_p == 0)
    {
      int i =  (*cur_p == 0) ? cur_ind + 1 : cur_ind;
      if (i == xargc)
	fprintf (stderr, " near end");
      else
	fprintf (stderr, " near %s", xargv[i]);
    }
  else
    fprintf (stderr, " near %s", cur_p);
  fprintf (stderr, "\n");
}

int
yylex()
{
  if (cur_ind == xargc)
    return 0;
  if (!cur_p)
    cur_p = xargv[cur_ind];
  if (*cur_p == 0)
    {
      cur_ind++;
      cur_p = NULL;
      return yylex ();
    }

  if (isdigit (*cur_p))
    {
      yylval.number = strtoul (cur_p, &cur_p, 10);
      return NUMBER;
    }

  if (isalpha (*cur_p))
    {
      char *p = cur_p;
      int len;
      
      while (*cur_p && *cur_p != ',' && *cur_p != '-')
	cur_p++;
      len = cur_p - p + 1;
      yylval.string = xmalloc (len);
      memcpy (yylval.string, p, len-1);
      yylval.string[len-1] = 0;
      return IDENT;
    }

  if (*cur_p == '/')
    {
      char *p = ++cur_p;
      int len;
      
      while (*cur_p && *cur_p != '/')
	cur_p++;
      len = cur_p - p + 1;
      cur_p++;
      yylval.string = xmalloc (len);
      memcpy (yylval.string, p, len-1);
      yylval.string[len-1] = 0;
      return REGEXP;
    }
  
  if (*cur_p == ':')
    {
      cur_p++;
      yylval.type = *cur_p++;
      return TYPE;
    }
  
  return *cur_p++;
}

int
msgset_parse (const int argc, char **argv, msgset_t **mset)
{
  int rc;
  xargc = argc;
  xargv = argv;
  cur_ind = 1;
  cur_p = NULL;
  result = NULL;
  rc = yyparse ();
  if (rc == 0)
    *mset = result;
  return rc;
}

void
msgset_free (msgset_t *msg_set)
{
  int i;
  msgset_t *next;
  
  if (!msg_set)
    return;
  while (msg_set)
    {
      next = msg_set->next;
      if (msg_set->msg_part)
	free (msg_set->msg_part);
      free (msg_set);
      msg_set = next;
    }
}

/* Create a message set consisting of a single msg_num and no subparts */
msgset_t *
msgset_make_1 (int number)
{
  msgset_t *mp;

  mp = xmalloc (sizeof (*mp));
  mp->next = NULL;
  mp->npart = 1;
  mp->msg_part = xmalloc (sizeof mp->msg_part[0]);
  mp->msg_part[0] = number;
  return mp;
}

msgset_t *
msgset_dup (const msgset_t *set)
{
  msgset_t *mp;
  mp = xmalloc (sizeof (*mp));
  mp->next = NULL;
  mp->npart = set->npart;
  mp->msg_part = xcalloc (mp->npart, sizeof mp->msg_part[0]);
  memcpy (mp->msg_part, set->msg_part, mp->npart * sizeof mp->msg_part[0]);
  return mp;
}

msgset_t *
msgset_append (msgset_t *one, msgset_t *two)
{
  msgset_t *last;

  if (!one)
    return two;
  for (last = one; last->next; last = last->next)
    ;
  last->next = two;
  return one;
}

msgset_t *
msgset_range (int low, int high)
{
  int i;
  msgset_t *mp, *first = NULL, *last;

  if (low == high)
    return msgset_make_1 (low);
  
  if (low >= high)
    {
      yyerror ("range error");
      return NULL;
    }

  for (i = 0; low <= high; i++, low++)
    {
      mp = msgset_make_1 (low);
      if (!first)
	first = mp;
      else
	last->next = mp;
      last = mp;
    }
  return first;
}
  
msgset_t *
msgset_expand (msgset_t *set, msgset_t *expand_by)
{
  msgset_t *i, *j;
  msgset_t *first = NULL, *last, *mp;
  
  for (i = set; i; i = i->next)
    for (j = expand_by; j; j = j->next)
      {
	mp = xmalloc (sizeof *mp);
	mp->next = NULL;
	mp->npart = i->npart + j->npart;
	mp->msg_part = xcalloc (mp->npart, sizeof mp->msg_part[0]);
	memcpy (mp->msg_part, i->msg_part, i->npart * sizeof i->msg_part[0]);
	memcpy (mp->msg_part + i->npart, j->msg_part,
		j->npart * sizeof j->msg_part[0]);

	if (!first)
	  first = mp;
	else
	  last->next = mp;
	last = mp;
      }
  return first;
}

msgset_t *
msgset_select (int (*sel)(), void *closure, int rev, int max_matches)
{
  size_t i, match_count = 0;
  msgset_t *first = NULL, *last, *mp;
  message_t msg = NULL;

  if (max_matches == 0)
    max_matches = total;

  if (rev)
    {
      for (i = total; i > 0; i--)
	{
	  mailbox_get_message (mbox, i, &msg);
	  if ((*sel)(msg, closure))
	    {
	      mp = msgset_make_1 (i);
	      if (!first)
		first = mp;
	      else
		last->next = mp;
	      last = mp;
	      if (++match_count == max_matches)
		break;
	    }
	}
    }
  else
    {
      for (i = 1; i <= total; i++)
	{
	  mailbox_get_message (mbox, i, &msg);
	  if ((*sel)(msg, closure))
	    {
	      mp = msgset_make_1 (i);
	      if (!first)
		first = mp;
	      else
		last->next = mp;
	      last = mp;
	      if (++match_count == max_matches)
		break;
	    }
	}
    }
  return first;
}  

int
select_subject (message_t msg, void *closure)
{
  char *expr = (char*) closure;
  header_t hdr;
  char *subject;
  message_get_header (msg, &hdr);
  if (header_aget_value (hdr, MU_HEADER_SUBJECT, &subject) == 0)
    {
      int rc;
      util_strupper (subject);
      rc = strstr (subject, expr) != NULL;
      free (subject);
      return rc;
    }
  return 0; 
}

int
select_sender (message_t msg, void *closure)
{
  char *sender = (char*) closure;
	  /* FIXME: all messages from sender argv[i] */
	  /* Annoying we can use address_create() for that
	     but to compare against what? The email ?  */
  return 0; 
}

int
select_type (message_t msg, void *closure)
{
  int type = *(int*) closure;
  attribute_t attr= NULL;

  message_get_attribute (msg, &attr);

  switch (type)
    {
    case 'd':
      return attribute_is_deleted (attr);
    case 'n':
      return attribute_is_recent (attr);
    case 'o':
      return attribute_is_seen (attr);
    case 'r':
      return attribute_is_read (attr);
    case 'u':
      return !attribute_is_read (attr);
    }
  return 0;
}

int
select_deleted (message_t msg, void *closure)
{
  attribute_t attr= NULL;
  int rc;
  
  message_get_attribute (msg, &attr);
  rc = attribute_is_deleted (attr);
  return strcmp (xargv[0], "undelete") == 0 ? rc : !rc;
}

#if 0
void
msgset_print (msgset_t *mset)
{
  int i;
  printf ("(");
  printf ("%d .", mset->msg_part[0]);
  for (i = 1; i < mset->npart; i++)
    {
      printf (" %d", mset->msg_part[i]);
    }
  printf (")\n");
}

int
main(int argc, char **argv)
{
  msgset_t *mset = NULL;
  int rc = parse_msgset (argc, argv, &mset);

  for (; mset; mset = mset->next)
    msgset_print (mset);
  return 0;
}
#endif

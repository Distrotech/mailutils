%{
/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005-2012, 2014-2016 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sieve-priv.h>
#include <mailutils/stdstream.h>

mu_sieve_machine_t mu_sieve_machine;
int mu_sieve_error_count;
static struct mu_sieve_node *sieve_tree;

static struct mu_sieve_node *node_alloc (enum mu_sieve_node_type,
					 struct mu_locus_range *);
static void cond_join (struct mu_sieve_node *node);

#define YYLLOC_DEFAULT(Current, Rhs, N)                         \
  do								\
    {								\
      if (N)							\
	{							\
	  (Current).beg = YYRHSLOC(Rhs, 1).beg;			\
	  (Current).end = YYRHSLOC(Rhs, N).end;			\
	}							\
      else							\
	{							\
	  (Current).beg = YYRHSLOC(Rhs, 0).end;			\
	  (Current).end = (Current).beg;			\
	}							\
    } while (0)

#define LOCUS_EQ(a,b) \
  ((((a)->mu_file == (b)->mu_file)                                      \
          || ((a)->mu_file && (b)->mu_file				\
	      && strcmp((a)->mu_file, (b)->mu_file) == 0))		\
   && (a)->mu_line == (b)->mu_line)
	 
#define YY_LOCATION_PRINT(File, Loc)				    \
  do								    \
    {								    \
      if (LOCUS_EQ(&(Loc).beg, &(Loc).end))			    \
	fprintf(File, "%s:%u.%u-%u.%u",				    \
		(Loc).beg.mu_file,				    \
		(Loc).beg.mu_line, (Loc).beg.mu_col,		    \
		(Loc).end.mu_line, (Loc).end.mu_col);		    \
      else							    \
	fprintf(File, "%s:%u.%u-%s:%u.%u",			    \
		(Loc).beg.mu_file,				    \
		(Loc).beg.mu_line, (Loc).beg.mu_col,		    \
		(Loc).end.mu_file,				    \
		(Loc).end.mu_line, (Loc).end.mu_col);		    \
    }								    \
  while (0)
%}

%error-verbose
%locations

%union {
  char *string;
  size_t number;
  mu_sieve_value_t *value;
  mu_list_t list;
  struct
  {
    char *ident;
    mu_list_t args;
  } command;
  struct node_list
  {
    struct mu_sieve_node *head, *tail;
  } node_list;
  struct mu_sieve_node *node;
}

%token <string> IDENT TAG
%token <number> NUMBER
%token <string> STRING MULTILINE
%token REQUIRE IF ELSIF ELSE ANYOF ALLOF NOT FALSE TRUE

%type <value> arg
%type <list> slist stringlist stringorlist arglist maybe_arglist
%type <command> command
%type <node> action test statement block cond
%type <node> else_part
%type <node_list> list testlist elsif_branch maybe_elsif 

%%

input        : /* empty */
               {
		 sieve_tree = NULL;
	       }
             | list
               {
		 sieve_tree = $1.head;
	       }
             ;

list         : statement
               {
		 $$.head = $$.tail = $1;
	       }
             | list statement
               {
		 if ($2)
		   {
		     $2->prev = $1.tail;
		     if ($1.tail)
		       $1.tail->next = $2;
		     else
		       $1.head = $2;
		     $1.tail = $2;
		   }
		 $$ = $1;
	       }
             ;

statement    : REQUIRE stringorlist ';'
               {
		 mu_sieve_require (mu_sieve_machine, $2);
		 /*  All the items in $2 are registered in memory_pool,
		     so we don't free them */
		 mu_list_destroy (&$2);
		 $$ = NULL;
	       }
             | action ';'
	     /* 1  2     3       4    */ 
             | IF cond block else_part
               {
		 $$ = node_alloc (mu_sieve_node_cond, &@1);
		 $$->v.cond.expr = $2;
		 $$->v.cond.iftrue = $3;
		 $$->v.cond.iffalse = $4;
	       }		 
             ;

else_part    : maybe_elsif
               {
		 cond_join ($1.head);
		 $$ = $1.head;
	       }
             | maybe_elsif ELSE block
               {
		 $3->prev = $1.tail;
		 if ($1.head)
		   {
		     $1.tail->next = $3;
		     $1.tail = $3;
		     cond_join ($1.head);
		     $$ = $1.head;
		   }
		 else
		   $$ = $3;
	       }
             ;

maybe_elsif  : /* empty */
               {
		 $$.head = $$.tail = NULL;
	       }
             | elsif_branch
             ;

elsif_branch : ELSIF cond block
               {
		 struct mu_sieve_node *node =
		   node_alloc (mu_sieve_node_cond, &@1);
		 node->v.cond.expr = $2;
		 node->v.cond.iftrue = $3;
		 node->v.cond.iffalse = NULL;
		 $$.head = $$.tail = node;
	       }
             | elsif_branch ELSIF cond block
               {
		 struct mu_sieve_node *node =
		   node_alloc (mu_sieve_node_cond, &@2);
		 node->v.cond.expr = $3;
		 node->v.cond.iftrue = $4;
		 node->v.cond.iffalse = NULL;

		 node->prev = $1.tail;
		 $1.tail->next = node;
		 $1.tail = node;
		 $$ = $1;
	       }
             ;

block        : '{' list '}'
               {
		 $$ = $2.head;
	       }
             ;

testlist     : cond
               {
		 $$.head = $$.tail = $1;
	       }
             | testlist ',' cond
               {
		 $3->prev = $1.tail;
		 $1.tail->next = $3;
		 $1.tail = $3;
	       }
             ;

cond         : test
             | ANYOF '(' testlist ')'
               {
		 $$ = node_alloc (mu_sieve_node_anyof, &@1);
		 $$->v.node = $3.head;
	       }
             | ALLOF '(' testlist ')'
               {
		 $$ = node_alloc (mu_sieve_node_allof, &@1);
		 $$->v.node = $3.head;
	       }
             | NOT cond
               {
		 $$ = node_alloc (mu_sieve_node_not, &@1);
		 $$->v.node = $2;
	       }
             ;

test         : command
               {
		 mu_sieve_register_t *reg;

		 mu_sieve_machine->locus = @1.beg;
		 reg = mu_sieve_test_lookup (mu_sieve_machine, $1.ident);
		 if (!reg)
		   {
		     mu_diag_at_locus (MU_LOG_ERROR, &@1.beg,
				       _("unknown test: %s"),
				       $1.ident);
		     mu_i_sv_error (mu_sieve_machine);
		   }
		 else if (!reg->required)
		   {
		     mu_diag_at_locus (MU_LOG_ERROR, &@1.beg,
				       _("test `%s' has not been required"),
				       $1.ident);
		     mu_i_sv_error (mu_sieve_machine);
		   }
		 
		 $$ = node_alloc (mu_sieve_node_test, &@1);
		 $$->v.command.reg = reg;
		 $$->v.command.arg = $1.args;
	       }
             | TRUE
	       {
		 $$ = node_alloc (mu_sieve_node_true, &@1);
	       }
             | FALSE
	       {
		 $$ = node_alloc (mu_sieve_node_false, &@1);
	       }
             ;

command      : IDENT maybe_arglist
               {
		 $$.ident = $1;
		 $$.args = $2;
	       }
             ;

action       : command
               {
		 mu_sieve_register_t *reg;

		 mu_sieve_machine->locus = @1.beg;
		 reg = mu_sieve_action_lookup (mu_sieve_machine, $1.ident);
		 
		 if (!reg)
		   {
		     mu_diag_at_locus (MU_LOG_ERROR, &@1.beg,
				       _("unknown action: %s"),
				       $1.ident);
		     mu_i_sv_error (mu_sieve_machine);
		   }
		 else if (!reg->required)
		   {
		     mu_diag_at_locus (MU_LOG_ERROR, &@1.beg,
				       _("action `%s' has not been required"),
				       $1.ident);
		     mu_i_sv_error (mu_sieve_machine);
		   }
		 
		 $$ = node_alloc(mu_sieve_node_action, &@1);
		 $$->v.command.reg = reg;
		 $$->v.command.arg = $1.args;
	       }
             ;

maybe_arglist: /* empty */
               {
		 $$ = NULL;
	       }
             | arglist
	     ;

arglist      : arg
               {
		 mu_list_create (&$$);
		 mu_list_append ($$, $1);
	       }		 
             | arglist arg
               {
		 mu_list_append ($1, $2);
		 $$ = $1;
	       }
             ;

arg          : stringlist
               {		 
		 $$ = mu_sieve_value_create (SVT_STRING_LIST, $1);
	       }
             | STRING
               {
		 $$ = mu_sieve_value_create (SVT_STRING, $1);
               } 
             | MULTILINE
               {
		 $$ = mu_sieve_value_create (SVT_STRING, $1);
	       }
             | NUMBER
               {
		 $$ = mu_sieve_value_create (SVT_NUMBER, &$1);
	       }
             | TAG
               {
		 $$ = mu_sieve_value_create (SVT_TAG, $1);
	       }
             ;

stringorlist : STRING
               {
		 mu_list_create (&$$);
		 mu_list_append ($$, $1);
	       }
             | stringlist
             ;

stringlist   : '[' slist ']'
               {
		 $$ = $2;
	       }
             ;

slist        : STRING
               {
		 mu_list_create (&$$);
		 mu_list_append ($$, $1);
	       }
             | slist ',' STRING
               {
		 mu_list_append ($1, $3);
		 $$ = $1;
	       }
             ;

%%

int
yyerror (const char *s)
{
  extern struct mu_locus mu_sieve_locus;

  mu_sieve_machine->locus = mu_sieve_locus;
  mu_diag_at_locus (MU_LOG_ERROR, &mu_sieve_locus, "%s", s);
  mu_i_sv_error (mu_sieve_machine);
  return 0;
}

static void
cond_join (struct mu_sieve_node *node)
{
  while (node)
    {
      struct mu_sieve_node *next = node->next;
      node->prev = node->next = NULL;
      node->v.cond.iffalse = next;
      node = next;
    }
}

static struct mu_sieve_node *
node_alloc (enum mu_sieve_node_type type, struct mu_locus_range *lr)
{
  struct mu_sieve_node *node = malloc (sizeof (*node));
  if (node)
    {
      node->prev = node->next = NULL;
      node->type = type;
      node->locus = *lr;
    }
  return node;
}

static void node_optimize (struct mu_sieve_node *node);
static void node_free (struct mu_sieve_node *node);
static void node_replace (struct mu_sieve_node *node,
			  struct mu_sieve_node *repl);
static int node_code (struct mu_sieve_machine *mach,
		      struct mu_sieve_node *node);
static void node_dump (mu_stream_t str, struct mu_sieve_node *node,
		       unsigned level);

static void tree_free (struct mu_sieve_node **tree);
static void tree_optimize (struct mu_sieve_node *tree);
static int tree_code (struct mu_sieve_machine *mach,
		      struct mu_sieve_node *tree);
static void tree_dump (mu_stream_t str, struct mu_sieve_node *tree, unsigned level);

static void
indent (mu_stream_t str, unsigned level)
{
#define tab "  "
#define tablen (sizeof (tab) - 1)
  while (level--)
    mu_stream_write (str, tab, tablen, NULL);
}

/* mu_sieve_node_noop */
static void
dump_node_noop (mu_stream_t str, struct mu_sieve_node *node, unsigned level)
{
  indent (str, level);
  mu_stream_printf (str, "NOOP\n");  
}

/* mu_sieve_node_false */
static void
dump_node_false (mu_stream_t str, struct mu_sieve_node *node, unsigned level)
{
  indent (str, level);
  mu_stream_printf (str, "FALSE\n");
}

/* mu_sieve_node_true */
static void
dump_node_true (mu_stream_t str, struct mu_sieve_node *node, unsigned level)
{
  indent (str, level);
  mu_stream_printf (str, "TRUE\n");
}

/* mu_sieve_node_test & mu_sieve_node_action */
static void
free_node_command (struct mu_sieve_node *node)
{
  mu_list_destroy (&node->v.command.arg);
}

static int
code_node_test (struct mu_sieve_machine *mach, struct mu_sieve_node *node)
{
  return mu_i_sv_code_test (mach, node->v.command.reg, node->v.command.arg);
}

static int
code_node_action (struct mu_sieve_machine *mach, struct mu_sieve_node *node)
{
  return mu_i_sv_code_action (mach, node->v.command.reg, node->v.command.arg);
}

struct string_dumper_data
{
  int init;
  mu_stream_t stream;
};

static int
string_dumper (void *item, void *data)
{
  struct string_dumper_data *dp = data;
  if (dp->init == 0)
    dp->init = 1;
  else
    mu_stream_printf (dp->stream, ", ");
  mu_stream_printf (dp->stream, "\"%s\"", (char*)item);
  return 0;
}

void
mu_i_sv_valf (mu_stream_t str, mu_sieve_value_t *val)
{
  mu_stream_printf (str, " ");
  switch (val->type)
    {
    case SVT_VOID:
      mu_stream_printf (str, "(void)");
      break;
      
    case SVT_NUMBER:
      mu_stream_printf (str, "%zu", val->v.number);
      break;
      
    case SVT_STRING:
      mu_stream_printf (str, "\"%s\"", val->v.string);
      break;
      
    case SVT_STRING_LIST:
      {
	struct string_dumper_data d;
	d.init = 0;
	d.stream = str;
	mu_stream_printf (str, "[");
	mu_list_foreach (val->v.list, string_dumper, &d);
	mu_stream_printf (str, "]");
      }
      break;
      
    case SVT_TAG:
      mu_stream_printf (str, ":%s", val->v.string);
      break;
      
    case SVT_IDENT:
      mu_stream_printf (str, "%s", val->v.string);
      break;
	
    case SVT_POINTER:
      mu_stream_printf (str, "%p", val->v.ptr);
      break;

    default:
      abort ();
    }
}
  
static int
dump_val (void *item, void *data)
{
  mu_sieve_value_t *val = item;
  mu_stream_t str = data;
  mu_i_sv_valf (str, val);
  return 0;
}

void
mu_i_sv_argf (mu_stream_t str, mu_list_t list)
{
  mu_list_foreach (list, dump_val, str);
}

static void
dump_node_command (mu_stream_t str, struct mu_sieve_node *node, unsigned level)
{
  indent (str, level);
  mu_stream_printf (str, "COMMAND %s", node->v.command.reg->name);
  mu_list_foreach (node->v.command.arg, dump_val, str);
  mu_stream_printf (str, "\n");
}

/* mu_sieve_node_cond */
static void
free_node_cond (struct mu_sieve_node *node)
{
  tree_free (&node->v.cond.expr);
  tree_free (&node->v.cond.iftrue);
  tree_free (&node->v.cond.iffalse);	     
}

static void
optimize_node_cond (struct mu_sieve_node *node)
{
  tree_optimize (node->v.cond.expr);
  switch (node->v.cond.expr->type)
    {
    case mu_sieve_node_true:
      tree_optimize (node->v.cond.iftrue);
      node_replace (node, node->v.cond.iftrue);
      break;

    case mu_sieve_node_false:
      tree_optimize (node->v.cond.iffalse);
      node_replace (node, node->v.cond.iffalse);
      break;

    default:
      tree_optimize (node->v.cond.iftrue);
      tree_optimize (node->v.cond.iffalse);
    }
}

static int
code_node_cond (struct mu_sieve_machine *mach, struct mu_sieve_node *node)
{
  size_t br1;
  
  tree_code (mach, node->v.cond.expr);
  mu_i_sv_code (mach, (sieve_op_t) _mu_i_sv_instr_brz);
  br1 = mach->pc;
  mu_i_sv_code (mach, (sieve_op_t) 0);
  tree_code (mach, node->v.cond.iftrue);
  
  if (node->v.cond.iffalse)
    {
      size_t br2;

      mu_i_sv_code (mach, (sieve_op_t) _mu_i_sv_instr_branch);
      br2 = mach->pc;
      mu_i_sv_code (mach, (sieve_op_t) 0);
      
      mach->prog[br1].pc = mach->pc - br1 - 1;

      tree_code (mach, node->v.cond.iffalse);
      mach->prog[br2].pc = mach->pc - br2 - 1;
    }
  else
    mach->prog[br1].pc = mach->pc - br1 - 1;
  return 0;
}
  
static void
dump_node_cond (mu_stream_t str, struct mu_sieve_node *node, unsigned level)
{
  indent (str, level);
  mu_stream_printf (str, "COND\n");

  ++level;

  indent (str, level);
  mu_stream_printf (str, "EXPR:\n");
  tree_dump (str, node->v.cond.expr, level + 1);

  indent (str, level);
  mu_stream_printf (str, "IFTRUE:\n");
  tree_dump (str, node->v.cond.iftrue, level + 1);

  indent (str, level);
  mu_stream_printf (str, "IFFALSE:\n");
  tree_dump (str, node->v.cond.iffalse, level + 1);
}

/* mu_sieve_node_anyof & mu_sieve_node_allof */
static void
free_node_x_of (struct mu_sieve_node *node)
{
  tree_free (&node->v.node);
}

static void
optimize_x_of (struct mu_sieve_node *node, enum mu_sieve_node_type solve)
{
  struct mu_sieve_node *cur;
  tree_optimize (node->v.node);
  cur = node->v.node;
  while (cur)
    {
      struct mu_sieve_node *next = cur->next;
      switch (cur->type)
	{
	case mu_sieve_node_false:
	case mu_sieve_node_true:
	  if (cur->type == solve)
	    {
	      tree_free (&node->v.node);
	      node->type = solve;
	      return;
	    }
	  else
	    {
	      if (cur->prev)
		cur->prev->next = next;
	      else
		node->v.node = next;
	      if (next)
		next->prev = cur->prev;
	      node_free (cur);
	    }
	  break;

	default:
	  break;
	}
      
      cur = next;
    }
  
  if (!node->v.node)
    node->type = solve == mu_sieve_node_false ? mu_sieve_node_true : mu_sieve_node_false;
}

static int
code_node_x_of (struct mu_sieve_machine *mach, struct mu_sieve_node *node,
		sieve_op_t op)
{
  struct mu_sieve_node *cur = node->v.node;
  size_t pc = 0;
  size_t end;
  
  while (cur)
    {
      node_code (mach, cur);
      if (cur->next)
	{
	  mu_i_sv_code (mach, op);
	  mu_i_sv_code (mach, (sieve_op_t) pc);
	  pc = mach->pc - 1;
	}
      cur = cur->next;
    }

  /* Fix-up locations */
  end = mach->pc;
  while (pc != 0)
    {
      size_t prev = mach->prog[pc].pc;
      mach->prog[pc].pc = end - pc - 1;
      pc = prev;
    }

  return 0;
}

static void
dump_node_x_of (mu_stream_t str, struct mu_sieve_node *node, unsigned level)
{
  indent (str, level);
  mu_stream_printf (str, "%s:\n",
		    node->type == mu_sieve_node_allof ? "ALLOF" : "ANYOF");

  ++level;
  node = node->v.node;
  while (node)
    {
      node_dump (str, node, level + 1);
      node = node->next;
      if (node)
	{
	  indent (str, level);
	  mu_stream_printf (str, "%s:\n",
			    node->type == mu_sieve_node_allof ? "AND" : "OR");
	}
    }
}
  
/* mu_sieve_node_anyof */
static void
optimize_node_anyof (struct mu_sieve_node *node)
{
  optimize_x_of (node, mu_sieve_node_true);
}

static int
code_node_anyof (struct mu_sieve_machine *mach, struct mu_sieve_node *node)
{
  return code_node_x_of (mach, node, (sieve_op_t) _mu_i_sv_instr_brnz);
}

/* mu_sieve_node_allof */
static void
optimize_node_allof (struct mu_sieve_node *node)
{
  return optimize_x_of (node, mu_sieve_node_false);
}

static int
code_node_allof (struct mu_sieve_machine *mach, struct mu_sieve_node *node)
{
  return code_node_x_of (mach, node, (sieve_op_t) _mu_i_sv_instr_brz);
}

/* mu_sieve_node_not */
static void
free_node_not (struct mu_sieve_node *node)
{
  tree_free (&node->v.node);
}

static void
optimize_node_not (struct mu_sieve_node *node)
{
  tree_optimize (node->v.node);
  switch (node->v.node->type)
    {
    case mu_sieve_node_false:
      tree_free (&node->v.node);
      node->type = mu_sieve_node_true;
      break;
      
    case mu_sieve_node_true:
      tree_free (&node->v.node);
      node->type = mu_sieve_node_false;
      break;

    default:
      break;
    }
}

static int
code_node_not (struct mu_sieve_machine *mach, struct mu_sieve_node *node)
{
  node_code (mach, node->v.node);
  return mu_i_sv_code (mach, (sieve_op_t) _mu_i_sv_instr_not);
}

static void
dump_node_not (mu_stream_t str, struct mu_sieve_node *node, unsigned level)
{
  indent (str, level);
  mu_stream_printf (str, "NOT\n");
  node_dump (str, node->v.node, level + 1);
}

struct node_descr
{
  int (*code_fn) (struct mu_sieve_machine *mach, struct mu_sieve_node *node);
  void (*optimize_fn) (struct mu_sieve_node *node);
  void (*free_fn) (struct mu_sieve_node *node);
  void (*dump_fn) (mu_stream_t str, struct mu_sieve_node *node, unsigned level);

};

static struct node_descr node_descr[] = {
  [mu_sieve_node_noop]  = { NULL, NULL, NULL, dump_node_noop },
  [mu_sieve_node_false]  = { NULL, NULL, NULL, dump_node_false },
  [mu_sieve_node_true]   = { NULL, NULL, NULL, dump_node_true },
  [mu_sieve_node_test]   = { code_node_test, NULL,
			     free_node_command, dump_node_command },
  [mu_sieve_node_action] = { code_node_action, NULL,
			     free_node_command, dump_node_command },
  [mu_sieve_node_cond]   = { code_node_cond, optimize_node_cond,
			     free_node_cond, dump_node_cond },
  [mu_sieve_node_anyof]  = { code_node_anyof, optimize_node_anyof,
			     free_node_x_of, dump_node_x_of },
  [mu_sieve_node_allof]  = { code_node_allof, optimize_node_allof,
			     free_node_x_of, dump_node_x_of },
  [mu_sieve_node_not]    = { code_node_not, optimize_node_not,
			     free_node_not, dump_node_not },
};

static void
node_optimize (struct mu_sieve_node *node)
{
  if ((int)node->type >= MU_ARRAY_SIZE (node_descr))
    abort ();
  if (node_descr[node->type].optimize_fn)
    node_descr[node->type].optimize_fn (node);
}

static void
node_free (struct mu_sieve_node *node)
{
  if ((int)node->type >= MU_ARRAY_SIZE (node_descr))
    abort ();
  if (node_descr[node->type].free_fn)
    node_descr[node->type].free_fn (node);
  free (node);
}

static void
node_replace (struct mu_sieve_node *node, struct mu_sieve_node *repl)
{
  struct mu_sieve_node copy;

  if ((int)node->type >= MU_ARRAY_SIZE (node_descr))
    abort ();
  
  copy = *node;
  if (repl)
    {
      node->type = repl->type;
      node->v = repl->v;

      switch (copy.type)
	{
	case mu_sieve_node_cond:
	  if (repl == copy.v.cond.expr)
	    copy.v.cond.expr = NULL;
	  else if (repl == copy.v.cond.iftrue)
	    copy.v.cond.iftrue = NULL;
	  else if (repl == copy.v.cond.iffalse)
	    copy.v.cond.iffalse = NULL;
	  break;
	  
	case mu_sieve_node_not:
	  if (repl == copy.v.node)
	    copy.v.node = NULL;
	  break;

	default:
	  break;
	}
    }
  else
    node->type = mu_sieve_node_noop;

  if (node_descr[node->type].free_fn)
    node_descr[node->type].free_fn (&copy);
}

static int
node_code (struct mu_sieve_machine *mach, struct mu_sieve_node *node)
{
  if ((int)node->type >= MU_ARRAY_SIZE (node_descr))
    abort ();

  if (!node_descr[node->type].code_fn)
    return 0;

  if (mu_i_sv_locus (mach, &node->locus))
    return 1;

  return node_descr[node->type].code_fn (mach, node);
}

static void
node_dump (mu_stream_t str, struct mu_sieve_node *node, unsigned level)
{
  if ((int)node->type >= MU_ARRAY_SIZE (node_descr)
      || !node_descr[node->type].dump_fn) 
    abort ();
  node_descr[node->type].dump_fn (str, node, level);
}


static void
tree_free (struct mu_sieve_node **tree)
{
  struct mu_sieve_node *cur = *tree;
  while (cur)
    {
      struct mu_sieve_node *next = cur->next;
      node_free (cur);
      cur = next;
    }
}

static void
tree_optimize (struct mu_sieve_node *tree)
{
  while (tree)
    {
      node_optimize (tree);
      tree = tree->next;
    }
}

static int
tree_code (struct mu_sieve_machine *mach, struct mu_sieve_node *tree)
{
  while (tree)
    {
      if (node_code (mach, tree))
	return 1;
      tree = tree->next;
    }
  return 0;
}

static void
tree_dump (mu_stream_t str, struct mu_sieve_node *tree, unsigned level)
{
  while (tree)
    {
      node_dump (str, tree, level);
      tree = tree->next;
    }
}  

void
mu_i_sv_error (mu_sieve_machine_t mach)
{
  mach->state = mu_sieve_state_error;
}

int
mu_sieve_machine_init (mu_sieve_machine_t *pmach)
{
  int rc;
  mu_sieve_machine_t mach;

  mu_sieve_debug_init ();
  mach = malloc (sizeof (*mach));
  if (!mach)
    return ENOMEM;
  memset (mach, 0, sizeof (*mach));
  rc = mu_list_create (&mach->memory_pool);
  if (rc)
    {
      free (mach);
      return rc;
    }

  rc = mu_opool_create (&mach->string_pool, MU_OPOOL_DEFAULT);
  if (rc)
    {
      mu_list_destroy (&mach->memory_pool);
      free (mach);
      return rc;
    }
  
  mach->source_list = NULL;

  mach->data = NULL;

  mu_sieve_set_diag_stream (mach, mu_strerr);
  mu_sieve_set_dbg_stream (mach, mu_strerr);
  
  *pmach = mach;
  return 0;
}

int
mu_sieve_machine_inherit (mu_sieve_machine_t const parent,
			  mu_sieve_machine_t *pmach)
{
  mu_sieve_machine_t child;
  int rc;
  
  if (!parent || parent->state == mu_sieve_state_error)
    return EINVAL;
  
  rc = mu_sieve_machine_init (&child);
  if (rc)
    return rc;

  child->dry_run     = parent->dry_run;

  child->state_flags = parent->state_flags;
  child->err_mode    = parent->err_mode;
  child->err_locus   = parent->err_locus;
  child->dbg_mode    = parent->dbg_mode;
  child->dbg_locus   = parent->dbg_locus;  

  child->errstream = parent->errstream;
  mu_stream_ref (child->errstream);
  child->dbgstream = parent->dbgstream;
  mu_stream_ref (child->dbgstream);
  
  child->data = parent->data;
  child->logger = parent->logger;
  child->daemon_email = parent->daemon_email;

  *pmach = child;
  return 0;
}

int
mu_sieve_machine_dup (mu_sieve_machine_t const in, mu_sieve_machine_t *out)
{
  int rc;
  mu_sieve_machine_t mach;

  if (!in || in->state == mu_sieve_state_error)
    return EINVAL; 
  mach = malloc (sizeof (*mach));
  if (!mach)
    return ENOMEM;
  memset (mach, 0, sizeof (*mach));
  rc = mu_list_create (&mach->memory_pool);
  if (rc)
    {
      free (mach);
      return rc;
    }
  mach->destr_list = NULL;
  mach->test_list = NULL;
  mach->action_list = NULL;
  mach->comp_list = NULL;

  mach->progsize = in->progsize;
  mach->prog = in->prog;

  switch (in->state)
    {
    case mu_sieve_state_running:
    case mu_sieve_state_disass:
      mach->state = mu_sieve_state_compiled;
      break;

    default:
      mach->state = in->state;
    }
  
  mach->pc = 0;
  mach->reg = 0;
  mach->stack = NULL;

  mach->dry_run = in->dry_run;

  mach->state_flags = in->state_flags;
  mach->err_mode    = in->err_mode;
  mach->err_locus   = in->err_locus;
  mach->dbg_mode    = in->dbg_mode;
  mach->dbg_locus   = in->dbg_locus;  

  mach->errstream = in->errstream;
  mu_stream_ref (mach->errstream);
  mach->dbgstream = in->dbgstream;
  mu_stream_ref (mach->dbgstream);
  
  mach->data = in->data;
  mach->logger = in->logger;
  mach->daemon_email = in->daemon_email;

  *out = mach;
  return 0;
}

void
mu_sieve_get_diag_stream (mu_sieve_machine_t mach, mu_stream_t *pstr)
{
  *pstr = mach->errstream;
  mu_stream_ref (*pstr);
}

void
mu_sieve_set_diag_stream (mu_sieve_machine_t mach, mu_stream_t str)
{
  mu_stream_unref (mach->errstream);
  mach->errstream = str;
  mu_stream_ref (mach->errstream);
}

void
mu_sieve_set_dbg_stream (mu_sieve_machine_t mach, mu_stream_t str)
{
  mu_stream_unref (mach->dbgstream);
  mach->dbgstream = str;
  mu_stream_ref (mach->dbgstream);
}

void
mu_sieve_get_dbg_stream (mu_sieve_machine_t mach, mu_stream_t *pstr)
{
  *pstr = mach->dbgstream;
  mu_stream_ref (*pstr);
}

void
mu_sieve_set_logger (mu_sieve_machine_t mach, mu_sieve_action_log_t logger)
{
  mach->logger = logger;
}

mu_mailer_t
mu_sieve_get_mailer (mu_sieve_machine_t mach)
{
  if (!mach->mailer)
    {
      int rc;

      rc = mu_mailer_create (&mach->mailer, NULL);
      if (rc)
	{
	  mu_sieve_error (mach,
			  _("%lu: cannot create mailer: %s"),
			  (unsigned long) mu_sieve_get_message_num (mach),
			  mu_strerror (rc));
	  return NULL;
	}
      rc = mu_mailer_open (mach->mailer, 0);
      if (rc)
	{
	  mu_url_t url = NULL;
	  mu_mailer_get_url (mach->mailer, &url);
	  mu_sieve_error (mach,
			  _("%lu: cannot open mailer %s: %s"),
			  (unsigned long) mu_sieve_get_message_num (mach),
			  mu_url_to_string (url), mu_strerror (rc));
	  mu_mailer_destroy (&mach->mailer);
	  return NULL;
	}
    }
  return mach->mailer;
}

void
mu_sieve_set_mailer (mu_sieve_machine_t mach, mu_mailer_t mailer)
{
  mu_mailer_destroy (&mach->mailer);
  mach->mailer = mailer;
}

#define MAILER_DAEMON_PFX "MAILER-DAEMON@"

char *
mu_sieve_get_daemon_email (mu_sieve_machine_t mach)
{
  if (!mach->daemon_email)
    {
      const char *domain = NULL;
      
      mu_get_user_email_domain (&domain);
      mach->daemon_email = mu_sieve_malloc (mach,
					    sizeof(MAILER_DAEMON_PFX) +
					    strlen (domain));
      sprintf (mach->daemon_email, "%s%s", MAILER_DAEMON_PFX, domain);
    }
  return mach->daemon_email;
}

void
mu_sieve_set_daemon_email (mu_sieve_machine_t mach, const char *email)
{
  mu_sieve_mfree (mach, (void *)mach->daemon_email);
  mach->daemon_email = mu_sieve_mstrdup (mach, email);
}

struct sieve_destr_record
{
  mu_sieve_destructor_t destr;
  void *ptr;
};

int
mu_sieve_machine_add_destructor (mu_sieve_machine_t mach,
				 mu_sieve_destructor_t destr,
				 void *ptr)
{
  struct sieve_destr_record *p;

  if (!mach->destr_list && mu_list_create (&mach->destr_list))
    return 1;
  p = mu_sieve_malloc (mach, sizeof (*p));
  if (!p)
    return 1;
  p->destr = destr;
  p->ptr = ptr;
  return mu_list_prepend (mach->destr_list, p);
}

static int
_run_destructor (void *data, void *unused)
{
  struct sieve_destr_record *p = data;
  p->destr (p->ptr);
  return 0;
}

void
mu_sieve_machine_destroy (mu_sieve_machine_t *pmach)
{
  mu_sieve_machine_t mach = *pmach;

  mu_stream_destroy (&mach->errstream);
  mu_stream_destroy (&mach->dbgstream);
  mu_mailer_destroy (&mach->mailer);
  mu_list_foreach (mach->destr_list, _run_destructor, NULL);
  mu_list_destroy (&mach->destr_list);
  mu_list_destroy (&mach->action_list);
  mu_list_destroy (&mach->test_list);
  mu_list_destroy (&mach->comp_list);
  mu_list_destroy (&mach->source_list);
  mu_opool_destroy (&mach->string_pool);
  mu_sieve_slist_destroy (&mach->memory_pool);
  free (mach);
  *pmach = NULL;
}

int
with_machine (mu_sieve_machine_t mach, char const *name,
	      int (*thunk) (void *), void *data)
{
  int rc = 0;
  mu_stream_t save_errstr;

  save_errstr = mu_strerr;  
  mu_stream_ref (save_errstr);
  mu_strerr = mach->errstream;
  mu_stream_ref (mu_strerr);

  mu_i_sv_register_standard_actions (mach);
  mu_i_sv_register_standard_tests (mach);
  mu_i_sv_register_standard_comparators (mach);
  mu_sieve_machine = mach;

  mu_sieve_stream_save (mach);
  rc = thunk (data);
  mu_sieve_stream_restore (mach);

  mu_stream_unref (save_errstr);
  mu_strerr = save_errstr;
  mu_stream_unref (mu_strerr);
  
  return rc;
}

static int
sieve_parse (void)
{
  int rc;

  sieve_tree = NULL;
  yydebug = mu_debug_level_p (mu_sieve_debug_handle, MU_DEBUG_TRACE3);

  rc = yyparse ();
  mu_i_sv_lex_finish (mu_sieve_machine);
  if (rc)
    mu_i_sv_error (mu_sieve_machine);
  if (mu_sieve_machine->state == mu_sieve_state_init)
    {
      if (mu_debug_level_p (mu_sieve_debug_handle, MU_DEBUG_TRACE1))
	{
	  mu_error (_("Unoptimized parse tree"));
	  tree_dump (mu_strerr, sieve_tree, 0);
	}
      tree_optimize (sieve_tree);
      if (mu_debug_level_p (mu_sieve_debug_handle, MU_DEBUG_TRACE2))
	{
	  mu_error (_("Optimized parse tree"));
	  tree_dump (mu_strerr, sieve_tree, 0);
	}
      mu_i_sv_code (mu_sieve_machine, (sieve_op_t) 0);

      /* Clear location, so that mu_i_sv_locus will do its job. */
      mu_sieve_machine->locus.mu_file = NULL;
      mu_sieve_machine->locus.mu_line = 0;
      mu_sieve_machine->locus.mu_col = 0;
      
      rc = tree_code (mu_sieve_machine, sieve_tree);
      if (rc)
	mu_i_sv_error (mu_sieve_machine);
      mu_i_sv_code (mu_sieve_machine, (sieve_op_t) 0);
    }
  
  if (rc == 0)
    {
      if (mu_sieve_machine->state == mu_sieve_state_error)
	rc = MU_ERR_PARSE;
      else
	mu_sieve_machine->state = mu_sieve_state_compiled;
    }

  tree_free (&sieve_tree);
  return rc;
}

static int
sieve_compile_file (void *name)
{
  if (mu_i_sv_lex_begin (name) == 0)
    return sieve_parse ();
  return MU_ERR_FAILURE;
}

int
mu_sieve_compile (mu_sieve_machine_t mach, const char *name)
{
  return with_machine (mach, name, sieve_compile_file, (void *) name);
}

struct strbuf
{
  const char *ptr;
  size_t size;
  const char *file;
  int line;
};

static int
sieve_compile_strbuf (void *name)
{
  struct strbuf *buf = name;
  if (mu_i_sv_lex_begin_string (buf->ptr, buf->size, buf->file, buf->line) == 0)
    return sieve_parse ();
  return MU_ERR_FAILURE;
} 

//FIXME: The API is clumsy
int
mu_sieve_compile_buffer (mu_sieve_machine_t mach,
			 const char *str, int strsize,
			 const char *fname, int line)
{
  struct strbuf buf;
  buf.ptr = str;
  buf.size = strsize;
  buf.file = fname;
  buf.line = line;
  return with_machine (mach, fname, sieve_compile_strbuf, &buf);
}





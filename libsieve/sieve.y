%{
/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sieve.h>

sieve_machine_t sieve_machine;
int sieve_error_count;

static void branch_fixup __P((size_t start, size_t end));
 
%}

%union {
  char *string;
  long number;
  sieve_instr_t instr;
  sieve_value_t *value;
  list_t list;
  size_t pc;
  struct {
    char *ident;
    list_t args;
  } command;
  struct {
    size_t begin;
    size_t cond;
    size_t branch;
  } branch;
}

%token <string> IDENT TAG
%token <number> NUMBER
%token <string> STRING MULTILINE
%token REQUIRE IF ELSIF ELSE ANYOF ALLOF NOT

%type <value> arg
%type <list> slist stringlist stringorlist arglist maybe_arglist
%type <command> command
%type <number> testlist
%type <pc> action test statement list elsif else cond begin if block
%type <branch> elsif_branch maybe_elsif else_part

%%

input        : /* empty */
             | list
               { /* to placate bison */ }
             ;

list         : statement
             | list statement
             ;

statement    : REQUIRE stringorlist ';'
               {
		 sieve_require ($2);
		 sieve_slist_destroy (&$2);
		 $$ = sieve_machine->pc;
	       }
             | action ';'
	     /* 1  2     3       4    */ 
             | if cond block else_part
               {
		 sieve_machine->prog[$2].pc = $4.begin - $2 - 1;
		 if ($4.branch)
		   branch_fixup ($4.branch, sieve_machine->pc);
	       }		 
             ;

if           : IF
               {
		 $$ = sieve_machine->pc;
	       }
             ;

else_part    : maybe_elsif
               {
		 if ($1.begin)
		   sieve_machine->prog[$1.cond].pc =
		                  sieve_machine->pc - $1.cond - 1;
		 else
		   {
		     $$.begin = sieve_machine->pc;
		     $$.branch = 0;
		   }
	       }
             | maybe_elsif else block
               {
		 if ($1.begin)
		   {
		     sieve_machine->prog[$1.cond].pc = $3 - $1.cond - 1;
		     sieve_machine->prog[$2].pc = $1.branch;
		     $$.begin = $1.begin;
		     $$.branch = $2;
		   }
		 else
		   {
		     $$.begin = $3;
		     $$.branch = $2;
		   }
	       }
             ;

maybe_elsif  : /* empty */
               {
		 $$.begin = 0;
	       }
             | elsif_branch
             ;

elsif_branch : elsif begin cond block
               {
		 $$.begin = $2; 
		 $$.branch = $1;
		 $$.cond = $3;
	       }
             | elsif_branch elsif begin cond block
               {
		 sieve_machine->prog[$1.cond].pc = $3 - $1.cond - 1;
		 sieve_machine->prog[$2].pc = $1.branch;
		 $$.begin = $1.begin;
		 $$.branch = $2;
		 $$.cond = $4;
	       }
             ;

elsif        : ELSIF
               {
		 sieve_code_instr (instr_branch);
		 $$ = sieve_machine->pc;
		 sieve_code_number (0);
	       }
             ;

else         : ELSE
               {
		 sieve_code_instr (instr_branch);
		 $$ = sieve_machine->pc;
		 sieve_code_number (0);
	       }
             ;

block        : '{' list '}'
               {
		 $$ = $2;
	       }
             ;

testlist     : cond_expr
               {
		 if (sieve_code_instr (instr_push))
		   YYERROR;
		 $$ = 1;
	       }
             | testlist ',' cond_expr
               {
		 if (sieve_code_instr (instr_push))
		   YYERROR;
		 $$ = $1 + 1;
	       }
             ;

cond         : cond_expr
               {
		 sieve_code_instr (instr_brz);
		 $$ = sieve_machine->pc;
		 sieve_code_number (0);
	       }
             ;

cond_expr    : test
               { /* to placate bison */ }
             | ANYOF '(' testlist ')'
               {
		 if (sieve_code_instr (instr_anyof)
		     || sieve_code_number ($3))
		   YYERROR;
	       }
             | ALLOF '(' testlist ')'
               {
		 if (sieve_code_instr (instr_allof)
		     || sieve_code_number ($3))
		   YYERROR;
	       }
             | NOT cond_expr
               {
		 if (sieve_code_instr (instr_not))
		   YYERROR;
	       }
             ;

begin        : /* empty */
               {
		 $$ = sieve_machine->pc;
	       }
             ; 

test         : command
               {
		 sieve_register_t *reg = sieve_test_lookup ($1.ident);
		 $$ = sieve_machine->pc;

		 if (!reg)
		   sieve_compile_error (sieve_filename, sieve_line_num,
                                "unknown test: %s",
				$1.ident);
		 else if (!reg->required)
		   sieve_compile_error (sieve_filename, sieve_line_num,
                                "test `%s' has not been required",
				$1.ident);
		 else if (sieve_code_test (reg, $1.args))
		   YYERROR;
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
		 sieve_register_t *reg = sieve_action_lookup ($1.ident);
		 
		 $$ = sieve_machine->pc;
		 if (!reg)
		   sieve_compile_error (sieve_filename, sieve_line_num,
                                "unknown action: %s",
				$1.ident);
		 else if (!reg->required)
		   sieve_compile_error (sieve_filename, sieve_line_num,
                                "action `%s' has not been required",
				$1.ident);
		 else if (sieve_code_action (reg, $1.args))
		   YYERROR;
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
		 list_create (&$$);
		 list_append ($$, $1);
	       }		 
             | arglist arg
               {
		 list_append ($1, $2);
		 $$ = $1;
	       }
             ;

arg          : stringlist
               {
		 $$ = sieve_value_create (SVT_STRING_LIST, $1);
	       }
             | STRING
               {
		 $$ = sieve_value_create (SVT_STRING, $1);
               } 
             | MULTILINE
               {
		 $$ = sieve_value_create (SVT_STRING, $1);
	       }
             | NUMBER
               {
		 $$ = sieve_value_create (SVT_NUMBER, &$1);
	       }
             | TAG
               {
		 $$ = sieve_value_create (SVT_TAG, $1);
	       }
             ;

stringorlist : STRING
               {
		 list_create (&$$);
		 list_append ($$, $1);
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
		 list_create (&$$);
		 list_append ($$, $1);
	       }
             | slist ',' STRING
               {
		 list_append ($1, $3);
		 $$ = $1;
	       }
             ;

%%

int
yyerror (char *s)
{
  sieve_compile_error (sieve_filename, sieve_line_num, "%s", s);
  return 0;
}

int
sieve_machine_init (sieve_machine_t *pmach, void *data)
{
  int rc;
  sieve_machine_t mach;
  
  mach = malloc (sizeof (*mach));
  if (!mach)
    return ENOMEM;
  memset (mach, 0, sizeof (*mach));
  rc = list_create (&mach->memory_pool);
  if (rc)
    {
      free (mach);
      return 1;
    }
  
  mach->data = data;
  mach->error_printer = _sieve_default_error_printer;
  mach->parse_error_printer = _sieve_default_parse_error;
  *pmach = mach;
  return 0;
}

void
sieve_set_error (sieve_machine_t mach, sieve_printf_t error_printer)
{
  mach->error_printer = error_printer ?
                           error_printer : _sieve_default_error_printer;
}

void
sieve_set_parse_error (sieve_machine_t mach, sieve_parse_error_t p)
{
  mach->parse_error_printer = p ? p : _sieve_default_parse_error;
}

void
sieve_set_debug (sieve_machine_t mach, sieve_printf_t debug)
{
  mach->debug_printer = debug;
}

void
sieve_set_debug_level (sieve_machine_t mach, mu_debug_t dbg, int level)
{
  mach->mu_debug = dbg;
  mach->debug_level = level;
}

void
sieve_set_logger (sieve_machine_t mach, sieve_action_log_t logger)
{
  mach->logger = logger;
}

void
sieve_set_ticket (sieve_machine_t mach, ticket_t ticket)
{
  mach->ticket = ticket;
}

ticket_t
sieve_get_ticket (sieve_machine_t mach)
{
  return mach->ticket;
}

mailer_t
sieve_get_mailer (sieve_machine_t mach)
{
  if (!mach->mailer)
    mailer_create (&mach->mailer, NULL);
  return mach->mailer;
}

void
sieve_set_mailer (sieve_machine_t mach, mailer_t mailer)
{
  mailer_destroy (&mach->mailer);
  mach->mailer = mailer;
}

#define MAILER_DAEMON_PFX "MAILER-DAEMON@"

char *
sieve_get_daemon_email (sieve_machine_t mach)
{
  if (!mach->daemon_email)
    {
      const char *domain = NULL;
      
      mu_get_user_email_domain (&domain);
      mach->daemon_email = sieve_palloc (&mach->memory_pool,
					 sizeof(MAILER_DAEMON_PFX) +
					 strlen (domain));
      sprintf (mach->daemon_email, "%s%s", MAILER_DAEMON_PFX, domain);
    }
  return mach->daemon_email;
}

void
sieve_set_daemon_email (sieve_machine_t mach, const char *email)
{
  sieve_pfree (&mach->memory_pool, (void *)mach->daemon_email);
  mach->daemon_email = sieve_pstrdup (&mach->memory_pool, email);
}

struct sieve_destr_record
{
  sieve_destructor_t destr;
  void *ptr;
};

int
sieve_machine_add_destructor (sieve_machine_t mach, sieve_destructor_t destr,
			      void *ptr)
{
  struct sieve_destr_record *p;
  
  if (!mach->destr_list && list_create (&mach->destr_list))
    return 1;
  p = sieve_palloc (&mach->memory_pool, sizeof (*p));
  if (!p)
    return 1;
  p->destr = destr;
  p->ptr = ptr;
  return list_append (mach->memory_pool, p);
}

static int
_run_destructor (void *data, void *unused)
{
  struct sieve_destr_record *p = data;
  p->destr (p->ptr);
  return 0;
}

void
sieve_machine_destroy (sieve_machine_t *pmach)
{
  sieve_machine_t mach = *pmach;
  mailer_destroy (&mach->mailer);
  list_do (mach->destr_list, _run_destructor, NULL);
  list_destroy (&mach->destr_list);
  sieve_slist_destroy (&mach->memory_pool);
  free (mach);
  *pmach = NULL;
}

/* FIXME: When posix thread support is added, sieve_machine_begin() should
   acquire the global mutex, locking the current compilation session, and
   sieve_machine_finish() should release it */
void
sieve_machine_begin (sieve_machine_t mach)
{
  sieve_machine = mach;
  sieve_error_count = 0;
  sieve_code_instr (NULL);
}

void
sieve_machine_finish (sieve_machine_t mach)
{
  sieve_code_instr (NULL);
}

int
sieve_compile (sieve_machine_t mach, const char *name)
{
  int rc;
  
  sieve_machine_begin (mach);
  sieve_register_standard_actions ();
  sieve_register_standard_tests ();
  sieve_register_standard_comparators ();
  
  if (sieve_lex_begin (name) == 0)
    {
      sieve_machine->filename = sieve_pstrdup (&sieve_machine->memory_pool,
					       name);
  rc = yyparse ();
  sieve_lex_finish ();
    }
  else
    rc = 1;
  sieve_machine_finish (mach);
  if (sieve_error_count)
    rc = 1;
  return rc;
}

static void
_branch_fixup (size_t start, size_t end)
{
  size_t prev = sieve_machine->prog[start].pc;
  if (!prev)
    return;
  branch_fixup (prev, end);
  sieve_machine->prog[prev].pc = end - prev - 1;
}

void
branch_fixup (size_t start, size_t end)
{
  _branch_fixup (start, end);
  sieve_machine->prog[start].pc = end - start - 1;
}

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

sieve_machine_t *sieve_machine;
int sieve_error_count; 
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
}

%token <string> IDENT TAG
%token <number> NUMBER
%token <string> STRING MULTILINE
%token REQUIRE IF ELSIF ELSE ANYOF ALLOF NOT

%type <value> arg
%type <list> slist stringlist arglist maybe_arglist
%type <command> command
%type <number> testlist
%type <pc> action test statement list

%%

input        : /* empty */
             | list
               { /* to placate bison */ }
             ;

list         : statement
             | list statement
             ;

statement    : REQUIRE stringlist ';'
               {
		 sieve_require ($2);
		 sieve_slist_destroy (&$2);
		 $$ = sieve_machine->pc;
	       }
             | action ';'
             | IF cond block maybe_elsif maybe_else
               {
		 /* FIXME!! */
		 $$ = sieve_machine->pc;
	       }
             ;

maybe_elsif  : /* empty */
             | elsif
             ;

elsif        : ELSIF cond block
             | elsif ELSIF cond block
             ;

maybe_else   : /* empty */
             | ELSE block
             ;

block        : '{' list '}'
             ;


testlist     : cond
               {
		 if (sieve_code_instr (instr_push))
		   YYERROR;
		 $$ = 1;
	       }
             | testlist ',' cond
               {
		 if (sieve_code_instr (instr_push))
		   YYERROR;
		 $$ = $1 + 1;
	       }
             ;

cond         : test
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
             | NOT cond
               {
		 if (sieve_code_instr (instr_not))
		   YYERROR;
	       }
             ;

test         : command
               {
		 sieve_register_t *reg = sieve_test_lookup ($1.ident);
		 $$ = sieve_machine->pc;

		 if (!reg)
		   sieve_error ("%s:%d: unknown test: %s",
				sieve_filename, sieve_line_num,
				$1.ident);
		 else if (!reg->required)
		   sieve_error ("%s:%d: test `%s' has not been required",
				sieve_filename, sieve_line_num,
				$1.ident);
		 if (sieve_code_test (reg, $1.args))
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
		   sieve_error ("%s:%d: unknown action: %s",
				sieve_filename, sieve_line_num,
				$1.ident);
		 else if (!reg->required)
		   sieve_error ("%s:%d: action `%s' has not been required",
				sieve_filename, sieve_line_num,
				$1.ident);
		 if (sieve_code_action (reg, $1.args))
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
 
stringlist   : STRING
               {
		 list_create (&$$);
		 list_append ($$, $1);
	       }
             | '[' slist ']'
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
  sieve_error ("%s:%d: %s", sieve_filename, sieve_line_num, s);
  return 0;
}

/* FIXME: When posix thread support is added, sieve_machine_begin() should
   aquire the global mutex, locking the current compilation session, and
   sieve_machine_finish() should release it */
void
sieve_machine_begin (sieve_machine_t *mach, sieve_printf_t err, void *data)
{
  memset (mach, 0, sizeof (*mach));

  mach->error_printer = err ? err : _sieve_default_error_printer;
  mach->data = data;
  sieve_machine = mach;
  sieve_error_count = 0;
  sieve_code_instr (NULL);
}

void
sieve_machine_finish (sieve_machine_t *mach)
{
  sieve_code_instr (NULL);
}

int
sieve_compile (sieve_machine_t *mach, const char *name,
	       void *extra_data, sieve_printf_t err)
{
  int rc;
  
  sieve_machine_begin (mach, err, extra_data);
  sieve_register_standard_actions ();
  sieve_register_standard_tests ();
  sieve_lex_begin (name);
  rc = yyparse ();
  sieve_lex_finish ();
  sieve_machine_finish (mach);
  if (sieve_error_count)
    rc = 1;
  return rc;
}


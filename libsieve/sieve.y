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
%}

%union {
  char *string;
  long number;
  sieve_instr_t instr;
  sieve_value_t *value;
  list_t list;
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

%%

input        : /* empty */
             | list
             ;

list         : statement
             | list statement
             ;

statement    : REQUIRE stringlist ';'
               {
		 sieve_require ($2);
		 sieve_slist_destroy ($2);
	       }
             | action ';'
             | IF cond block maybe_elsif maybe_else
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
             | testlist ',' cond
             ;

cond         : test
             | ANYOF '(' testlist ')'
             | ALLOF '(' testlist ')'
             | NOT cond
             ;

test         : command
               {
		 sieve_register_t *reg = sieve_test_lookup ($1.ident);
		 if (!reg)
		   sieve_error ("%s:%d: unknown test: %s",
				sieve_filename, sieve_line_num,
				$1.ident);
		 else if (!reg->required)
		   sieve_error ("%s:%d: test `%s' has not been required",
				sieve_filename, sieve_line_num,
				$1.ident);
		 /*free unneeded memory */
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
		 if (!reg)
		   sieve_error ("%s:%d: unknown action: %s",
				sieve_filename, sieve_line_num,
				$1.ident);
		 else if (!reg->required)
		   sieve_error ("%s:%d: action `%s' has not been required",
				sieve_filename, sieve_line_num,
				$1.ident);
		 /*free unneeded memory */
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
		 list_append ($$, &$1);
	       }		 
             | arglist arg
               {
		 list_append ($1, &$2);
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

int
sieve_parse (const char *name)
{
  sieve_register_standard_actions ();
  sieve_register_standard_tests ();
  sieve_open_source (name);
  return yyparse ();
}


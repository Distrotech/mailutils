%{
/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  
#include <stdio.h>
#include <assert.h>
#include <sieve.h>

%}

%token IDENT TAG NUMBER STRING MULTILINE
%token REQUIRE IF ELSIF ELSE ANYOF ALLOF TRUE FALSE NOT
%%

input        : /* empty */
             | list
             ;

list         : statement
             | list statement
             ;

statement    : REQUIRE stringlist ';'
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


testlist     : test
             | testlist ',' test
             ;

cond         : test
             | ANYOF '(' testlist ')'
             | ALLOF '(' testlist ')'
             | NOT cond
             ;

test         : FALSE
             | TRUE
             | command
             ;

command      : IDENT maybe_arglist
             ;

action       : command
             ;

maybe_arglist: /* empty */
             | arglist
	     ;

arglist      : arg
             | arglist arg
             ;

arg          : stringlist
             | MULTILINE
             | NUMBER
             | TAG
             ;
 
stringlist   : STRING
             | '[' slist ']'
             ;

slist        : STRING
             | slist ',' STRING
             ;

%%

int
yyerror (char *s)
{
  fprintf (stderr, "%s:%d: ", sieve_filename, sieve_line_num);
  fprintf (stderr, "%s\n", s);
}

int
sieve_parse (const char *name)
{
  sieve_open_source (name);
  return yyparse ();
}


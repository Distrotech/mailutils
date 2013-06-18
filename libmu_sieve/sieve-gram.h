/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int mu_sieve_yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum mu_sieve_yytokentype {
     IDENT = 258,
     TAG = 259,
     NUMBER = 260,
     STRING = 261,
     MULTILINE = 262,
     REQUIRE = 263,
     IF = 264,
     ELSIF = 265,
     ELSE = 266,
     ANYOF = 267,
     ALLOF = 268,
     NOT = 269
   };
#endif
/* Tokens.  */
#define IDENT 258
#define TAG 259
#define NUMBER 260
#define STRING 261
#define MULTILINE 262
#define REQUIRE 263
#define IF 264
#define ELSIF 265
#define ELSE 266
#define ANYOF 267
#define ALLOF 268
#define NOT 269



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 33 "sieve.y"

  char *string;
  size_t number;
  sieve_instr_t instr;
  mu_sieve_value_t *value;
  mu_list_t list;
  size_t pc;
  struct {
    size_t start;
    size_t end;
  } pclist;
  struct {
    char *ident;
    mu_list_t args;
  } command;
  struct {
    size_t begin;
    size_t cond;
    size_t branch;
  } branch;


/* Line 2058 of yacc.c  */
#line 108 "sieve-gram.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define mu_sieve_yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE mu_sieve_yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int mu_sieve_yyparse (void *YYPARSE_PARAM);
#else
int mu_sieve_yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int mu_sieve_yyparse (void);
#else
int mu_sieve_yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

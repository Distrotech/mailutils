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
extern int pick_yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum pick_yytokentype {
     T_COMP = 258,
     T_DATEFIELD = 259,
     T_STRING = 260,
     T_CFLAGS = 261,
     T_LBRACE = 262,
     T_RBRACE = 263,
     T_BEFORE = 264,
     T_AFTER = 265,
     T_OR = 266,
     T_AND = 267,
     T_NOT = 268
   };
#endif
/* Tokens.  */
#define T_COMP 258
#define T_DATEFIELD 259
#define T_STRING 260
#define T_CFLAGS 261
#define T_LBRACE 262
#define T_RBRACE 263
#define T_BEFORE 264
#define T_AFTER 265
#define T_OR 266
#define T_AND 267
#define T_NOT 268



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 47 "pick.y"

  char *string;
  node_t *node;
  regex_t regex;


/* Line 2058 of yacc.c  */
#line 90 "pick-gram.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define pick_yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE pick_yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int pick_yyparse (void *YYPARSE_PARAM);
#else
int pick_yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int pick_yyparse (void);
#else
int pick_yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

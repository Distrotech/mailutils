/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison implementation for Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with mu_sieve_yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 1 "sieve.y"

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005-2012 Free Software Foundation, Inc.

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

mu_sieve_machine_t mu_sieve_machine;
int mu_sieve_error_count;

static void branch_fixup (size_t start, size_t end);

/* Line 371 of yacc.c  */
#line 100 "sieve-gram.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
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
/* Line 387 of yacc.c  */
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


/* Line 387 of yacc.c  */
#line 194 "sieve-gram.c"
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

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 222 "sieve-gram.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 mu_sieve_yytype_uint8;
#else
typedef unsigned char mu_sieve_yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 mu_sieve_yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char mu_sieve_yytype_int8;
#else
typedef short int mu_sieve_yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 mu_sieve_yytype_uint16;
#else
typedef unsigned short int mu_sieve_yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 mu_sieve_yytype_int16;
#else
typedef short int mu_sieve_yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int mu_sieve_yyi)
#else
static int
YYID (mu_sieve_yyi)
    int mu_sieve_yyi;
#endif
{
  return mu_sieve_yyi;
}
#endif

#if ! defined mu_sieve_yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined mu_sieve_yyoverflow || YYERROR_VERBOSE */


#if (! defined mu_sieve_yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union mu_sieve_yyalloc
{
  mu_sieve_yytype_int16 mu_sieve_yyss_alloc;
  YYSTYPE mu_sieve_yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union mu_sieve_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (mu_sieve_yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T mu_sieve_yynewbytes;						\
	YYCOPY (&mu_sieve_yyptr->Stack_alloc, Stack, mu_sieve_yysize);			\
	Stack = &mu_sieve_yyptr->Stack_alloc;					\
	mu_sieve_yynewbytes = mu_sieve_yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	mu_sieve_yyptr += mu_sieve_yynewbytes / sizeof (*mu_sieve_yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T mu_sieve_yyi;                         \
          for (mu_sieve_yyi = 0; mu_sieve_yyi < (Count); mu_sieve_yyi++)   \
            (Dst)[mu_sieve_yyi] = (Src)[mu_sieve_yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  22
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   54

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  23
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  24
/* YYNRULES -- Number of rules.  */
#define YYNRULES  43
/* YYNRULES -- Number of states.  */
#define YYNSTATES  68

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   269

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? mu_sieve_yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const mu_sieve_yytype_uint8 mu_sieve_yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      19,    20,     2,     2,    18,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    15,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    21,     2,    22,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    16,     2,    17,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const mu_sieve_yytype_uint8 mu_sieve_yyprhs[] =
{
       0,     0,     3,     4,     6,     8,    11,    15,    18,    23,
      25,    27,    31,    32,    34,    39,    45,    47,    49,    53,
      55,    59,    61,    63,    68,    73,    76,    77,    79,    82,
      84,    85,    87,    89,    92,    94,    96,    98,   100,   102,
     104,   106,   110,   112
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const mu_sieve_yytype_int8 mu_sieve_yyrhs[] =
{
      24,     0,    -1,    -1,    25,    -1,    26,    -1,    25,    26,
      -1,     8,    44,    15,    -1,    40,    15,    -1,    27,    35,
      33,    28,    -1,     9,    -1,    29,    -1,    29,    32,    33,
      -1,    -1,    30,    -1,    31,    37,    35,    33,    -1,    30,
      31,    37,    35,    33,    -1,    10,    -1,    11,    -1,    16,
      25,    17,    -1,    36,    -1,    34,    18,    36,    -1,    36,
      -1,    38,    -1,    12,    19,    34,    20,    -1,    13,    19,
      34,    20,    -1,    14,    36,    -1,    -1,    39,    -1,     3,
      41,    -1,    39,    -1,    -1,    42,    -1,    43,    -1,    42,
      43,    -1,    45,    -1,     6,    -1,     7,    -1,     5,    -1,
       4,    -1,     6,    -1,    45,    -1,    21,    46,    22,    -1,
       6,    -1,    46,    18,     6,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const mu_sieve_yytype_uint16 mu_sieve_yyrline[] =
{
       0,    69,    69,    70,    74,    75,    78,    86,    88,    96,
     102,   113,   131,   134,   137,   143,   153,   161,   169,   175,
     182,   193,   201,   203,   207,   211,   219,   224,   243,   250,
     270,   273,   276,   281,   288,   292,   296,   300,   304,   310,
     315,   318,   324,   329
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const mu_sieve_yytname[] =
{
  "$end", "error", "$undefined", "IDENT", "TAG", "NUMBER", "STRING",
  "MULTILINE", "REQUIRE", "IF", "ELSIF", "ELSE", "ANYOF", "ALLOF", "NOT",
  "';'", "'{'", "'}'", "','", "'('", "')'", "'['", "']'", "$accept",
  "input", "list", "statement", "if", "else_part", "maybe_elsif",
  "elsif_branch", "elsif", "else", "block", "testlist", "cond",
  "cond_expr", "begin", "test", "command", "action", "maybe_arglist",
  "arglist", "arg", "stringorlist", "stringlist", "slist", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const mu_sieve_yytype_uint16 mu_sieve_yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,    59,   123,   125,    44,    40,
      41,    91,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const mu_sieve_yytype_uint8 mu_sieve_yyr1[] =
{
       0,    23,    24,    24,    25,    25,    26,    26,    26,    27,
      28,    28,    29,    29,    30,    30,    31,    32,    33,    34,
      34,    35,    36,    36,    36,    36,    37,    38,    39,    40,
      41,    41,    42,    42,    43,    43,    43,    43,    43,    44,
      44,    45,    46,    46
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const mu_sieve_yytype_uint8 mu_sieve_yyr2[] =
{
       0,     2,     0,     1,     1,     2,     3,     2,     4,     1,
       1,     3,     0,     1,     4,     5,     1,     1,     3,     1,
       3,     1,     1,     4,     4,     2,     0,     1,     2,     1,
       0,     1,     1,     2,     1,     1,     1,     1,     1,     1,
       1,     3,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const mu_sieve_yytype_uint8 mu_sieve_yydefact[] =
{
       2,    30,     0,     9,     0,     3,     4,     0,    29,     0,
      38,    37,    35,    36,     0,    28,    31,    32,    34,    39,
       0,    40,     1,     5,     0,     0,     0,     0,    21,    22,
      27,     7,    42,     0,    33,     6,     0,     0,    25,     0,
      12,     0,    41,     0,    19,     0,     0,    16,     8,    10,
      13,    26,    43,     0,    23,    24,    18,    17,     0,    26,
       0,    20,    11,     0,     0,     0,    14,    15
};

/* YYDEFGOTO[NTERM-NUM].  */
static const mu_sieve_yytype_int8 mu_sieve_yydefgoto[] =
{
      -1,     4,     5,     6,     7,    48,    49,    50,    51,    58,
      40,    43,    27,    28,    60,    29,    30,     9,    15,    16,
      17,    20,    18,    33
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -47
static const mu_sieve_yytype_int8 mu_sieve_yypact[] =
{
      24,     2,    -3,   -47,     4,    24,   -47,     8,   -47,    13,
     -47,   -47,   -47,   -47,    25,   -47,     2,   -47,   -47,   -47,
      26,   -47,   -47,   -47,    21,    28,     8,    27,   -47,   -47,
     -47,   -47,   -47,    12,   -47,   -47,     8,     8,   -47,    24,
      34,    39,   -47,    17,   -47,    18,     7,   -47,   -47,    37,
      34,   -47,   -47,     8,   -47,   -47,   -47,   -47,    27,   -47,
       8,   -47,   -47,     8,    27,    27,   -47,   -47
};

/* YYPGOTO[NTERM-NUM].  */
static const mu_sieve_yytype_int8 mu_sieve_yypgoto[] =
{
     -47,   -47,    10,    -4,   -47,   -47,   -47,   -47,     1,   -47,
     -39,    15,   -46,   -24,    -9,   -47,     0,   -47,   -47,   -47,
      38,   -47,    51,   -47
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const mu_sieve_yytype_uint8 mu_sieve_yytable[] =
{
       8,    23,    38,    19,    22,     8,    10,    11,    12,    13,
       1,     1,    44,    44,    64,     2,     3,    65,    14,    62,
      24,    25,    26,    14,    56,    66,    67,     1,    31,    61,
      41,    32,     2,     3,    42,    53,    53,    54,    55,     8,
      36,    35,    23,    39,    47,    52,     8,    37,    57,    46,
      63,    59,    45,    21,    34
};

#define mu_sieve_yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-47)))

#define mu_sieve_yytable_value_is_error(Yytable_value) \
  YYID (0)

static const mu_sieve_yytype_uint8 mu_sieve_yycheck[] =
{
       0,     5,    26,     6,     0,     5,     4,     5,     6,     7,
       3,     3,    36,    37,    60,     8,     9,    63,    21,    58,
      12,    13,    14,    21,    17,    64,    65,     3,    15,    53,
      18,     6,     8,     9,    22,    18,    18,    20,    20,    39,
      19,    15,    46,    16,    10,     6,    46,    19,    11,    39,
      59,    50,    37,     2,    16
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const mu_sieve_yytype_uint8 mu_sieve_yystos[] =
{
       0,     3,     8,     9,    24,    25,    26,    27,    39,    40,
       4,     5,     6,     7,    21,    41,    42,    43,    45,     6,
      44,    45,     0,    26,    12,    13,    14,    35,    36,    38,
      39,    15,     6,    46,    43,    15,    19,    19,    36,    16,
      33,    18,    22,    34,    36,    34,    25,    10,    28,    29,
      30,    31,     6,    18,    20,    20,    17,    11,    32,    31,
      37,    36,    33,    37,    35,    35,    33,    33
};

#define mu_sieve_yyerrok		(mu_sieve_yyerrstatus = 0)
#define mu_sieve_yyclearin	(mu_sieve_yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto mu_sieve_yyacceptlab
#define YYABORT		goto mu_sieve_yyabortlab
#define YYERROR		goto mu_sieve_yyerrorlab


/* Like YYERROR except do call mu_sieve_yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto mu_sieve_yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!mu_sieve_yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (mu_sieve_yychar == YYEMPTY)                                        \
    {                                                           \
      mu_sieve_yychar = (Token);                                         \
      mu_sieve_yylval = (Value);                                         \
      YYPOPSTACK (mu_sieve_yylen);                                       \
      mu_sieve_yystate = *mu_sieve_yyssp;                                         \
      goto mu_sieve_yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      mu_sieve_yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `mu_sieve_yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX mu_sieve_yylex (YYLEX_PARAM)
#else
# define YYLEX mu_sieve_yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (mu_sieve_yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (mu_sieve_yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      mu_sieve_yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
mu_sieve_yy_symbol_value_print (FILE *mu_sieve_yyoutput, int mu_sieve_yytype, YYSTYPE const * const mu_sieve_yyvaluep)
#else
static void
mu_sieve_yy_symbol_value_print (mu_sieve_yyoutput, mu_sieve_yytype, mu_sieve_yyvaluep)
    FILE *mu_sieve_yyoutput;
    int mu_sieve_yytype;
    YYSTYPE const * const mu_sieve_yyvaluep;
#endif
{
  FILE *mu_sieve_yyo = mu_sieve_yyoutput;
  YYUSE (mu_sieve_yyo);
  if (!mu_sieve_yyvaluep)
    return;
# ifdef YYPRINT
  if (mu_sieve_yytype < YYNTOKENS)
    YYPRINT (mu_sieve_yyoutput, mu_sieve_yytoknum[mu_sieve_yytype], *mu_sieve_yyvaluep);
# else
  YYUSE (mu_sieve_yyoutput);
# endif
  switch (mu_sieve_yytype)
    {
      default:
        break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
mu_sieve_yy_symbol_print (FILE *mu_sieve_yyoutput, int mu_sieve_yytype, YYSTYPE const * const mu_sieve_yyvaluep)
#else
static void
mu_sieve_yy_symbol_print (mu_sieve_yyoutput, mu_sieve_yytype, mu_sieve_yyvaluep)
    FILE *mu_sieve_yyoutput;
    int mu_sieve_yytype;
    YYSTYPE const * const mu_sieve_yyvaluep;
#endif
{
  if (mu_sieve_yytype < YYNTOKENS)
    YYFPRINTF (mu_sieve_yyoutput, "token %s (", mu_sieve_yytname[mu_sieve_yytype]);
  else
    YYFPRINTF (mu_sieve_yyoutput, "nterm %s (", mu_sieve_yytname[mu_sieve_yytype]);

  mu_sieve_yy_symbol_value_print (mu_sieve_yyoutput, mu_sieve_yytype, mu_sieve_yyvaluep);
  YYFPRINTF (mu_sieve_yyoutput, ")");
}

/*------------------------------------------------------------------.
| mu_sieve_yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
mu_sieve_yy_stack_print (mu_sieve_yytype_int16 *mu_sieve_yybottom, mu_sieve_yytype_int16 *mu_sieve_yytop)
#else
static void
mu_sieve_yy_stack_print (mu_sieve_yybottom, mu_sieve_yytop)
    mu_sieve_yytype_int16 *mu_sieve_yybottom;
    mu_sieve_yytype_int16 *mu_sieve_yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; mu_sieve_yybottom <= mu_sieve_yytop; mu_sieve_yybottom++)
    {
      int mu_sieve_yybot = *mu_sieve_yybottom;
      YYFPRINTF (stderr, " %d", mu_sieve_yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (mu_sieve_yydebug)							\
    mu_sieve_yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
mu_sieve_yy_reduce_print (YYSTYPE *mu_sieve_yyvsp, int mu_sieve_yyrule)
#else
static void
mu_sieve_yy_reduce_print (mu_sieve_yyvsp, mu_sieve_yyrule)
    YYSTYPE *mu_sieve_yyvsp;
    int mu_sieve_yyrule;
#endif
{
  int mu_sieve_yynrhs = mu_sieve_yyr2[mu_sieve_yyrule];
  int mu_sieve_yyi;
  unsigned long int mu_sieve_yylno = mu_sieve_yyrline[mu_sieve_yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     mu_sieve_yyrule - 1, mu_sieve_yylno);
  /* The symbols being reduced.  */
  for (mu_sieve_yyi = 0; mu_sieve_yyi < mu_sieve_yynrhs; mu_sieve_yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", mu_sieve_yyi + 1);
      mu_sieve_yy_symbol_print (stderr, mu_sieve_yyrhs[mu_sieve_yyprhs[mu_sieve_yyrule] + mu_sieve_yyi],
		       &(mu_sieve_yyvsp[(mu_sieve_yyi + 1) - (mu_sieve_yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (mu_sieve_yydebug)				\
    mu_sieve_yy_reduce_print (mu_sieve_yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int mu_sieve_yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef mu_sieve_yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define mu_sieve_yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
mu_sieve_yystrlen (const char *mu_sieve_yystr)
#else
static YYSIZE_T
mu_sieve_yystrlen (mu_sieve_yystr)
    const char *mu_sieve_yystr;
#endif
{
  YYSIZE_T mu_sieve_yylen;
  for (mu_sieve_yylen = 0; mu_sieve_yystr[mu_sieve_yylen]; mu_sieve_yylen++)
    continue;
  return mu_sieve_yylen;
}
#  endif
# endif

# ifndef mu_sieve_yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define mu_sieve_yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
mu_sieve_yystpcpy (char *mu_sieve_yydest, const char *mu_sieve_yysrc)
#else
static char *
mu_sieve_yystpcpy (mu_sieve_yydest, mu_sieve_yysrc)
    char *mu_sieve_yydest;
    const char *mu_sieve_yysrc;
#endif
{
  char *mu_sieve_yyd = mu_sieve_yydest;
  const char *mu_sieve_yys = mu_sieve_yysrc;

  while ((*mu_sieve_yyd++ = *mu_sieve_yys++) != '\0')
    continue;

  return mu_sieve_yyd - 1;
}
#  endif
# endif

# ifndef mu_sieve_yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for mu_sieve_yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from mu_sieve_yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
mu_sieve_yytnamerr (char *mu_sieve_yyres, const char *mu_sieve_yystr)
{
  if (*mu_sieve_yystr == '"')
    {
      YYSIZE_T mu_sieve_yyn = 0;
      char const *mu_sieve_yyp = mu_sieve_yystr;

      for (;;)
	switch (*++mu_sieve_yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++mu_sieve_yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (mu_sieve_yyres)
	      mu_sieve_yyres[mu_sieve_yyn] = *mu_sieve_yyp;
	    mu_sieve_yyn++;
	    break;

	  case '"':
	    if (mu_sieve_yyres)
	      mu_sieve_yyres[mu_sieve_yyn] = '\0';
	    return mu_sieve_yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! mu_sieve_yyres)
    return mu_sieve_yystrlen (mu_sieve_yystr);

  return mu_sieve_yystpcpy (mu_sieve_yyres, mu_sieve_yystr) - mu_sieve_yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
mu_sieve_yysyntax_error (YYSIZE_T *mu_sieve_yymsg_alloc, char **mu_sieve_yymsg,
                mu_sieve_yytype_int16 *mu_sieve_yyssp, int mu_sieve_yytoken)
{
  YYSIZE_T mu_sieve_yysize0 = mu_sieve_yytnamerr (YY_NULL, mu_sieve_yytname[mu_sieve_yytoken]);
  YYSIZE_T mu_sieve_yysize = mu_sieve_yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *mu_sieve_yyformat = YY_NULL;
  /* Arguments of mu_sieve_yyformat. */
  char const *mu_sieve_yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int mu_sieve_yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in mu_sieve_yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated mu_sieve_yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (mu_sieve_yytoken != YYEMPTY)
    {
      int mu_sieve_yyn = mu_sieve_yypact[*mu_sieve_yyssp];
      mu_sieve_yyarg[mu_sieve_yycount++] = mu_sieve_yytname[mu_sieve_yytoken];
      if (!mu_sieve_yypact_value_is_default (mu_sieve_yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int mu_sieve_yyxbegin = mu_sieve_yyn < 0 ? -mu_sieve_yyn : 0;
          /* Stay within bounds of both mu_sieve_yycheck and mu_sieve_yytname.  */
          int mu_sieve_yychecklim = YYLAST - mu_sieve_yyn + 1;
          int mu_sieve_yyxend = mu_sieve_yychecklim < YYNTOKENS ? mu_sieve_yychecklim : YYNTOKENS;
          int mu_sieve_yyx;

          for (mu_sieve_yyx = mu_sieve_yyxbegin; mu_sieve_yyx < mu_sieve_yyxend; ++mu_sieve_yyx)
            if (mu_sieve_yycheck[mu_sieve_yyx + mu_sieve_yyn] == mu_sieve_yyx && mu_sieve_yyx != YYTERROR
                && !mu_sieve_yytable_value_is_error (mu_sieve_yytable[mu_sieve_yyx + mu_sieve_yyn]))
              {
                if (mu_sieve_yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    mu_sieve_yycount = 1;
                    mu_sieve_yysize = mu_sieve_yysize0;
                    break;
                  }
                mu_sieve_yyarg[mu_sieve_yycount++] = mu_sieve_yytname[mu_sieve_yyx];
                {
                  YYSIZE_T mu_sieve_yysize1 = mu_sieve_yysize + mu_sieve_yytnamerr (YY_NULL, mu_sieve_yytname[mu_sieve_yyx]);
                  if (! (mu_sieve_yysize <= mu_sieve_yysize1
                         && mu_sieve_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  mu_sieve_yysize = mu_sieve_yysize1;
                }
              }
        }
    }

  switch (mu_sieve_yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        mu_sieve_yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T mu_sieve_yysize1 = mu_sieve_yysize + mu_sieve_yystrlen (mu_sieve_yyformat);
    if (! (mu_sieve_yysize <= mu_sieve_yysize1 && mu_sieve_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    mu_sieve_yysize = mu_sieve_yysize1;
  }

  if (*mu_sieve_yymsg_alloc < mu_sieve_yysize)
    {
      *mu_sieve_yymsg_alloc = 2 * mu_sieve_yysize;
      if (! (mu_sieve_yysize <= *mu_sieve_yymsg_alloc
             && *mu_sieve_yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *mu_sieve_yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *mu_sieve_yyp = *mu_sieve_yymsg;
    int mu_sieve_yyi = 0;
    while ((*mu_sieve_yyp = *mu_sieve_yyformat) != '\0')
      if (*mu_sieve_yyp == '%' && mu_sieve_yyformat[1] == 's' && mu_sieve_yyi < mu_sieve_yycount)
        {
          mu_sieve_yyp += mu_sieve_yytnamerr (mu_sieve_yyp, mu_sieve_yyarg[mu_sieve_yyi++]);
          mu_sieve_yyformat += 2;
        }
      else
        {
          mu_sieve_yyp++;
          mu_sieve_yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
mu_sieve_yydestruct (const char *mu_sieve_yymsg, int mu_sieve_yytype, YYSTYPE *mu_sieve_yyvaluep)
#else
static void
mu_sieve_yydestruct (mu_sieve_yymsg, mu_sieve_yytype, mu_sieve_yyvaluep)
    const char *mu_sieve_yymsg;
    int mu_sieve_yytype;
    YYSTYPE *mu_sieve_yyvaluep;
#endif
{
  YYUSE (mu_sieve_yyvaluep);

  if (!mu_sieve_yymsg)
    mu_sieve_yymsg = "Deleting";
  YY_SYMBOL_PRINT (mu_sieve_yymsg, mu_sieve_yytype, mu_sieve_yyvaluep, mu_sieve_yylocationp);

  switch (mu_sieve_yytype)
    {

      default:
        break;
    }
}




/* The lookahead symbol.  */
int mu_sieve_yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE mu_sieve_yylval YY_INITIAL_VALUE(mu_sieve_yyval_default);

/* Number of syntax errors so far.  */
int mu_sieve_yynerrs;


/*----------.
| mu_sieve_yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
mu_sieve_yyparse (void *YYPARSE_PARAM)
#else
int
mu_sieve_yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
mu_sieve_yyparse (void)
#else
int
mu_sieve_yyparse ()

#endif
#endif
{
    int mu_sieve_yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int mu_sieve_yyerrstatus;

    /* The stacks and their tools:
       `mu_sieve_yyss': related to states.
       `mu_sieve_yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow mu_sieve_yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    mu_sieve_yytype_int16 mu_sieve_yyssa[YYINITDEPTH];
    mu_sieve_yytype_int16 *mu_sieve_yyss;
    mu_sieve_yytype_int16 *mu_sieve_yyssp;

    /* The semantic value stack.  */
    YYSTYPE mu_sieve_yyvsa[YYINITDEPTH];
    YYSTYPE *mu_sieve_yyvs;
    YYSTYPE *mu_sieve_yyvsp;

    YYSIZE_T mu_sieve_yystacksize;

  int mu_sieve_yyn;
  int mu_sieve_yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int mu_sieve_yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE mu_sieve_yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char mu_sieve_yymsgbuf[128];
  char *mu_sieve_yymsg = mu_sieve_yymsgbuf;
  YYSIZE_T mu_sieve_yymsg_alloc = sizeof mu_sieve_yymsgbuf;
#endif

#define YYPOPSTACK(N)   (mu_sieve_yyvsp -= (N), mu_sieve_yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int mu_sieve_yylen = 0;

  mu_sieve_yyssp = mu_sieve_yyss = mu_sieve_yyssa;
  mu_sieve_yyvsp = mu_sieve_yyvs = mu_sieve_yyvsa;
  mu_sieve_yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  mu_sieve_yystate = 0;
  mu_sieve_yyerrstatus = 0;
  mu_sieve_yynerrs = 0;
  mu_sieve_yychar = YYEMPTY; /* Cause a token to be read.  */
  goto mu_sieve_yysetstate;

/*------------------------------------------------------------.
| mu_sieve_yynewstate -- Push a new state, which is found in mu_sieve_yystate.  |
`------------------------------------------------------------*/
 mu_sieve_yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  mu_sieve_yyssp++;

 mu_sieve_yysetstate:
  *mu_sieve_yyssp = mu_sieve_yystate;

  if (mu_sieve_yyss + mu_sieve_yystacksize - 1 <= mu_sieve_yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T mu_sieve_yysize = mu_sieve_yyssp - mu_sieve_yyss + 1;

#ifdef mu_sieve_yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *mu_sieve_yyvs1 = mu_sieve_yyvs;
	mu_sieve_yytype_int16 *mu_sieve_yyss1 = mu_sieve_yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if mu_sieve_yyoverflow is a macro.  */
	mu_sieve_yyoverflow (YY_("memory exhausted"),
		    &mu_sieve_yyss1, mu_sieve_yysize * sizeof (*mu_sieve_yyssp),
		    &mu_sieve_yyvs1, mu_sieve_yysize * sizeof (*mu_sieve_yyvsp),
		    &mu_sieve_yystacksize);

	mu_sieve_yyss = mu_sieve_yyss1;
	mu_sieve_yyvs = mu_sieve_yyvs1;
      }
#else /* no mu_sieve_yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto mu_sieve_yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= mu_sieve_yystacksize)
	goto mu_sieve_yyexhaustedlab;
      mu_sieve_yystacksize *= 2;
      if (YYMAXDEPTH < mu_sieve_yystacksize)
	mu_sieve_yystacksize = YYMAXDEPTH;

      {
	mu_sieve_yytype_int16 *mu_sieve_yyss1 = mu_sieve_yyss;
	union mu_sieve_yyalloc *mu_sieve_yyptr =
	  (union mu_sieve_yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (mu_sieve_yystacksize));
	if (! mu_sieve_yyptr)
	  goto mu_sieve_yyexhaustedlab;
	YYSTACK_RELOCATE (mu_sieve_yyss_alloc, mu_sieve_yyss);
	YYSTACK_RELOCATE (mu_sieve_yyvs_alloc, mu_sieve_yyvs);
#  undef YYSTACK_RELOCATE
	if (mu_sieve_yyss1 != mu_sieve_yyssa)
	  YYSTACK_FREE (mu_sieve_yyss1);
      }
# endif
#endif /* no mu_sieve_yyoverflow */

      mu_sieve_yyssp = mu_sieve_yyss + mu_sieve_yysize - 1;
      mu_sieve_yyvsp = mu_sieve_yyvs + mu_sieve_yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) mu_sieve_yystacksize));

      if (mu_sieve_yyss + mu_sieve_yystacksize - 1 <= mu_sieve_yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", mu_sieve_yystate));

  if (mu_sieve_yystate == YYFINAL)
    YYACCEPT;

  goto mu_sieve_yybackup;

/*-----------.
| mu_sieve_yybackup.  |
`-----------*/
mu_sieve_yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  mu_sieve_yyn = mu_sieve_yypact[mu_sieve_yystate];
  if (mu_sieve_yypact_value_is_default (mu_sieve_yyn))
    goto mu_sieve_yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (mu_sieve_yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      mu_sieve_yychar = YYLEX;
    }

  if (mu_sieve_yychar <= YYEOF)
    {
      mu_sieve_yychar = mu_sieve_yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      mu_sieve_yytoken = YYTRANSLATE (mu_sieve_yychar);
      YY_SYMBOL_PRINT ("Next token is", mu_sieve_yytoken, &mu_sieve_yylval, &mu_sieve_yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  mu_sieve_yyn += mu_sieve_yytoken;
  if (mu_sieve_yyn < 0 || YYLAST < mu_sieve_yyn || mu_sieve_yycheck[mu_sieve_yyn] != mu_sieve_yytoken)
    goto mu_sieve_yydefault;
  mu_sieve_yyn = mu_sieve_yytable[mu_sieve_yyn];
  if (mu_sieve_yyn <= 0)
    {
      if (mu_sieve_yytable_value_is_error (mu_sieve_yyn))
        goto mu_sieve_yyerrlab;
      mu_sieve_yyn = -mu_sieve_yyn;
      goto mu_sieve_yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (mu_sieve_yyerrstatus)
    mu_sieve_yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", mu_sieve_yytoken, &mu_sieve_yylval, &mu_sieve_yylloc);

  /* Discard the shifted token.  */
  mu_sieve_yychar = YYEMPTY;

  mu_sieve_yystate = mu_sieve_yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++mu_sieve_yyvsp = mu_sieve_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto mu_sieve_yynewstate;


/*-----------------------------------------------------------.
| mu_sieve_yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
mu_sieve_yydefault:
  mu_sieve_yyn = mu_sieve_yydefact[mu_sieve_yystate];
  if (mu_sieve_yyn == 0)
    goto mu_sieve_yyerrlab;
  goto mu_sieve_yyreduce;


/*-----------------------------.
| mu_sieve_yyreduce -- Do a reduction.  |
`-----------------------------*/
mu_sieve_yyreduce:
  /* mu_sieve_yyn is the number of a rule to reduce with.  */
  mu_sieve_yylen = mu_sieve_yyr2[mu_sieve_yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  mu_sieve_yyval = mu_sieve_yyvsp[1-mu_sieve_yylen];


  YY_REDUCE_PRINT (mu_sieve_yyn);
  switch (mu_sieve_yyn)
    {
        case 3:
/* Line 1792 of yacc.c  */
#line 71 "sieve.y"
    { /* to placate bison */ }
    break;

  case 6:
/* Line 1792 of yacc.c  */
#line 79 "sieve.y"
    {
		 mu_sieve_require ((mu_sieve_yyvsp[(2) - (3)].list));
		 /*  All the items in $2 are registered in memory_pool,
		     so we don't free them */
		 mu_list_destroy (&(mu_sieve_yyvsp[(2) - (3)].list));
		 (mu_sieve_yyval.pc) = mu_sieve_machine->pc;
	       }
    break;

  case 8:
/* Line 1792 of yacc.c  */
#line 89 "sieve.y"
    {
		 mu_sieve_machine->prog[(mu_sieve_yyvsp[(2) - (4)].pc)].pc = (mu_sieve_yyvsp[(4) - (4)].branch).begin - (mu_sieve_yyvsp[(2) - (4)].pc) - 1;
		 if ((mu_sieve_yyvsp[(4) - (4)].branch).branch)
		   branch_fixup ((mu_sieve_yyvsp[(4) - (4)].branch).branch, mu_sieve_machine->pc);
	       }
    break;

  case 9:
/* Line 1792 of yacc.c  */
#line 97 "sieve.y"
    {
		 (mu_sieve_yyval.pc) = mu_sieve_machine->pc;
	       }
    break;

  case 10:
/* Line 1792 of yacc.c  */
#line 103 "sieve.y"
    {
		 if ((mu_sieve_yyvsp[(1) - (1)].branch).begin)
		   mu_sieve_machine->prog[(mu_sieve_yyvsp[(1) - (1)].branch).cond].pc =
		                  mu_sieve_machine->pc - (mu_sieve_yyvsp[(1) - (1)].branch).cond - 1;
		 else
		   {
		     (mu_sieve_yyval.branch).begin = mu_sieve_machine->pc;
		     (mu_sieve_yyval.branch).branch = 0;
		   }
	       }
    break;

  case 11:
/* Line 1792 of yacc.c  */
#line 114 "sieve.y"
    {
		 if ((mu_sieve_yyvsp[(1) - (3)].branch).begin)
		   {
		     mu_sieve_machine->prog[(mu_sieve_yyvsp[(1) - (3)].branch).cond].pc = (mu_sieve_yyvsp[(3) - (3)].pc) - (mu_sieve_yyvsp[(1) - (3)].branch).cond - 1;
		     mu_sieve_machine->prog[(mu_sieve_yyvsp[(2) - (3)].pc)].pc = (mu_sieve_yyvsp[(1) - (3)].branch).branch;
		     (mu_sieve_yyval.branch).begin = (mu_sieve_yyvsp[(1) - (3)].branch).begin;
		     (mu_sieve_yyval.branch).branch = (mu_sieve_yyvsp[(2) - (3)].pc);
		   }
		 else
		   {
		     (mu_sieve_yyval.branch).begin = (mu_sieve_yyvsp[(3) - (3)].pc);
		     (mu_sieve_yyval.branch).branch = (mu_sieve_yyvsp[(2) - (3)].pc);
		   }
	       }
    break;

  case 12:
/* Line 1792 of yacc.c  */
#line 131 "sieve.y"
    {
		 (mu_sieve_yyval.branch).begin = 0;
	       }
    break;

  case 14:
/* Line 1792 of yacc.c  */
#line 138 "sieve.y"
    {
		 (mu_sieve_yyval.branch).begin = (mu_sieve_yyvsp[(2) - (4)].pc); 
		 (mu_sieve_yyval.branch).branch = (mu_sieve_yyvsp[(1) - (4)].pc);
		 (mu_sieve_yyval.branch).cond = (mu_sieve_yyvsp[(3) - (4)].pc);
	       }
    break;

  case 15:
/* Line 1792 of yacc.c  */
#line 144 "sieve.y"
    {
		 mu_sieve_machine->prog[(mu_sieve_yyvsp[(1) - (5)].branch).cond].pc = (mu_sieve_yyvsp[(3) - (5)].pc) - (mu_sieve_yyvsp[(1) - (5)].branch).cond - 1;
		 mu_sieve_machine->prog[(mu_sieve_yyvsp[(2) - (5)].pc)].pc = (mu_sieve_yyvsp[(1) - (5)].branch).branch;
		 (mu_sieve_yyval.branch).begin = (mu_sieve_yyvsp[(1) - (5)].branch).begin;
		 (mu_sieve_yyval.branch).branch = (mu_sieve_yyvsp[(2) - (5)].pc);
		 (mu_sieve_yyval.branch).cond = (mu_sieve_yyvsp[(4) - (5)].pc);
	       }
    break;

  case 16:
/* Line 1792 of yacc.c  */
#line 154 "sieve.y"
    {
		 mu_sv_code_instr (_mu_sv_instr_branch);
		 (mu_sieve_yyval.pc) = mu_sieve_machine->pc;
		 mu_sv_code_number (0);
	       }
    break;

  case 17:
/* Line 1792 of yacc.c  */
#line 162 "sieve.y"
    {
		 mu_sv_code_instr (_mu_sv_instr_branch);
		 (mu_sieve_yyval.pc) = mu_sieve_machine->pc;
		 mu_sv_code_number (0);
	       }
    break;

  case 18:
/* Line 1792 of yacc.c  */
#line 170 "sieve.y"
    {
		 (mu_sieve_yyval.pc) = (mu_sieve_yyvsp[(2) - (3)].pc);
	       }
    break;

  case 19:
/* Line 1792 of yacc.c  */
#line 176 "sieve.y"
    {
		 (mu_sieve_yyval.pclist).start = (mu_sieve_yyval.pclist).end = mu_sieve_machine->pc;
		 if (mu_sv_code_instr (_mu_sv_instr_brz)
		     || mu_sv_code_number (0))
		   YYERROR;
	       }
    break;

  case 20:
/* Line 1792 of yacc.c  */
#line 183 "sieve.y"
    {
		 mu_sieve_machine->prog[(mu_sieve_yyvsp[(1) - (3)].pclist).end+1].pc = mu_sieve_machine->pc;
		 (mu_sieve_yyvsp[(1) - (3)].pclist).end = mu_sieve_machine->pc;
		 if (mu_sv_code_instr (_mu_sv_instr_brz)
		     || mu_sv_code_number (0))
		   YYERROR;
		 (mu_sieve_yyval.pclist) = (mu_sieve_yyvsp[(1) - (3)].pclist);
	       }
    break;

  case 21:
/* Line 1792 of yacc.c  */
#line 194 "sieve.y"
    {
		 mu_sv_code_instr (_mu_sv_instr_brz);
		 (mu_sieve_yyval.pc) = mu_sieve_machine->pc;
		 mu_sv_code_number (0);
	       }
    break;

  case 22:
/* Line 1792 of yacc.c  */
#line 202 "sieve.y"
    { /* to placate bison */ }
    break;

  case 23:
/* Line 1792 of yacc.c  */
#line 204 "sieve.y"
    {
		 mu_sv_code_anyof ((mu_sieve_yyvsp[(3) - (4)].pclist).start);
	       }
    break;

  case 24:
/* Line 1792 of yacc.c  */
#line 208 "sieve.y"
    {
		 mu_sv_code_allof ((mu_sieve_yyvsp[(3) - (4)].pclist).start);
	       }
    break;

  case 25:
/* Line 1792 of yacc.c  */
#line 212 "sieve.y"
    {
		 if (mu_sv_code_instr (_mu_sv_instr_not))
		   YYERROR;
	       }
    break;

  case 26:
/* Line 1792 of yacc.c  */
#line 219 "sieve.y"
    {
		 (mu_sieve_yyval.pc) = mu_sieve_machine->pc;
	       }
    break;

  case 27:
/* Line 1792 of yacc.c  */
#line 225 "sieve.y"
    {
		 mu_sieve_register_t *reg = 
		        mu_sieve_test_lookup (mu_sieve_machine, (mu_sieve_yyvsp[(1) - (1)].command).ident);
		 (mu_sieve_yyval.pc) = mu_sieve_machine->pc;

		 if (!reg)
		   mu_sv_compile_error (&mu_sieve_locus,
					_("unknown test: %s"),
					(mu_sieve_yyvsp[(1) - (1)].command).ident);
		 else if (!reg->required)
		   mu_sv_compile_error (&mu_sieve_locus,
					_("test `%s' has not been required"),
					(mu_sieve_yyvsp[(1) - (1)].command).ident);
		 else if (mu_sv_code_test (reg, (mu_sieve_yyvsp[(1) - (1)].command).args))
		   YYERROR;
	       }
    break;

  case 28:
/* Line 1792 of yacc.c  */
#line 244 "sieve.y"
    {
		 (mu_sieve_yyval.command).ident = (mu_sieve_yyvsp[(1) - (2)].string);
		 (mu_sieve_yyval.command).args = (mu_sieve_yyvsp[(2) - (2)].list);
	       }
    break;

  case 29:
/* Line 1792 of yacc.c  */
#line 251 "sieve.y"
    {
		 mu_sieve_register_t *reg = 
		        mu_sieve_action_lookup (mu_sieve_machine, (mu_sieve_yyvsp[(1) - (1)].command).ident);
		 
		 (mu_sieve_yyval.pc) = mu_sieve_machine->pc;
		 if (!reg)
		   mu_sv_compile_error (&mu_sieve_locus,
					_("unknown action: %s"),
					(mu_sieve_yyvsp[(1) - (1)].command).ident);
		 else if (!reg->required)
		   mu_sv_compile_error (&mu_sieve_locus,
					_("action `%s' has not been required"),
					(mu_sieve_yyvsp[(1) - (1)].command).ident);
		 else if (mu_sv_code_action (reg, (mu_sieve_yyvsp[(1) - (1)].command).args))
		   YYERROR;
	       }
    break;

  case 30:
/* Line 1792 of yacc.c  */
#line 270 "sieve.y"
    {
		 (mu_sieve_yyval.list) = NULL;
	       }
    break;

  case 32:
/* Line 1792 of yacc.c  */
#line 277 "sieve.y"
    {
		 mu_list_create (&(mu_sieve_yyval.list));
		 mu_list_append ((mu_sieve_yyval.list), (mu_sieve_yyvsp[(1) - (1)].value));
	       }
    break;

  case 33:
/* Line 1792 of yacc.c  */
#line 282 "sieve.y"
    {
		 mu_list_append ((mu_sieve_yyvsp[(1) - (2)].list), (mu_sieve_yyvsp[(2) - (2)].value));
		 (mu_sieve_yyval.list) = (mu_sieve_yyvsp[(1) - (2)].list);
	       }
    break;

  case 34:
/* Line 1792 of yacc.c  */
#line 289 "sieve.y"
    {
		 (mu_sieve_yyval.value) = mu_sieve_value_create (SVT_STRING_LIST, (mu_sieve_yyvsp[(1) - (1)].list));
	       }
    break;

  case 35:
/* Line 1792 of yacc.c  */
#line 293 "sieve.y"
    {
		 (mu_sieve_yyval.value) = mu_sieve_value_create (SVT_STRING, (mu_sieve_yyvsp[(1) - (1)].string));
               }
    break;

  case 36:
/* Line 1792 of yacc.c  */
#line 297 "sieve.y"
    {
		 (mu_sieve_yyval.value) = mu_sieve_value_create (SVT_STRING, (mu_sieve_yyvsp[(1) - (1)].string));
	       }
    break;

  case 37:
/* Line 1792 of yacc.c  */
#line 301 "sieve.y"
    {
		 (mu_sieve_yyval.value) = mu_sieve_value_create (SVT_NUMBER, &(mu_sieve_yyvsp[(1) - (1)].number));
	       }
    break;

  case 38:
/* Line 1792 of yacc.c  */
#line 305 "sieve.y"
    {
		 (mu_sieve_yyval.value) = mu_sieve_value_create (SVT_TAG, (mu_sieve_yyvsp[(1) - (1)].string));
	       }
    break;

  case 39:
/* Line 1792 of yacc.c  */
#line 311 "sieve.y"
    {
		 mu_list_create (&(mu_sieve_yyval.list));
		 mu_list_append ((mu_sieve_yyval.list), (mu_sieve_yyvsp[(1) - (1)].string));
	       }
    break;

  case 41:
/* Line 1792 of yacc.c  */
#line 319 "sieve.y"
    {
		 (mu_sieve_yyval.list) = (mu_sieve_yyvsp[(2) - (3)].list);
	       }
    break;

  case 42:
/* Line 1792 of yacc.c  */
#line 325 "sieve.y"
    {
		 mu_list_create (&(mu_sieve_yyval.list));
		 mu_list_append ((mu_sieve_yyval.list), (mu_sieve_yyvsp[(1) - (1)].string));
	       }
    break;

  case 43:
/* Line 1792 of yacc.c  */
#line 330 "sieve.y"
    {
		 mu_list_append ((mu_sieve_yyvsp[(1) - (3)].list), (mu_sieve_yyvsp[(3) - (3)].string));
		 (mu_sieve_yyval.list) = (mu_sieve_yyvsp[(1) - (3)].list);
	       }
    break;


/* Line 1792 of yacc.c  */
#line 1817 "sieve-gram.c"
      default: break;
    }
  /* User semantic actions sometimes alter mu_sieve_yychar, and that requires
     that mu_sieve_yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of mu_sieve_yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering mu_sieve_yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", mu_sieve_yyr1[mu_sieve_yyn], &mu_sieve_yyval, &mu_sieve_yyloc);

  YYPOPSTACK (mu_sieve_yylen);
  mu_sieve_yylen = 0;
  YY_STACK_PRINT (mu_sieve_yyss, mu_sieve_yyssp);

  *++mu_sieve_yyvsp = mu_sieve_yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  mu_sieve_yyn = mu_sieve_yyr1[mu_sieve_yyn];

  mu_sieve_yystate = mu_sieve_yypgoto[mu_sieve_yyn - YYNTOKENS] + *mu_sieve_yyssp;
  if (0 <= mu_sieve_yystate && mu_sieve_yystate <= YYLAST && mu_sieve_yycheck[mu_sieve_yystate] == *mu_sieve_yyssp)
    mu_sieve_yystate = mu_sieve_yytable[mu_sieve_yystate];
  else
    mu_sieve_yystate = mu_sieve_yydefgoto[mu_sieve_yyn - YYNTOKENS];

  goto mu_sieve_yynewstate;


/*------------------------------------.
| mu_sieve_yyerrlab -- here on detecting error |
`------------------------------------*/
mu_sieve_yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  mu_sieve_yytoken = mu_sieve_yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (mu_sieve_yychar);

  /* If not already recovering from an error, report this error.  */
  if (!mu_sieve_yyerrstatus)
    {
      ++mu_sieve_yynerrs;
#if ! YYERROR_VERBOSE
      mu_sieve_yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR mu_sieve_yysyntax_error (&mu_sieve_yymsg_alloc, &mu_sieve_yymsg, \
                                        mu_sieve_yyssp, mu_sieve_yytoken)
      {
        char const *mu_sieve_yymsgp = YY_("syntax error");
        int mu_sieve_yysyntax_error_status;
        mu_sieve_yysyntax_error_status = YYSYNTAX_ERROR;
        if (mu_sieve_yysyntax_error_status == 0)
          mu_sieve_yymsgp = mu_sieve_yymsg;
        else if (mu_sieve_yysyntax_error_status == 1)
          {
            if (mu_sieve_yymsg != mu_sieve_yymsgbuf)
              YYSTACK_FREE (mu_sieve_yymsg);
            mu_sieve_yymsg = (char *) YYSTACK_ALLOC (mu_sieve_yymsg_alloc);
            if (!mu_sieve_yymsg)
              {
                mu_sieve_yymsg = mu_sieve_yymsgbuf;
                mu_sieve_yymsg_alloc = sizeof mu_sieve_yymsgbuf;
                mu_sieve_yysyntax_error_status = 2;
              }
            else
              {
                mu_sieve_yysyntax_error_status = YYSYNTAX_ERROR;
                mu_sieve_yymsgp = mu_sieve_yymsg;
              }
          }
        mu_sieve_yyerror (mu_sieve_yymsgp);
        if (mu_sieve_yysyntax_error_status == 2)
          goto mu_sieve_yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (mu_sieve_yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (mu_sieve_yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (mu_sieve_yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  mu_sieve_yydestruct ("Error: discarding",
		      mu_sieve_yytoken, &mu_sieve_yylval);
	  mu_sieve_yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto mu_sieve_yyerrlab1;


/*---------------------------------------------------.
| mu_sieve_yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
mu_sieve_yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label mu_sieve_yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto mu_sieve_yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (mu_sieve_yylen);
  mu_sieve_yylen = 0;
  YY_STACK_PRINT (mu_sieve_yyss, mu_sieve_yyssp);
  mu_sieve_yystate = *mu_sieve_yyssp;
  goto mu_sieve_yyerrlab1;


/*-------------------------------------------------------------.
| mu_sieve_yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
mu_sieve_yyerrlab1:
  mu_sieve_yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      mu_sieve_yyn = mu_sieve_yypact[mu_sieve_yystate];
      if (!mu_sieve_yypact_value_is_default (mu_sieve_yyn))
	{
	  mu_sieve_yyn += YYTERROR;
	  if (0 <= mu_sieve_yyn && mu_sieve_yyn <= YYLAST && mu_sieve_yycheck[mu_sieve_yyn] == YYTERROR)
	    {
	      mu_sieve_yyn = mu_sieve_yytable[mu_sieve_yyn];
	      if (0 < mu_sieve_yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (mu_sieve_yyssp == mu_sieve_yyss)
	YYABORT;


      mu_sieve_yydestruct ("Error: popping",
		  mu_sieve_yystos[mu_sieve_yystate], mu_sieve_yyvsp);
      YYPOPSTACK (1);
      mu_sieve_yystate = *mu_sieve_yyssp;
      YY_STACK_PRINT (mu_sieve_yyss, mu_sieve_yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++mu_sieve_yyvsp = mu_sieve_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", mu_sieve_yystos[mu_sieve_yyn], mu_sieve_yyvsp, mu_sieve_yylsp);

  mu_sieve_yystate = mu_sieve_yyn;
  goto mu_sieve_yynewstate;


/*-------------------------------------.
| mu_sieve_yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
mu_sieve_yyacceptlab:
  mu_sieve_yyresult = 0;
  goto mu_sieve_yyreturn;

/*-----------------------------------.
| mu_sieve_yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
mu_sieve_yyabortlab:
  mu_sieve_yyresult = 1;
  goto mu_sieve_yyreturn;

#if !defined mu_sieve_yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| mu_sieve_yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
mu_sieve_yyexhaustedlab:
  mu_sieve_yyerror (YY_("memory exhausted"));
  mu_sieve_yyresult = 2;
  /* Fall through.  */
#endif

mu_sieve_yyreturn:
  if (mu_sieve_yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      mu_sieve_yytoken = YYTRANSLATE (mu_sieve_yychar);
      mu_sieve_yydestruct ("Cleanup: discarding lookahead",
                  mu_sieve_yytoken, &mu_sieve_yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (mu_sieve_yylen);
  YY_STACK_PRINT (mu_sieve_yyss, mu_sieve_yyssp);
  while (mu_sieve_yyssp != mu_sieve_yyss)
    {
      mu_sieve_yydestruct ("Cleanup: popping",
		  mu_sieve_yystos[*mu_sieve_yyssp], mu_sieve_yyvsp);
      YYPOPSTACK (1);
    }
#ifndef mu_sieve_yyoverflow
  if (mu_sieve_yyss != mu_sieve_yyssa)
    YYSTACK_FREE (mu_sieve_yyss);
#endif
#if YYERROR_VERBOSE
  if (mu_sieve_yymsg != mu_sieve_yymsgbuf)
    YYSTACK_FREE (mu_sieve_yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (mu_sieve_yyresult);
}


/* Line 2055 of yacc.c  */
#line 336 "sieve.y"


int
mu_sieve_yyerror (const char *s)
{
  mu_sv_compile_error (&mu_sieve_locus, "%s", s);
  return 0;
}

int
mu_sieve_machine_init_ex (mu_sieve_machine_t *pmach,
			  void *data, mu_stream_t errstream)
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

  mach->data = data;
  mach->errstream = errstream;
  mu_stream_ref (errstream);
  
  *pmach = mach;
  return 0;
}

int
mu_sieve_machine_init (mu_sieve_machine_t *pmach)
{
  return mu_sieve_machine_init_ex (pmach, NULL, mu_strerr);
}

int
mu_sieve_machine_inherit (mu_sieve_machine_t const parent,
			  mu_sieve_machine_t *pmach)
{
  mu_sieve_machine_t child;
  int rc;
  
  rc = mu_sieve_machine_init_ex (&child, parent->data, parent->errstream);
  if (rc)
    return rc;

  child->logger = parent->logger;
  child->debug_level = parent->debug_level;
  *pmach = child;
  return 0;
}

int
mu_sieve_machine_dup (mu_sieve_machine_t const in, mu_sieve_machine_t *out)
{
  int rc;
  mu_sieve_machine_t mach;
  
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

  mach->pc = 0;
  mach->reg = 0;
  mach->stack = NULL;

  mach->debug_level = in->debug_level;
  
  mach->errstream = in->errstream;
  mu_stream_ref (mach->errstream);
  
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
mu_sieve_set_debug_level (mu_sieve_machine_t mach, int level)
{
  mach->debug_level = level;
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
  /* FIXME: Restore stream state (locus & mode) */
  mu_stream_ioctl (mach->errstream, MU_IOCTL_LOGSTREAM,
                   MU_IOCTL_LOGSTREAM_SET_LOCUS, NULL);
  mu_stream_destroy (&mach->errstream);
  mu_mailer_destroy (&mach->mailer);
  mu_list_foreach (mach->destr_list, _run_destructor, NULL);
  mu_list_destroy (&mach->destr_list);
  mu_list_destroy (&mach->action_list);
  mu_list_destroy (&mach->test_list);
  mu_list_destroy (&mach->comp_list);
  mu_list_destroy (&mach->source_list);
  mu_sieve_slist_destroy (&mach->memory_pool);
  free (mach);
  *pmach = NULL;
}

static int
string_comp (const void *item, const void *value)
{
  return strcmp (item, value);
}

void
mu_sieve_machine_begin (mu_sieve_machine_t mach, const char *file)
{
  mu_sieve_machine = mach;
  mu_sieve_error_count = 0;
  mu_sv_code_instr (NULL);

  mu_list_create (&mach->source_list);
  mu_list_set_comparator (mach->source_list, string_comp);
  
  mu_sv_register_standard_actions (mach);
  mu_sv_register_standard_tests (mach);
  mu_sv_register_standard_comparators (mach);
}

void
mu_sieve_machine_finish (mu_sieve_machine_t mach)
{
  mu_sv_code_instr (NULL);
}

int
mu_sieve_compile (mu_sieve_machine_t mach, const char *name)
{
  int rc = 0;
  
  mu_sieve_machine_begin (mach, name);

  if (mu_sv_lex_begin (name) == 0)
    {
      if (mu_sieve_yyparse () || mu_sieve_error_count)
	rc = MU_ERR_PARSE;
      mu_sv_lex_finish ();
    }
  else
    rc = MU_ERR_FAILURE;
  
  mu_sieve_machine_finish (mach);
  return rc;
}

int
mu_sieve_compile_buffer (mu_sieve_machine_t mach,
			 const char *buf, int bufsize,
			 const char *fname, int line)
{
  int rc;
  
  mu_sieve_machine_begin (mach, fname);

  if (mu_sv_lex_begin_string (buf, bufsize, fname, line) == 0)
    {
      rc = mu_sieve_yyparse ();
      if (mu_sieve_error_count)
	rc = 1;
      mu_sv_lex_finish ();
    }
  else
    rc = 1;
  
  mu_sieve_machine_finish (mach);
  return rc;
}

static void
_branch_fixup (size_t start, size_t end)
{
  size_t prev = mu_sieve_machine->prog[start].pc;
  if (!prev)
    return;
  branch_fixup (prev, end);
  mu_sieve_machine->prog[prev].pc = end - prev - 1;
}

static void
branch_fixup (size_t start, size_t end)
{
  _branch_fixup (start, end);
  mu_sieve_machine->prog[start].pc = end - start - 1;
}




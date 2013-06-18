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

/* All symbols defined below should begin with pick_yy or YY, to avoid
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
#line 1 "pick.y"

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2007, 2009-2012 Free Software Foundation, Inc.

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

#include <mh.h>
#include <regex.h>  
#include <pick.h>
  
static node_t *pick_node_create (node_type type, void *a, void *b);
static void set_cflags (char *str);
 
static regex_t *
regex_dup (regex_t *re)
{
  regex_t *rp = mu_alloc (sizeof (*rp));
  *rp = *re;
  return rp;
}

int pick_yyerror (const char *s);
int pick_yylex (void); 
 
static node_t *parse_tree;
static int nesting_level;
static int reg_flags = REG_EXTENDED|REG_ICASE;

/* Line 371 of yacc.c  */
#line 108 "pick-gram.c"

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
/* Line 387 of yacc.c  */
#line 47 "pick.y"

  char *string;
  node_t *node;
  regex_t regex;


/* Line 387 of yacc.c  */
#line 184 "pick-gram.c"
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

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 212 "pick-gram.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 pick_yytype_uint8;
#else
typedef unsigned char pick_yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 pick_yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char pick_yytype_int8;
#else
typedef short int pick_yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 pick_yytype_uint16;
#else
typedef unsigned short int pick_yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 pick_yytype_int16;
#else
typedef short int pick_yytype_int16;
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
YYID (int pick_yyi)
#else
static int
YYID (pick_yyi)
    int pick_yyi;
#endif
{
  return pick_yyi;
}
#endif

#if ! defined pick_yyoverflow || YYERROR_VERBOSE

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
#endif /* ! defined pick_yyoverflow || YYERROR_VERBOSE */


#if (! defined pick_yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union pick_yyalloc
{
  pick_yytype_int16 pick_yyss_alloc;
  YYSTYPE pick_yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union pick_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (pick_yytype_int16) + sizeof (YYSTYPE)) \
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
	YYSIZE_T pick_yynewbytes;						\
	YYCOPY (&pick_yyptr->Stack_alloc, Stack, pick_yysize);			\
	Stack = &pick_yyptr->Stack_alloc;					\
	pick_yynewbytes = pick_yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	pick_yyptr += pick_yynewbytes / sizeof (*pick_yyptr);				\
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
          YYSIZE_T pick_yyi;                         \
          for (pick_yyi = 0; pick_yyi < (Count); pick_yyi++)   \
            (Dst)[pick_yyi] = (Src)[pick_yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  16
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   45

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  14
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  8
/* YYNRULES -- Number of rules.  */
#define YYNRULES  19
/* YYNRULES -- Number of states.  */
#define YYNSTATES  29

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   268

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? pick_yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const pick_yytype_uint8 pick_yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
       5,     6,     7,     8,     9,    10,    11,    12,    13
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const pick_yytype_uint8 pick_yyprhs[] =
{
       0,     0,     3,     4,     6,     8,    11,    12,    14,    17,
      21,    25,    27,    29,    32,    35,    39,    43,    46,    48
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const pick_yytype_int8 pick_yyrhs[] =
{
      15,     0,    -1,    -1,    16,    -1,    19,    -1,    16,    19,
      -1,    -1,     6,    -1,    17,     5,    -1,    20,    16,    21,
      -1,    17,     3,    18,    -1,    18,    -1,     4,    -1,     9,
       5,    -1,    10,     5,    -1,    19,    12,    19,    -1,    19,
      11,    19,    -1,    13,    19,    -1,     7,    -1,     8,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const pick_yytype_uint8 pick_yyrline[] =
{
       0,    59,    59,    62,    68,    69,    75,    76,    82,    96,
     100,   104,   108,   112,   123,   134,   138,   142,   148,   154
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const pick_yytname[] =
{
  "$end", "error", "$undefined", "T_COMP", "T_DATEFIELD", "T_STRING",
  "T_CFLAGS", "T_LBRACE", "T_RBRACE", "T_BEFORE", "T_AFTER", "T_OR",
  "T_AND", "T_NOT", "$accept", "input", "exprlist", "cflags", "regex",
  "expr", "lbrace", "rbrace", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const pick_yytype_uint16 pick_yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const pick_yytype_uint8 pick_yyr1[] =
{
       0,    14,    15,    15,    16,    16,    17,    17,    18,    19,
      19,    19,    19,    19,    19,    19,    19,    19,    20,    21
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const pick_yytype_uint8 pick_yyr2[] =
{
       0,     2,     0,     1,     1,     2,     0,     1,     2,     3,
       3,     1,     1,     2,     2,     3,     3,     2,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const pick_yytype_uint8 pick_yydefact[] =
{
       6,    12,     7,    18,     0,     0,     6,     0,     6,     0,
      11,     4,     6,    13,    14,    17,     1,     5,     6,     8,
       6,     6,     6,     0,    10,    16,    15,    19,     9
};

/* YYDEFGOTO[NTERM-NUM].  */
static const pick_yytype_int8 pick_yydefgoto[] =
{
      -1,     7,     8,     9,    10,    11,    12,    28
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -7
static const pick_yytype_int8 pick_yypact[] =
{
      13,    -7,    -7,    -7,     3,     7,    29,    18,    21,     6,
      -7,    32,    29,    -7,    -7,    -7,    -7,    32,    23,    -7,
      29,    29,    -3,    19,    -7,    20,    -7,    -7,    -7
};

/* YYPGOTO[NTERM-NUM].  */
static const pick_yytype_int8 pick_yypgoto[] =
{
      -7,    -7,    25,    22,    27,    -6,    -7,    -7
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -4
static const pick_yytype_int8 pick_yytable[] =
{
      15,     1,    17,     2,     3,    27,     4,     5,    13,    18,
       6,    19,    14,    -2,    25,    26,    17,     1,    16,     2,
       3,    -3,     4,     5,    19,     1,     6,     2,     3,     2,
       4,     5,    21,     1,     6,     2,     3,    22,     4,     5,
      23,     0,     6,    20,    21,    24
};

#define pick_yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-7)))

#define pick_yytable_value_is_error(Yytable_value) \
  YYID (0)

static const pick_yytype_int8 pick_yycheck[] =
{
       6,     4,     8,     6,     7,     8,     9,    10,     5,     3,
      13,     5,     5,     0,    20,    21,    22,     4,     0,     6,
       7,     0,     9,    10,     5,     4,    13,     6,     7,     6,
       9,    10,    12,     4,    13,     6,     7,    12,     9,    10,
      18,    -1,    13,    11,    12,    18
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const pick_yytype_uint8 pick_yystos[] =
{
       0,     4,     6,     7,     9,    10,    13,    15,    16,    17,
      18,    19,    20,     5,     5,    19,     0,    19,     3,     5,
      11,    12,    16,    17,    18,    19,    19,     8,    21
};

#define pick_yyerrok		(pick_yyerrstatus = 0)
#define pick_yyclearin	(pick_yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto pick_yyacceptlab
#define YYABORT		goto pick_yyabortlab
#define YYERROR		goto pick_yyerrorlab


/* Like YYERROR except do call pick_yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto pick_yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!pick_yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (pick_yychar == YYEMPTY)                                        \
    {                                                           \
      pick_yychar = (Token);                                         \
      pick_yylval = (Value);                                         \
      YYPOPSTACK (pick_yylen);                                       \
      pick_yystate = *pick_yyssp;                                         \
      goto pick_yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      pick_yyerror (YY_("syntax error: cannot back up")); \
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


/* YYLEX -- calling `pick_yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX pick_yylex (YYLEX_PARAM)
#else
# define YYLEX pick_yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (pick_yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (pick_yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      pick_yy_symbol_print (stderr,						  \
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
pick_yy_symbol_value_print (FILE *pick_yyoutput, int pick_yytype, YYSTYPE const * const pick_yyvaluep)
#else
static void
pick_yy_symbol_value_print (pick_yyoutput, pick_yytype, pick_yyvaluep)
    FILE *pick_yyoutput;
    int pick_yytype;
    YYSTYPE const * const pick_yyvaluep;
#endif
{
  FILE *pick_yyo = pick_yyoutput;
  YYUSE (pick_yyo);
  if (!pick_yyvaluep)
    return;
# ifdef YYPRINT
  if (pick_yytype < YYNTOKENS)
    YYPRINT (pick_yyoutput, pick_yytoknum[pick_yytype], *pick_yyvaluep);
# else
  YYUSE (pick_yyoutput);
# endif
  switch (pick_yytype)
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
pick_yy_symbol_print (FILE *pick_yyoutput, int pick_yytype, YYSTYPE const * const pick_yyvaluep)
#else
static void
pick_yy_symbol_print (pick_yyoutput, pick_yytype, pick_yyvaluep)
    FILE *pick_yyoutput;
    int pick_yytype;
    YYSTYPE const * const pick_yyvaluep;
#endif
{
  if (pick_yytype < YYNTOKENS)
    YYFPRINTF (pick_yyoutput, "token %s (", pick_yytname[pick_yytype]);
  else
    YYFPRINTF (pick_yyoutput, "nterm %s (", pick_yytname[pick_yytype]);

  pick_yy_symbol_value_print (pick_yyoutput, pick_yytype, pick_yyvaluep);
  YYFPRINTF (pick_yyoutput, ")");
}

/*------------------------------------------------------------------.
| pick_yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
pick_yy_stack_print (pick_yytype_int16 *pick_yybottom, pick_yytype_int16 *pick_yytop)
#else
static void
pick_yy_stack_print (pick_yybottom, pick_yytop)
    pick_yytype_int16 *pick_yybottom;
    pick_yytype_int16 *pick_yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; pick_yybottom <= pick_yytop; pick_yybottom++)
    {
      int pick_yybot = *pick_yybottom;
      YYFPRINTF (stderr, " %d", pick_yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (pick_yydebug)							\
    pick_yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
pick_yy_reduce_print (YYSTYPE *pick_yyvsp, int pick_yyrule)
#else
static void
pick_yy_reduce_print (pick_yyvsp, pick_yyrule)
    YYSTYPE *pick_yyvsp;
    int pick_yyrule;
#endif
{
  int pick_yynrhs = pick_yyr2[pick_yyrule];
  int pick_yyi;
  unsigned long int pick_yylno = pick_yyrline[pick_yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     pick_yyrule - 1, pick_yylno);
  /* The symbols being reduced.  */
  for (pick_yyi = 0; pick_yyi < pick_yynrhs; pick_yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", pick_yyi + 1);
      pick_yy_symbol_print (stderr, pick_yyrhs[pick_yyprhs[pick_yyrule] + pick_yyi],
		       &(pick_yyvsp[(pick_yyi + 1) - (pick_yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (pick_yydebug)				\
    pick_yy_reduce_print (pick_yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int pick_yydebug;
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

# ifndef pick_yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define pick_yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
pick_yystrlen (const char *pick_yystr)
#else
static YYSIZE_T
pick_yystrlen (pick_yystr)
    const char *pick_yystr;
#endif
{
  YYSIZE_T pick_yylen;
  for (pick_yylen = 0; pick_yystr[pick_yylen]; pick_yylen++)
    continue;
  return pick_yylen;
}
#  endif
# endif

# ifndef pick_yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define pick_yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
pick_yystpcpy (char *pick_yydest, const char *pick_yysrc)
#else
static char *
pick_yystpcpy (pick_yydest, pick_yysrc)
    char *pick_yydest;
    const char *pick_yysrc;
#endif
{
  char *pick_yyd = pick_yydest;
  const char *pick_yys = pick_yysrc;

  while ((*pick_yyd++ = *pick_yys++) != '\0')
    continue;

  return pick_yyd - 1;
}
#  endif
# endif

# ifndef pick_yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for pick_yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from pick_yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
pick_yytnamerr (char *pick_yyres, const char *pick_yystr)
{
  if (*pick_yystr == '"')
    {
      YYSIZE_T pick_yyn = 0;
      char const *pick_yyp = pick_yystr;

      for (;;)
	switch (*++pick_yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++pick_yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (pick_yyres)
	      pick_yyres[pick_yyn] = *pick_yyp;
	    pick_yyn++;
	    break;

	  case '"':
	    if (pick_yyres)
	      pick_yyres[pick_yyn] = '\0';
	    return pick_yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! pick_yyres)
    return pick_yystrlen (pick_yystr);

  return pick_yystpcpy (pick_yyres, pick_yystr) - pick_yyres;
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
pick_yysyntax_error (YYSIZE_T *pick_yymsg_alloc, char **pick_yymsg,
                pick_yytype_int16 *pick_yyssp, int pick_yytoken)
{
  YYSIZE_T pick_yysize0 = pick_yytnamerr (YY_NULL, pick_yytname[pick_yytoken]);
  YYSIZE_T pick_yysize = pick_yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *pick_yyformat = YY_NULL;
  /* Arguments of pick_yyformat. */
  char const *pick_yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int pick_yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in pick_yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated pick_yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (pick_yytoken != YYEMPTY)
    {
      int pick_yyn = pick_yypact[*pick_yyssp];
      pick_yyarg[pick_yycount++] = pick_yytname[pick_yytoken];
      if (!pick_yypact_value_is_default (pick_yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int pick_yyxbegin = pick_yyn < 0 ? -pick_yyn : 0;
          /* Stay within bounds of both pick_yycheck and pick_yytname.  */
          int pick_yychecklim = YYLAST - pick_yyn + 1;
          int pick_yyxend = pick_yychecklim < YYNTOKENS ? pick_yychecklim : YYNTOKENS;
          int pick_yyx;

          for (pick_yyx = pick_yyxbegin; pick_yyx < pick_yyxend; ++pick_yyx)
            if (pick_yycheck[pick_yyx + pick_yyn] == pick_yyx && pick_yyx != YYTERROR
                && !pick_yytable_value_is_error (pick_yytable[pick_yyx + pick_yyn]))
              {
                if (pick_yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    pick_yycount = 1;
                    pick_yysize = pick_yysize0;
                    break;
                  }
                pick_yyarg[pick_yycount++] = pick_yytname[pick_yyx];
                {
                  YYSIZE_T pick_yysize1 = pick_yysize + pick_yytnamerr (YY_NULL, pick_yytname[pick_yyx]);
                  if (! (pick_yysize <= pick_yysize1
                         && pick_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  pick_yysize = pick_yysize1;
                }
              }
        }
    }

  switch (pick_yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        pick_yyformat = S;                       \
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
    YYSIZE_T pick_yysize1 = pick_yysize + pick_yystrlen (pick_yyformat);
    if (! (pick_yysize <= pick_yysize1 && pick_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    pick_yysize = pick_yysize1;
  }

  if (*pick_yymsg_alloc < pick_yysize)
    {
      *pick_yymsg_alloc = 2 * pick_yysize;
      if (! (pick_yysize <= *pick_yymsg_alloc
             && *pick_yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *pick_yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *pick_yyp = *pick_yymsg;
    int pick_yyi = 0;
    while ((*pick_yyp = *pick_yyformat) != '\0')
      if (*pick_yyp == '%' && pick_yyformat[1] == 's' && pick_yyi < pick_yycount)
        {
          pick_yyp += pick_yytnamerr (pick_yyp, pick_yyarg[pick_yyi++]);
          pick_yyformat += 2;
        }
      else
        {
          pick_yyp++;
          pick_yyformat++;
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
pick_yydestruct (const char *pick_yymsg, int pick_yytype, YYSTYPE *pick_yyvaluep)
#else
static void
pick_yydestruct (pick_yymsg, pick_yytype, pick_yyvaluep)
    const char *pick_yymsg;
    int pick_yytype;
    YYSTYPE *pick_yyvaluep;
#endif
{
  YYUSE (pick_yyvaluep);

  if (!pick_yymsg)
    pick_yymsg = "Deleting";
  YY_SYMBOL_PRINT (pick_yymsg, pick_yytype, pick_yyvaluep, pick_yylocationp);

  switch (pick_yytype)
    {

      default:
        break;
    }
}




/* The lookahead symbol.  */
int pick_yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE pick_yylval YY_INITIAL_VALUE(pick_yyval_default);

/* Number of syntax errors so far.  */
int pick_yynerrs;


/*----------.
| pick_yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
pick_yyparse (void *YYPARSE_PARAM)
#else
int
pick_yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
pick_yyparse (void)
#else
int
pick_yyparse ()

#endif
#endif
{
    int pick_yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int pick_yyerrstatus;

    /* The stacks and their tools:
       `pick_yyss': related to states.
       `pick_yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow pick_yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    pick_yytype_int16 pick_yyssa[YYINITDEPTH];
    pick_yytype_int16 *pick_yyss;
    pick_yytype_int16 *pick_yyssp;

    /* The semantic value stack.  */
    YYSTYPE pick_yyvsa[YYINITDEPTH];
    YYSTYPE *pick_yyvs;
    YYSTYPE *pick_yyvsp;

    YYSIZE_T pick_yystacksize;

  int pick_yyn;
  int pick_yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int pick_yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE pick_yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char pick_yymsgbuf[128];
  char *pick_yymsg = pick_yymsgbuf;
  YYSIZE_T pick_yymsg_alloc = sizeof pick_yymsgbuf;
#endif

#define YYPOPSTACK(N)   (pick_yyvsp -= (N), pick_yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int pick_yylen = 0;

  pick_yyssp = pick_yyss = pick_yyssa;
  pick_yyvsp = pick_yyvs = pick_yyvsa;
  pick_yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  pick_yystate = 0;
  pick_yyerrstatus = 0;
  pick_yynerrs = 0;
  pick_yychar = YYEMPTY; /* Cause a token to be read.  */
  goto pick_yysetstate;

/*------------------------------------------------------------.
| pick_yynewstate -- Push a new state, which is found in pick_yystate.  |
`------------------------------------------------------------*/
 pick_yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  pick_yyssp++;

 pick_yysetstate:
  *pick_yyssp = pick_yystate;

  if (pick_yyss + pick_yystacksize - 1 <= pick_yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T pick_yysize = pick_yyssp - pick_yyss + 1;

#ifdef pick_yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *pick_yyvs1 = pick_yyvs;
	pick_yytype_int16 *pick_yyss1 = pick_yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if pick_yyoverflow is a macro.  */
	pick_yyoverflow (YY_("memory exhausted"),
		    &pick_yyss1, pick_yysize * sizeof (*pick_yyssp),
		    &pick_yyvs1, pick_yysize * sizeof (*pick_yyvsp),
		    &pick_yystacksize);

	pick_yyss = pick_yyss1;
	pick_yyvs = pick_yyvs1;
      }
#else /* no pick_yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto pick_yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= pick_yystacksize)
	goto pick_yyexhaustedlab;
      pick_yystacksize *= 2;
      if (YYMAXDEPTH < pick_yystacksize)
	pick_yystacksize = YYMAXDEPTH;

      {
	pick_yytype_int16 *pick_yyss1 = pick_yyss;
	union pick_yyalloc *pick_yyptr =
	  (union pick_yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (pick_yystacksize));
	if (! pick_yyptr)
	  goto pick_yyexhaustedlab;
	YYSTACK_RELOCATE (pick_yyss_alloc, pick_yyss);
	YYSTACK_RELOCATE (pick_yyvs_alloc, pick_yyvs);
#  undef YYSTACK_RELOCATE
	if (pick_yyss1 != pick_yyssa)
	  YYSTACK_FREE (pick_yyss1);
      }
# endif
#endif /* no pick_yyoverflow */

      pick_yyssp = pick_yyss + pick_yysize - 1;
      pick_yyvsp = pick_yyvs + pick_yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) pick_yystacksize));

      if (pick_yyss + pick_yystacksize - 1 <= pick_yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", pick_yystate));

  if (pick_yystate == YYFINAL)
    YYACCEPT;

  goto pick_yybackup;

/*-----------.
| pick_yybackup.  |
`-----------*/
pick_yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  pick_yyn = pick_yypact[pick_yystate];
  if (pick_yypact_value_is_default (pick_yyn))
    goto pick_yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (pick_yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      pick_yychar = YYLEX;
    }

  if (pick_yychar <= YYEOF)
    {
      pick_yychar = pick_yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      pick_yytoken = YYTRANSLATE (pick_yychar);
      YY_SYMBOL_PRINT ("Next token is", pick_yytoken, &pick_yylval, &pick_yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  pick_yyn += pick_yytoken;
  if (pick_yyn < 0 || YYLAST < pick_yyn || pick_yycheck[pick_yyn] != pick_yytoken)
    goto pick_yydefault;
  pick_yyn = pick_yytable[pick_yyn];
  if (pick_yyn <= 0)
    {
      if (pick_yytable_value_is_error (pick_yyn))
        goto pick_yyerrlab;
      pick_yyn = -pick_yyn;
      goto pick_yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (pick_yyerrstatus)
    pick_yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", pick_yytoken, &pick_yylval, &pick_yylloc);

  /* Discard the shifted token.  */
  pick_yychar = YYEMPTY;

  pick_yystate = pick_yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++pick_yyvsp = pick_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto pick_yynewstate;


/*-----------------------------------------------------------.
| pick_yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
pick_yydefault:
  pick_yyn = pick_yydefact[pick_yystate];
  if (pick_yyn == 0)
    goto pick_yyerrlab;
  goto pick_yyreduce;


/*-----------------------------.
| pick_yyreduce -- Do a reduction.  |
`-----------------------------*/
pick_yyreduce:
  /* pick_yyn is the number of a rule to reduce with.  */
  pick_yylen = pick_yyr2[pick_yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  pick_yyval = pick_yyvsp[1-pick_yylen];


  YY_REDUCE_PRINT (pick_yyn);
  switch (pick_yyn)
    {
        case 2:
/* Line 1792 of yacc.c  */
#line 59 "pick.y"
    {
	     parse_tree = NULL;
	   }
    break;

  case 3:
/* Line 1792 of yacc.c  */
#line 63 "pick.y"
    {
	     parse_tree = (pick_yyvsp[(1) - (1)].node);
	   }
    break;

  case 5:
/* Line 1792 of yacc.c  */
#line 70 "pick.y"
    {
	     (pick_yyval.node) = pick_node_create (node_and, (pick_yyvsp[(1) - (2)].node), (pick_yyvsp[(2) - (2)].node));
	   }
    break;

  case 7:
/* Line 1792 of yacc.c  */
#line 77 "pick.y"
    {
	     set_cflags ((pick_yyvsp[(1) - (1)].string));
	   }
    break;

  case 8:
/* Line 1792 of yacc.c  */
#line 83 "pick.y"
    {
	     int rc = regcomp (&(pick_yyval.regex), (pick_yyvsp[(2) - (2)].string), reg_flags|REG_NOSUB);
	     if (rc)
	       {
		 char errbuf[512];
		 regerror (rc, &(pick_yyval.regex), errbuf, sizeof (errbuf));
		 mu_error ("error compiling regex \"%s\": %s",
			   (pick_yyvsp[(2) - (2)].string), errbuf);
		 YYERROR;
	       }
	   }
    break;

  case 9:
/* Line 1792 of yacc.c  */
#line 97 "pick.y"
    {
	     (pick_yyval.node) = (pick_yyvsp[(2) - (3)].node);
	   }
    break;

  case 10:
/* Line 1792 of yacc.c  */
#line 101 "pick.y"
    {
	     (pick_yyval.node) = pick_node_create (node_regex, (pick_yyvsp[(2) - (3)].string), regex_dup (&(pick_yyvsp[(3) - (3)].regex)));
	   }
    break;

  case 11:
/* Line 1792 of yacc.c  */
#line 105 "pick.y"
    {
	     (pick_yyval.node) = pick_node_create (node_regex, NULL, regex_dup (&(pick_yyvsp[(1) - (1)].regex)));
	   }
    break;

  case 12:
/* Line 1792 of yacc.c  */
#line 109 "pick.y"
    {
	     (pick_yyval.node) = pick_node_create (node_datefield, (pick_yyvsp[(1) - (1)].string), NULL);
	   }
    break;

  case 13:
/* Line 1792 of yacc.c  */
#line 113 "pick.y"
    {
	     time_t t;
	     if (mu_parse_date ((pick_yyvsp[(2) - (2)].string), &t, NULL))
	       {
		 mu_error (_("bad date format: %s"), (pick_yyvsp[(2) - (2)].string));
		 exit (1);
	       }
	     (pick_yyval.node) = pick_node_create (node_before, NULL, NULL);
	     (pick_yyval.node)->v.time = t;
	   }
    break;

  case 14:
/* Line 1792 of yacc.c  */
#line 124 "pick.y"
    {
	     time_t t;
	     if (mu_parse_date ((pick_yyvsp[(2) - (2)].string), &t, NULL))
	       {
		 mu_error (_("bad date format: %s"), (pick_yyvsp[(2) - (2)].string));
		 exit (1);
	       }
	     (pick_yyval.node) = pick_node_create (node_after, NULL, NULL);
	     (pick_yyval.node)->v.time = t;
	   }
    break;

  case 15:
/* Line 1792 of yacc.c  */
#line 135 "pick.y"
    {
	     (pick_yyval.node) = pick_node_create (node_and, (pick_yyvsp[(1) - (3)].node), (pick_yyvsp[(3) - (3)].node));
	   }
    break;

  case 16:
/* Line 1792 of yacc.c  */
#line 139 "pick.y"
    {
	     (pick_yyval.node) = pick_node_create (node_or, (pick_yyvsp[(1) - (3)].node), (pick_yyvsp[(3) - (3)].node));
	   }
    break;

  case 17:
/* Line 1792 of yacc.c  */
#line 143 "pick.y"
    {
	     (pick_yyval.node) = pick_node_create (node_not, (pick_yyvsp[(2) - (2)].node), NULL);
	   }
    break;

  case 18:
/* Line 1792 of yacc.c  */
#line 149 "pick.y"
    {
	     nesting_level++;
	   }
    break;

  case 19:
/* Line 1792 of yacc.c  */
#line 155 "pick.y"
    {
	     nesting_level--;
	   }
    break;


/* Line 1792 of yacc.c  */
#line 1563 "pick-gram.c"
      default: break;
    }
  /* User semantic actions sometimes alter pick_yychar, and that requires
     that pick_yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of pick_yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering pick_yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", pick_yyr1[pick_yyn], &pick_yyval, &pick_yyloc);

  YYPOPSTACK (pick_yylen);
  pick_yylen = 0;
  YY_STACK_PRINT (pick_yyss, pick_yyssp);

  *++pick_yyvsp = pick_yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  pick_yyn = pick_yyr1[pick_yyn];

  pick_yystate = pick_yypgoto[pick_yyn - YYNTOKENS] + *pick_yyssp;
  if (0 <= pick_yystate && pick_yystate <= YYLAST && pick_yycheck[pick_yystate] == *pick_yyssp)
    pick_yystate = pick_yytable[pick_yystate];
  else
    pick_yystate = pick_yydefgoto[pick_yyn - YYNTOKENS];

  goto pick_yynewstate;


/*------------------------------------.
| pick_yyerrlab -- here on detecting error |
`------------------------------------*/
pick_yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  pick_yytoken = pick_yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (pick_yychar);

  /* If not already recovering from an error, report this error.  */
  if (!pick_yyerrstatus)
    {
      ++pick_yynerrs;
#if ! YYERROR_VERBOSE
      pick_yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR pick_yysyntax_error (&pick_yymsg_alloc, &pick_yymsg, \
                                        pick_yyssp, pick_yytoken)
      {
        char const *pick_yymsgp = YY_("syntax error");
        int pick_yysyntax_error_status;
        pick_yysyntax_error_status = YYSYNTAX_ERROR;
        if (pick_yysyntax_error_status == 0)
          pick_yymsgp = pick_yymsg;
        else if (pick_yysyntax_error_status == 1)
          {
            if (pick_yymsg != pick_yymsgbuf)
              YYSTACK_FREE (pick_yymsg);
            pick_yymsg = (char *) YYSTACK_ALLOC (pick_yymsg_alloc);
            if (!pick_yymsg)
              {
                pick_yymsg = pick_yymsgbuf;
                pick_yymsg_alloc = sizeof pick_yymsgbuf;
                pick_yysyntax_error_status = 2;
              }
            else
              {
                pick_yysyntax_error_status = YYSYNTAX_ERROR;
                pick_yymsgp = pick_yymsg;
              }
          }
        pick_yyerror (pick_yymsgp);
        if (pick_yysyntax_error_status == 2)
          goto pick_yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (pick_yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (pick_yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (pick_yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  pick_yydestruct ("Error: discarding",
		      pick_yytoken, &pick_yylval);
	  pick_yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto pick_yyerrlab1;


/*---------------------------------------------------.
| pick_yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
pick_yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label pick_yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto pick_yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (pick_yylen);
  pick_yylen = 0;
  YY_STACK_PRINT (pick_yyss, pick_yyssp);
  pick_yystate = *pick_yyssp;
  goto pick_yyerrlab1;


/*-------------------------------------------------------------.
| pick_yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
pick_yyerrlab1:
  pick_yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      pick_yyn = pick_yypact[pick_yystate];
      if (!pick_yypact_value_is_default (pick_yyn))
	{
	  pick_yyn += YYTERROR;
	  if (0 <= pick_yyn && pick_yyn <= YYLAST && pick_yycheck[pick_yyn] == YYTERROR)
	    {
	      pick_yyn = pick_yytable[pick_yyn];
	      if (0 < pick_yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (pick_yyssp == pick_yyss)
	YYABORT;


      pick_yydestruct ("Error: popping",
		  pick_yystos[pick_yystate], pick_yyvsp);
      YYPOPSTACK (1);
      pick_yystate = *pick_yyssp;
      YY_STACK_PRINT (pick_yyss, pick_yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++pick_yyvsp = pick_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", pick_yystos[pick_yyn], pick_yyvsp, pick_yylsp);

  pick_yystate = pick_yyn;
  goto pick_yynewstate;


/*-------------------------------------.
| pick_yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
pick_yyacceptlab:
  pick_yyresult = 0;
  goto pick_yyreturn;

/*-----------------------------------.
| pick_yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
pick_yyabortlab:
  pick_yyresult = 1;
  goto pick_yyreturn;

#if !defined pick_yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| pick_yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
pick_yyexhaustedlab:
  pick_yyerror (YY_("memory exhausted"));
  pick_yyresult = 2;
  /* Fall through.  */
#endif

pick_yyreturn:
  if (pick_yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      pick_yytoken = YYTRANSLATE (pick_yychar);
      pick_yydestruct ("Cleanup: discarding lookahead",
                  pick_yytoken, &pick_yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (pick_yylen);
  YY_STACK_PRINT (pick_yyss, pick_yyssp);
  while (pick_yyssp != pick_yyss)
    {
      pick_yydestruct ("Cleanup: popping",
		  pick_yystos[*pick_yyssp], pick_yyvsp);
      YYPOPSTACK (1);
    }
#ifndef pick_yyoverflow
  if (pick_yyss != pick_yyssa)
    YYSTACK_FREE (pick_yyss);
#endif
#if YYERROR_VERBOSE
  if (pick_yymsg != pick_yymsgbuf)
    YYSTACK_FREE (pick_yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (pick_yyresult);
}


/* Line 2055 of yacc.c  */
#line 160 "pick.y"


/* Lexical analizer */

struct token
{
  int tok;
  char *val;
};

static mu_iterator_t iterator;

int
pick_yylex ()
{
  struct token *tok;
  
  if (mu_iterator_is_done (iterator))
    return 0;
  mu_iterator_current (iterator, (void **)&tok);
  mu_iterator_next (iterator);
  pick_yylval.string = tok->val;
  return tok->tok;
}

static char *
tokname (int tok)
{
  switch (tok)
    {
    case T_DATEFIELD:
      return "--datefield";
      
    case T_BEFORE:
      return "--before";
      
    case T_AFTER:
      return "--after";
      
    case T_LBRACE:
      return "--lbrace";
      
    case T_RBRACE:
      return "--rbrace";
      
    case T_OR:
      return "--or";
      
    case T_AND:
      return "--and";
      
    case T_NOT:
      return "--not";
    }
  return NULL;
}

int
pick_yyerror (const char *s)
{
  int tok = pick_yylex ();
  const char *str;
  
  if (!tok)
    str = _("end of input");
  else if (pick_yylval.string)
    str = pick_yylval.string;
  else
    str = tokname (tok);

  if (nesting_level)
    mu_error (_("%s near %s (missing closing brace?)"), s, str);
  else
    mu_error (_("%s near %s"), s, str);
  return 0;
}
  
void
pick_add_token (mu_list_t *list, int tok, char *val)
{
  struct token *tp;
  int rc;
  
  if (!*list && (rc = mu_list_create (list)))
    {
      mu_error(_("cannot create list: %s"), mu_strerror (rc));
      exit (1);
    }
  tp = mu_alloc (sizeof (*tp));
  tp->tok = tok;
  tp->val = val;
  mu_list_append (*list, tp);
}

/* Main entry point */
int
pick_parse (mu_list_t toklist)
{
  int rc;
  
  if (!toklist)
    {
      parse_tree = NULL;
      return 0;
    }

  if (mu_list_get_iterator (toklist, &iterator))
    return -1;
  mu_iterator_first (iterator);
  rc = pick_yyparse ();
  mu_iterator_destroy (&iterator);
  return rc;
}


/* Parse tree functions */

node_t *
pick_node_create (node_type type, void *a, void *b)
{
  node_t *node;

  node = mu_alloc (sizeof (*node));
  node->type = type;
  node->v.gen.a = a;
  node->v.gen.b = b;
  return node;
}

struct eval_env
{
  mu_message_t msg;
  char *datefield;
};

static int
match_header (mu_message_t msg, char *comp, regex_t *regex)
{
  int rc;
  size_t i, count;
  mu_header_t hdr = NULL;
  const char *buf;
  
  rc = mu_message_get_header (msg, &hdr);
  if (rc)
    {
      mu_error (_("cannot get header: %s"), mu_strerror (rc));
      return 0;
    }
  mu_header_get_field_count (hdr, &count);
  for (i = 1; i <= count; i++)
    {
      mu_header_sget_field_name (hdr, i, &buf);
      if (mu_c_strcasecmp (buf, comp) == 0)
	{
	  mu_header_sget_field_value (hdr, i, &buf);
	  if (regexec (regex, buf, 0, NULL, 0) == 0)
	    return 1;
	}
    }
  return 0;
}

static int
match_message (mu_message_t msg, regex_t *regex)
{
  mu_stream_t str = NULL;
  char buf[128];
  size_t n;
  
  mu_message_get_streamref (msg, &str);
  while (mu_stream_readline (str, buf, sizeof buf, &n) == 0
	 && n > 0)
    {
      buf[n] = 0;
      if (regexec (regex, buf, 0, NULL, 0) == 0)
	return 1;
    }
  mu_stream_destroy (&str);
  return 0;
}

static int
get_date_field (struct eval_env *env, time_t *t)
{
  mu_header_t hdr;
  char buf[128];
  
  if (mu_message_get_header (env->msg, &hdr))
    return 1;
  if (mu_header_get_value (hdr, env->datefield, buf, sizeof buf, NULL))
    return 1;
  return mu_parse_date (buf, t, NULL);
}

static int
pick_eval_node (node_t *node, struct eval_env *env)
{
  time_t t;
  
  switch (node->type)
    {
    case node_and:
      if (!pick_eval_node (node->v.op.larg, env))
	return 0;
      return pick_eval_node (node->v.op.rarg, env);
	
    case node_or:
      if (pick_eval_node (node->v.op.larg, env))
	return 1;
      return pick_eval_node (node->v.op.rarg, env);

    case node_not:
      return !pick_eval_node (node->v.op.larg, env);
      
    case node_regex:
      if (node->v.re.comp)
	return match_header (env->msg, node->v.re.comp, node->v.re.regex);
      else
	return match_message (env->msg, node->v.re.regex);
      
    case node_datefield:
      env->datefield = node->v.df.datefield;
      return 1;

    case node_before:
      if (get_date_field (env, &t))
	break;
      return t < node->v.time;
      
    case node_after:
      if (get_date_field (env, &t))
	break;
      return t > node->v.time;
    }

  return 0;
}

int
pick_eval (mu_message_t msg)
{
  struct eval_env env;
  
  if (!parse_tree)
    return 1;
  env.msg = msg;
  env.datefield = "date";
  return pick_eval_node (parse_tree, &env);
}

void
set_cflags (char *str)
{
  reg_flags = 0;
  for (; *str; str++)
    {
      switch (*str)
	{
	case 'b':
	case 'B':
	  reg_flags &= ~REG_EXTENDED;
	  break;

	case 'e':
	case 'E':
	  reg_flags |= REG_EXTENDED;
	  break;

	case 'c':
	case 'C':
	  reg_flags &= ~REG_ICASE;
	  break;
	  
	case 'i':
	case 'I':
	  reg_flags |= REG_ICASE;
	  break;

	default:
	  mu_error (_("Invalid regular expression flag: %c"), *str);
	  exit (1);
	}
    }
}

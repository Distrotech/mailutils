/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* All symbols defined below should begin with mu_cfg_yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum mu_cfg_yytokentype {
     MU_CFG_EOL_TOKEN = 258,
     MU_CFG_START_TOKEN = 259,
     MU_CFG_END_TOKEN = 260,
     MU_CFG_STRING_TOKEN = 261
   };
#endif
/* Tokens.  */
#define MU_CFG_EOL_TOKEN 258
#define MU_CFG_START_TOKEN 259
#define MU_CFG_END_TOKEN 260
#define MU_CFG_STRING_TOKEN 261




/* Copy the first part of user declarations.  */
#line 1 "cfg_parser.y"

/* cfg_parser.y -- general-purpose configuration file parser 
   Copyright (C) 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
  
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>	
#include <string.h>
#include <netdb.h>
#include "intprops.h"
#include <mailutils/nls.h>
#include <mailutils/cfg.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
  
static mu_cfg_node_t *parse_tree;
mu_cfg_locus_t mu_cfg_locus;
int mu_cfg_tie_in;
 
static int _mu_cfg_errcnt;
static mu_cfg_lexer_t _mu_cfg_lexer;
mu_cfg_perror_t mu_cfg_perror; 
static void *_mu_cfg_lexer_data;
mu_cfg_alloc_t _mu_cfg_alloc;
mu_cfg_free_t _mu_cfg_free;

static int
mu_cfg_yyerror (char *s)
{
  mu_cfg_perror (_mu_cfg_lexer_data, &mu_cfg_locus, "%s", s);
  return 0;
}
 
static int
mu_cfg_yylex ()
{
  return _mu_cfg_lexer (_mu_cfg_lexer_data);
}

static mu_cfg_node_t *
mu_cfg_alloc_node (enum mu_cfg_node_type type, mu_cfg_locus_t *loc,
		   char *tag, char *label, mu_cfg_node_t *node)
{
  char *p;
  mu_cfg_node_t *np;
  size_t size = sizeof *np + strlen (tag) + 1
                + (label ? (strlen (label) + 1) : 0);
  np = _mu_cfg_alloc (size);
  if (!np)
    {
      mu_cfg_perror (_mu_cfg_lexer_data, &mu_cfg_locus,
		     _("Not enough memory"));
      abort();
    }
  np->type = type;
  np->locus = *loc;
  p = (char*) (np + 1);
  np->tag_name = p;
  strcpy (p, tag);
  p += strlen (p) + 1;
  if (label)
    {
      np->tag_label = p;
      strcpy (p, label);
    }
  else
    np->tag_label = label;
  np->node = node;
  np->next = NULL;
  return np;
}

static void
_mu_cfg_default_perror (void *ptr, const mu_cfg_locus_t *loc,
			const char *fmt, ...)
{
  va_list ap;
  
  fprintf (stderr, "%s:", loc->file ? loc->file : _("unknown file"));
  if (loc->line > 0)
    fprintf (stderr, "%lu", (unsigned long) loc->line);
  else
    fprintf (stderr, "%s", _("unknown line"));
  fprintf (stderr, ": ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, "\n");
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 115 "cfg_parser.y"
{
  mu_cfg_node_t node;
  mu_cfg_node_t *pnode;
  struct
  {
    mu_cfg_node_t *head, *tail;
  } nodelist;
  char *string;
}
/* Line 187 of yacc.c.  */
#line 224 "cfg_parser.c"
	YYSTYPE;
# define mu_cfg_yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 237 "cfg_parser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 mu_cfg_yytype_uint8;
#else
typedef unsigned char mu_cfg_yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 mu_cfg_yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char mu_cfg_yytype_int8;
#else
typedef short int mu_cfg_yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 mu_cfg_yytype_uint16;
#else
typedef unsigned short int mu_cfg_yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 mu_cfg_yytype_int16;
#else
typedef short int mu_cfg_yytype_int16;
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
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined mu_cfg_yyoverflow || YYERROR_VERBOSE

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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined mu_cfg_yyoverflow || YYERROR_VERBOSE */


#if (! defined mu_cfg_yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union mu_cfg_yyalloc
{
  mu_cfg_yytype_int16 mu_cfg_yyss;
  YYSTYPE mu_cfg_yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union mu_cfg_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (mu_cfg_yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T mu_cfg_yyi;				\
	  for (mu_cfg_yyi = 0; mu_cfg_yyi < (Count); mu_cfg_yyi++)	\
	    (To)[mu_cfg_yyi] = (From)[mu_cfg_yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T mu_cfg_yynewbytes;						\
	YYCOPY (&mu_cfg_yyptr->Stack, Stack, mu_cfg_yysize);				\
	Stack = &mu_cfg_yyptr->Stack;						\
	mu_cfg_yynewbytes = mu_cfg_yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	mu_cfg_yyptr += mu_cfg_yynewbytes / sizeof (*mu_cfg_yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  10
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   17

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  7
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  8
/* YYNRULES -- Number of rules.  */
#define YYNRULES  12
/* YYNRULES -- Number of states.  */
#define YYNSTATES  19

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   261

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? mu_cfg_yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const mu_cfg_yytype_uint8 mu_cfg_yytranslate[] =
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
       5,     6
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const mu_cfg_yytype_uint8 mu_cfg_yyprhs[] =
{
       0,     0,     3,     5,     7,    10,    11,    13,    15,    18,
      24,    25,    26
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const mu_cfg_yytype_int8 mu_cfg_yyrhs[] =
{
       8,     0,    -1,     9,    -1,    12,    -1,     9,    12,    -1,
      -1,    11,    -1,     3,    -1,    11,     3,    -1,     4,    10,
       9,     5,     3,    -1,    -1,    -1,     6,    13,     6,    14,
       3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const mu_cfg_yytype_uint8 mu_cfg_yyrline[] =
{
       0,   127,   127,   133,   137,   145,   146,   149,   150,   153,
     170,   171,   170
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const mu_cfg_yytname[] =
{
  "$end", "error", "$undefined", "MU_CFG_EOL_TOKEN", "MU_CFG_START_TOKEN",
  "MU_CFG_END_TOKEN", "MU_CFG_STRING_TOKEN", "$accept", "input", "taglist",
  "opt_eol", "eol", "tag", "@1", "@2", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const mu_cfg_yytype_uint16 mu_cfg_yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const mu_cfg_yytype_uint8 mu_cfg_yyr1[] =
{
       0,     7,     8,     9,     9,    10,    10,    11,    11,    12,
      13,    14,    12
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const mu_cfg_yytype_uint8 mu_cfg_yyr2[] =
{
       0,     2,     1,     1,     2,     0,     1,     1,     2,     5,
       0,     0,     5
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const mu_cfg_yytype_uint8 mu_cfg_yydefact[] =
{
       0,     5,    10,     0,     2,     3,     7,     0,     6,     0,
       1,     4,     0,     8,    11,     0,     0,     9,    12
};

/* YYDEFGOTO[NTERM-NUM].  */
static const mu_cfg_yytype_int8 mu_cfg_yydefgoto[] =
{
      -1,     3,     4,     7,     8,     5,     9,    16
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -5
static const mu_cfg_yytype_int8 mu_cfg_yypact[] =
{
       0,     2,    -5,     7,     0,    -5,    -5,     0,     6,     4,
      -5,    -5,    -3,    -5,    -5,     8,     9,    -5,    -5
};

/* YYPGOTO[NTERM-NUM].  */
static const mu_cfg_yytype_int8 mu_cfg_yypgoto[] =
{
      -5,    -5,    10,    -5,    -5,    -4,    -5,    -5
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const mu_cfg_yytype_uint8 mu_cfg_yytable[] =
{
      11,     1,    15,     2,     1,     6,     2,    10,    11,    13,
      14,    17,    18,     0,     0,     0,     0,    12
};

static const mu_cfg_yytype_int8 mu_cfg_yycheck[] =
{
       4,     4,     5,     6,     4,     3,     6,     0,    12,     3,
       6,     3,     3,    -1,    -1,    -1,    -1,     7
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const mu_cfg_yytype_uint8 mu_cfg_yystos[] =
{
       0,     4,     6,     8,     9,    12,     3,    10,    11,    13,
       0,    12,     9,     3,     6,     5,    14,     3,     3
};

#define mu_cfg_yyerrok		(mu_cfg_yyerrstatus = 0)
#define mu_cfg_yyclearin	(mu_cfg_yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto mu_cfg_yyacceptlab
#define YYABORT		goto mu_cfg_yyabortlab
#define YYERROR		goto mu_cfg_yyerrorlab


/* Like YYERROR except do call mu_cfg_yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto mu_cfg_yyerrlab

#define YYRECOVERING()  (!!mu_cfg_yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (mu_cfg_yychar == YYEMPTY && mu_cfg_yylen == 1)				\
    {								\
      mu_cfg_yychar = (Token);						\
      mu_cfg_yylval = (Value);						\
      mu_cfg_yytoken = YYTRANSLATE (mu_cfg_yychar);				\
      YYPOPSTACK (1);						\
      goto mu_cfg_yybackup;						\
    }								\
  else								\
    {								\
      mu_cfg_yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `mu_cfg_yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX mu_cfg_yylex (YYLEX_PARAM)
#else
# define YYLEX mu_cfg_yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (mu_cfg_yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (mu_cfg_yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      mu_cfg_yy_symbol_print (stderr,						  \
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
mu_cfg_yy_symbol_value_print (FILE *mu_cfg_yyoutput, int mu_cfg_yytype, YYSTYPE const * const mu_cfg_yyvaluep)
#else
static void
mu_cfg_yy_symbol_value_print (mu_cfg_yyoutput, mu_cfg_yytype, mu_cfg_yyvaluep)
    FILE *mu_cfg_yyoutput;
    int mu_cfg_yytype;
    YYSTYPE const * const mu_cfg_yyvaluep;
#endif
{
  if (!mu_cfg_yyvaluep)
    return;
# ifdef YYPRINT
  if (mu_cfg_yytype < YYNTOKENS)
    YYPRINT (mu_cfg_yyoutput, mu_cfg_yytoknum[mu_cfg_yytype], *mu_cfg_yyvaluep);
# else
  YYUSE (mu_cfg_yyoutput);
# endif
  switch (mu_cfg_yytype)
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
mu_cfg_yy_symbol_print (FILE *mu_cfg_yyoutput, int mu_cfg_yytype, YYSTYPE const * const mu_cfg_yyvaluep)
#else
static void
mu_cfg_yy_symbol_print (mu_cfg_yyoutput, mu_cfg_yytype, mu_cfg_yyvaluep)
    FILE *mu_cfg_yyoutput;
    int mu_cfg_yytype;
    YYSTYPE const * const mu_cfg_yyvaluep;
#endif
{
  if (mu_cfg_yytype < YYNTOKENS)
    YYFPRINTF (mu_cfg_yyoutput, "token %s (", mu_cfg_yytname[mu_cfg_yytype]);
  else
    YYFPRINTF (mu_cfg_yyoutput, "nterm %s (", mu_cfg_yytname[mu_cfg_yytype]);

  mu_cfg_yy_symbol_value_print (mu_cfg_yyoutput, mu_cfg_yytype, mu_cfg_yyvaluep);
  YYFPRINTF (mu_cfg_yyoutput, ")");
}

/*------------------------------------------------------------------.
| mu_cfg_yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
mu_cfg_yy_stack_print (mu_cfg_yytype_int16 *bottom, mu_cfg_yytype_int16 *top)
#else
static void
mu_cfg_yy_stack_print (bottom, top)
    mu_cfg_yytype_int16 *bottom;
    mu_cfg_yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (mu_cfg_yydebug)							\
    mu_cfg_yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
mu_cfg_yy_reduce_print (YYSTYPE *mu_cfg_yyvsp, int mu_cfg_yyrule)
#else
static void
mu_cfg_yy_reduce_print (mu_cfg_yyvsp, mu_cfg_yyrule)
    YYSTYPE *mu_cfg_yyvsp;
    int mu_cfg_yyrule;
#endif
{
  int mu_cfg_yynrhs = mu_cfg_yyr2[mu_cfg_yyrule];
  int mu_cfg_yyi;
  unsigned long int mu_cfg_yylno = mu_cfg_yyrline[mu_cfg_yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     mu_cfg_yyrule - 1, mu_cfg_yylno);
  /* The symbols being reduced.  */
  for (mu_cfg_yyi = 0; mu_cfg_yyi < mu_cfg_yynrhs; mu_cfg_yyi++)
    {
      fprintf (stderr, "   $%d = ", mu_cfg_yyi + 1);
      mu_cfg_yy_symbol_print (stderr, mu_cfg_yyrhs[mu_cfg_yyprhs[mu_cfg_yyrule] + mu_cfg_yyi],
		       &(mu_cfg_yyvsp[(mu_cfg_yyi + 1) - (mu_cfg_yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (mu_cfg_yydebug)				\
    mu_cfg_yy_reduce_print (mu_cfg_yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int mu_cfg_yydebug;
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

# ifndef mu_cfg_yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define mu_cfg_yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
mu_cfg_yystrlen (const char *mu_cfg_yystr)
#else
static YYSIZE_T
mu_cfg_yystrlen (mu_cfg_yystr)
    const char *mu_cfg_yystr;
#endif
{
  YYSIZE_T mu_cfg_yylen;
  for (mu_cfg_yylen = 0; mu_cfg_yystr[mu_cfg_yylen]; mu_cfg_yylen++)
    continue;
  return mu_cfg_yylen;
}
#  endif
# endif

# ifndef mu_cfg_yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define mu_cfg_yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
mu_cfg_yystpcpy (char *mu_cfg_yydest, const char *mu_cfg_yysrc)
#else
static char *
mu_cfg_yystpcpy (mu_cfg_yydest, mu_cfg_yysrc)
    char *mu_cfg_yydest;
    const char *mu_cfg_yysrc;
#endif
{
  char *mu_cfg_yyd = mu_cfg_yydest;
  const char *mu_cfg_yys = mu_cfg_yysrc;

  while ((*mu_cfg_yyd++ = *mu_cfg_yys++) != '\0')
    continue;

  return mu_cfg_yyd - 1;
}
#  endif
# endif

# ifndef mu_cfg_yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for mu_cfg_yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from mu_cfg_yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
mu_cfg_yytnamerr (char *mu_cfg_yyres, const char *mu_cfg_yystr)
{
  if (*mu_cfg_yystr == '"')
    {
      YYSIZE_T mu_cfg_yyn = 0;
      char const *mu_cfg_yyp = mu_cfg_yystr;

      for (;;)
	switch (*++mu_cfg_yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++mu_cfg_yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (mu_cfg_yyres)
	      mu_cfg_yyres[mu_cfg_yyn] = *mu_cfg_yyp;
	    mu_cfg_yyn++;
	    break;

	  case '"':
	    if (mu_cfg_yyres)
	      mu_cfg_yyres[mu_cfg_yyn] = '\0';
	    return mu_cfg_yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! mu_cfg_yyres)
    return mu_cfg_yystrlen (mu_cfg_yystr);

  return mu_cfg_yystpcpy (mu_cfg_yyres, mu_cfg_yystr) - mu_cfg_yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
mu_cfg_yysyntax_error (char *mu_cfg_yyresult, int mu_cfg_yystate, int mu_cfg_yychar)
{
  int mu_cfg_yyn = mu_cfg_yypact[mu_cfg_yystate];

  if (! (YYPACT_NINF < mu_cfg_yyn && mu_cfg_yyn <= YYLAST))
    return 0;
  else
    {
      int mu_cfg_yytype = YYTRANSLATE (mu_cfg_yychar);
      YYSIZE_T mu_cfg_yysize0 = mu_cfg_yytnamerr (0, mu_cfg_yytname[mu_cfg_yytype]);
      YYSIZE_T mu_cfg_yysize = mu_cfg_yysize0;
      YYSIZE_T mu_cfg_yysize1;
      int mu_cfg_yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *mu_cfg_yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int mu_cfg_yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *mu_cfg_yyfmt;
      char const *mu_cfg_yyf;
      static char const mu_cfg_yyunexpected[] = "syntax error, unexpected %s";
      static char const mu_cfg_yyexpecting[] = ", expecting %s";
      static char const mu_cfg_yyor[] = " or %s";
      char mu_cfg_yyformat[sizeof mu_cfg_yyunexpected
		    + sizeof mu_cfg_yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof mu_cfg_yyor - 1))];
      char const *mu_cfg_yyprefix = mu_cfg_yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int mu_cfg_yyxbegin = mu_cfg_yyn < 0 ? -mu_cfg_yyn : 0;

      /* Stay within bounds of both mu_cfg_yycheck and mu_cfg_yytname.  */
      int mu_cfg_yychecklim = YYLAST - mu_cfg_yyn + 1;
      int mu_cfg_yyxend = mu_cfg_yychecklim < YYNTOKENS ? mu_cfg_yychecklim : YYNTOKENS;
      int mu_cfg_yycount = 1;

      mu_cfg_yyarg[0] = mu_cfg_yytname[mu_cfg_yytype];
      mu_cfg_yyfmt = mu_cfg_yystpcpy (mu_cfg_yyformat, mu_cfg_yyunexpected);

      for (mu_cfg_yyx = mu_cfg_yyxbegin; mu_cfg_yyx < mu_cfg_yyxend; ++mu_cfg_yyx)
	if (mu_cfg_yycheck[mu_cfg_yyx + mu_cfg_yyn] == mu_cfg_yyx && mu_cfg_yyx != YYTERROR)
	  {
	    if (mu_cfg_yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		mu_cfg_yycount = 1;
		mu_cfg_yysize = mu_cfg_yysize0;
		mu_cfg_yyformat[sizeof mu_cfg_yyunexpected - 1] = '\0';
		break;
	      }
	    mu_cfg_yyarg[mu_cfg_yycount++] = mu_cfg_yytname[mu_cfg_yyx];
	    mu_cfg_yysize1 = mu_cfg_yysize + mu_cfg_yytnamerr (0, mu_cfg_yytname[mu_cfg_yyx]);
	    mu_cfg_yysize_overflow |= (mu_cfg_yysize1 < mu_cfg_yysize);
	    mu_cfg_yysize = mu_cfg_yysize1;
	    mu_cfg_yyfmt = mu_cfg_yystpcpy (mu_cfg_yyfmt, mu_cfg_yyprefix);
	    mu_cfg_yyprefix = mu_cfg_yyor;
	  }

      mu_cfg_yyf = YY_(mu_cfg_yyformat);
      mu_cfg_yysize1 = mu_cfg_yysize + mu_cfg_yystrlen (mu_cfg_yyf);
      mu_cfg_yysize_overflow |= (mu_cfg_yysize1 < mu_cfg_yysize);
      mu_cfg_yysize = mu_cfg_yysize1;

      if (mu_cfg_yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (mu_cfg_yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *mu_cfg_yyp = mu_cfg_yyresult;
	  int mu_cfg_yyi = 0;
	  while ((*mu_cfg_yyp = *mu_cfg_yyf) != '\0')
	    {
	      if (*mu_cfg_yyp == '%' && mu_cfg_yyf[1] == 's' && mu_cfg_yyi < mu_cfg_yycount)
		{
		  mu_cfg_yyp += mu_cfg_yytnamerr (mu_cfg_yyp, mu_cfg_yyarg[mu_cfg_yyi++]);
		  mu_cfg_yyf += 2;
		}
	      else
		{
		  mu_cfg_yyp++;
		  mu_cfg_yyf++;
		}
	    }
	}
      return mu_cfg_yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
mu_cfg_yydestruct (const char *mu_cfg_yymsg, int mu_cfg_yytype, YYSTYPE *mu_cfg_yyvaluep)
#else
static void
mu_cfg_yydestruct (mu_cfg_yymsg, mu_cfg_yytype, mu_cfg_yyvaluep)
    const char *mu_cfg_yymsg;
    int mu_cfg_yytype;
    YYSTYPE *mu_cfg_yyvaluep;
#endif
{
  YYUSE (mu_cfg_yyvaluep);

  if (!mu_cfg_yymsg)
    mu_cfg_yymsg = "Deleting";
  YY_SYMBOL_PRINT (mu_cfg_yymsg, mu_cfg_yytype, mu_cfg_yyvaluep, mu_cfg_yylocationp);

  switch (mu_cfg_yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int mu_cfg_yyparse (void *YYPARSE_PARAM);
#else
int mu_cfg_yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int mu_cfg_yyparse (void);
#else
int mu_cfg_yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int mu_cfg_yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE mu_cfg_yylval;

/* Number of syntax errors so far.  */
int mu_cfg_yynerrs;



/*----------.
| mu_cfg_yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
mu_cfg_yyparse (void *YYPARSE_PARAM)
#else
int
mu_cfg_yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
mu_cfg_yyparse (void)
#else
int
mu_cfg_yyparse ()

#endif
#endif
{
  
  int mu_cfg_yystate;
  int mu_cfg_yyn;
  int mu_cfg_yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int mu_cfg_yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int mu_cfg_yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char mu_cfg_yymsgbuf[128];
  char *mu_cfg_yymsg = mu_cfg_yymsgbuf;
  YYSIZE_T mu_cfg_yymsg_alloc = sizeof mu_cfg_yymsgbuf;
#endif

  /* Three stacks and their tools:
     `mu_cfg_yyss': related to states,
     `mu_cfg_yyvs': related to semantic values,
     `mu_cfg_yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow mu_cfg_yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  mu_cfg_yytype_int16 mu_cfg_yyssa[YYINITDEPTH];
  mu_cfg_yytype_int16 *mu_cfg_yyss = mu_cfg_yyssa;
  mu_cfg_yytype_int16 *mu_cfg_yyssp;

  /* The semantic value stack.  */
  YYSTYPE mu_cfg_yyvsa[YYINITDEPTH];
  YYSTYPE *mu_cfg_yyvs = mu_cfg_yyvsa;
  YYSTYPE *mu_cfg_yyvsp;



#define YYPOPSTACK(N)   (mu_cfg_yyvsp -= (N), mu_cfg_yyssp -= (N))

  YYSIZE_T mu_cfg_yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE mu_cfg_yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int mu_cfg_yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  mu_cfg_yystate = 0;
  mu_cfg_yyerrstatus = 0;
  mu_cfg_yynerrs = 0;
  mu_cfg_yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  mu_cfg_yyssp = mu_cfg_yyss;
  mu_cfg_yyvsp = mu_cfg_yyvs;

  goto mu_cfg_yysetstate;

/*------------------------------------------------------------.
| mu_cfg_yynewstate -- Push a new state, which is found in mu_cfg_yystate.  |
`------------------------------------------------------------*/
 mu_cfg_yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  mu_cfg_yyssp++;

 mu_cfg_yysetstate:
  *mu_cfg_yyssp = mu_cfg_yystate;

  if (mu_cfg_yyss + mu_cfg_yystacksize - 1 <= mu_cfg_yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T mu_cfg_yysize = mu_cfg_yyssp - mu_cfg_yyss + 1;

#ifdef mu_cfg_yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *mu_cfg_yyvs1 = mu_cfg_yyvs;
	mu_cfg_yytype_int16 *mu_cfg_yyss1 = mu_cfg_yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if mu_cfg_yyoverflow is a macro.  */
	mu_cfg_yyoverflow (YY_("memory exhausted"),
		    &mu_cfg_yyss1, mu_cfg_yysize * sizeof (*mu_cfg_yyssp),
		    &mu_cfg_yyvs1, mu_cfg_yysize * sizeof (*mu_cfg_yyvsp),

		    &mu_cfg_yystacksize);

	mu_cfg_yyss = mu_cfg_yyss1;
	mu_cfg_yyvs = mu_cfg_yyvs1;
      }
#else /* no mu_cfg_yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto mu_cfg_yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= mu_cfg_yystacksize)
	goto mu_cfg_yyexhaustedlab;
      mu_cfg_yystacksize *= 2;
      if (YYMAXDEPTH < mu_cfg_yystacksize)
	mu_cfg_yystacksize = YYMAXDEPTH;

      {
	mu_cfg_yytype_int16 *mu_cfg_yyss1 = mu_cfg_yyss;
	union mu_cfg_yyalloc *mu_cfg_yyptr =
	  (union mu_cfg_yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (mu_cfg_yystacksize));
	if (! mu_cfg_yyptr)
	  goto mu_cfg_yyexhaustedlab;
	YYSTACK_RELOCATE (mu_cfg_yyss);
	YYSTACK_RELOCATE (mu_cfg_yyvs);

#  undef YYSTACK_RELOCATE
	if (mu_cfg_yyss1 != mu_cfg_yyssa)
	  YYSTACK_FREE (mu_cfg_yyss1);
      }
# endif
#endif /* no mu_cfg_yyoverflow */

      mu_cfg_yyssp = mu_cfg_yyss + mu_cfg_yysize - 1;
      mu_cfg_yyvsp = mu_cfg_yyvs + mu_cfg_yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) mu_cfg_yystacksize));

      if (mu_cfg_yyss + mu_cfg_yystacksize - 1 <= mu_cfg_yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", mu_cfg_yystate));

  goto mu_cfg_yybackup;

/*-----------.
| mu_cfg_yybackup.  |
`-----------*/
mu_cfg_yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  mu_cfg_yyn = mu_cfg_yypact[mu_cfg_yystate];
  if (mu_cfg_yyn == YYPACT_NINF)
    goto mu_cfg_yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (mu_cfg_yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      mu_cfg_yychar = YYLEX;
    }

  if (mu_cfg_yychar <= YYEOF)
    {
      mu_cfg_yychar = mu_cfg_yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      mu_cfg_yytoken = YYTRANSLATE (mu_cfg_yychar);
      YY_SYMBOL_PRINT ("Next token is", mu_cfg_yytoken, &mu_cfg_yylval, &mu_cfg_yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  mu_cfg_yyn += mu_cfg_yytoken;
  if (mu_cfg_yyn < 0 || YYLAST < mu_cfg_yyn || mu_cfg_yycheck[mu_cfg_yyn] != mu_cfg_yytoken)
    goto mu_cfg_yydefault;
  mu_cfg_yyn = mu_cfg_yytable[mu_cfg_yyn];
  if (mu_cfg_yyn <= 0)
    {
      if (mu_cfg_yyn == 0 || mu_cfg_yyn == YYTABLE_NINF)
	goto mu_cfg_yyerrlab;
      mu_cfg_yyn = -mu_cfg_yyn;
      goto mu_cfg_yyreduce;
    }

  if (mu_cfg_yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (mu_cfg_yyerrstatus)
    mu_cfg_yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", mu_cfg_yytoken, &mu_cfg_yylval, &mu_cfg_yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (mu_cfg_yychar != YYEOF)
    mu_cfg_yychar = YYEMPTY;

  mu_cfg_yystate = mu_cfg_yyn;
  *++mu_cfg_yyvsp = mu_cfg_yylval;

  goto mu_cfg_yynewstate;


/*-----------------------------------------------------------.
| mu_cfg_yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
mu_cfg_yydefault:
  mu_cfg_yyn = mu_cfg_yydefact[mu_cfg_yystate];
  if (mu_cfg_yyn == 0)
    goto mu_cfg_yyerrlab;
  goto mu_cfg_yyreduce;


/*-----------------------------.
| mu_cfg_yyreduce -- Do a reduction.  |
`-----------------------------*/
mu_cfg_yyreduce:
  /* mu_cfg_yyn is the number of a rule to reduce with.  */
  mu_cfg_yylen = mu_cfg_yyr2[mu_cfg_yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  mu_cfg_yyval = mu_cfg_yyvsp[1-mu_cfg_yylen];


  YY_REDUCE_PRINT (mu_cfg_yyn);
  switch (mu_cfg_yyn)
    {
        case 2:
#line 128 "cfg_parser.y"
    {
	    parse_tree = (mu_cfg_yyvsp[(1) - (1)].nodelist).head;
          }
    break;

  case 3:
#line 134 "cfg_parser.y"
    {
	    (mu_cfg_yyval.nodelist).head = (mu_cfg_yyval.nodelist).tail = (mu_cfg_yyvsp[(1) - (1)].pnode);
	  }
    break;

  case 4:
#line 138 "cfg_parser.y"
    {
	    (mu_cfg_yyval.nodelist) = (mu_cfg_yyvsp[(1) - (2)].nodelist);
	    (mu_cfg_yyval.nodelist).tail->next = (mu_cfg_yyvsp[(2) - (2)].pnode);
	    (mu_cfg_yyval.nodelist).tail = (mu_cfg_yyvsp[(2) - (2)].pnode);
	  }
    break;

  case 9:
#line 154 "cfg_parser.y"
    {
	    if ((mu_cfg_yyvsp[(4) - (5)].node).tag_name && strcmp ((mu_cfg_yyvsp[(4) - (5)].node).tag_name, (mu_cfg_yyvsp[(1) - (5)].node).tag_name))
	      {
		mu_cfg_perror (_mu_cfg_lexer_data,
			       &(mu_cfg_yyvsp[(1) - (5)].node).locus,
			       _("Tag %s not closed"),
			       (mu_cfg_yyvsp[(1) - (5)].node).tag_name);
		mu_cfg_perror (_mu_cfg_lexer_data,
			       &(mu_cfg_yyvsp[(4) - (5)].node).locus,
			       _("Found closing %s tag instead"),
			       (mu_cfg_yyvsp[(4) - (5)].node).tag_name);
		_mu_cfg_errcnt++;
	      }
	    (mu_cfg_yyval.pnode) = mu_cfg_alloc_node (mu_cfg_node_tag, &(mu_cfg_yyvsp[(1) - (5)].node).locus,
				    (mu_cfg_yyvsp[(1) - (5)].node).tag_name, (mu_cfg_yyvsp[(1) - (5)].node).tag_label, (mu_cfg_yyvsp[(3) - (5)].nodelist).head);
	  }
    break;

  case 10:
#line 170 "cfg_parser.y"
    { mu_cfg_tie_in++; }
    break;

  case 11:
#line 171 "cfg_parser.y"
    { mu_cfg_tie_in = 0; }
    break;

  case 12:
#line 172 "cfg_parser.y"
    {
	    (mu_cfg_yyval.pnode) = mu_cfg_alloc_node (mu_cfg_node_param, &mu_cfg_locus, (mu_cfg_yyvsp[(1) - (5)].string), (mu_cfg_yyvsp[(3) - (5)].string),
				    NULL);
	  }
    break;


/* Line 1267 of yacc.c.  */
#line 1491 "cfg_parser.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", mu_cfg_yyr1[mu_cfg_yyn], &mu_cfg_yyval, &mu_cfg_yyloc);

  YYPOPSTACK (mu_cfg_yylen);
  mu_cfg_yylen = 0;
  YY_STACK_PRINT (mu_cfg_yyss, mu_cfg_yyssp);

  *++mu_cfg_yyvsp = mu_cfg_yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  mu_cfg_yyn = mu_cfg_yyr1[mu_cfg_yyn];

  mu_cfg_yystate = mu_cfg_yypgoto[mu_cfg_yyn - YYNTOKENS] + *mu_cfg_yyssp;
  if (0 <= mu_cfg_yystate && mu_cfg_yystate <= YYLAST && mu_cfg_yycheck[mu_cfg_yystate] == *mu_cfg_yyssp)
    mu_cfg_yystate = mu_cfg_yytable[mu_cfg_yystate];
  else
    mu_cfg_yystate = mu_cfg_yydefgoto[mu_cfg_yyn - YYNTOKENS];

  goto mu_cfg_yynewstate;


/*------------------------------------.
| mu_cfg_yyerrlab -- here on detecting error |
`------------------------------------*/
mu_cfg_yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!mu_cfg_yyerrstatus)
    {
      ++mu_cfg_yynerrs;
#if ! YYERROR_VERBOSE
      mu_cfg_yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T mu_cfg_yysize = mu_cfg_yysyntax_error (0, mu_cfg_yystate, mu_cfg_yychar);
	if (mu_cfg_yymsg_alloc < mu_cfg_yysize && mu_cfg_yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T mu_cfg_yyalloc = 2 * mu_cfg_yysize;
	    if (! (mu_cfg_yysize <= mu_cfg_yyalloc && mu_cfg_yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      mu_cfg_yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (mu_cfg_yymsg != mu_cfg_yymsgbuf)
	      YYSTACK_FREE (mu_cfg_yymsg);
	    mu_cfg_yymsg = (char *) YYSTACK_ALLOC (mu_cfg_yyalloc);
	    if (mu_cfg_yymsg)
	      mu_cfg_yymsg_alloc = mu_cfg_yyalloc;
	    else
	      {
		mu_cfg_yymsg = mu_cfg_yymsgbuf;
		mu_cfg_yymsg_alloc = sizeof mu_cfg_yymsgbuf;
	      }
	  }

	if (0 < mu_cfg_yysize && mu_cfg_yysize <= mu_cfg_yymsg_alloc)
	  {
	    (void) mu_cfg_yysyntax_error (mu_cfg_yymsg, mu_cfg_yystate, mu_cfg_yychar);
	    mu_cfg_yyerror (mu_cfg_yymsg);
	  }
	else
	  {
	    mu_cfg_yyerror (YY_("syntax error"));
	    if (mu_cfg_yysize != 0)
	      goto mu_cfg_yyexhaustedlab;
	  }
      }
#endif
    }



  if (mu_cfg_yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (mu_cfg_yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (mu_cfg_yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  mu_cfg_yydestruct ("Error: discarding",
		      mu_cfg_yytoken, &mu_cfg_yylval);
	  mu_cfg_yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto mu_cfg_yyerrlab1;


/*---------------------------------------------------.
| mu_cfg_yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
mu_cfg_yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label mu_cfg_yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto mu_cfg_yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (mu_cfg_yylen);
  mu_cfg_yylen = 0;
  YY_STACK_PRINT (mu_cfg_yyss, mu_cfg_yyssp);
  mu_cfg_yystate = *mu_cfg_yyssp;
  goto mu_cfg_yyerrlab1;


/*-------------------------------------------------------------.
| mu_cfg_yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
mu_cfg_yyerrlab1:
  mu_cfg_yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      mu_cfg_yyn = mu_cfg_yypact[mu_cfg_yystate];
      if (mu_cfg_yyn != YYPACT_NINF)
	{
	  mu_cfg_yyn += YYTERROR;
	  if (0 <= mu_cfg_yyn && mu_cfg_yyn <= YYLAST && mu_cfg_yycheck[mu_cfg_yyn] == YYTERROR)
	    {
	      mu_cfg_yyn = mu_cfg_yytable[mu_cfg_yyn];
	      if (0 < mu_cfg_yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (mu_cfg_yyssp == mu_cfg_yyss)
	YYABORT;


      mu_cfg_yydestruct ("Error: popping",
		  mu_cfg_yystos[mu_cfg_yystate], mu_cfg_yyvsp);
      YYPOPSTACK (1);
      mu_cfg_yystate = *mu_cfg_yyssp;
      YY_STACK_PRINT (mu_cfg_yyss, mu_cfg_yyssp);
    }

  if (mu_cfg_yyn == YYFINAL)
    YYACCEPT;

  *++mu_cfg_yyvsp = mu_cfg_yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", mu_cfg_yystos[mu_cfg_yyn], mu_cfg_yyvsp, mu_cfg_yylsp);

  mu_cfg_yystate = mu_cfg_yyn;
  goto mu_cfg_yynewstate;


/*-------------------------------------.
| mu_cfg_yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
mu_cfg_yyacceptlab:
  mu_cfg_yyresult = 0;
  goto mu_cfg_yyreturn;

/*-----------------------------------.
| mu_cfg_yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
mu_cfg_yyabortlab:
  mu_cfg_yyresult = 1;
  goto mu_cfg_yyreturn;

#ifndef mu_cfg_yyoverflow
/*-------------------------------------------------.
| mu_cfg_yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
mu_cfg_yyexhaustedlab:
  mu_cfg_yyerror (YY_("memory exhausted"));
  mu_cfg_yyresult = 2;
  /* Fall through.  */
#endif

mu_cfg_yyreturn:
  if (mu_cfg_yychar != YYEOF && mu_cfg_yychar != YYEMPTY)
     mu_cfg_yydestruct ("Cleanup: discarding lookahead",
		 mu_cfg_yytoken, &mu_cfg_yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (mu_cfg_yylen);
  YY_STACK_PRINT (mu_cfg_yyss, mu_cfg_yyssp);
  while (mu_cfg_yyssp != mu_cfg_yyss)
    {
      mu_cfg_yydestruct ("Cleanup: popping",
		  mu_cfg_yystos[*mu_cfg_yyssp], mu_cfg_yyvsp);
      YYPOPSTACK (1);
    }
#ifndef mu_cfg_yyoverflow
  if (mu_cfg_yyss != mu_cfg_yyssa)
    YYSTACK_FREE (mu_cfg_yyss);
#endif
#if YYERROR_VERBOSE
  if (mu_cfg_yymsg != mu_cfg_yymsgbuf)
    YYSTACK_FREE (mu_cfg_yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (mu_cfg_yyresult);
}


#line 178 "cfg_parser.y"


int
mu_cfg_parse (mu_cfg_node_t **ptree,
	      void *data, mu_cfg_lexer_t lexer,
	      mu_cfg_perror_t perror,
	      mu_cfg_alloc_t palloc, mu_cfg_free_t pfree)
{
  int rc;
	
  _mu_cfg_lexer = lexer;
  _mu_cfg_lexer_data = data;
  mu_cfg_perror = perror ? perror : _mu_cfg_default_perror;
  _mu_cfg_alloc = palloc ? palloc : malloc;
  _mu_cfg_free = pfree ? pfree : free;
  _mu_cfg_errcnt = 0;
  mu_cfg_tie_in = 0;
  rc = mu_cfg_yyparse ();
  if (rc == 0 && _mu_cfg_errcnt)
    rc = 1;
  /* FIXME if (rc) free_memory; else */
  *ptree = parse_tree;
  return rc;
}
	


static void
print_level (FILE *fp, int level)
{
  while (level--)
    {
      fputc (' ', fp);
      fputc (' ', fp);
    }
}

struct tree_print
{
  unsigned level;
  FILE *fp;
};

int
print_node (const mu_cfg_node_t *node, void *data)
{
  struct tree_print *tp = data;
  
  print_level (tp->fp, tp->level);
  switch (node->type)
    {
    case mu_cfg_node_undefined:
      fprintf (tp->fp, _("<UNDEFINED>"));
      break;

    case mu_cfg_node_tag:
      fprintf (tp->fp, "<%s", node->tag_name);
      if (node->tag_label)
	fprintf (tp->fp, " %s", node->tag_label);
      fprintf (tp->fp, ">");
      tp->level++;
      break;

    case mu_cfg_node_param:
      fprintf (tp->fp, "%s", node->tag_name);
      if (node->tag_label)
	fprintf (tp->fp, " %s", node->tag_label);
      break;
    }
  fprintf (tp->fp, "\n");
  return MU_CFG_ITER_OK;
}

int
print_node_end (const mu_cfg_node_t *node, void *data)
{
  struct tree_print *tp = data;
  tp->level--;
  print_level (tp->fp, tp->level);
  fprintf (tp->fp, "</%s>\n", node->tag_name);
  return MU_CFG_ITER_OK;
}

void
mu_cfg_print_tree (FILE *fp, mu_cfg_node_t *node)
{
  struct tree_print t;
  t.level = 0;
  t.fp = fp;
  mu_cfg_preorder (node, print_node, print_node_end, &t);
}



static int
_mu_cfg_preorder_recursive (mu_cfg_node_t *node,
			    mu_cfg_iter_func_t beg, mu_cfg_iter_func_t end,
			    void *data)
{
  switch (node->type)
    {
    case mu_cfg_node_undefined:
      abort ();

    case mu_cfg_node_tag:
      switch (beg (node, data))
	{
	case MU_CFG_ITER_OK:
	  if (mu_cfg_preorder (node->node, beg, end, data))
	    return MU_CFG_ITER_STOP;
	  if (end && end (node, data) == MU_CFG_ITER_STOP)
	    return MU_CFG_ITER_STOP;
	  break;
	  
	case MU_CFG_ITER_SKIP:
	  break;
	  
	case MU_CFG_ITER_STOP:
	  return MU_CFG_ITER_STOP;
	}
      break;

    case mu_cfg_node_param:
      return beg (node, data);
    }
  return MU_CFG_ITER_OK;
}

int
mu_cfg_preorder(mu_cfg_node_t *node,
		mu_cfg_iter_func_t beg, mu_cfg_iter_func_t end, void *data)
{
  for (; node; node = node->next)
    if (_mu_cfg_preorder_recursive(node, beg, end, data)  == MU_CFG_ITER_STOP)
      return 1;
  return 0;
}

static int
_mu_cfg_postorder_recursive(mu_cfg_node_t *node,
			    mu_cfg_iter_func_t beg, mu_cfg_iter_func_t end,
			    void *data)
{
  switch (node->type)
    {
    case mu_cfg_node_undefined:
      abort ();

    case mu_cfg_node_tag:
      switch (beg (node, data))
	{
	case MU_CFG_ITER_OK:
	  if (mu_cfg_postorder (node->node, beg, end, data))
	    return MU_CFG_ITER_STOP;
	  if (end && end (node, data) == MU_CFG_ITER_STOP)
	    return MU_CFG_ITER_STOP;
	  break;
	  
	case MU_CFG_ITER_SKIP:
	  break;
	  
	case MU_CFG_ITER_STOP:
	  return MU_CFG_ITER_STOP;
	}
      break;
      
    case mu_cfg_node_param:
      return beg (node, data);
    }
  return 0;
}

int
mu_cfg_postorder (mu_cfg_node_t *node,
		  mu_cfg_iter_func_t beg, mu_cfg_iter_func_t end, void *data)
{
  if (node->next
      && mu_cfg_postorder (node->next, beg, end, data) == MU_CFG_ITER_STOP)
    return 1;
  return _mu_cfg_postorder_recursive (node, beg, end, data)
                == MU_CFG_ITER_STOP;
}


static int
free_section (const mu_cfg_node_t *node, void *data)
{
  mu_cfg_free_t free_fn = data;
  if (node->type == mu_cfg_node_tag)
    free_fn ((void *) node);
  return MU_CFG_ITER_OK;
}

static int
free_param (const mu_cfg_node_t *node, void *data)
{
  mu_cfg_free_t free_fn = data;
  if (node->type == mu_cfg_node_param)
    free_fn ((void*) node);
  return MU_CFG_ITER_OK;
}

void
mu_cfg_destroy_tree (mu_cfg_node_t **tree)
{
  if (tree && *tree)
    {
      mu_cfg_postorder (*tree, free_param, free_section, _mu_cfg_free);
      *tree = NULL;
    }
}



struct find_data
{
  char *tag;
  char *label;
  char *next;
  const mu_cfg_node_t *node;
};

static void
parse_tag (struct find_data *fptr)
{
  char *p = strchr (fptr->tag, '=');
  if (p)
    {
      *p++ = 0;
      fptr->label = p;
      fptr->next = p + strlen (p) + 1;
    }
  else
    {
      fptr->label = NULL;
      fptr->next = fptr->tag + strlen (fptr->tag) + 1;
    }
}

static int
node_finder (const mu_cfg_node_t *node, void *data)
{
  struct find_data *fdptr = data;
  if (strcmp (fdptr->tag, node->tag_name) == 0
      && (!fdptr->label || strcmp (fdptr->label, node->tag_label) == 0))
    {
      fdptr->tag = fdptr->next;
      parse_tag (fdptr);
      if (fdptr->tag[0] == 0)
	{
	  fdptr->node = node;
	  return MU_CFG_ITER_STOP;
	}
    }
  return MU_CFG_ITER_OK;
}

int	    
mu_cfg_find_node (mu_cfg_node_t *tree, const char *path, mu_cfg_node_t **pval)
{
  int rc;
  char *p;
  char *xpath;
  size_t len;
  struct find_data data;
  
  len = strlen (path) + 1;
  xpath = calloc (1, len + 1);
  if (!xpath)
    return 1;
  strcpy (xpath, path);
  xpath[len-1] = '/';
  data.tag = xpath;
  for (p = data.tag; *p; p++)
    if (*p == '/')
      *p = 0;
  parse_tag (&data);
  rc = mu_cfg_preorder (tree, node_finder, NULL, &data);
  free (xpath);
  if (rc)
    {
      *pval = (mu_cfg_node_t *) data.node;
      return 0;
    }
  return MU_ERR_NOENT;
}

int	    
mu_cfg_find_node_label (mu_cfg_node_t *tree, const char *path,
			const char **pval)
{
  mu_cfg_node_t *node;
  int rc = mu_cfg_find_node (tree, path, &node);
  if (rc)
    *pval = node->tag_label;
  return rc;
}


struct mu_cfg_section_list
{
  struct mu_cfg_section_list *next;
  struct mu_cfg_section *sec;
};

struct scan_tree_data
{
  struct mu_cfg_section_list *list;
  void *call_data;
  int error;
};

static struct mu_cfg_section *
find_subsection (struct mu_cfg_section *sec, const char *ident, size_t len)
{
  if (sec)
    {
      sec = sec->subsec;
      if (sec)
	{
	  if (len == 0)
	    len = strlen (ident);
	  
	  for (; sec->ident; sec++)
	    if (strlen (sec->ident) == len
		&& memcmp (sec->ident, ident, len) == 0)
	      return sec;
	}
    }
  return NULL;
}

static struct mu_cfg_param *
find_param (struct mu_cfg_section *sec, const char *ident, size_t len)
{
  if (sec)
    {
      struct mu_cfg_param *p = sec->param;
      if (p)
	{
	  if (len == 0)
	    len = strlen (ident);
	  for (; p->ident; p++)
	    if (strlen (p->ident) == len && memcmp (p->ident, ident, len) == 0)
	      return p;
	}
    }
  return NULL;
}

static int
push_section (struct scan_tree_data *dat, struct mu_cfg_section *sec)
{
  struct mu_cfg_section_list *p = _mu_cfg_alloc (sizeof *p);
  if (!p)
    {
      mu_cfg_perror (dat->call_data, NULL, _("not enough memory"));
      return 1;
    }
  p->sec = sec;
  p->next = dat->list;
  dat->list = p;
  return 0;
}

static struct mu_cfg_section *
pop_section (struct scan_tree_data *dat)
{
  struct mu_cfg_section_list *p = dat->list;
  struct mu_cfg_section *sec = p->sec;
  dat->list = p->next;
  _mu_cfg_free (p);
  return sec;
}

#define STRTONUM(s, type, base, res, limit)				      \
{									      \
  type sum = 0;							      	      \
									      \
  while (*s)								      \
    {							      		      \
      type x;							      	      \
      									      \
      if ('0' <= *s && *s <= '9')				      	      \
	x = sum * base + *s - '0';			      		      \
      else if (base == 16 && 'a' <= *s && *s <= 'f')		      	      \
	x = sum * base + *s - 'a';			      		      \
      else if (base == 16 && 'A' <= *s && *s <= 'F')		      	      \
	x = sum * base + *s - 'A';			      		      \
      else							      	      \
	break;						      		      \
      if (x <= sum)							      \
	{						      		      \
	  mu_cfg_perror (sdata->call_data,                         	      \
			 &node->locus,                           	      \
			 _("numeric overflow"));                 	      \
	  return 1;					      		      \
	}								      \
      else if (limit && x > limit)					      \
	{			      					      \
	  mu_cfg_perror (sdata->call_data,                         	      \
			 &node->locus,                           	      \
			 _("value out of allowed range"));       	      \
	  return 1;					      		      \
	}							      	      \
      sum = x;						      		      \
      *s++;							      	      \
    }								      	      \
  res = sum;                                                                  \
}

#define STRxTONUM(s, type, res, limit)				              \
{								              \
  int base;						              	      \
  if (*s == '0')							      \
    {					              			      \
      s++;						              	      \
      if (*s == 0)					              	      \
	base = 10;				              		      \
      else if (*s == 'x' || *s == 'X')					      \
	{		              					      \
	  s++;					              		      \
	  base = 16;				              		      \
	}								      \
      else						              	      \
	base = 8;				              		      \
    } else							              \
      base = 10;					              	      \
  STRTONUM (s, type, base, res, limit);			                      \
}

#define GETUNUM(str, type, res)						      \
{									      \
  type tmpres;							      	      \
  const char *s = str;                                                        \
  STRxTONUM (s, type, tmpres, 0);					      \
  if (*s)								      \
    {							      		      \
      mu_cfg_perror (sdata->call_data,                                 	      \
		     &node->locus,                                   	      \
		     _("not a number (stopped near `%s')"),          	      \
		     s);    					      	      \
      return 1;					      	      		      \
    }								      	      \
  res = tmpres;							              \
}

#define GETSNUM(str, type, res)	               				      \
{									      \
  unsigned type tmpres;						      	      \
  const char *s = str;						      	      \
  int sign;							      	      \
  unsigned type limit;						      	      \
									      \
  if (*s == '-')							      \
    {						      			      \
      sign++;							      	      \
      s++;							      	      \
      limit = TYPE_MINIMUM (type);				      	      \
      limit = - limit;                                                        \
    }									      \
  else									      \
    {							      		      \
      sign = 0;						      		      \
      limit = TYPE_MAXIMUM (type);				      	      \
    }								      	      \
									      \
  STRxTONUM (s, unsigned type, tmpres, limit);	      		      	      \
  if (*s)								      \
    {							      		      \
      mu_cfg_perror (sdata->call_data,                                 	      \
		     &node->locus,                                   	      \
		     _("not a number (stopped near `%s')"),          	      \
		     s);    					      	      \
      return 1;					      	      		      \
    }								      	      \
  res = sign ? - tmpres : tmpres;					      \
}

static int
parse_ipv4 (struct scan_tree_data *sdata, const mu_cfg_node_t *node,
	    struct in_addr *res)
{
  struct in_addr addr;
  if (inet_aton (node->tag_label, &addr) == 0)
    {
      mu_cfg_perror (sdata->call_data, 
		     &node->locus,
		     _("not an IPv4"));
      return 1;
    }
  addr.s_addr = ntohl (addr.s_addr);
  *res = addr;
  return 0;
}		

static int
parse_host (struct scan_tree_data *sdata, const mu_cfg_node_t *node,
	    struct in_addr *res)
{
  struct in_addr addr;
  struct hostent *hp = gethostbyname (node->tag_label);
  if (hp)
    {
      addr.s_addr = *(unsigned long *)hp->h_addr;
    }
  else if (inet_aton(node->tag_label, &addr) == 0)
    {
      mu_cfg_perror (sdata->call_data, 
		     &node->locus,
		     _("cannot resolve hostname `%s'"),
		     node->tag_label);
      return 1;
    } 
  addr.s_addr = ntohl (addr.s_addr);
  *res = addr;
  return 0;
}		

static int
parse_cidr (struct scan_tree_data *sdata, const mu_cfg_node_t *node,
	    mu_cfg_cidr_t *res)
{
  struct in_addr addr;
  unsigned long mask;
  char astr[16], *p, *s;
  
  p = strchr (node->tag_label, '/');
  if (*p)
    {
      int len = p - node->tag_label;
      if (len > sizeof astr - 1) {
	mu_cfg_perror (sdata->call_data, 
		       &node->locus,
		       _("not a valid IPv4 address in CIDR"));
	return 1;
      }
      memcpy (astr, node->tag_label, len);
      astr[len] = 0;
      if (inet_aton (astr, &addr) == 0)
	{
	  mu_cfg_perror (sdata->call_data, 
			 &node->locus,
			 _("not a valid IPv4 address in CIDR"));
	  return 1;
	}
      addr.s_addr = ntohl (addr.s_addr);

      p++;
      s = p;
      STRxTONUM (s, unsigned long, mask, 0);
      if (*s == '.')
	{
	  struct in_addr a;
	  if (inet_aton (p, &a) == 0)
	    {
	      mu_cfg_perror (sdata->call_data, 
			     &node->locus,
			     _("not a valid network in CIDR"));
	      return 1;
	    }
	  a.s_addr = ntohl (a.s_addr);
	  for (mask = 0; (a.s_addr & 1) == 0 && mask < 32; )
	    {
	      mask++;
	      a.s_addr >>= 1;
	    }
	  mask = 32 - mask;
	}
      else if (mask > 32)
	{
	  mu_cfg_perror (sdata->call_data, 
			 &node->locus,
			 _("not a valid network mask in CIDR"));
	  return 1;
	}
    }
  else
    {
      int i;
      unsigned short x;
      addr.s_addr = 0;
      
      for (i = 0; i < 3; i++)
	{
	  STRxTONUM(p, unsigned short, x, 255);
	  if (*p != '.')
	    break;
	  addr.s_addr = (addr.s_addr << 8) + x;
	}
		
      if (*p)
	{
	  mu_cfg_perror (sdata->call_data,
			 &node->locus,
			 _("not a CIDR (stopped near `%s')"),
			 p);
	  return 1;
	}

      mask = i * 8;
      
      addr.s_addr <<= (4 - i) * 8;
    }
			
  res->addr = addr;
  res->mask = mask;
  return 0;
}	

static int
parse_bool (struct scan_tree_data *sdata, const mu_cfg_node_t *node, int *res)
{
  if (strcmp (node->tag_label, "yes") == 0
      || strcmp (node->tag_label, "on") == 0
      || strcmp (node->tag_label, "t") == 0
      || strcmp (node->tag_label, "true") == 0
      || strcmp (node->tag_label, "1") == 0)
    *res = 1;
  else if (strcmp (node->tag_label, "no") == 0
	   || strcmp (node->tag_label, "off") == 0
	   || strcmp (node->tag_label, "nil") == 0
	   || strcmp (node->tag_label, "false") == 0
	   || strcmp (node->tag_label, "0") == 0)
    *res = 0;
  else
    {
      mu_cfg_perror (sdata->call_data, &node->locus, _("not a boolean"));
      return 1;
    }
  return 0;
}

static int
parse_param (struct scan_tree_data *sdata, const mu_cfg_node_t *node)
{
  struct mu_cfg_param *param = find_param (sdata->list->sec, node->tag_name,
					   0);
  if (!param)
    {
      mu_cfg_perror (sdata->call_data,
		     &node->locus,
		     _("unknown keyword `%s'"),
		     node->tag_name);
      return 1;
    }

  switch (param->type)
    {
    case mu_cfg_string:
      {
	size_t len = strlen (node->tag_label);
	char *s = _mu_cfg_alloc (len + 1);
	if (!s)
	  {
	    mu_cfg_perror (sdata->call_data,
			   &node->locus,
			   _("not enough memory"));
	    return 1;
	  }
	strcpy (s, node->tag_label);
	/* FIXME: free param->data? */
	*(char**)param->data = s;
	break;
      }
		
    case mu_cfg_short:
      GETSNUM (node->tag_label, short, *(short*)param->data);
      break;
		
    case mu_cfg_ushort:
      GETUNUM (node->tag_label, unsigned short, *(unsigned short*)param->data);
      break;
		
    case mu_cfg_int:
      GETSNUM (node->tag_label, int, *(int*)param->data);
      break;
		
    case mu_cfg_uint:
      GETUNUM (node->tag_label, unsigned int, *(unsigned int*)param->data);
      break;
            
    case mu_cfg_long:
      GETSNUM (node->tag_label, long, *(long*)param->data);
      break;
      
    case mu_cfg_ulong:
      GETUNUM (node->tag_label, unsigned long, *(unsigned long*)param->data);
      break;
		
    case mu_cfg_size:
      GETUNUM (node->tag_label, size_t, *(size_t*)param->data);
      break;
		
    case mu_cfg_off:
      mu_cfg_perror (sdata->call_data, &node->locus, _("not implemented yet"));
      /* GETSNUM(node->tag_label, off_t, *(off_t*)param->data); */
      return 1;

    case mu_cfg_bool:
      if (parse_bool (sdata, node, (int*) param->data))
	return 1;
      break;
		
    case mu_cfg_ipv4: 
      if (parse_ipv4 (sdata, node, (struct in_addr *)param->data))
	return 1;
      break;
      
    case mu_cfg_cidr:
      if (parse_cidr (sdata, node, (mu_cfg_cidr_t *)param->data))
	return 1;
      break;
      
    case mu_cfg_host:
      if (parse_host (sdata, node, (struct in_addr *)param->data))
	return 1;
      break;

    case mu_cfg_callback:
      if (param->callback (&node->locus, param->data, node->tag_label))
	  return 1;
      break;
      
    default:
      abort ();
    }
  return 0;
}


static int
_scan_tree_helper (const mu_cfg_node_t *node, void *data)
{
  struct scan_tree_data *sdata = data;
  struct mu_cfg_section *sec;
  
  switch (node->type)
    {
    case mu_cfg_node_undefined:
      abort ();
		
    case mu_cfg_node_tag:
      sec = find_subsection (sdata->list->sec, node->tag_name, 0);
      if (!sec)
	{
	  mu_cfg_perror (sdata->call_data,
			 &node->locus,
			 _("unknown section `%s'"),
			 node->tag_name);
	  sdata->error++;
	  return MU_CFG_ITER_SKIP;
	}
      if (!sec->subsec && !sec->param)
	return MU_CFG_ITER_SKIP;
      if (sec->parser &&
	  sec->parser (mu_cfg_section_start, node,
		       sec->data, sdata->call_data))
	{
	  sdata->error++;
	  return MU_CFG_ITER_SKIP;
	}
      push_section(sdata, sec);
      break;
		
    case mu_cfg_node_param:
      if (parse_param (sdata, node))
	{
	  sdata->error++;
	  return MU_CFG_ITER_SKIP;
	}
      break;
    }
  return MU_CFG_ITER_OK;
}

static int
_scan_tree_end_helper (const mu_cfg_node_t *node, void *data)
{
  struct scan_tree_data *sdata = data;
  struct mu_cfg_section *sec;
  
  switch (node->type)
    {
    default:
      abort ();
		
    case mu_cfg_node_tag:
      sec = pop_section (sdata);
      if (sec && sec->parser)
	{
	  if (sec->parser (mu_cfg_section_end, node, sec->data,
			   sdata->call_data))
	    {
	      sdata->error++;
	      return MU_CFG_ITER_SKIP;
	    }
	}
    }
  return MU_CFG_ITER_OK;
}

int
mu_cfg_scan_tree (mu_cfg_node_t *node,
		  struct mu_cfg_section *sections,
		  void *data, mu_cfg_perror_t perror,
		  mu_cfg_alloc_t palloc, mu_cfg_free_t pfree)
{
  struct scan_tree_data dat;
  dat.list = NULL;
  mu_cfg_perror = perror ? perror : _mu_cfg_default_perror;
  _mu_cfg_alloc = palloc ? palloc : malloc;
  _mu_cfg_free = pfree ? pfree : free;
  dat.call_data = data;
  dat.error = 0;
  if (push_section (&dat, sections))
    return 1;
  mu_cfg_preorder (node, _scan_tree_helper, _scan_tree_end_helper, &dat);
  pop_section (&dat);
  return dat.error;
}

int
mu_cfg_find_section (struct mu_cfg_section *root_sec,
		     const char *path, struct mu_cfg_section **retval)
{
  while (path[0])
    {
      struct mu_cfg_section *sec;
      size_t len;
      const char *p;
      
      while (*path == '/')
	path++;

      if (*path == 0)
	return MU_ERR_NOENT;
      
      p = strchr (path, '/');
      if (p)
	len = p - path;
      else
	len = strlen (path);
      
      sec = find_subsection (root_sec, path, len);
      if (!sec)
	return MU_ERR_NOENT;
      root_sec = sec;
      path += len;
    }
  if (retval)
    *retval = root_sec;
  return 0;
}






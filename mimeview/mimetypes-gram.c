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

/* All symbols defined below should begin with mimetypes_yy or YY, to avoid
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
#line 1 "mimetypes.y"

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005, 2007, 2009-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
  
#include <mailutils/cctype.h>
#include <mimeview.h>
#include <mimetypes-decl.h>
  
static void
mimetypes_yyprint (FILE *output, unsigned short toknum, YYSTYPE val)
{
  switch (toknum)
    {
    case IDENT:
    case IDENT_L:
    case STRING:
      fprintf (output, "[%lu] %s", (unsigned long) val.string.len,
	       val.string.ptr);
      break;

    case EOL:
    default:
      break;
    }
}

#define YYPRINT mimetypes_yyprint

static mu_list_t arg_list; /* For error recovery */

#define L_OR  0
#define L_AND 1

enum node_type
  {
    functional_node,
    binary_node,
    negation_node,
    suffix_node
  };

union argument
{
  struct mimetypes_string *string;
  unsigned number;
  int c;
};

typedef int (*builtin_t) (union argument *args);

struct node
{
  enum node_type type;
  union
  {
    struct
    {
      builtin_t fun;
      union argument *args;
    } function;
    struct node *arg;
    struct
    {
      int op;
      struct node *arg1;
      struct node *arg2;
    } bin; 
    struct mimetypes_string suffix;
  } v;
};

static struct node *make_binary_node (int op,
				      struct node *left, struct node *rigth);
static struct node *make_negation_node (struct node *p);

static struct node *make_suffix_node (struct mimetypes_string *suffix);
static struct node *make_functional_node (char *ident, mu_list_t list);

static int eval_rule (struct node *root);

struct rule_tab
{
  char *type;
  struct node *node;
};

static mu_list_t rule_list;


/* Line 371 of yacc.c  */
#line 175 "mimetypes-gram.c"

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
extern int mimetypes_yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum mimetypes_yytokentype {
     IDENT = 258,
     IDENT_L = 259,
     STRING = 260,
     EOL = 261,
     BOGUS = 262
   };
#endif
/* Tokens.  */
#define IDENT 258
#define IDENT_L 259
#define STRING 260
#define EOL 261
#define BOGUS 262



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 387 of yacc.c  */
#line 119 "mimetypes.y"

  struct mimetypes_string string;
  mu_list_t list;
  int result;
  struct node *node;


/* Line 387 of yacc.c  */
#line 240 "mimetypes-gram.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define mimetypes_yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE mimetypes_yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int mimetypes_yyparse (void *YYPARSE_PARAM);
#else
int mimetypes_yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int mimetypes_yyparse (void);
#else
int mimetypes_yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 268 "mimetypes-gram.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 mimetypes_yytype_uint8;
#else
typedef unsigned char mimetypes_yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 mimetypes_yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char mimetypes_yytype_int8;
#else
typedef short int mimetypes_yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 mimetypes_yytype_uint16;
#else
typedef unsigned short int mimetypes_yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 mimetypes_yytype_int16;
#else
typedef short int mimetypes_yytype_int16;
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
YYID (int mimetypes_yyi)
#else
static int
YYID (mimetypes_yyi)
    int mimetypes_yyi;
#endif
{
  return mimetypes_yyi;
}
#endif

#if ! defined mimetypes_yyoverflow || YYERROR_VERBOSE

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
#endif /* ! defined mimetypes_yyoverflow || YYERROR_VERBOSE */


#if (! defined mimetypes_yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union mimetypes_yyalloc
{
  mimetypes_yytype_int16 mimetypes_yyss_alloc;
  YYSTYPE mimetypes_yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union mimetypes_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (mimetypes_yytype_int16) + sizeof (YYSTYPE)) \
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
	YYSIZE_T mimetypes_yynewbytes;						\
	YYCOPY (&mimetypes_yyptr->Stack_alloc, Stack, mimetypes_yysize);			\
	Stack = &mimetypes_yyptr->Stack_alloc;					\
	mimetypes_yynewbytes = mimetypes_yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	mimetypes_yyptr += mimetypes_yynewbytes / sizeof (*mimetypes_yyptr);				\
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
          YYSIZE_T mimetypes_yyi;                         \
          for (mimetypes_yyi = 0; mimetypes_yyi < (Count); mimetypes_yyi++)   \
            (Dst)[mimetypes_yyi] = (Src)[mimetypes_yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  10
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   54

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  14
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  12
/* YYNRULES -- Number of rules.  */
#define YYNRULES  24
/* YYNRULES -- Number of states.  */
#define YYNSTATES  38

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   262

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? mimetypes_yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const mimetypes_yytype_uint8 mimetypes_yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    11,     2,     2,     2,     2,     2,     2,
      12,    13,     2,     9,     8,     2,     2,    10,     2,     2,
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
       5,     6,     7
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const mimetypes_yytype_uint8 mimetypes_yyprhs[] =
{
       0,     0,     3,     5,     7,    11,    12,    15,    18,    20,
      23,    27,    29,    32,    36,    40,    43,    47,    49,    51,
      53,    55,    59,    61,    65
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const mimetypes_yytype_int8 mimetypes_yyrhs[] =
{
      15,     0,    -1,    16,    -1,    17,    -1,    16,    18,    17,
      -1,    -1,    19,    20,    -1,     1,    18,    -1,     6,    -1,
      18,     6,    -1,     3,    10,     3,    -1,    21,    -1,    20,
      20,    -1,    20,     8,    20,    -1,    20,     9,    20,    -1,
      11,    21,    -1,    12,    20,    13,    -1,    22,    -1,    23,
      -1,     5,    -1,     3,    -1,     4,    24,    13,    -1,    25,
      -1,    24,     8,    25,    -1,    22,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const mimetypes_yytype_uint8 mimetypes_yyrline[] =
{
       0,   128,   128,   131,   132,   135,   136,   145,   154,   155,
     158,   164,   165,   169,   173,   179,   183,   187,   191,   194,
     195,   198,   207,   213,   220
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const mimetypes_yytname[] =
{
  "$end", "error", "$undefined", "IDENT", "IDENT_L", "STRING", "EOL",
  "BOGUS", "','", "'+'", "'/'", "'!'", "'('", "')'", "$accept", "input",
  "list", "rule_line", "eol", "type", "rule", "stmt", "string", "function",
  "arglist", "arg", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const mimetypes_yytype_uint16 mimetypes_yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,    44,    43,
      47,    33,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const mimetypes_yytype_uint8 mimetypes_yyr1[] =
{
       0,    14,    15,    16,    16,    17,    17,    17,    18,    18,
      19,    20,    20,    20,    20,    21,    21,    21,    21,    22,
      22,    23,    24,    24,    25
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const mimetypes_yytype_uint8 mimetypes_yyr2[] =
{
       0,     2,     1,     1,     3,     0,     2,     2,     1,     2,
       3,     1,     2,     3,     3,     2,     3,     1,     1,     1,
       1,     3,     1,     3,     1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const mimetypes_yytype_uint8 mimetypes_yydefact[] =
{
       0,     0,     0,     0,     2,     3,     0,     8,     7,     0,
       1,     0,    20,     0,    19,     0,     0,     6,    11,    17,
      18,     9,    10,     4,    24,     0,    22,    15,     0,     0,
       0,    12,     0,    21,    16,    13,    14,    23
};

/* YYDEFGOTO[NTERM-NUM].  */
static const mimetypes_yytype_int8 mimetypes_yydefgoto[] =
{
      -1,     3,     4,     5,     8,     6,    31,    18,    19,    20,
      25,    26
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -13
static const mimetypes_yytype_int8 mimetypes_yypact[] =
{
      15,    -4,    -3,    17,    -4,   -13,    35,   -13,    16,     3,
     -13,    48,   -13,    47,   -13,    35,    35,    22,   -13,   -13,
     -13,   -13,   -13,   -13,   -13,     6,   -13,   -13,     0,    35,
      35,    32,    47,   -13,   -13,    32,    35,   -13
};

/* YYPGOTO[NTERM-NUM].  */
static const mimetypes_yytype_int8 mimetypes_yypgoto[] =
{
     -13,   -13,   -13,    18,    24,   -13,    -6,    27,   -12,   -13,
     -13,    13
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -6
static const mimetypes_yytype_int8 mimetypes_yytable[] =
{
      17,    24,     7,    12,    13,    14,    22,     9,    29,    30,
      28,    15,    16,    34,    32,    -5,     1,    10,     2,    33,
      24,    -5,    21,    35,    36,    12,    13,    14,    11,    23,
      29,    30,     0,    15,    16,    12,    13,    14,    12,    13,
      14,    30,    27,    15,    16,    37,    15,    16,    -5,     1,
      12,     2,    14,     0,    21
};

#define mimetypes_yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-13)))

#define mimetypes_yytable_value_is_error(Yytable_value) \
  YYID (0)

static const mimetypes_yytype_int8 mimetypes_yycheck[] =
{
       6,    13,     6,     3,     4,     5,     3,    10,     8,     9,
      16,    11,    12,    13,     8,     0,     1,     0,     3,    13,
      32,     6,     6,    29,    30,     3,     4,     5,     4,    11,
       8,     9,    -1,    11,    12,     3,     4,     5,     3,     4,
       5,     9,    15,    11,    12,    32,    11,    12,     0,     1,
       3,     3,     5,    -1,     6
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const mimetypes_yytype_uint8 mimetypes_yystos[] =
{
       0,     1,     3,    15,    16,    17,    19,     6,    18,    10,
       0,    18,     3,     4,     5,    11,    12,    20,    21,    22,
      23,     6,     3,    17,    22,    24,    25,    21,    20,     8,
       9,    20,     8,    13,    13,    20,    20,    25
};

#define mimetypes_yyerrok		(mimetypes_yyerrstatus = 0)
#define mimetypes_yyclearin	(mimetypes_yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto mimetypes_yyacceptlab
#define YYABORT		goto mimetypes_yyabortlab
#define YYERROR		goto mimetypes_yyerrorlab


/* Like YYERROR except do call mimetypes_yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto mimetypes_yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!mimetypes_yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (mimetypes_yychar == YYEMPTY)                                        \
    {                                                           \
      mimetypes_yychar = (Token);                                         \
      mimetypes_yylval = (Value);                                         \
      YYPOPSTACK (mimetypes_yylen);                                       \
      mimetypes_yystate = *mimetypes_yyssp;                                         \
      goto mimetypes_yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      mimetypes_yyerror (YY_("syntax error: cannot back up")); \
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


/* YYLEX -- calling `mimetypes_yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX mimetypes_yylex (YYLEX_PARAM)
#else
# define YYLEX mimetypes_yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (mimetypes_yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (mimetypes_yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      mimetypes_yy_symbol_print (stderr,						  \
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
mimetypes_yy_symbol_value_print (FILE *mimetypes_yyoutput, int mimetypes_yytype, YYSTYPE const * const mimetypes_yyvaluep)
#else
static void
mimetypes_yy_symbol_value_print (mimetypes_yyoutput, mimetypes_yytype, mimetypes_yyvaluep)
    FILE *mimetypes_yyoutput;
    int mimetypes_yytype;
    YYSTYPE const * const mimetypes_yyvaluep;
#endif
{
  FILE *mimetypes_yyo = mimetypes_yyoutput;
  YYUSE (mimetypes_yyo);
  if (!mimetypes_yyvaluep)
    return;
# ifdef YYPRINT
  if (mimetypes_yytype < YYNTOKENS)
    YYPRINT (mimetypes_yyoutput, mimetypes_yytoknum[mimetypes_yytype], *mimetypes_yyvaluep);
# else
  YYUSE (mimetypes_yyoutput);
# endif
  switch (mimetypes_yytype)
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
mimetypes_yy_symbol_print (FILE *mimetypes_yyoutput, int mimetypes_yytype, YYSTYPE const * const mimetypes_yyvaluep)
#else
static void
mimetypes_yy_symbol_print (mimetypes_yyoutput, mimetypes_yytype, mimetypes_yyvaluep)
    FILE *mimetypes_yyoutput;
    int mimetypes_yytype;
    YYSTYPE const * const mimetypes_yyvaluep;
#endif
{
  if (mimetypes_yytype < YYNTOKENS)
    YYFPRINTF (mimetypes_yyoutput, "token %s (", mimetypes_yytname[mimetypes_yytype]);
  else
    YYFPRINTF (mimetypes_yyoutput, "nterm %s (", mimetypes_yytname[mimetypes_yytype]);

  mimetypes_yy_symbol_value_print (mimetypes_yyoutput, mimetypes_yytype, mimetypes_yyvaluep);
  YYFPRINTF (mimetypes_yyoutput, ")");
}

/*------------------------------------------------------------------.
| mimetypes_yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
mimetypes_yy_stack_print (mimetypes_yytype_int16 *mimetypes_yybottom, mimetypes_yytype_int16 *mimetypes_yytop)
#else
static void
mimetypes_yy_stack_print (mimetypes_yybottom, mimetypes_yytop)
    mimetypes_yytype_int16 *mimetypes_yybottom;
    mimetypes_yytype_int16 *mimetypes_yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; mimetypes_yybottom <= mimetypes_yytop; mimetypes_yybottom++)
    {
      int mimetypes_yybot = *mimetypes_yybottom;
      YYFPRINTF (stderr, " %d", mimetypes_yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (mimetypes_yydebug)							\
    mimetypes_yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
mimetypes_yy_reduce_print (YYSTYPE *mimetypes_yyvsp, int mimetypes_yyrule)
#else
static void
mimetypes_yy_reduce_print (mimetypes_yyvsp, mimetypes_yyrule)
    YYSTYPE *mimetypes_yyvsp;
    int mimetypes_yyrule;
#endif
{
  int mimetypes_yynrhs = mimetypes_yyr2[mimetypes_yyrule];
  int mimetypes_yyi;
  unsigned long int mimetypes_yylno = mimetypes_yyrline[mimetypes_yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     mimetypes_yyrule - 1, mimetypes_yylno);
  /* The symbols being reduced.  */
  for (mimetypes_yyi = 0; mimetypes_yyi < mimetypes_yynrhs; mimetypes_yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", mimetypes_yyi + 1);
      mimetypes_yy_symbol_print (stderr, mimetypes_yyrhs[mimetypes_yyprhs[mimetypes_yyrule] + mimetypes_yyi],
		       &(mimetypes_yyvsp[(mimetypes_yyi + 1) - (mimetypes_yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (mimetypes_yydebug)				\
    mimetypes_yy_reduce_print (mimetypes_yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int mimetypes_yydebug;
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

# ifndef mimetypes_yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define mimetypes_yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
mimetypes_yystrlen (const char *mimetypes_yystr)
#else
static YYSIZE_T
mimetypes_yystrlen (mimetypes_yystr)
    const char *mimetypes_yystr;
#endif
{
  YYSIZE_T mimetypes_yylen;
  for (mimetypes_yylen = 0; mimetypes_yystr[mimetypes_yylen]; mimetypes_yylen++)
    continue;
  return mimetypes_yylen;
}
#  endif
# endif

# ifndef mimetypes_yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define mimetypes_yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
mimetypes_yystpcpy (char *mimetypes_yydest, const char *mimetypes_yysrc)
#else
static char *
mimetypes_yystpcpy (mimetypes_yydest, mimetypes_yysrc)
    char *mimetypes_yydest;
    const char *mimetypes_yysrc;
#endif
{
  char *mimetypes_yyd = mimetypes_yydest;
  const char *mimetypes_yys = mimetypes_yysrc;

  while ((*mimetypes_yyd++ = *mimetypes_yys++) != '\0')
    continue;

  return mimetypes_yyd - 1;
}
#  endif
# endif

# ifndef mimetypes_yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for mimetypes_yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from mimetypes_yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
mimetypes_yytnamerr (char *mimetypes_yyres, const char *mimetypes_yystr)
{
  if (*mimetypes_yystr == '"')
    {
      YYSIZE_T mimetypes_yyn = 0;
      char const *mimetypes_yyp = mimetypes_yystr;

      for (;;)
	switch (*++mimetypes_yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++mimetypes_yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (mimetypes_yyres)
	      mimetypes_yyres[mimetypes_yyn] = *mimetypes_yyp;
	    mimetypes_yyn++;
	    break;

	  case '"':
	    if (mimetypes_yyres)
	      mimetypes_yyres[mimetypes_yyn] = '\0';
	    return mimetypes_yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! mimetypes_yyres)
    return mimetypes_yystrlen (mimetypes_yystr);

  return mimetypes_yystpcpy (mimetypes_yyres, mimetypes_yystr) - mimetypes_yyres;
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
mimetypes_yysyntax_error (YYSIZE_T *mimetypes_yymsg_alloc, char **mimetypes_yymsg,
                mimetypes_yytype_int16 *mimetypes_yyssp, int mimetypes_yytoken)
{
  YYSIZE_T mimetypes_yysize0 = mimetypes_yytnamerr (YY_NULL, mimetypes_yytname[mimetypes_yytoken]);
  YYSIZE_T mimetypes_yysize = mimetypes_yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *mimetypes_yyformat = YY_NULL;
  /* Arguments of mimetypes_yyformat. */
  char const *mimetypes_yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int mimetypes_yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in mimetypes_yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated mimetypes_yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (mimetypes_yytoken != YYEMPTY)
    {
      int mimetypes_yyn = mimetypes_yypact[*mimetypes_yyssp];
      mimetypes_yyarg[mimetypes_yycount++] = mimetypes_yytname[mimetypes_yytoken];
      if (!mimetypes_yypact_value_is_default (mimetypes_yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int mimetypes_yyxbegin = mimetypes_yyn < 0 ? -mimetypes_yyn : 0;
          /* Stay within bounds of both mimetypes_yycheck and mimetypes_yytname.  */
          int mimetypes_yychecklim = YYLAST - mimetypes_yyn + 1;
          int mimetypes_yyxend = mimetypes_yychecklim < YYNTOKENS ? mimetypes_yychecklim : YYNTOKENS;
          int mimetypes_yyx;

          for (mimetypes_yyx = mimetypes_yyxbegin; mimetypes_yyx < mimetypes_yyxend; ++mimetypes_yyx)
            if (mimetypes_yycheck[mimetypes_yyx + mimetypes_yyn] == mimetypes_yyx && mimetypes_yyx != YYTERROR
                && !mimetypes_yytable_value_is_error (mimetypes_yytable[mimetypes_yyx + mimetypes_yyn]))
              {
                if (mimetypes_yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    mimetypes_yycount = 1;
                    mimetypes_yysize = mimetypes_yysize0;
                    break;
                  }
                mimetypes_yyarg[mimetypes_yycount++] = mimetypes_yytname[mimetypes_yyx];
                {
                  YYSIZE_T mimetypes_yysize1 = mimetypes_yysize + mimetypes_yytnamerr (YY_NULL, mimetypes_yytname[mimetypes_yyx]);
                  if (! (mimetypes_yysize <= mimetypes_yysize1
                         && mimetypes_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  mimetypes_yysize = mimetypes_yysize1;
                }
              }
        }
    }

  switch (mimetypes_yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        mimetypes_yyformat = S;                       \
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
    YYSIZE_T mimetypes_yysize1 = mimetypes_yysize + mimetypes_yystrlen (mimetypes_yyformat);
    if (! (mimetypes_yysize <= mimetypes_yysize1 && mimetypes_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    mimetypes_yysize = mimetypes_yysize1;
  }

  if (*mimetypes_yymsg_alloc < mimetypes_yysize)
    {
      *mimetypes_yymsg_alloc = 2 * mimetypes_yysize;
      if (! (mimetypes_yysize <= *mimetypes_yymsg_alloc
             && *mimetypes_yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *mimetypes_yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *mimetypes_yyp = *mimetypes_yymsg;
    int mimetypes_yyi = 0;
    while ((*mimetypes_yyp = *mimetypes_yyformat) != '\0')
      if (*mimetypes_yyp == '%' && mimetypes_yyformat[1] == 's' && mimetypes_yyi < mimetypes_yycount)
        {
          mimetypes_yyp += mimetypes_yytnamerr (mimetypes_yyp, mimetypes_yyarg[mimetypes_yyi++]);
          mimetypes_yyformat += 2;
        }
      else
        {
          mimetypes_yyp++;
          mimetypes_yyformat++;
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
mimetypes_yydestruct (const char *mimetypes_yymsg, int mimetypes_yytype, YYSTYPE *mimetypes_yyvaluep)
#else
static void
mimetypes_yydestruct (mimetypes_yymsg, mimetypes_yytype, mimetypes_yyvaluep)
    const char *mimetypes_yymsg;
    int mimetypes_yytype;
    YYSTYPE *mimetypes_yyvaluep;
#endif
{
  YYUSE (mimetypes_yyvaluep);

  if (!mimetypes_yymsg)
    mimetypes_yymsg = "Deleting";
  YY_SYMBOL_PRINT (mimetypes_yymsg, mimetypes_yytype, mimetypes_yyvaluep, mimetypes_yylocationp);

  switch (mimetypes_yytype)
    {

      default:
        break;
    }
}




/* The lookahead symbol.  */
int mimetypes_yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE mimetypes_yylval YY_INITIAL_VALUE(mimetypes_yyval_default);

/* Number of syntax errors so far.  */
int mimetypes_yynerrs;


/*----------.
| mimetypes_yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
mimetypes_yyparse (void *YYPARSE_PARAM)
#else
int
mimetypes_yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
mimetypes_yyparse (void)
#else
int
mimetypes_yyparse ()

#endif
#endif
{
    int mimetypes_yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int mimetypes_yyerrstatus;

    /* The stacks and their tools:
       `mimetypes_yyss': related to states.
       `mimetypes_yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow mimetypes_yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    mimetypes_yytype_int16 mimetypes_yyssa[YYINITDEPTH];
    mimetypes_yytype_int16 *mimetypes_yyss;
    mimetypes_yytype_int16 *mimetypes_yyssp;

    /* The semantic value stack.  */
    YYSTYPE mimetypes_yyvsa[YYINITDEPTH];
    YYSTYPE *mimetypes_yyvs;
    YYSTYPE *mimetypes_yyvsp;

    YYSIZE_T mimetypes_yystacksize;

  int mimetypes_yyn;
  int mimetypes_yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int mimetypes_yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE mimetypes_yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char mimetypes_yymsgbuf[128];
  char *mimetypes_yymsg = mimetypes_yymsgbuf;
  YYSIZE_T mimetypes_yymsg_alloc = sizeof mimetypes_yymsgbuf;
#endif

#define YYPOPSTACK(N)   (mimetypes_yyvsp -= (N), mimetypes_yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int mimetypes_yylen = 0;

  mimetypes_yyssp = mimetypes_yyss = mimetypes_yyssa;
  mimetypes_yyvsp = mimetypes_yyvs = mimetypes_yyvsa;
  mimetypes_yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  mimetypes_yystate = 0;
  mimetypes_yyerrstatus = 0;
  mimetypes_yynerrs = 0;
  mimetypes_yychar = YYEMPTY; /* Cause a token to be read.  */
  goto mimetypes_yysetstate;

/*------------------------------------------------------------.
| mimetypes_yynewstate -- Push a new state, which is found in mimetypes_yystate.  |
`------------------------------------------------------------*/
 mimetypes_yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  mimetypes_yyssp++;

 mimetypes_yysetstate:
  *mimetypes_yyssp = mimetypes_yystate;

  if (mimetypes_yyss + mimetypes_yystacksize - 1 <= mimetypes_yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T mimetypes_yysize = mimetypes_yyssp - mimetypes_yyss + 1;

#ifdef mimetypes_yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *mimetypes_yyvs1 = mimetypes_yyvs;
	mimetypes_yytype_int16 *mimetypes_yyss1 = mimetypes_yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if mimetypes_yyoverflow is a macro.  */
	mimetypes_yyoverflow (YY_("memory exhausted"),
		    &mimetypes_yyss1, mimetypes_yysize * sizeof (*mimetypes_yyssp),
		    &mimetypes_yyvs1, mimetypes_yysize * sizeof (*mimetypes_yyvsp),
		    &mimetypes_yystacksize);

	mimetypes_yyss = mimetypes_yyss1;
	mimetypes_yyvs = mimetypes_yyvs1;
      }
#else /* no mimetypes_yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto mimetypes_yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= mimetypes_yystacksize)
	goto mimetypes_yyexhaustedlab;
      mimetypes_yystacksize *= 2;
      if (YYMAXDEPTH < mimetypes_yystacksize)
	mimetypes_yystacksize = YYMAXDEPTH;

      {
	mimetypes_yytype_int16 *mimetypes_yyss1 = mimetypes_yyss;
	union mimetypes_yyalloc *mimetypes_yyptr =
	  (union mimetypes_yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (mimetypes_yystacksize));
	if (! mimetypes_yyptr)
	  goto mimetypes_yyexhaustedlab;
	YYSTACK_RELOCATE (mimetypes_yyss_alloc, mimetypes_yyss);
	YYSTACK_RELOCATE (mimetypes_yyvs_alloc, mimetypes_yyvs);
#  undef YYSTACK_RELOCATE
	if (mimetypes_yyss1 != mimetypes_yyssa)
	  YYSTACK_FREE (mimetypes_yyss1);
      }
# endif
#endif /* no mimetypes_yyoverflow */

      mimetypes_yyssp = mimetypes_yyss + mimetypes_yysize - 1;
      mimetypes_yyvsp = mimetypes_yyvs + mimetypes_yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) mimetypes_yystacksize));

      if (mimetypes_yyss + mimetypes_yystacksize - 1 <= mimetypes_yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", mimetypes_yystate));

  if (mimetypes_yystate == YYFINAL)
    YYACCEPT;

  goto mimetypes_yybackup;

/*-----------.
| mimetypes_yybackup.  |
`-----------*/
mimetypes_yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  mimetypes_yyn = mimetypes_yypact[mimetypes_yystate];
  if (mimetypes_yypact_value_is_default (mimetypes_yyn))
    goto mimetypes_yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (mimetypes_yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      mimetypes_yychar = YYLEX;
    }

  if (mimetypes_yychar <= YYEOF)
    {
      mimetypes_yychar = mimetypes_yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      mimetypes_yytoken = YYTRANSLATE (mimetypes_yychar);
      YY_SYMBOL_PRINT ("Next token is", mimetypes_yytoken, &mimetypes_yylval, &mimetypes_yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  mimetypes_yyn += mimetypes_yytoken;
  if (mimetypes_yyn < 0 || YYLAST < mimetypes_yyn || mimetypes_yycheck[mimetypes_yyn] != mimetypes_yytoken)
    goto mimetypes_yydefault;
  mimetypes_yyn = mimetypes_yytable[mimetypes_yyn];
  if (mimetypes_yyn <= 0)
    {
      if (mimetypes_yytable_value_is_error (mimetypes_yyn))
        goto mimetypes_yyerrlab;
      mimetypes_yyn = -mimetypes_yyn;
      goto mimetypes_yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (mimetypes_yyerrstatus)
    mimetypes_yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", mimetypes_yytoken, &mimetypes_yylval, &mimetypes_yylloc);

  /* Discard the shifted token.  */
  mimetypes_yychar = YYEMPTY;

  mimetypes_yystate = mimetypes_yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++mimetypes_yyvsp = mimetypes_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto mimetypes_yynewstate;


/*-----------------------------------------------------------.
| mimetypes_yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
mimetypes_yydefault:
  mimetypes_yyn = mimetypes_yydefact[mimetypes_yystate];
  if (mimetypes_yyn == 0)
    goto mimetypes_yyerrlab;
  goto mimetypes_yyreduce;


/*-----------------------------.
| mimetypes_yyreduce -- Do a reduction.  |
`-----------------------------*/
mimetypes_yyreduce:
  /* mimetypes_yyn is the number of a rule to reduce with.  */
  mimetypes_yylen = mimetypes_yyr2[mimetypes_yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  mimetypes_yyval = mimetypes_yyvsp[1-mimetypes_yylen];


  YY_REDUCE_PRINT (mimetypes_yyn);
  switch (mimetypes_yyn)
    {
        case 6:
/* Line 1792 of yacc.c  */
#line 137 "mimetypes.y"
    {
	     struct rule_tab *p = mimetypes_malloc (sizeof (*p));
	     if (!rule_list)
	       mu_list_create (&rule_list);
	     p->type = (mimetypes_yyvsp[(1) - (2)].string).ptr;
	     p->node = (mimetypes_yyvsp[(2) - (2)].node);
	     mu_list_append (rule_list, p);
	   }
    break;

  case 7:
/* Line 1792 of yacc.c  */
#line 146 "mimetypes.y"
    {
	     if (arg_list)
	       mu_list_destroy (&arg_list);
	     arg_list = NULL;
	     reset_lex ();
	   }
    break;

  case 10:
/* Line 1792 of yacc.c  */
#line 159 "mimetypes.y"
    {
	     (mimetypes_yyval.string) = mimetypes_append_string2 (&(mimetypes_yyvsp[(1) - (3)].string), '/', &(mimetypes_yyvsp[(3) - (3)].string));
	   }
    break;

  case 12:
/* Line 1792 of yacc.c  */
#line 166 "mimetypes.y"
    {
	     (mimetypes_yyval.node) = make_binary_node (L_OR, (mimetypes_yyvsp[(1) - (2)].node), (mimetypes_yyvsp[(2) - (2)].node));
	   }
    break;

  case 13:
/* Line 1792 of yacc.c  */
#line 170 "mimetypes.y"
    {
	     (mimetypes_yyval.node) = make_binary_node (L_OR, (mimetypes_yyvsp[(1) - (3)].node), (mimetypes_yyvsp[(3) - (3)].node));
	   }
    break;

  case 14:
/* Line 1792 of yacc.c  */
#line 174 "mimetypes.y"
    {
	     (mimetypes_yyval.node) = make_binary_node (L_AND, (mimetypes_yyvsp[(1) - (3)].node), (mimetypes_yyvsp[(3) - (3)].node));
	   }
    break;

  case 15:
/* Line 1792 of yacc.c  */
#line 180 "mimetypes.y"
    {
	     (mimetypes_yyval.node) = make_negation_node ((mimetypes_yyvsp[(2) - (2)].node));
	   }
    break;

  case 16:
/* Line 1792 of yacc.c  */
#line 184 "mimetypes.y"
    {
	     (mimetypes_yyval.node) = (mimetypes_yyvsp[(2) - (3)].node);
	   }
    break;

  case 17:
/* Line 1792 of yacc.c  */
#line 188 "mimetypes.y"
    {
	     (mimetypes_yyval.node) = make_suffix_node (&(mimetypes_yyvsp[(1) - (1)].string));
	   }
    break;

  case 21:
/* Line 1792 of yacc.c  */
#line 199 "mimetypes.y"
    {
	     reset_lex ();
	     (mimetypes_yyval.node) = make_functional_node ((mimetypes_yyvsp[(1) - (3)].string).ptr, (mimetypes_yyvsp[(2) - (3)].list));
	     if (!(mimetypes_yyval.node))
	       YYERROR;
	   }
    break;

  case 22:
/* Line 1792 of yacc.c  */
#line 208 "mimetypes.y"
    {
	     mu_list_create (&arg_list);
	     (mimetypes_yyval.list) = arg_list;
	     mu_list_append ((mimetypes_yyval.list), mimetypes_string_dup (&(mimetypes_yyvsp[(1) - (1)].string)));
	   }
    break;

  case 23:
/* Line 1792 of yacc.c  */
#line 214 "mimetypes.y"
    {
	     mu_list_append ((mimetypes_yyvsp[(1) - (3)].list), mimetypes_string_dup (&(mimetypes_yyvsp[(3) - (3)].string)));
	     (mimetypes_yyval.list) = (mimetypes_yyvsp[(1) - (3)].list);
	   }
    break;


/* Line 1792 of yacc.c  */
#line 1592 "mimetypes-gram.c"
      default: break;
    }
  /* User semantic actions sometimes alter mimetypes_yychar, and that requires
     that mimetypes_yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of mimetypes_yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering mimetypes_yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", mimetypes_yyr1[mimetypes_yyn], &mimetypes_yyval, &mimetypes_yyloc);

  YYPOPSTACK (mimetypes_yylen);
  mimetypes_yylen = 0;
  YY_STACK_PRINT (mimetypes_yyss, mimetypes_yyssp);

  *++mimetypes_yyvsp = mimetypes_yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  mimetypes_yyn = mimetypes_yyr1[mimetypes_yyn];

  mimetypes_yystate = mimetypes_yypgoto[mimetypes_yyn - YYNTOKENS] + *mimetypes_yyssp;
  if (0 <= mimetypes_yystate && mimetypes_yystate <= YYLAST && mimetypes_yycheck[mimetypes_yystate] == *mimetypes_yyssp)
    mimetypes_yystate = mimetypes_yytable[mimetypes_yystate];
  else
    mimetypes_yystate = mimetypes_yydefgoto[mimetypes_yyn - YYNTOKENS];

  goto mimetypes_yynewstate;


/*------------------------------------.
| mimetypes_yyerrlab -- here on detecting error |
`------------------------------------*/
mimetypes_yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  mimetypes_yytoken = mimetypes_yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (mimetypes_yychar);

  /* If not already recovering from an error, report this error.  */
  if (!mimetypes_yyerrstatus)
    {
      ++mimetypes_yynerrs;
#if ! YYERROR_VERBOSE
      mimetypes_yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR mimetypes_yysyntax_error (&mimetypes_yymsg_alloc, &mimetypes_yymsg, \
                                        mimetypes_yyssp, mimetypes_yytoken)
      {
        char const *mimetypes_yymsgp = YY_("syntax error");
        int mimetypes_yysyntax_error_status;
        mimetypes_yysyntax_error_status = YYSYNTAX_ERROR;
        if (mimetypes_yysyntax_error_status == 0)
          mimetypes_yymsgp = mimetypes_yymsg;
        else if (mimetypes_yysyntax_error_status == 1)
          {
            if (mimetypes_yymsg != mimetypes_yymsgbuf)
              YYSTACK_FREE (mimetypes_yymsg);
            mimetypes_yymsg = (char *) YYSTACK_ALLOC (mimetypes_yymsg_alloc);
            if (!mimetypes_yymsg)
              {
                mimetypes_yymsg = mimetypes_yymsgbuf;
                mimetypes_yymsg_alloc = sizeof mimetypes_yymsgbuf;
                mimetypes_yysyntax_error_status = 2;
              }
            else
              {
                mimetypes_yysyntax_error_status = YYSYNTAX_ERROR;
                mimetypes_yymsgp = mimetypes_yymsg;
              }
          }
        mimetypes_yyerror (mimetypes_yymsgp);
        if (mimetypes_yysyntax_error_status == 2)
          goto mimetypes_yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (mimetypes_yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (mimetypes_yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (mimetypes_yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  mimetypes_yydestruct ("Error: discarding",
		      mimetypes_yytoken, &mimetypes_yylval);
	  mimetypes_yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto mimetypes_yyerrlab1;


/*---------------------------------------------------.
| mimetypes_yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
mimetypes_yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label mimetypes_yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto mimetypes_yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (mimetypes_yylen);
  mimetypes_yylen = 0;
  YY_STACK_PRINT (mimetypes_yyss, mimetypes_yyssp);
  mimetypes_yystate = *mimetypes_yyssp;
  goto mimetypes_yyerrlab1;


/*-------------------------------------------------------------.
| mimetypes_yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
mimetypes_yyerrlab1:
  mimetypes_yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      mimetypes_yyn = mimetypes_yypact[mimetypes_yystate];
      if (!mimetypes_yypact_value_is_default (mimetypes_yyn))
	{
	  mimetypes_yyn += YYTERROR;
	  if (0 <= mimetypes_yyn && mimetypes_yyn <= YYLAST && mimetypes_yycheck[mimetypes_yyn] == YYTERROR)
	    {
	      mimetypes_yyn = mimetypes_yytable[mimetypes_yyn];
	      if (0 < mimetypes_yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (mimetypes_yyssp == mimetypes_yyss)
	YYABORT;


      mimetypes_yydestruct ("Error: popping",
		  mimetypes_yystos[mimetypes_yystate], mimetypes_yyvsp);
      YYPOPSTACK (1);
      mimetypes_yystate = *mimetypes_yyssp;
      YY_STACK_PRINT (mimetypes_yyss, mimetypes_yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++mimetypes_yyvsp = mimetypes_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", mimetypes_yystos[mimetypes_yyn], mimetypes_yyvsp, mimetypes_yylsp);

  mimetypes_yystate = mimetypes_yyn;
  goto mimetypes_yynewstate;


/*-------------------------------------.
| mimetypes_yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
mimetypes_yyacceptlab:
  mimetypes_yyresult = 0;
  goto mimetypes_yyreturn;

/*-----------------------------------.
| mimetypes_yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
mimetypes_yyabortlab:
  mimetypes_yyresult = 1;
  goto mimetypes_yyreturn;

#if !defined mimetypes_yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| mimetypes_yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
mimetypes_yyexhaustedlab:
  mimetypes_yyerror (YY_("memory exhausted"));
  mimetypes_yyresult = 2;
  /* Fall through.  */
#endif

mimetypes_yyreturn:
  if (mimetypes_yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      mimetypes_yytoken = YYTRANSLATE (mimetypes_yychar);
      mimetypes_yydestruct ("Cleanup: discarding lookahead",
                  mimetypes_yytoken, &mimetypes_yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (mimetypes_yylen);
  YY_STACK_PRINT (mimetypes_yyss, mimetypes_yyssp);
  while (mimetypes_yyssp != mimetypes_yyss)
    {
      mimetypes_yydestruct ("Cleanup: popping",
		  mimetypes_yystos[*mimetypes_yyssp], mimetypes_yyvsp);
      YYPOPSTACK (1);
    }
#ifndef mimetypes_yyoverflow
  if (mimetypes_yyss != mimetypes_yyssa)
    YYSTACK_FREE (mimetypes_yyss);
#endif
#if YYERROR_VERBOSE
  if (mimetypes_yymsg != mimetypes_yymsgbuf)
    YYSTACK_FREE (mimetypes_yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (mimetypes_yyresult);
}


/* Line 2055 of yacc.c  */
#line 223 "mimetypes.y"


int
mimetypes_parse (const char *name)
{
  int rc;
  if (mimetypes_open (name))
    return 1;
  rc = mimetypes_yyparse ();
  mimetypes_close ();
  return rule_list == NULL;
}
  
void
mimetypes_gram_debug (int level)
{
  mimetypes_yydebug = level;
}


static struct node *
make_node (enum node_type type)
{
  struct node *p = mimetypes_malloc (sizeof *p);
  p->type = type;
  return p;
}

static struct node *
make_binary_node (int op, struct node *left, struct node *right)
{
  struct node *node = make_node (binary_node);

  node->v.bin.op = op;
  node->v.bin.arg1 = left;
  node->v.bin.arg2 = right;
  return node;
}

struct node *
make_negation_node (struct node *p)
{
  struct node *node = make_node (negation_node);
  node->v.arg = p;
  return node;
}

struct node *
make_suffix_node (struct mimetypes_string *suffix)
{
  struct node *node = make_node (suffix_node);
  node->v.suffix = *suffix;
  return node;
}

struct builtin_tab
{
  char *name;
  char *args;
  builtin_t handler;
};

/*        match("pattern")
            Pattern match on filename
*/
static int
b_match (union argument *args)
{
  return fnmatch (args[0].string->ptr, mimeview_file, 0) == 0;
}

/*       ascii(offset,length)
            True if bytes are valid printable ASCII (CR, NL, TAB,
            BS, 32-126)
*/
static int
b_ascii (union argument *args)
{
  int i;
  int rc;

  rc = mu_stream_seek (mimeview_stream, args[0].number, MU_SEEK_SET, NULL);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_seek", NULL, rc);
      return 0;
    }

  for (i = 0; i < args[1].number; i++)
    {
      char c;
      size_t n;

      rc = mu_stream_read (mimeview_stream, &c, 1, &n);
      if (rc || n == 0)
	break;
      if (!mu_isascii (c))
	return 0;
    }
      
  return 1;
}

/*       printable(offset,length)
            True if bytes are printable 8-bit chars (CR, NL, TAB,
            BS, 32-126, 128-254)
*/
#define ISPRINT(c) ((c) &&\
                    (strchr ("\n\r\t\b",c) \
                     || (32<=(c) && (c)<=126) \
                     || (128<=(c) && (c)<=254)))
static int
b_printable (union argument *args)
{
  int i;
  int rc;

  rc = mu_stream_seek (mimeview_stream, args[0].number, MU_SEEK_SET, NULL);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_seek", NULL, rc);
      return 0;
    }

  for (i = 0; i < args[1].number; i++)
    {
      char c;
      size_t n;

      rc = mu_stream_read (mimeview_stream, &c, 1, &n);
      if (rc || n == 0)
	break;
      if (!ISPRINT ((unsigned)c))
	return 0;
    }
  return 1;
}

/*        string(offset,"string")
            True if bytes are identical to string
*/
static int
b_string (union argument *args)
{
  struct mimetypes_string *str = args[1].string;
  int i;
  int rc;
  
  rc = mu_stream_seek (mimeview_stream, args[0].number, MU_SEEK_SET, NULL);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_seek", NULL, rc);
      return 0;
    }

  for (i = 0; i < str->len; i++)
    {
      char c;
      size_t n;

      rc = mu_stream_read (mimeview_stream, &c, 1, &n);
      if (rc || n == 0 || c != str->ptr[i])
	return 0;
    }
  return 1;
}

/*        istring(offset,"string")
            True if a case-insensitive comparison of the bytes is
            identical
*/
static int
b_istring (union argument *args)
{
  int i;
  struct mimetypes_string *str = args[1].string;
  
  int rc;

  rc = mu_stream_seek (mimeview_stream, args[0].number, MU_SEEK_SET, NULL);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_seek", NULL, rc);
      return 0;
    }

  for (i = 0; i < str->len; i++)
    {
      char c;
      size_t n;

      rc = mu_stream_read (mimeview_stream, &c, 1, &n);
      if (rc || n == 0 || mu_tolower (c) != mu_tolower (str->ptr[i]))
	return 0;
    }
  return 1;
}

int
compare_bytes (union argument *args, void *sample, void *buf, size_t size)
{
  int rc;
  size_t n;
  
  rc = mu_stream_seek (mimeview_stream, args[0].number, MU_SEEK_SET, NULL);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_seek", NULL, rc);
      return 0;
    }
  
  rc = mu_stream_read (mimeview_stream, buf, sizeof (buf), &n);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_read", NULL, rc);
      return 0;
    }
  else if (n != size)
    return 0;
  return memcmp (sample, buf, size) == 0;
}

/*       char(offset,value)
            True if byte is identical
*/
static int
b_char (union argument *args)
{
  char val = args[1].number;
  char buf;
  return compare_bytes (args, &val, &buf, sizeof (buf));
}

/*        short(offset,value)
            True if 16-bit integer is identical
	  FIXME: Byte order  
*/
static int
b_short (union argument *args)
{
  unsigned short val = args[1].number;
  unsigned short buf;
  return compare_bytes (args, &val, &buf, sizeof (buf));
}

/*        int(offset,value)
            True if 32-bit integer is identical
          FIXME: Byte order
*/
static int
b_int (union argument *args)
{
  unsigned int val = args[1].number;
  unsigned int buf;
  return compare_bytes (args, &val, &buf, sizeof (buf));
}

/*        locale("string")
            True if current locale matches string
*/
static int
b_locale (union argument *args)
{
  abort (); /* FIXME */
  return 0;
}

/*        contains(offset,range,"string")
            True if the range contains the string
*/
static int
b_contains (union argument *args)
{
  size_t i, count;
  char *buf;
  struct mimetypes_string *str = args[2].string;
  int rc;

  rc = mu_stream_seek (mimeview_stream, args[0].number, MU_SEEK_SET, NULL);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_seek", NULL, rc);
      return 0;
    }

  buf = mu_alloc (args[1].number);
  rc = mu_stream_read (mimeview_stream, buf, args[1].number, &count);
  if (count != args[1].number)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_read", NULL, rc);
    }
  else if (count > str->len)
    for (i = 0; i < count - str->len; i++)
      if (buf[i] == str->ptr[0] && memcmp (buf + i, str->ptr, str->len) == 0)
	{
	  free (buf);
	  return 1;
	}
  free (buf);
  return 0;
}

static struct builtin_tab builtin_tab[] = {
  { "match", "s", b_match },
  { "ascii", "dd", b_ascii },
  { "printable", "dd", b_printable },
  { "string", "ds", b_string },
  { "istring", "ds", b_istring },
  { "char", "dc", b_char },
  { "short", "dd", b_short },
  { "int", "dd", b_int },
  { "locale", "s", b_locale },
  { "contains", "dds", b_contains },
  { NULL }
};
  
struct node *
make_functional_node (char *ident, mu_list_t list)
{
  size_t count, i;
  struct builtin_tab *p;
  struct node *node;
  union argument *args;
  mu_iterator_t itr;
  
  for (p = builtin_tab; ; p++)
    {
      if (!p->name)
	{
	  char *s;
	  mu_asprintf (&s, _("%s: unknown function"), ident);
	  mimetypes_yyerror (s);
	  free (s);
	  return NULL;
	}

      if (strcmp (ident, p->name) == 0)
	break;
    }

  mu_list_count (list, &count);
  i = strlen (p->args);

  if (count < i)
    {
      char *s;
      mu_asprintf (&s, _("too few arguments in call to `%s'"), ident);
      mimetypes_yyerror (s);
      free (s);
      return NULL;
    }
  else if (count > i)
    {
      char *s;
      mu_asprintf (&s, _("too many arguments in call to `%s'"), ident);
      mimetypes_yyerror (s);
      free (s);
      return NULL;
    }

  args = mimetypes_malloc (count * sizeof *args);
  
  mu_list_get_iterator (list, &itr);
  for (i = 0, mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr), i++)
    {
      struct mimetypes_string *data;
      char *tmp;
      
      mu_iterator_current (itr, (void **)&data);
      switch (p->args[i])
	{
	case 'd':
	  args[i].number = strtoul (data->ptr, &tmp, 0);
	  if (*tmp)
	    goto err;
	  break;
	  
	case 's':
	  args[i].string = data;
	  break;
	  
	case 'c':
	  args[i].c = strtoul (data->ptr, &tmp, 0);
	  if (*tmp)
	    goto err;
	  break;
	  
	default:
	  abort ();
	}
    }

  node = make_node (functional_node);
  node->v.function.fun = p->handler;
  node->v.function.args = args;
  return node;
  
 err:
  {
    char *s;
    mu_asprintf (&s,
	         _("argument %lu has wrong type in call to `%s'"),
	         (unsigned long) i, ident);
    mimetypes_yyerror (s);
    free (s);
    return NULL;
  }
}

static int
check_suffix (char *suf)
{
  char *p = strrchr (mimeview_file, '.');
  if (!p)
    return 0;
  return strcmp (p+1, suf) == 0;
}

static int
eval_rule (struct node *root)
{
  int result;
  
  switch (root->type)
    {
    case functional_node:
      result = root->v.function.fun (root->v.function.args);
      break;
      
    case binary_node:
      result = eval_rule (root->v.bin.arg1);
      switch (root->v.bin.op)
	{
	case L_OR:
	  if (!result)
	    result |= eval_rule (root->v.bin.arg2);
	  break;
	  
	case L_AND:
	  if (result)
	    result &= eval_rule (root->v.bin.arg2);
	  break;
	  
	default:
	  abort ();
	}
      break;
      
    case negation_node:
      result = !eval_rule (root->v.arg);
      break;
      
    case suffix_node:
      result = check_suffix (root->v.suffix.ptr);
      break;

    default:
      abort ();
    }
  return result;
}

static int
evaluate (void *item, void *data)
{
  struct rule_tab *p = item;
  char **ptype = data;
    
  if (eval_rule (p->node))
    {
      *ptype = p->type;
      return 1;
    }
  return 0;
}

const char *
get_file_type ()
{
  const char *type = NULL;
  mu_list_foreach (rule_list, evaluate, &type);
  return type;
}
    

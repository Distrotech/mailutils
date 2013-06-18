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

/* All symbols defined below should begin with ali_yy or YY, to avoid
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
#line 1 "mh_alias.y"

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2007, 2010-2012 Free Software Foundation, Inc.

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
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
  
struct mh_alias
{
  char *name;
  mu_list_t rcpt_list;
  int inclusive;
};

static mu_list_t alias_list;

static mu_list_t
list_create_or_die ()
{
  int status;
  mu_list_t list;

  status = mu_list_create (&list);
  if (status)
    {
      ali_parse_error (_("can't create list: %s"), mu_strerror (status));
      exit (1);
    }
  return list;
}

static char *
ali_list_to_string (mu_list_t *plist)
{
  size_t n;
  char *string;
  
  mu_list_count (*plist, &n);
  if (n == 1)
    {
      mu_list_get (*plist, 0, (void **)&string);
    }
  else
    {
      char *p;
      size_t length = 0;
      mu_iterator_t itr;
      mu_list_get_iterator (*plist, &itr);
      for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next(itr))
	{
	  char *s;
	  mu_iterator_current (itr, (void**) &s);
	  length += strlen (s) + 1;
	}
  
      string = mu_alloc (length + 1);
      p = string;
      for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next(itr))
	{
	  char *s;
	  mu_iterator_current (itr, (void**) &s);
	  strcpy (p, s);
	  p += strlen (s);
	  *p++ = ' ';
	}
      *--p = 0;
      mu_iterator_destroy (&itr);
    }
  mu_list_destroy (plist);
  return string;
}

static mu_list_t unix_group_to_list (char *name);
static mu_list_t unix_gid_to_list (char *name);
static mu_list_t unix_passwd_to_list (void);

int ali_yyerror (char *s);
int ali_yylex (void);


/* Line 371 of yacc.c  */
#line 164 "mh_alias_gram.c"

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
extern int ali_yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum ali_yytokentype {
     STRING = 258
   };
#endif
/* Tokens.  */
#define STRING 258



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 387 of yacc.c  */
#line 97 "mh_alias.y"

  char *string;
  mu_list_t list;
  struct mh_alias *alias;


/* Line 387 of yacc.c  */
#line 220 "mh_alias_gram.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define ali_yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE ali_yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int ali_yyparse (void *YYPARSE_PARAM);
#else
int ali_yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int ali_yyparse (void);
#else
int ali_yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 248 "mh_alias_gram.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 ali_yytype_uint8;
#else
typedef unsigned char ali_yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 ali_yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char ali_yytype_int8;
#else
typedef short int ali_yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 ali_yytype_uint16;
#else
typedef unsigned short int ali_yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 ali_yytype_int16;
#else
typedef short int ali_yytype_int16;
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
YYID (int ali_yyi)
#else
static int
YYID (ali_yyi)
    int ali_yyi;
#endif
{
  return ali_yyi;
}
#endif

#if ! defined ali_yyoverflow || YYERROR_VERBOSE

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
#endif /* ! defined ali_yyoverflow || YYERROR_VERBOSE */


#if (! defined ali_yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union ali_yyalloc
{
  ali_yytype_int16 ali_yyss_alloc;
  YYSTYPE ali_yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union ali_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (ali_yytype_int16) + sizeof (YYSTYPE)) \
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
	YYSIZE_T ali_yynewbytes;						\
	YYCOPY (&ali_yyptr->Stack_alloc, Stack, ali_yysize);			\
	Stack = &ali_yyptr->Stack_alloc;					\
	ali_yynewbytes = ali_yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	ali_yyptr += ali_yynewbytes / sizeof (*ali_yyptr);				\
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
          YYSIZE_T ali_yyi;                         \
          for (ali_yyi = 0; ali_yyi < (Count); ali_yyi++)   \
            (Dst)[ali_yyi] = (Src)[ali_yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  9
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   25

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  11
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  11
/* YYNRULES -- Number of rules.  */
#define YYNRULES  23
/* YYNRULES -- Number of states.  */
#define YYNSTATES  31

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   258

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? ali_yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const ali_yytype_uint8 ali_yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       4,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     9,     8,    10,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     5,     6,
       2,     7,     2,     2,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,     3
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const ali_yytype_uint8 ali_yyprhs[] =
{
       0,     0,     3,     4,     6,     9,    12,    16,    18,    22,
      24,    27,    28,    33,    34,    39,    41,    44,    47,    49,
      51,    55,    57,    59
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const ali_yytype_int8 ali_yyrhs[] =
{
      12,     0,    -1,    -1,    13,    -1,    13,    14,    -1,    14,
      13,    -1,    14,    13,    14,    -1,    15,    -1,    13,    14,
      15,    -1,     4,    -1,    14,     4,    -1,    -1,     3,     5,
      16,    18,    -1,    -1,     3,     6,    17,    18,    -1,    19,
      -1,     7,     3,    -1,     8,     3,    -1,     9,    -1,    20,
      -1,    19,    10,    20,    -1,    21,    -1,     3,    -1,    21,
       3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const ali_yytype_uint8 ali_yyrline[] =
{
       0,   110,   110,   111,   112,   113,   114,   117,   123,   129,
     130,   133,   133,   141,   141,   151,   152,   157,   162,   168,
     173,   180,   186,   191
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const ali_yytname[] =
{
  "$end", "error", "$undefined", "STRING", "'\\n'", "':'", "';'", "'='",
  "'+'", "'*'", "','", "$accept", "input", "alias_list", "nl", "alias",
  "$@1", "$@2", "address_group", "address_list", "address", "string_list", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const ali_yytype_uint16 ali_yytoknum[] =
{
       0,   256,   257,   258,    10,    58,    59,    61,    43,    42,
      44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const ali_yytype_uint8 ali_yyr1[] =
{
       0,    11,    12,    12,    12,    12,    12,    13,    13,    14,
      14,    16,    15,    17,    15,    18,    18,    18,    18,    19,
      19,    20,    21,    21
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const ali_yytype_uint8 ali_yyr2[] =
{
       0,     2,     0,     1,     2,     2,     3,     1,     3,     1,
       2,     0,     4,     0,     4,     1,     2,     2,     1,     1,
       3,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const ali_yytype_uint8 ali_yydefact[] =
{
       2,     0,     9,     0,     3,     0,     7,    11,    13,     1,
       4,    10,     5,     0,     0,     8,     6,    22,     0,     0,
      18,    12,    15,    19,    21,    14,    16,    17,     0,    23,
      20
};

/* YYDEFGOTO[NTERM-NUM].  */
static const ali_yytype_int8 ali_yydefgoto[] =
{
      -1,     3,     4,     5,     6,    13,    14,    21,    22,    23,
      24
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -8
static const ali_yytype_int8 ali_yypact[] =
{
       7,     8,    -8,     2,     0,    12,    -8,    -8,    -8,    -8,
      12,    -8,     0,    -2,    -2,    -8,    12,    -8,     9,    14,
      -8,    -8,    10,    -8,    15,    -8,    -8,    -8,    16,    -8,
      -8
};

/* YYPGOTO[NTERM-NUM].  */
static const ali_yytype_int8 ali_yypgoto[] =
{
      -8,    -8,    17,    -4,    -7,    -8,    -8,    11,    -8,    -5,
      -8
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const ali_yytype_uint8 ali_yytable[] =
{
      10,    17,     9,    15,     2,    18,    19,    20,    16,    15,
       1,     2,    26,     7,     8,     1,    11,    27,    29,    17,
      28,     0,    12,    30,     0,    25
};

#define ali_yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-8)))

#define ali_yytable_value_is_error(Yytable_value) \
  YYID (0)

static const ali_yytype_int8 ali_yycheck[] =
{
       4,     3,     0,    10,     4,     7,     8,     9,    12,    16,
       3,     4,     3,     5,     6,     3,     4,     3,     3,     3,
      10,    -1,     5,    28,    -1,    14
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const ali_yytype_uint8 ali_yystos[] =
{
       0,     3,     4,    12,    13,    14,    15,     5,     6,     0,
      14,     4,    13,    16,    17,    15,    14,     3,     7,     8,
       9,    18,    19,    20,    21,    18,     3,     3,    10,     3,
      20
};

#define ali_yyerrok		(ali_yyerrstatus = 0)
#define ali_yyclearin	(ali_yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto ali_yyacceptlab
#define YYABORT		goto ali_yyabortlab
#define YYERROR		goto ali_yyerrorlab


/* Like YYERROR except do call ali_yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto ali_yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!ali_yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (ali_yychar == YYEMPTY)                                        \
    {                                                           \
      ali_yychar = (Token);                                         \
      ali_yylval = (Value);                                         \
      YYPOPSTACK (ali_yylen);                                       \
      ali_yystate = *ali_yyssp;                                         \
      goto ali_yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      ali_yyerror (YY_("syntax error: cannot back up")); \
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


/* YYLEX -- calling `ali_yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX ali_yylex (YYLEX_PARAM)
#else
# define YYLEX ali_yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (ali_yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (ali_yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      ali_yy_symbol_print (stderr,						  \
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
ali_yy_symbol_value_print (FILE *ali_yyoutput, int ali_yytype, YYSTYPE const * const ali_yyvaluep)
#else
static void
ali_yy_symbol_value_print (ali_yyoutput, ali_yytype, ali_yyvaluep)
    FILE *ali_yyoutput;
    int ali_yytype;
    YYSTYPE const * const ali_yyvaluep;
#endif
{
  FILE *ali_yyo = ali_yyoutput;
  YYUSE (ali_yyo);
  if (!ali_yyvaluep)
    return;
# ifdef YYPRINT
  if (ali_yytype < YYNTOKENS)
    YYPRINT (ali_yyoutput, ali_yytoknum[ali_yytype], *ali_yyvaluep);
# else
  YYUSE (ali_yyoutput);
# endif
  switch (ali_yytype)
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
ali_yy_symbol_print (FILE *ali_yyoutput, int ali_yytype, YYSTYPE const * const ali_yyvaluep)
#else
static void
ali_yy_symbol_print (ali_yyoutput, ali_yytype, ali_yyvaluep)
    FILE *ali_yyoutput;
    int ali_yytype;
    YYSTYPE const * const ali_yyvaluep;
#endif
{
  if (ali_yytype < YYNTOKENS)
    YYFPRINTF (ali_yyoutput, "token %s (", ali_yytname[ali_yytype]);
  else
    YYFPRINTF (ali_yyoutput, "nterm %s (", ali_yytname[ali_yytype]);

  ali_yy_symbol_value_print (ali_yyoutput, ali_yytype, ali_yyvaluep);
  YYFPRINTF (ali_yyoutput, ")");
}

/*------------------------------------------------------------------.
| ali_yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
ali_yy_stack_print (ali_yytype_int16 *ali_yybottom, ali_yytype_int16 *ali_yytop)
#else
static void
ali_yy_stack_print (ali_yybottom, ali_yytop)
    ali_yytype_int16 *ali_yybottom;
    ali_yytype_int16 *ali_yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; ali_yybottom <= ali_yytop; ali_yybottom++)
    {
      int ali_yybot = *ali_yybottom;
      YYFPRINTF (stderr, " %d", ali_yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (ali_yydebug)							\
    ali_yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
ali_yy_reduce_print (YYSTYPE *ali_yyvsp, int ali_yyrule)
#else
static void
ali_yy_reduce_print (ali_yyvsp, ali_yyrule)
    YYSTYPE *ali_yyvsp;
    int ali_yyrule;
#endif
{
  int ali_yynrhs = ali_yyr2[ali_yyrule];
  int ali_yyi;
  unsigned long int ali_yylno = ali_yyrline[ali_yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     ali_yyrule - 1, ali_yylno);
  /* The symbols being reduced.  */
  for (ali_yyi = 0; ali_yyi < ali_yynrhs; ali_yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", ali_yyi + 1);
      ali_yy_symbol_print (stderr, ali_yyrhs[ali_yyprhs[ali_yyrule] + ali_yyi],
		       &(ali_yyvsp[(ali_yyi + 1) - (ali_yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (ali_yydebug)				\
    ali_yy_reduce_print (ali_yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int ali_yydebug;
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

# ifndef ali_yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define ali_yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
ali_yystrlen (const char *ali_yystr)
#else
static YYSIZE_T
ali_yystrlen (ali_yystr)
    const char *ali_yystr;
#endif
{
  YYSIZE_T ali_yylen;
  for (ali_yylen = 0; ali_yystr[ali_yylen]; ali_yylen++)
    continue;
  return ali_yylen;
}
#  endif
# endif

# ifndef ali_yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define ali_yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
ali_yystpcpy (char *ali_yydest, const char *ali_yysrc)
#else
static char *
ali_yystpcpy (ali_yydest, ali_yysrc)
    char *ali_yydest;
    const char *ali_yysrc;
#endif
{
  char *ali_yyd = ali_yydest;
  const char *ali_yys = ali_yysrc;

  while ((*ali_yyd++ = *ali_yys++) != '\0')
    continue;

  return ali_yyd - 1;
}
#  endif
# endif

# ifndef ali_yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for ali_yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from ali_yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
ali_yytnamerr (char *ali_yyres, const char *ali_yystr)
{
  if (*ali_yystr == '"')
    {
      YYSIZE_T ali_yyn = 0;
      char const *ali_yyp = ali_yystr;

      for (;;)
	switch (*++ali_yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++ali_yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (ali_yyres)
	      ali_yyres[ali_yyn] = *ali_yyp;
	    ali_yyn++;
	    break;

	  case '"':
	    if (ali_yyres)
	      ali_yyres[ali_yyn] = '\0';
	    return ali_yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! ali_yyres)
    return ali_yystrlen (ali_yystr);

  return ali_yystpcpy (ali_yyres, ali_yystr) - ali_yyres;
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
ali_yysyntax_error (YYSIZE_T *ali_yymsg_alloc, char **ali_yymsg,
                ali_yytype_int16 *ali_yyssp, int ali_yytoken)
{
  YYSIZE_T ali_yysize0 = ali_yytnamerr (YY_NULL, ali_yytname[ali_yytoken]);
  YYSIZE_T ali_yysize = ali_yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *ali_yyformat = YY_NULL;
  /* Arguments of ali_yyformat. */
  char const *ali_yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int ali_yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in ali_yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated ali_yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (ali_yytoken != YYEMPTY)
    {
      int ali_yyn = ali_yypact[*ali_yyssp];
      ali_yyarg[ali_yycount++] = ali_yytname[ali_yytoken];
      if (!ali_yypact_value_is_default (ali_yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int ali_yyxbegin = ali_yyn < 0 ? -ali_yyn : 0;
          /* Stay within bounds of both ali_yycheck and ali_yytname.  */
          int ali_yychecklim = YYLAST - ali_yyn + 1;
          int ali_yyxend = ali_yychecklim < YYNTOKENS ? ali_yychecklim : YYNTOKENS;
          int ali_yyx;

          for (ali_yyx = ali_yyxbegin; ali_yyx < ali_yyxend; ++ali_yyx)
            if (ali_yycheck[ali_yyx + ali_yyn] == ali_yyx && ali_yyx != YYTERROR
                && !ali_yytable_value_is_error (ali_yytable[ali_yyx + ali_yyn]))
              {
                if (ali_yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    ali_yycount = 1;
                    ali_yysize = ali_yysize0;
                    break;
                  }
                ali_yyarg[ali_yycount++] = ali_yytname[ali_yyx];
                {
                  YYSIZE_T ali_yysize1 = ali_yysize + ali_yytnamerr (YY_NULL, ali_yytname[ali_yyx]);
                  if (! (ali_yysize <= ali_yysize1
                         && ali_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  ali_yysize = ali_yysize1;
                }
              }
        }
    }

  switch (ali_yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        ali_yyformat = S;                       \
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
    YYSIZE_T ali_yysize1 = ali_yysize + ali_yystrlen (ali_yyformat);
    if (! (ali_yysize <= ali_yysize1 && ali_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    ali_yysize = ali_yysize1;
  }

  if (*ali_yymsg_alloc < ali_yysize)
    {
      *ali_yymsg_alloc = 2 * ali_yysize;
      if (! (ali_yysize <= *ali_yymsg_alloc
             && *ali_yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *ali_yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *ali_yyp = *ali_yymsg;
    int ali_yyi = 0;
    while ((*ali_yyp = *ali_yyformat) != '\0')
      if (*ali_yyp == '%' && ali_yyformat[1] == 's' && ali_yyi < ali_yycount)
        {
          ali_yyp += ali_yytnamerr (ali_yyp, ali_yyarg[ali_yyi++]);
          ali_yyformat += 2;
        }
      else
        {
          ali_yyp++;
          ali_yyformat++;
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
ali_yydestruct (const char *ali_yymsg, int ali_yytype, YYSTYPE *ali_yyvaluep)
#else
static void
ali_yydestruct (ali_yymsg, ali_yytype, ali_yyvaluep)
    const char *ali_yymsg;
    int ali_yytype;
    YYSTYPE *ali_yyvaluep;
#endif
{
  YYUSE (ali_yyvaluep);

  if (!ali_yymsg)
    ali_yymsg = "Deleting";
  YY_SYMBOL_PRINT (ali_yymsg, ali_yytype, ali_yyvaluep, ali_yylocationp);

  switch (ali_yytype)
    {

      default:
        break;
    }
}




/* The lookahead symbol.  */
int ali_yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE ali_yylval YY_INITIAL_VALUE(ali_yyval_default);

/* Number of syntax errors so far.  */
int ali_yynerrs;


/*----------.
| ali_yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
ali_yyparse (void *YYPARSE_PARAM)
#else
int
ali_yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
ali_yyparse (void)
#else
int
ali_yyparse ()

#endif
#endif
{
    int ali_yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int ali_yyerrstatus;

    /* The stacks and their tools:
       `ali_yyss': related to states.
       `ali_yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow ali_yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    ali_yytype_int16 ali_yyssa[YYINITDEPTH];
    ali_yytype_int16 *ali_yyss;
    ali_yytype_int16 *ali_yyssp;

    /* The semantic value stack.  */
    YYSTYPE ali_yyvsa[YYINITDEPTH];
    YYSTYPE *ali_yyvs;
    YYSTYPE *ali_yyvsp;

    YYSIZE_T ali_yystacksize;

  int ali_yyn;
  int ali_yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int ali_yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE ali_yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char ali_yymsgbuf[128];
  char *ali_yymsg = ali_yymsgbuf;
  YYSIZE_T ali_yymsg_alloc = sizeof ali_yymsgbuf;
#endif

#define YYPOPSTACK(N)   (ali_yyvsp -= (N), ali_yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int ali_yylen = 0;

  ali_yyssp = ali_yyss = ali_yyssa;
  ali_yyvsp = ali_yyvs = ali_yyvsa;
  ali_yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  ali_yystate = 0;
  ali_yyerrstatus = 0;
  ali_yynerrs = 0;
  ali_yychar = YYEMPTY; /* Cause a token to be read.  */
  goto ali_yysetstate;

/*------------------------------------------------------------.
| ali_yynewstate -- Push a new state, which is found in ali_yystate.  |
`------------------------------------------------------------*/
 ali_yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  ali_yyssp++;

 ali_yysetstate:
  *ali_yyssp = ali_yystate;

  if (ali_yyss + ali_yystacksize - 1 <= ali_yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T ali_yysize = ali_yyssp - ali_yyss + 1;

#ifdef ali_yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *ali_yyvs1 = ali_yyvs;
	ali_yytype_int16 *ali_yyss1 = ali_yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if ali_yyoverflow is a macro.  */
	ali_yyoverflow (YY_("memory exhausted"),
		    &ali_yyss1, ali_yysize * sizeof (*ali_yyssp),
		    &ali_yyvs1, ali_yysize * sizeof (*ali_yyvsp),
		    &ali_yystacksize);

	ali_yyss = ali_yyss1;
	ali_yyvs = ali_yyvs1;
      }
#else /* no ali_yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto ali_yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= ali_yystacksize)
	goto ali_yyexhaustedlab;
      ali_yystacksize *= 2;
      if (YYMAXDEPTH < ali_yystacksize)
	ali_yystacksize = YYMAXDEPTH;

      {
	ali_yytype_int16 *ali_yyss1 = ali_yyss;
	union ali_yyalloc *ali_yyptr =
	  (union ali_yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (ali_yystacksize));
	if (! ali_yyptr)
	  goto ali_yyexhaustedlab;
	YYSTACK_RELOCATE (ali_yyss_alloc, ali_yyss);
	YYSTACK_RELOCATE (ali_yyvs_alloc, ali_yyvs);
#  undef YYSTACK_RELOCATE
	if (ali_yyss1 != ali_yyssa)
	  YYSTACK_FREE (ali_yyss1);
      }
# endif
#endif /* no ali_yyoverflow */

      ali_yyssp = ali_yyss + ali_yysize - 1;
      ali_yyvsp = ali_yyvs + ali_yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) ali_yystacksize));

      if (ali_yyss + ali_yystacksize - 1 <= ali_yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", ali_yystate));

  if (ali_yystate == YYFINAL)
    YYACCEPT;

  goto ali_yybackup;

/*-----------.
| ali_yybackup.  |
`-----------*/
ali_yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  ali_yyn = ali_yypact[ali_yystate];
  if (ali_yypact_value_is_default (ali_yyn))
    goto ali_yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (ali_yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      ali_yychar = YYLEX;
    }

  if (ali_yychar <= YYEOF)
    {
      ali_yychar = ali_yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      ali_yytoken = YYTRANSLATE (ali_yychar);
      YY_SYMBOL_PRINT ("Next token is", ali_yytoken, &ali_yylval, &ali_yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  ali_yyn += ali_yytoken;
  if (ali_yyn < 0 || YYLAST < ali_yyn || ali_yycheck[ali_yyn] != ali_yytoken)
    goto ali_yydefault;
  ali_yyn = ali_yytable[ali_yyn];
  if (ali_yyn <= 0)
    {
      if (ali_yytable_value_is_error (ali_yyn))
        goto ali_yyerrlab;
      ali_yyn = -ali_yyn;
      goto ali_yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (ali_yyerrstatus)
    ali_yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", ali_yytoken, &ali_yylval, &ali_yylloc);

  /* Discard the shifted token.  */
  ali_yychar = YYEMPTY;

  ali_yystate = ali_yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++ali_yyvsp = ali_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto ali_yynewstate;


/*-----------------------------------------------------------.
| ali_yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
ali_yydefault:
  ali_yyn = ali_yydefact[ali_yystate];
  if (ali_yyn == 0)
    goto ali_yyerrlab;
  goto ali_yyreduce;


/*-----------------------------.
| ali_yyreduce -- Do a reduction.  |
`-----------------------------*/
ali_yyreduce:
  /* ali_yyn is the number of a rule to reduce with.  */
  ali_yylen = ali_yyr2[ali_yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  ali_yyval = ali_yyvsp[1-ali_yylen];


  YY_REDUCE_PRINT (ali_yyn);
  switch (ali_yyn)
    {
        case 7:
/* Line 1792 of yacc.c  */
#line 118 "mh_alias.y"
    {
		 if (!alias_list)
		   alias_list = list_create_or_die ();
		 mu_list_append (alias_list, (ali_yyvsp[(1) - (1)].alias));
	       }
    break;

  case 8:
/* Line 1792 of yacc.c  */
#line 124 "mh_alias.y"
    {
		 mu_list_append (alias_list, (ali_yyvsp[(3) - (3)].alias));
	       }
    break;

  case 11:
/* Line 1792 of yacc.c  */
#line 133 "mh_alias.y"
    { ali_verbatim (1); }
    break;

  case 12:
/* Line 1792 of yacc.c  */
#line 134 "mh_alias.y"
    {
		 ali_verbatim (0);
		 (ali_yyval.alias) = mu_alloc (sizeof (*(ali_yyval.alias)));
		 (ali_yyval.alias)->name = (ali_yyvsp[(1) - (4)].string);
		 (ali_yyval.alias)->rcpt_list = (ali_yyvsp[(4) - (4)].list);
		 (ali_yyval.alias)->inclusive = 0;
	       }
    break;

  case 13:
/* Line 1792 of yacc.c  */
#line 141 "mh_alias.y"
    { ali_verbatim (1); }
    break;

  case 14:
/* Line 1792 of yacc.c  */
#line 142 "mh_alias.y"
    {
		 ali_verbatim (0);
		 (ali_yyval.alias) = mu_alloc (sizeof (*(ali_yyval.alias)));
		 (ali_yyval.alias)->name = (ali_yyvsp[(1) - (4)].string);
		 (ali_yyval.alias)->rcpt_list = (ali_yyvsp[(4) - (4)].list);
		 (ali_yyval.alias)->inclusive = 1;
	       }
    break;

  case 16:
/* Line 1792 of yacc.c  */
#line 153 "mh_alias.y"
    {
		 (ali_yyval.list) = unix_group_to_list ((ali_yyvsp[(2) - (2)].string));
		 free ((ali_yyvsp[(2) - (2)].string));
	       }
    break;

  case 17:
/* Line 1792 of yacc.c  */
#line 158 "mh_alias.y"
    {
		 (ali_yyval.list) = unix_gid_to_list ((ali_yyvsp[(2) - (2)].string));
		 free ((ali_yyvsp[(2) - (2)].string));
	       }
    break;

  case 18:
/* Line 1792 of yacc.c  */
#line 163 "mh_alias.y"
    {
		 (ali_yyval.list) = unix_passwd_to_list ();
	       }
    break;

  case 19:
/* Line 1792 of yacc.c  */
#line 169 "mh_alias.y"
    {
		 (ali_yyval.list) = list_create_or_die ();
		 mu_list_append ((ali_yyval.list), (ali_yyvsp[(1) - (1)].string));
	       }
    break;

  case 20:
/* Line 1792 of yacc.c  */
#line 174 "mh_alias.y"
    {
		 mu_list_append ((ali_yyvsp[(1) - (3)].list), (ali_yyvsp[(3) - (3)].string));
		 (ali_yyval.list) = (ali_yyvsp[(1) - (3)].list);
	       }
    break;

  case 21:
/* Line 1792 of yacc.c  */
#line 181 "mh_alias.y"
    {
		 (ali_yyval.string) = ali_list_to_string (&(ali_yyvsp[(1) - (1)].list));
	       }
    break;

  case 22:
/* Line 1792 of yacc.c  */
#line 187 "mh_alias.y"
    {
		 mu_list_create(&(ali_yyval.list));
		 mu_list_append((ali_yyval.list), (ali_yyvsp[(1) - (1)].string));
	       }
    break;

  case 23:
/* Line 1792 of yacc.c  */
#line 192 "mh_alias.y"
    {
		 mu_list_append((ali_yyvsp[(1) - (2)].list), (ali_yyvsp[(2) - (2)].string));
		 (ali_yyval.list) = (ali_yyvsp[(1) - (2)].list);
	       }
    break;


/* Line 1792 of yacc.c  */
#line 1578 "mh_alias_gram.c"
      default: break;
    }
  /* User semantic actions sometimes alter ali_yychar, and that requires
     that ali_yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of ali_yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering ali_yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", ali_yyr1[ali_yyn], &ali_yyval, &ali_yyloc);

  YYPOPSTACK (ali_yylen);
  ali_yylen = 0;
  YY_STACK_PRINT (ali_yyss, ali_yyssp);

  *++ali_yyvsp = ali_yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  ali_yyn = ali_yyr1[ali_yyn];

  ali_yystate = ali_yypgoto[ali_yyn - YYNTOKENS] + *ali_yyssp;
  if (0 <= ali_yystate && ali_yystate <= YYLAST && ali_yycheck[ali_yystate] == *ali_yyssp)
    ali_yystate = ali_yytable[ali_yystate];
  else
    ali_yystate = ali_yydefgoto[ali_yyn - YYNTOKENS];

  goto ali_yynewstate;


/*------------------------------------.
| ali_yyerrlab -- here on detecting error |
`------------------------------------*/
ali_yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  ali_yytoken = ali_yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (ali_yychar);

  /* If not already recovering from an error, report this error.  */
  if (!ali_yyerrstatus)
    {
      ++ali_yynerrs;
#if ! YYERROR_VERBOSE
      ali_yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR ali_yysyntax_error (&ali_yymsg_alloc, &ali_yymsg, \
                                        ali_yyssp, ali_yytoken)
      {
        char const *ali_yymsgp = YY_("syntax error");
        int ali_yysyntax_error_status;
        ali_yysyntax_error_status = YYSYNTAX_ERROR;
        if (ali_yysyntax_error_status == 0)
          ali_yymsgp = ali_yymsg;
        else if (ali_yysyntax_error_status == 1)
          {
            if (ali_yymsg != ali_yymsgbuf)
              YYSTACK_FREE (ali_yymsg);
            ali_yymsg = (char *) YYSTACK_ALLOC (ali_yymsg_alloc);
            if (!ali_yymsg)
              {
                ali_yymsg = ali_yymsgbuf;
                ali_yymsg_alloc = sizeof ali_yymsgbuf;
                ali_yysyntax_error_status = 2;
              }
            else
              {
                ali_yysyntax_error_status = YYSYNTAX_ERROR;
                ali_yymsgp = ali_yymsg;
              }
          }
        ali_yyerror (ali_yymsgp);
        if (ali_yysyntax_error_status == 2)
          goto ali_yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (ali_yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (ali_yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (ali_yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  ali_yydestruct ("Error: discarding",
		      ali_yytoken, &ali_yylval);
	  ali_yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto ali_yyerrlab1;


/*---------------------------------------------------.
| ali_yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
ali_yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label ali_yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto ali_yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (ali_yylen);
  ali_yylen = 0;
  YY_STACK_PRINT (ali_yyss, ali_yyssp);
  ali_yystate = *ali_yyssp;
  goto ali_yyerrlab1;


/*-------------------------------------------------------------.
| ali_yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
ali_yyerrlab1:
  ali_yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      ali_yyn = ali_yypact[ali_yystate];
      if (!ali_yypact_value_is_default (ali_yyn))
	{
	  ali_yyn += YYTERROR;
	  if (0 <= ali_yyn && ali_yyn <= YYLAST && ali_yycheck[ali_yyn] == YYTERROR)
	    {
	      ali_yyn = ali_yytable[ali_yyn];
	      if (0 < ali_yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (ali_yyssp == ali_yyss)
	YYABORT;


      ali_yydestruct ("Error: popping",
		  ali_yystos[ali_yystate], ali_yyvsp);
      YYPOPSTACK (1);
      ali_yystate = *ali_yyssp;
      YY_STACK_PRINT (ali_yyss, ali_yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++ali_yyvsp = ali_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", ali_yystos[ali_yyn], ali_yyvsp, ali_yylsp);

  ali_yystate = ali_yyn;
  goto ali_yynewstate;


/*-------------------------------------.
| ali_yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
ali_yyacceptlab:
  ali_yyresult = 0;
  goto ali_yyreturn;

/*-----------------------------------.
| ali_yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
ali_yyabortlab:
  ali_yyresult = 1;
  goto ali_yyreturn;

#if !defined ali_yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| ali_yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
ali_yyexhaustedlab:
  ali_yyerror (YY_("memory exhausted"));
  ali_yyresult = 2;
  /* Fall through.  */
#endif

ali_yyreturn:
  if (ali_yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      ali_yytoken = YYTRANSLATE (ali_yychar);
      ali_yydestruct ("Cleanup: discarding lookahead",
                  ali_yytoken, &ali_yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (ali_yylen);
  YY_STACK_PRINT (ali_yyss, ali_yyssp);
  while (ali_yyssp != ali_yyss)
    {
      ali_yydestruct ("Cleanup: popping",
		  ali_yystos[*ali_yyssp], ali_yyvsp);
      YYPOPSTACK (1);
    }
#ifndef ali_yyoverflow
  if (ali_yyss != ali_yyssa)
    YYSTACK_FREE (ali_yyss);
#endif
#if YYERROR_VERBOSE
  if (ali_yymsg != ali_yymsgbuf)
    YYSTACK_FREE (ali_yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (ali_yyresult);
}


/* Line 2055 of yacc.c  */
#line 198 "mh_alias.y"


static mu_list_t
ali_list_dup (mu_list_t src)
{
  mu_list_t dst;
  mu_iterator_t itr;

  if (mu_list_create (&dst))
    return NULL;

  if (mu_list_get_iterator (src, &itr))
    {
      mu_list_destroy (&dst);
      return NULL;
    }
  
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      void *data;
      mu_iterator_current (itr, (void **)&data);
      mu_list_append (dst, data);
    }
  mu_iterator_destroy (&itr);
  return dst;
}

static int
ali_member (mu_list_t list, const char *name)
{
  mu_iterator_t itr;
  int found = 0;

  if (mu_list_get_iterator (list, &itr))
    return 0;
  for (mu_iterator_first (itr); !found && !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      char *item;
      mu_address_t tmp;
      
      mu_iterator_current (itr, (void **)&item);
      if (strcmp (item, name) == 0)
	found = 1;
      else if (mu_address_create (&tmp, item) == 0)
	{
	  found = mu_address_contains_email (tmp, name);
	  mu_address_destroy (&tmp);
	}
    }
  mu_iterator_destroy (&itr);
  return found;
}

int
aliascmp (const char *pattern, const char *name)
{
  int len = strlen (pattern);

  if (len > 1 && pattern[len - 1] == '*')
    return strncmp (pattern, name, len - 2);
  return strcmp (pattern, name);
}

static int mh_alias_get_internal (const char *name, mu_iterator_t start,
				  mu_list_t *return_list, int *inclusive);

int
alias_expand_list (mu_list_t name_list, mu_iterator_t orig_itr, int *inclusive)
{
  mu_iterator_t itr;

  if (mu_list_get_iterator (name_list, &itr))
    return 1;
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      char *name;
      mu_list_t exlist;
      
      mu_iterator_current (itr, (void **)&name);
      if (mh_alias_get_internal (name, orig_itr, &exlist, inclusive) == 0)
	{
	  /* Insert exlist after name */
	  mu_iterator_ctl (itr, mu_itrctl_insert_list, exlist);
	  mu_list_destroy (&exlist);
	  /* Remove name */
	  mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	}
    }
  mu_iterator_destroy (&itr);
  return 0;
}  

/* Look up the named alias. If found, return the list of recipient
   names associated with it */
static int
mh_alias_get_internal (const char *name,
		       mu_iterator_t start, mu_list_t *return_list,
		       int *inclusive) 
{
  mu_iterator_t itr;
  int rc = 1;

  if (!start)
    {
      if (mu_list_get_iterator (alias_list, &itr))
	return 1;
      mu_iterator_first (itr);
    }
  else
    {
      mu_iterator_dup (&itr, start);
      mu_iterator_next (itr);
    }
	
  for (; !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      struct mh_alias *alias;
      mu_iterator_current (itr, (void **)&alias);
      if (aliascmp (alias->name, name) == 0)
	{
	  if (inclusive)
	    *inclusive |= alias->inclusive;
	  *return_list = ali_list_dup (alias->rcpt_list);
	  alias_expand_list (*return_list, itr, inclusive);
	  rc = 0;
	  break;
	}
    }
  
  mu_iterator_destroy (&itr);
  return rc;
}

int
mh_alias_get (const char *name, mu_list_t *return_list)
{
  return mh_alias_get_internal (name, NULL, return_list, NULL);
}

int
mh_alias_get_address (const char *name, mu_address_t *paddr, int *incl)
{
  mu_iterator_t itr;
  mu_list_t list;

  if (incl)
    *incl = 0;
  if (mh_alias_get_internal (name, NULL, &list, incl))
    return 1;
  if (mu_list_is_empty (list))
    {
      mu_list_destroy (&list);
      return 1;
    }
  
  if (mu_list_get_iterator (list, &itr) == 0)
    {
      for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
	{
	  char *item;
	  mu_address_t a;
	  char *ptr = NULL; 

	  mu_iterator_current (itr, (void **)&item);
	  if (mu_address_create (&a, item))
	    {
	      mu_error (_("Error expanding aliases -- invalid address `%s'"),
			item);
	    }
	  else
	    {
	      if (incl && *incl)
		mu_address_set_personal (a, 1, name);
	      mu_address_union (paddr, a);
	      mu_address_destroy (&a);
	    }
	  if (ptr)
	    free (ptr);
	}
      mu_iterator_destroy (&itr);
    }
  mu_list_destroy (&list);
  return 0;
}

/* Look up the given user name in the aliases. Return the list of
   alias names this user is member of */
int
mh_alias_get_alias (const char *uname, mu_list_t *return_list)
{
  mu_iterator_t itr;
  int rc = 1;
  
  if (mu_list_get_iterator (alias_list, &itr))
    return 1;
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      struct mh_alias *alias;
      mu_iterator_current (itr, (void **)&alias);
      if (ali_member (alias->rcpt_list, uname))
	{
	  if (*return_list == NULL && mu_list_create (return_list))
	    break;
	  mu_list_append (*return_list, alias->name);
	  rc = 0;
	}
    }
  
  mu_iterator_destroy (&itr);
  return rc;
}

void
mh_alias_enumerate (mh_alias_enumerator_t fun, void *data)
{
  mu_iterator_t itr;
  int rc = 0;
  
  if (mu_list_get_iterator (alias_list, &itr))
    return ;
  for (mu_iterator_first (itr);
       rc == 0 && !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      struct mh_alias *alias;
      mu_list_t tmp;
      
      mu_iterator_current (itr, (void **)&alias);

      tmp = ali_list_dup (alias->rcpt_list);
      alias_expand_list (tmp, itr, NULL);

      rc = fun (alias->name, tmp, data);
      mu_list_destroy (&tmp);
    }
  mu_iterator_destroy (&itr);
}

static mu_list_t
unix_group_to_list (char *name)
{
  struct group *grp = getgrnam (name);
  mu_list_t lst = list_create_or_die ();
  
  if (grp)
    {
      char **p;

      for (p = grp->gr_mem; *p; p++)
	mu_list_append (lst, mu_strdup (*p));
    }      
  
  return lst;
}

static mu_list_t
unix_gid_to_list (char *name)
{
  struct group *grp = getgrnam (name);
  mu_list_t lst = list_create_or_die ();

  if (grp)
    {
      struct passwd *pw;
      setpwent();
      while ((pw = getpwent ()))
	{
	  if (pw->pw_gid == grp->gr_gid)
	    mu_list_append (lst, mu_strdup (pw->pw_name));
	}
      endpwent();
    }
  return lst;
}

static mu_list_t
unix_passwd_to_list ()
{
  mu_list_t lst = list_create_or_die ();
  struct passwd *pw;

  setpwent();
  while ((pw = getpwent ()))
    {
      if (pw->pw_uid > 200)
	mu_list_append (lst, mu_strdup (pw->pw_name));
    }
  endpwent();
  return lst;
}

int
mh_read_aliases ()
{
  const char *p;
  
  p = mh_global_profile_get ("Aliasfile", NULL);
  if (p)
    {
      struct mu_wordsplit ws;

      if (mu_wordsplit (p, &ws, MU_WRDSF_DEFFLAGS))
	{
	  mu_error (_("cannot split line `%s': %s"), p,
		    mu_wordsplit_strerror (&ws));
	}
      else
	{
	  size_t i;
	  for (i = 0; i < ws.ws_wordc; i++) 
	    mh_alias_read (ws.ws_wordv[i], 1);
	  mu_wordsplit_free (&ws);
	}
    }
  mh_alias_read (DEFAULT_ALIAS_FILE, 0);
  return 0;
}

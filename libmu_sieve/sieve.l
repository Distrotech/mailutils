%top {
/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2005, 2007-2012, 2014-2016 Free
   Software Foundation, Inc.

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
}

%{
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <mailutils/cctype.h>
#include <mailutils/wordsplit.h>
#include <sieve-priv.h>
#include <sieve-gram.h>
  
static char *multiline_delimiter;
static int strip_tabs; 

static int number (void);
static int string (void);
static void line_begin (void);
static void line_add (char *text, size_t len);
static void line_finish (void);
static void multiline_begin (void);
static void multiline_add (char *);
static void multiline_finish (void);
static char *multiline_strip_tabs (char *text);
static void ident (const char *text);
static void sieve_include (void);
static void sieve_searchpath (void);
static char *str_unescape (char *text, size_t len);
static int isemptystr (char *text);

static ino_t sieve_source_inode;
struct mu_locus mu_sieve_locus;
static int newline;
static mu_list_t file_names;

static mu_stream_t input_stream;
 
static int
fillbuf (char *buf, size_t max_size)
{
  int rc;

  if (!input_stream)
    return 0;
  
  rc = mu_stream_read (input_stream, buf, max_size, &max_size);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_read",
		       mu_sieve_locus.mu_file, rc);
      return 0;
    }
  return max_size;
}
 
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) result = fillbuf (buf, max_size)
#define LEX_BUFFER_STATE YY_BUFFER_STATE
#define SET_BUFFER_STATE(s) do { \
        (s) = YY_CURRENT_BUFFER; \
        yy_switch_to_buffer(yy_create_buffer (yyin, YY_BUF_SIZE)); \
} while (0)
#define RESTORE_BUFFER_STATE(s) do { \
        yy_delete_buffer (YY_CURRENT_BUFFER); \
        yy_switch_to_buffer (s); \
} while (0)

static int
file_name_cmp (const void *a, const void *b)  
{
  return strcmp (a, b);
}

static void
init_locus (char const *name, ino_t ino)
{
  mu_sieve_locus.mu_file = NULL;
  if (name)
    {
      if (!file_names)
	{
	  mu_list_create (&file_names);
	  mu_list_set_comparator (file_names, file_name_cmp);
	  mu_list_set_destroy_item (file_names, mu_list_free_item);
	}
      else
	{
	  mu_list_locate (file_names, (void*) name,
			  (void**) &mu_sieve_locus.mu_file);
	}
      if (!mu_sieve_locus.mu_file)
	{
	  mu_sieve_locus.mu_file = strdup (name);//FIXME: Error checking
	  mu_list_append (file_names, mu_sieve_locus.mu_file);
	}
    }
  mu_sieve_locus.mu_line = 1;
  mu_sieve_locus.mu_col = 0;
  newline = 0;
  sieve_source_inode = ino;
}
 
static void
advance_locus (void)
{
  if (newline)
    {
      mu_sieve_locus.mu_line++;
      mu_sieve_locus.mu_col = 0;
      yylloc.beg = yylloc.end = mu_sieve_locus;                        
    }
  else
    {
      mu_sieve_locus.mu_col += yyleng;
      yylloc.beg = yylloc.end = mu_sieve_locus;                        
      yylloc.beg.mu_col -= yyleng;                             
    }
  newline = yytext[yyleng-1] == '\n';
}
 
#define YY_USER_ACTION advance_locus ();
   
struct buffer_ctx
{
  struct buffer_ctx *prev;
  struct mu_locus locus;
  ino_t i_node;
  mu_stream_t input;
  LEX_BUFFER_STATE state;
};

static struct buffer_ctx *context_stack;

static struct buffer_ctx *ctx_lookup (ino_t ino);
static int push_source (const char *name);
static int pop_source (void);

struct buffer_ctx *
ctx_lookup (ino_t ino)
{
  struct buffer_ctx *ctx;

  for (ctx = context_stack; ctx; ctx = ctx->prev)
    if (ctx->i_node == ino)
      break;
  return ctx;
}
        
int
push_source (const char *name)
{
  int rc;
  mu_stream_t stream;
  struct buffer_ctx *ctx;
  struct stat st;
        
  if (stat (name, &st))
    {
      mu_diag_at_locus (MU_LOG_ERROR, &mu_sieve_locus,
			_("cannot stat `%s': %s"), name, strerror (errno));
      mu_i_sv_error (mu_sieve_machine);
      return 1;
    }

  if (mu_sieve_locus.mu_file && st.st_ino == sieve_source_inode)
    {
      yyerror (_("recursive inclusion"));
      return 1;
    }
  if ((ctx = ctx_lookup (st.st_ino)))
    {
      yyerror (_("recursive inclusion"));
      if (ctx->prev)
	{
	  mu_diag_at_locus (MU_LOG_ERROR, &ctx->prev->locus,
			    _("`%s' already included here"),
			    name);
	  mu_i_sv_error (mu_sieve_machine);
	}
      else
	{
	  mu_diag_at_locus (MU_LOG_ERROR, &mu_sieve_locus,
			    _("`%s' already included at top level"),
			    name);
	  mu_i_sv_error (mu_sieve_machine);
	}	  
      return 1;
    }

  rc = mu_file_stream_create (&stream, name, MU_STREAM_READ);
  if (rc)
    {
      mu_diag_at_locus (MU_LOG_ERROR, &mu_sieve_locus,
			_("cannot open file `%s': %s"),
			name, mu_strerror (rc));
      mu_i_sv_error (mu_sieve_machine);
      return 1;
    }

  /* Push current context */
  if (mu_sieve_locus.mu_file)
    {
      advance_locus ();
      ctx = mu_sieve_alloc (sizeof (*ctx));
      ctx->locus = mu_sieve_locus;
      ctx->i_node = sieve_source_inode;
      ctx->input = input_stream;
      ctx->prev = context_stack;
      context_stack = ctx;

      /* Switch to the new context */
      SET_BUFFER_STATE (ctx->state);
    }
  input_stream = stream;

  init_locus (name, st.st_ino);

  return 0;
}

int
pop_source ()
{
  struct buffer_ctx *ctx;

  mu_stream_destroy (&input_stream);

  if (!context_stack)
    {
      input_stream = NULL;
      init_locus (NULL, 0);
      return 1;
    }
  /* Restore previous context */
  input_stream = context_stack->input;
  mu_sieve_locus = context_stack->locus;
  sieve_source_inode = context_stack->i_node;
  RESTORE_BUFFER_STATE (context_stack->state);
  ctx = context_stack->prev;
  free (context_stack);
  context_stack = ctx;

  return 0;
}
%}

%option nounput
%option noinput

%x COMMENT ML STR

WS [ \t][ \t]*
IDENT [a-zA-Z_][a-zA-Z_0-9]+
SIZESUF [kKmMgG]

%%
         /* C-style comments */
"/*"                    { BEGIN(COMMENT); }
<COMMENT>[^*\n]*        /* eat anything that's not a '*' */
<COMMENT>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
<COMMENT>\n             ;
<COMMENT>"*"+"/"        { BEGIN (INITIAL); }
         /* Preprocessor directives (an extension) */
#[ \t]*include.*\n      { sieve_include (); }
#[ \t]*searchpath.*\n   { sieve_searchpath (); } 
         /* End-of-line comments */
#.*\n   ;
#.*     /* end-of-file comment */;
         /* Reserved words */
require return REQUIRE; 
if	return IF;      
elsif	return ELSIF;  
else	return ELSE;    
anyof	return ANYOF;   
allof	return ALLOF;   
not     return NOT;
false   return FALSE;
true    return TRUE;
         /* Identifiers */
{IDENT}  { ident (yytext); return IDENT; } 
:{IDENT} { ident (yytext + 1); return TAG; }
         /* Numbers */
0[0-7]*{SIZESUF}*                     { return number (); }
0x[0-9a-fA-F][0-9a-fA-F]+{SIZESUF}*   { return number (); }
[1-9][0-9]*{SIZESUF}*                 { return number (); }
         /* Quoted strings */
\"[^\\"\n]*\"                 { return string (); }
\"[^\\"\n]*\\.    { BEGIN(STR);
                    line_begin ();
  		    line_add (str_unescape (yytext + 1, yyleng - 1), 0); }
<STR>[^\\"\n]*\\. { line_add (str_unescape (yytext, yyleng), 0); }
<STR>[^\\"\n]*\" { BEGIN(INITIAL);
                  if (yyleng > 1) 
                     line_add (yytext, yyleng - 1); 
                   line_finish ();
		   return STRING; }
         /* Multiline strings */
text:-?[ \t]*#.*\n       { BEGIN(ML);
                           multiline_begin (); }
text:-?[ \t]*\n          { BEGIN(ML);
                           multiline_begin (); }
text:-?\\?{IDENT}[ \t]*#.*\n { BEGIN(ML);
                               multiline_begin (); }
text:-?\\?{IDENT}[ \t]*\n    { BEGIN(ML);
                               multiline_begin (); }
<ML>#[ \t]*include.*\n    { if (multiline_delimiter[0] == '\\')
                              multiline_add (NULL);
                            else
                              sieve_include (); }
<ML>.*\n { char *p = multiline_strip_tabs (yytext);
	   
           if (strncmp (p, multiline_delimiter, strlen (multiline_delimiter))
	        == 0
	       && isemptystr (p + strlen (multiline_delimiter)))
	     {
	       free (multiline_delimiter);
	       multiline_delimiter = NULL;
	       BEGIN(INITIAL);
	       multiline_finish ();
	       return MULTILINE;
	     }
	    multiline_add (NULL); } 
{WS}     ;
         /* Other tokens */
\n       ;
. return yytext[0];

%%

int
yywrap ()
{
  return pop_source();
}

static char *
get_file_name (char *p, char *endp, int *usepath)
{
  char exp, *name, *startp;
  int n;
  
  if (usepath)
    *usepath = 0;
  switch (*p)
    {
    case '"':
      exp = '"';
      break;

    case '<':
      exp = '>';
      if (usepath)
	*usepath = 1;
      break;

    default:
      yyerror (_("preprocessor syntax"));
      return NULL;
    }

  for (startp = ++p; p < endp && *p != exp; p++)
    ;

  if (*p != exp)
    {
      yyerror (_("missing closing quote in preprocessor statement"));
      return NULL;
    }
  
  n = p - startp;
  name = mu_sieve_alloc (n + 1);
  memcpy (name, startp, n);
  name[n] = 0;
  return name;
}

static int
_try_include (void *item, void *data)
{
  char **dir = data;
  char *name = mu_make_file_name ((char*) item, *dir);

  if (!name)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_make_file_name", NULL, ENOMEM);
      return 0;
    }
  if (access (name, R_OK) == 0)
    {
      *(char**) data = name;
      return 1;
    }
  free (name);
  return 0;
}

static void
sieve_include ()
{
  char *p, *endp = yytext + yyleng, *name;
  int usepath;
  
  p = strstr (yytext, "include");
  for (p += 7; p < endp && mu_isspace (*p); p++)
    ;

  name = get_file_name (p, endp, &usepath);
  if (!name)
    return;
  
  if (usepath && name[0] != '/' && memcmp (name, "..", 2))
    {
      char *p = name;
      if (mu_sieve_include_path
	  && mu_list_foreach (mu_sieve_include_path, _try_include, &p))
	{
	  push_source (p);
	  free (name);
	  free (p);
	  return;
	}
    }

  push_source (name);
  free (name);
}

static void
sieve_searchpath (void)
{
  char *p, *endp = yytext + yyleng, *name;
  
  p = strstr (yytext, "searchpath");
  for (p += 10; p < endp && mu_isspace (*p); p++)
    ;
  name = get_file_name (p, endp, NULL);
  if (name)
    {
      mu_i_sv_load_add_dir (mu_sieve_machine, name);
      free (name);
    }
}

int
mu_i_sv_lex_begin (const char *name)
{
  return push_source (name);
}

int
mu_i_sv_lex_begin_string (const char *buf, int bufsize,
			  const char *fname, int line)
{
  int rc;
  
  if (!fname)
    return 1;
  yyrestart (NULL);

  rc = mu_static_memory_stream_create (&input_stream, buf, bufsize);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_static_memory_stream_create",
		       NULL, rc);
      return 1;
    }

  init_locus (fname, 0);

  return 0;
}

void
mu_i_sv_lex_finish (struct mu_sieve_machine *mach)
{
  while (pop_source () == 0)
    ;
  mach->source_list = file_names;
  file_names = NULL;
}
  
static int
number ()
{
  char *p;
  yylval.number = strtoul (yytext, &p, 0);
  switch (*p)
    {
    case 'k':
    case 'K':
      yylval.number *= 1024L;
      break;
      
    case 'm':
    case 'M':
      yylval.number *= 1024*1024L;
      break;
      
    case 'g':
    case 'G':
      yylval.number *= 1024*1024*1024L;
    }
  return NUMBER;
}

static int
string (void)
{
  line_begin ();
  line_add (yytext + 1, yyleng - 2);
  line_finish ();
  return STRING; 
}

static int
isemptystr (char *text)
{
  for (; *text && mu_isspace (*text); text++)
    ;
  return *text == 0;
}

static char *
multiline_strip_tabs (char *text)
{
  if (strip_tabs)
    for (; *text == '\t'; text++)
      ;
  return text;
}

static void
line_add (char *text, size_t len)
{
  mu_opool_append (mu_sieve_machine->string_pool, text, len);
}

static void
multiline_add (char *s)
{
  if (!s)
    s = multiline_strip_tabs (yytext);
  mu_opool_appendz (mu_sieve_machine->string_pool, s);
}

static void
line_begin (void)
{
  /* nothing */
}

static void
multiline_begin (void)
{
  char *p = yytext + 5; /* past the text: keyword */

  if (*p == '-')
    {
      strip_tabs = 1;
      p++;
    }
  else
    strip_tabs = 0;

  if (!mu_isspace (*p))
    {
      char *endp;
      int len;
      
      for (endp = p; *endp; endp++)
	if (mu_isspace (*endp))
	  break;

      len = endp - p;
      multiline_delimiter = mu_sieve_alloc (len + 1);
      memcpy (multiline_delimiter, p, len);
      multiline_delimiter[len] = 0;
    }
  else
    {
      multiline_delimiter = strdup (".");
      if (!multiline_delimiter)
	{
	  yyerror (_("not enough memory"));
	  exit (1);
	}
    }

  line_begin ();
}

static void
multiline_finish (void)
{
  line_finish ();
}

static void
ident (const char *text)
{
  yylval.string = strdup (text);
  if (!yylval.string)
    {
      yyerror (_("not enough memory"));
      exit (1);
    }
}

/* Escapes the last character from yytext */
static char *
str_unescape (char *text, size_t len)
{
  char *str = mu_sieve_alloc (len);
  memcpy (str, text, len - 2);
  str[len - 2] = mu_wordsplit_c_unquote_char (text[len - 1]);
  str[len - 1] = 0;
  return str;
}

static mu_i_sv_interp_t string_interpreter;

static void
line_finish (void)
{
  char *str;
  
  mu_opool_append_char (mu_sieve_machine->string_pool, 0);
  str = mu_opool_finish (mu_sieve_machine->string_pool, NULL);
  if (string_interpreter)
    {
      char *exp;
      int rc = mu_i_sv_string_expand (str, string_interpreter, NULL, &exp);
      if (rc == 0)
	{
	  mu_opool_free (mu_sieve_machine->string_pool, str);
	  mu_opool_appendz (mu_sieve_machine->string_pool, exp);
	  mu_opool_append_char (mu_sieve_machine->string_pool, 0);
	  free (exp);
	  str = mu_opool_finish (mu_sieve_machine->string_pool, NULL);
	}
      else if (rc != MU_ERR_CANCELED)
	{
	  mu_diag_at_locus (MU_LOG_ERROR, &mu_sieve_locus,
			    _("error expandind string: %s"),
			    mu_strerror (rc));
	}
    }
  yylval.string = str;
}

int
mu_sieve_require_encoded_character (mu_sieve_machine_t mach,
				    const char *name)
{
  string_interpreter = mu_i_sv_expand_encoded_char;
  return 0;
}

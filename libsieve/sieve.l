%{
/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2004, 2005
   Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <mailutils/argcv.h>
#include <sieve.h>
#include <sieve-gram.h>
  
char *sieve_filename;  
int sieve_line_num;  
ino_t sieve_source_inode;

static mu_list_t string_list;
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
 
#ifdef FLEX_SCANNER
#define xinput() (yyin ? getc(yyin) : EOF)
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size)  do { \
        int i;                                  \
        for (i = 0; i < max_size; i++) {        \
                int ch = xinput();              \
                if (ch == EOF)                  \
                        break;                  \
                buf[i] = ch;                    \
        }                                       \
        result = i;                             \
} while (0) 
#define LEX_BUFFER_STATE YY_BUFFER_STATE
#define SET_BUFFER_STATE(s) do { \
        (s) = YY_CURRENT_BUFFER; \
        yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE)); \
} while (0)
#define RESTORE_BUFFER_STATE(s) do { \
        yy_delete_buffer(YY_CURRENT_BUFFER); \
        yy_switch_to_buffer(s); \
} while (0)

#else
/* AT&T Lex */
                                               
static void lex_set_buffer (FILE *fp);
static void lex_delete_buffer (LEX_BUFFER_STATE buf);
static int xinput (void); 
static int xunput (void);
        
#undef unput
#define unput(c) xunput(c)
#undef input
#define input() xinput()

#define LEX_BUF_SIZE 16384
#define LEX_PUTBACK_SIZE 32
                                               
typedef struct {
  FILE *yyin;
  char *buffer;
  size_t bufsize;
  size_t level;
  char *ptr;
  char *putback;
  size_t pb_size;
  size_t pb_level;
} LEX_BUFFER_STATE;
LEX_BUFFER_STATE current_buffer;
 
#define SET_BUFFER_STATE(s) do { \
        (s) = current_buffer;    \
        lex_set_buffer(yyin);    \
} while (0)
#define RESTORE_BUFFER_STATE(s) do { \
        lex_delete_buffer(current_buffer); \
        current_buffer = (s); \
        yyin = current_buffer.yyin;  \
} while (0)
                                    
void
lex_set_buffer (FILE *fp)
{
  char *buf;
  size_t size;
        
  for (size = LEX_BUF_SIZE; size > 1; size /= 2)
    if (buf = malloc (size))
      break;
  
  if (!buf)
    {
      sieve_compile_error (sieve_filename, sieve_line_num, _("Not enough memory"));
      abort ();
    }

  current_buffer.yyin = yyin;
  current_buffer.buffer = buf;
  current_buffer.bufsize = size;
  current_buffer.level = 0;
  current_buffer.ptr = current_buffer.buffer;
  current_buffer.pb_size = current_buffer.pb_level = 0;
  current_buffer.putback = NULL;
}
                
void
lex_delete_buffer (LEX_BUFFER_STATE buf)
{
  free (buf.buffer);
  if (buf.putback)
    free (buf.putback);
}

int
xinput ()
{
  if (!yyin)
    return EOF;
  
  if (current_buffer.pb_level) 
    return current_buffer.putback[--current_buffer.pb_level];

  if (current_buffer.level <= 0)
    {
      int n;

      if (feof (yyin))
	return 0;
      n = fread (current_buffer.buffer, 1,
		 current_buffer.bufsize, yyin);
      if (n <= 0)
	return 0;
      current_buffer.level = n;
      current_buffer.ptr = current_buffer.buffer;
    }
  current_buffer.level--;
  return *current_buffer.ptr++;
}

int
xunput (int c)
{
  if (current_buffer.pb_level == current_buffer.pb_size)
    {
      char *putback;
      current_buffer.pb_size += LEX_PUTBACK_SIZE;
      putback = mu_sieve_alloc (current_buffer.pb_size);
      memcpy (putback, current_buffer.putback,
	      current_buffer.pb_level);
      free (current_buffer.putback);
      current_buffer.putback = putback;
    }
  current_buffer.putback[current_buffer.pb_level++] = c;
  return c;
}
                
#endif                                         

struct buffer_ctx {
  struct buffer_ctx *prev;
  char *filename;
  int line;
  ino_t i_node;
  FILE *yyin;
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
  FILE *fp;
  struct buffer_ctx *ctx;
  struct stat st;
        
  if (stat (name, &st))
    {
      sieve_compile_error (sieve_filename, sieve_line_num,
                           _("cannot stat `%s': %s"), name, strerror (errno));
      return 1;
    }

  if (sieve_filename && st.st_ino == sieve_source_inode)
    {
      yyerror (_("recursive inclusion"));
      return 1;
    }
  if ((ctx = ctx_lookup (st.st_ino)))
    {
      yyerror (_("recursive inclusion"));
      if (ctx->prev)
	sieve_compile_error (ctx->prev->filename, ctx->prev->line,
                             _("`%s' already included here"),
                             name);
      else
	sieve_compile_error (sieve_filename, sieve_line_num, 
                             _("`%s' already included at top level"),
		             name);
      return 1;
    }
                
  fp = fopen (name, "r");
  if (!fp)
    {
      sieve_compile_error (sieve_filename, sieve_line_num, 
                           _("cannot open `%s': %s"), name, strerror (errno));
      return 1;
    }

  /* Push current context */
  if (sieve_filename)
    {
      ctx = mu_sieve_alloc (sizeof (*ctx));
      ctx->filename = sieve_filename;
      ctx->line = sieve_line_num;
      ctx->i_node = sieve_source_inode;
      ctx->yyin = yyin;
      ctx->prev = context_stack;
      context_stack = ctx;
      
      /* Switch to the new context */
      yyin = fp;
      SET_BUFFER_STATE (ctx->state);
    }
  else
    {
#ifdef FLEX_SCANNER
      yyrestart (fp);
#else           
      yyin = fp;
      lex_set_buffer (yyin);
#endif
    }
  sieve_filename = strdup (name);
  sieve_line_num = 1;
  sieve_source_inode = st.st_ino;

  sieve_change_source ();
  
  return 0;
}

int
pop_source ()
{
  struct buffer_ctx *ctx;

  if (yyin)
    fclose (yyin);
#ifndef FLEX_SCANNER
  lex_delete_buffer (current_buffer);
#endif
  if (sieve_filename)
    free (sieve_filename);
  if (!context_stack)
    {
      yyin = NULL;
      sieve_filename = NULL;
      return 1;
    }
  /* Restore previous context */
  sieve_filename = context_stack->filename;
  sieve_line_num = context_stack->line + 1; /* #include rule did not increment
					       it */
  sieve_source_inode = context_stack->i_node;
  RESTORE_BUFFER_STATE (context_stack->state);
  ctx = context_stack->prev;
  free (context_stack);
  context_stack = ctx;

  sieve_change_source ();

  return 0;
}
%}
%x COMMENT ML STR

WS [ \t][ \t]*
IDENT [a-zA-Z_][a-zA-Z_0-9]+
SIZESUF [kKmMgG]

%%
         /* C-style comments */
"/*"         BEGIN(COMMENT);
<COMMENT>[^*\n]*        /* eat anything that's not a '*' */
<COMMENT>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
<COMMENT>\n             ++sieve_line_num;
<COMMENT>"*"+"/"        BEGIN(INITIAL);
         /* Preprocessor directives (an extension) */
#[ \t]*include.*\n      { sieve_include (); }
#[ \t]*searchpath.*\n          { sieve_searchpath (); } 
         /* End-of-line comments */
#.*\n   { sieve_line_num++; }
#.*     /* end-of-file comment */;
         /* Reserved words */
require return REQUIRE; 
if	return IF;      
elsif	return ELSIF;  
else	return ELSE;    
anyof	return ANYOF;   
allof	return ALLOF;   
not     return NOT;
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
text:-?[ \t]*#.*\n       { BEGIN(ML); multiline_begin (); sieve_line_num++; }
text:-?[ \t]*\n          { BEGIN(ML); multiline_begin (); sieve_line_num++; }
text:-?\\?{IDENT}[ \t]*#.*\n { BEGIN(ML); multiline_begin ();
                               sieve_line_num++; }
text:-?\\?{IDENT}[ \t]*\n    { BEGIN(ML); multiline_begin ();
                               sieve_line_num++; }
<ML>#[ \t]*include.*\n    { if (multiline_delimiter[0] == '\\')
                              {
				sieve_line_num++;
                                multiline_add (NULL);
			      }
                            else
                              sieve_include (); }
<ML>.*\n { char *p = multiline_strip_tabs (yytext);
           sieve_line_num++;
	   
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
\n { sieve_line_num++; } 
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
  char *name = malloc (strlen (item) + 1 + strlen (*dir) + 1);

  if (!name)
    return 0;
  sprintf (name, "%s/%s", (char*) item, *dir);
  if (access (name, R_OK) == 0)
    {
      *(char**) data = name;
      return 1;
    }
  free (name);
  return 0;
}

void
sieve_include ()
{
  char *p, *endp = yytext + yyleng, *name;
  int usepath;
  
  p = strstr (yytext, "include");
  for (p += 7; p < endp && isspace (*p); p++)
    ;

  name = get_file_name (p, endp, &usepath);
  if (!name)
    return;
  
  if (usepath && name[0] != '/' && memcmp (name, "..", 2))
    {
      char *p = name;
      if (mu_sieve_include_path
	  && mu_list_do (mu_sieve_include_path, _try_include, &p))
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

void
sieve_searchpath ()
{
  int append = 0;
  char *p, *endp = yytext + yyleng, *name;
  
  p = strstr (yytext, "searchpath");
  for (p += 10; p < endp && isspace (*p); p++)
    ;
  if (strcmp (p, "add") == 0)
    {
      append = 1;
      for (p += 3; p < endp && isspace (*p); p++)
	;
    }
  name = get_file_name (p, endp, NULL);
  if (name)
    {
      sieve_load_add_dir (sieve_machine, name);
      free (name);
    }
}

int
sieve_lex_begin (const char *name)
{
  return push_source (name);
}

void
sieve_lex_finish ()
{
  while (pop_source () == 0)
    ;
}
  
int
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

int
string ()
{
  yylval.string = mu_sieve_malloc (sieve_machine, yyleng - 1);
  memcpy (yylval.string, yytext + 1, yyleng - 2);
  yylval.string[yyleng - 2] = 0;
  return STRING; 
}

int
isemptystr (char *text)
{
  for (; *text && isspace (*text); text++)
    ;
  return *text == 0;
}

char *
multiline_strip_tabs (char *text)
{
  if (strip_tabs)
    for (; *text == '\t'; text++)
      ;
  return text;
}

void
line_add (char *text, size_t len)
{
  char *s;
  
  if (len == 0)
    len = strlen (text);
  s = malloc (len + 1);
  if (!s)
    {
      yyerror (_("Not enough memory"));
      exit (1);
    }
  memcpy (s, text, len);
  s[len] = 0;
  mu_list_append (string_list, s);
}

void
multiline_add (char *s)
{
  if (!s)
    s = multiline_strip_tabs (yytext);
  line_add (s, 0);
}

void
line_begin ()
{
  int status;

  if (string_list)
    mu_sieve_slist_destroy (&string_list);
  status = mu_list_create (&string_list);
  if (status)
    {
      sieve_compile_error (sieve_filename, sieve_line_num,
                           "mu_list_create: %s", mu_strerror (status));
      exit (1);
    }
}

void
multiline_begin ()
{
  char *p = yytext + 5; /* past the text: keyword */

  if (*p == '-')
    {
      strip_tabs = 1;
      p++;
    }
  else
    strip_tabs = 0;

  if (!isspace (*p))
    {
      char *endp;
      int len;
      
      for (endp = p; *endp; endp++)
	if (isspace (*endp))
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
	  yyerror (_("Not enough memory"));
	  exit (1);
	}
    }

  line_begin ();
}

void
line_finish ()
{
  mu_iterator_t itr;
  int length = 0;
  char *p;
  
  if (!string_list || mu_list_get_iterator (string_list, &itr))
    return;

  /* Count number of characters in the multiline */
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      char *s;
      mu_iterator_current (itr, (void **)&s);
      length += strlen (s);
    }

  /* Copy the contents */
  yylval.string = mu_sieve_malloc (sieve_machine, length + 1);
  p = yylval.string;
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      char *s;
      mu_iterator_current (itr, (void **)&s);
      strcpy (p, s);
      p += strlen (s);
      free (s);
    }
  *p = 0;
  mu_iterator_destroy (&itr);
  mu_list_destroy (&string_list);
}

void
multiline_finish ()
{
  line_finish ();
}

void
ident (const char *text)
{
  yylval.string = strdup (text);
  if (!yylval.string)
    {
      yyerror (_("Not enough memory"));
      exit (1);
    }
}

/* Escapes the last character from yytext */
char *
str_unescape (char *text, size_t len)
{
  char *str = mu_sieve_alloc (len);
  memcpy (str, text, len - 2);
  str[len - 2] = mu_argcv_unquote_char (text[len - 1]);
  str[len - 1] = 0;
  return str;
}

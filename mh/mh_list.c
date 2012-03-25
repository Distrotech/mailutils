/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005-2007, 2009-2012 Free Software Foundation,
   Inc.

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

/* MH mhl command */

#include <mh.h>

/* *********************** Compiler for MHL formats *********************** */

typedef struct
{
  char *filename;
  int line;
}
locus_t;

enum mhl_type
{
  stmt_cleartext,
  stmt_component,
  stmt_variable
};

enum mhl_datatype
{
  dt_flag,
  dt_integer,
  dt_string,
  dt_format
};

typedef union mhl_value {
  char *str;
  int num;
  mh_format_t *fmt;
} mhl_value_t;

typedef struct mhl_variable
{
  int id;
  char *name;
  int type;
}
mhl_variable_t;  

typedef struct mhl_stmt mhl_stmt_t;
typedef struct mhl_stmt_variable mhl_stmt_variable_t;
typedef struct mhl_stmt_component mhl_stmt_component_t;

struct mhl_stmt_variable
{
  mhl_variable_t *id;
  mhl_value_t value;
};
  
struct mhl_stmt_component
{
  char *name;
  mu_list_t format;
};

struct mhl_stmt
{
  enum mhl_type type;
  union
  {
    char *cleartext;
    mhl_stmt_variable_t variable;
    mhl_stmt_component_t component;
  } v;
};

static mhl_variable_t *variable_lookup (char *name);

static mhl_stmt_t *
stmt_alloc (enum mhl_type type)
{
  mhl_stmt_t *p = mu_alloc (sizeof (*p));
  p->type = type;
  return p;
}

static int
compdecl (char **str, char **compname)
{
  char *p;
  
  for (p = *str; *p && !mu_isspace (*p); p++)
    {
      if (*p == ':')
	{
	  int len = p - *str;
	  *compname = mu_alloc (len + 1);
	  memcpy (*compname, *str, len);
	  (*compname)[len] = 0;
	  *str = p + 1;
	  return 0;
	}
    }
  return 1;
}

static void parse_variable (locus_t *loc, mu_list_t formlist, char *str);

static void
parse_cleartext (locus_t *loc, mu_list_t formlist, char *str)
{
  int len;
  mhl_stmt_t *stmt = stmt_alloc (stmt_cleartext);
  stmt->v.cleartext = mu_strdup (str);
  len = strlen (stmt->v.cleartext);
  if (len > 0 && stmt->v.cleartext[len-1] == '\n')
    stmt->v.cleartext[len-1] = 0;
  mu_list_append (formlist, stmt);
}

static void
parse_component (locus_t *loc, mu_list_t formlist, char *compname, char *str)
{
  int rc;
  mhl_stmt_t *stmt = stmt_alloc (stmt_component);
  stmt->v.component.name = compname;
  if ((rc = mu_list_create (&stmt->v.component.format)) != 0)
    {
      mu_error ("%s:%d: mu_list_create: %s",
		loc->filename,
		loc->line,
		mu_strerror (rc));
      exit (1); /* FIXME */
    }
  parse_variable (loc, stmt->v.component.format, str);
  mu_list_append (formlist, stmt);
}

static void
parse_variable (locus_t *loc, mu_list_t formlist, char *str)
{
  size_t i;
  struct mu_wordsplit ws;
  mh_format_t fmt;
  int wsflags;
  
  if (strncmp (str, "ignores=", 8) == 0 && str[8] != '"')
    {
      /* A hack to allow for traditional use of "ignores=", i.e.
	 as a single statement on a line, without double-quotes around
	 the argument */
      wsflags = MU_WRDSF_NOCMD | MU_WRDSF_NOVAR | MU_WRDSF_NOSPLIT;
    }
  else
    {
      ws.ws_delim = ",";
      wsflags = MU_WRDSF_DEFFLAGS|MU_WRDSF_DELIM|
	        MU_WRDSF_WS|MU_WRDSF_RETURN_DELIMS;
    }
  if (mu_wordsplit (str, &ws, wsflags))
    {
      mu_error ("%s:%d: mu_wordsplit(%s): %s",
		loc->filename,
		loc->line,
		str,
		mu_wordsplit_strerror (&ws));
      exit (1);
    }

  for (i = 0; i < ws.ws_wordc; i++)
    {
      mhl_stmt_t *stmt;
      char *name = ws.ws_wordv[i];
      char *value = NULL;
      mhl_variable_t *var;
      
      value = strchr (name, '=');
      if (value)
	*value++ = 0;
      stmt = stmt_alloc (stmt_variable);
      var = variable_lookup (name);
      if (!var)
	{
	  mu_error (_("%s:%d: unknown variable: %s"),
		    loc->filename,
		    loc->line,
		    name);
	  exit (1);
	}

      if ((var->type == dt_flag && value)
	  || (var->type != dt_flag && !value))
	{
	  mu_error (_("%s:%d: wrong datatype for %s"),
		    loc->filename,
		    loc->line,
		    var->name);
	  exit (1);
	}

      switch (var->type)
	{
	case dt_string:
	  stmt->v.variable.value.str = mu_strdup (value);
	  break;

	case dt_integer:
	  stmt->v.variable.value.num = strtoul (value, NULL, 0);
	  break;

	case dt_format:
	  if (mh_format_parse (value, &fmt))
	    {
	      mu_error (_("%s:%d: bad format string"),
			loc->filename,
			loc->line);
	      exit (1);
	    }
	  stmt->v.variable.value.fmt = mu_alloc (sizeof (mh_format_t));
	  *stmt->v.variable.value.fmt = fmt;
	  break;

	case dt_flag:
	  stmt->v.variable.value.num = strcmp (var->name, name) == 0;
	  break;
	}
      stmt->v.variable.id = var;
      mu_list_append (formlist, stmt);

      i++;
      if (i < ws.ws_wordc && ws.ws_wordv[i][0] != ',')
	{
	  mu_error (_("%s:%d: syntax error"), loc->filename, loc->line);
	  exit (1);
	}
    }
  mu_wordsplit_free (&ws);
}

static int
parse_line (locus_t *loc, mu_list_t formlist, char *str)
{
  char *compname;
  
  if (*str == ':')
    parse_cleartext (loc, formlist, str+1);
  else if (compdecl (&str, &compname) == 0)
    parse_component (loc, formlist, compname, str);
  else
    parse_variable (loc, formlist, str);
  return 0;
}

mu_list_t 
mhl_format_compile (char *name)
{
  mu_stream_t stream;
  mu_list_t formlist;
  char *buf = NULL;
  size_t size = 0, n;
  locus_t loc;
  int rc;

  rc = mu_file_stream_create (&stream, name, MU_STREAM_READ);
  if (rc)
    {
      mu_error (_("cannot open format file %s: %s"), name, mu_strerror (rc));
      return NULL;
    }

  if ((rc = mu_list_create (&formlist)) != 0)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_create", NULL, rc);
      mu_stream_destroy (&stream);
      return NULL;
    }

  loc.filename = name;
  loc.line = 1;
  while (mu_stream_getline (stream, &buf, &size, &n) == 0 && n > 0)
    {
      char *p = mu_str_stripws (buf);
      if (!(*p == 0 || *p == ';'))
	parse_line (&loc, formlist, p);
      loc.line++;
    }

  free (buf);
  mu_stream_destroy (&stream);
  
  return formlist;
}


/* ********************** Destroy compiled MHL format ********************** */

static void
_destroy_value (enum mhl_datatype type, mhl_value_t *val)
{
  switch (type)
    {
    case dt_string:
      free (val->str);
      break;
	  
    case dt_flag:
    case dt_integer:
      break;

    case dt_format:
      mh_format_free (val->fmt);
      free (val->fmt);
      break;

    default:
      abort ();
    }
}

static int
_destroy_stmt (void *item, void *data)
{
  mhl_stmt_t *stmt = item;

  switch (stmt->type)
    {
    case stmt_cleartext:
      free (stmt->v.cleartext);
      break;

    case stmt_component:
      free (stmt->v.component.name);
      mhl_format_destroy (&stmt->v.component.format);
      break;

    case stmt_variable:
      _destroy_value (stmt->v.variable.id->type, &stmt->v.variable.value);
      break;
      
    default:
      abort ();
    }
  return 0;
}

void
mhl_format_destroy (mu_list_t *fmt)
{
  mu_list_foreach (*fmt, _destroy_stmt, NULL);
  mu_list_destroy (fmt);
}


/* *************************** Runtime functions *************************** */
/* Integer variables */
#define I_WIDTH           0
#define I_LENGTH          1 
#define I_OFFSET          2
#define I_OVERFLOWOFFSET  3
#define I_COMPWIDTH       4
#define I_MAX             5

/* Boolean (flag) variables */
#define B_UPPERCASE       0
#define B_CLEARSCREEN     1
#define B_BELL            2
#define B_NOCOMPONENT     3
#define B_CENTER          4
#define B_LEFTADJUST      5
#define B_COMPRESS        6
#define B_SPLIT           7
#define B_NEWLINE         8
#define B_ADDRFIELD       9
#define B_DATEFIELD      10
#define B_DECODE         12
#define B_DISABLE_BODY   13
#define B_MAX            14
		      
/* String variables */
#define S_OVERFLOWTEXT   0
#define S_COMPONENT      1
#define S_IGNORES        2
#define S_MAX            3
		      
/* Format variables */
#define F_FORMATFIELD    0
#define F_MAX            1

static mhl_variable_t vartab[] = {
  /* Integer variables */
  { I_WIDTH,          "width",          dt_integer  },
  { I_LENGTH,         "length",         dt_integer },
  { I_OFFSET,         "offset",         dt_integer },
  { I_OVERFLOWOFFSET, "overflowoffset", dt_integer },
  { I_COMPWIDTH,      "compwidth",      dt_integer },

  /* Boolean (flag) variables */
  { B_UPPERCASE,      "uppercase",      dt_flag },
  { B_CLEARSCREEN,    "clearscreen",    dt_flag },
  { B_BELL,           "bell",           dt_flag },
  { B_NOCOMPONENT,    "nocomponent",    dt_flag },
  { B_CENTER,         "center",         dt_flag },
  { B_LEFTADJUST,     "leftadjust",     dt_flag },
  { B_COMPRESS,       "compress",       dt_flag },
  { B_SPLIT,          "split",          dt_flag },
  { B_NEWLINE,        "newline",        dt_flag },
  { B_ADDRFIELD,      "addrfield",      dt_flag },
  { B_DATEFIELD,      "datefield",      dt_flag },
  { B_DECODE,         "decode",         dt_flag },
  
  /* String variables */
  { S_OVERFLOWTEXT,   "overflowtext",   dt_string },
  { S_COMPONENT,      "component",      dt_string },
  { S_IGNORES,        "ignores",        dt_string },
  
  /* Format variables */
  { F_FORMATFIELD,    "formatfield",    dt_format },
  { 0 }
};
	      
static mhl_variable_t *
variable_lookup (char *name)
{
  mhl_variable_t *p;

  for (p = vartab; p->name; p++)
    {
      if (p->type == dt_flag
	  && memcmp (name, "no", 2) == 0
	  && strcmp (p->name, name + 2) == 0)
	return p;
	  
      if (strcmp (p->name, name) == 0)
	return p;
    }
  return NULL;
}

struct eval_env
{
  mu_message_t msg;
  mu_stream_t output;
  mu_list_t printed_fields;  /* A list of printed header names */
  int pos;
  int nlines;
  int ivar[I_MAX];
  int bvar[B_MAX];
  char *svar[S_MAX];
  mh_format_t *fvar[F_MAX];
  char *prefix;
};

static int eval_stmt (void *item, void *data);
static void newline (struct eval_env *env);
static void goto_offset (struct eval_env *env, int count);
static void print (struct eval_env *env, char *str, int nloff);

static int
_comp_name (void *item, void *date)
{
  return mu_c_strcasecmp (item, date) == 0;
}

int
header_is_printed (struct eval_env *env, const char *name)
{
  return mu_list_foreach (env->printed_fields, _comp_name, (void*) name) == 1;
}

int
want_header (struct eval_env *env, const char *name)
{
  const char *p, *str;

  for (str = env->svar[S_IGNORES], p = name; *str; str++)
    {
      if (p)
	{
	  if (*p == 0 && *str == ',')
	    break;
	  if (mu_tolower (*p) == mu_tolower (*str))
	    p++;
	  else
	    p = NULL;
	}
      else if (*str == ',')
	p = name;
    }
  if (p && *p == 0)
    return 0;
  return 1;
}

static int
eval_var (struct eval_env *env, mhl_stmt_variable_t *var)
{
  switch (var->id->type)
    {
    case dt_flag:
      env->bvar[var->id->id] = var->value.num;
      break;
      
    case dt_integer:
      env->ivar[var->id->id] = var->value.num;
      break;
      
    case dt_string:
      env->svar[var->id->id] = var->value.str;
      break;

    case dt_format:
      env->fvar[var->id->id] = var->value.fmt;
      break;

    default:
      abort ();
    }
  return 0;
}

static void
ovf_print (struct eval_env *env, char *str, int size, int nloff)
{
  int ovf = 0;

  while (size)
    {
      int len = size;
      if (ovf)
	{
	  goto_offset (env, env->ivar[I_OVERFLOWOFFSET]);
	  if (env->svar[S_OVERFLOWTEXT])
	    {
	      int l = strlen (env->svar[S_OVERFLOWTEXT]);
	      mu_stream_write (env->output,
			       env->svar[S_OVERFLOWTEXT], l, NULL);
	      env->pos += l;
	    }
	}
      else
	{
	  if (env->prefix && !env->bvar[B_NOCOMPONENT])
	    {
	      goto_offset (env, env->ivar[I_OFFSET]);
	      
	      mu_stream_write (env->output, env->prefix,
			       strlen (env->prefix), NULL);
	      env->pos += strlen (env->prefix);
	    }
	  goto_offset (env, nloff);
	}
      
      if (env->pos + len > env->ivar[I_WIDTH])
	{
	  ovf = 1;
	  len = env->ivar[I_WIDTH] - env->pos;
	}
      
      mu_stream_write (env->output, str, len, NULL);
      env->pos += len;
      if (env->pos >= env->ivar[I_WIDTH])
	newline (env);
      str += len;
      size -= len;
    }
}

static void
print (struct eval_env *env, char *str, int nloff)
{
  do
    {
      if (*str == '\n')
	{
	  newline (env);
	  str++;
	}
      else if (*str)
	{
	  size_t size = strcspn (str, "\n");
	  ovf_print (env, str, size, nloff);
	  str += size;
	  if (*str == '\n')
	    {
	      newline (env);
	      str = mu_str_skip_class (str + 1, MU_CTYPE_SPACE);
	    }
	}
    }
  while (*str);
}

static void
newline (struct eval_env *env)
{
  mu_stream_write (env->output, "\n", 1, NULL);
  env->pos = 0;
  if (env->ivar[I_LENGTH] && ++env->nlines >= env->ivar[I_LENGTH])
    {
      /* FIXME: Better to write it directly on the terminal */
      if (env->bvar[B_BELL])
	mu_stream_write (env->output, "\a", 1, NULL);
      if (env->bvar[B_CLEARSCREEN])
	mu_stream_write (env->output, "\f", 1, NULL);
      env->nlines = 0;
    }
}

static void
goto_offset (struct eval_env *env, int count)
{
  for (; env->pos < count; env->pos++)
    mu_stream_write (env->output, " ", 1, NULL);
}

int
print_header_value (struct eval_env *env, char *val)
{
  char *p;

  if (env->fvar[F_FORMATFIELD])
    {
      if (mh_format_str (env->fvar[F_FORMATFIELD], val,
			 env->ivar[I_WIDTH], &p))
	val = p;
    }
    
  if (env->bvar[B_DECODE])
    {
      if (mh_decode_2047 (val, &p) == 0)
	val = p;
    }
  
  if (env->bvar[B_UPPERCASE])
    {
      for (p = val; *p; p++)
	*p = mu_toupper (*p);
    }

  if (env->bvar[B_COMPRESS])
    for (p = val; *p; p++)
      if (*p == '\n')
	*p = ' ';

  if (env->bvar[B_LEFTADJUST])
    {
      for (p = val; *p && mu_isspace (*p); p++)
	;
    }
  else
    p = val;
  
  print (env, p, env->ivar[I_COMPWIDTH]);
  return 0;
}

void
print_component_name (struct eval_env *env)
{
  if (!env->bvar[B_NOCOMPONENT])
    {
      print (env, env->svar[S_COMPONENT], 0);
      if (mu_c_strcasecmp (env->svar[S_COMPONENT], "body"))
	print (env, ": ", 0);
    }
}

int
eval_component (struct eval_env *env, char *name)
{
  mu_header_t hdr;
  char *val;

  mu_message_get_header (env->msg, &hdr);
  if (mu_header_aget_value (hdr, name, &val))
    return 0;

  print_component_name (env);
  mu_list_append (env->printed_fields, name);
  print_header_value (env, val);
  free (val);
  if (env->bvar[B_NEWLINE])
    newline (env);
  return 0;
}

int
eval_body (struct eval_env *env)
{
  mu_stream_t input = NULL;
  mu_stream_t dstr = NULL;
  char buf[128]; /* FIXME: Fixed size. Bad */
  size_t n;
  mu_body_t body = NULL;
  int nl;
  
  if (env->bvar[B_DISABLE_BODY])
    return 0;

  env->prefix = env->svar[S_COMPONENT];

  mu_message_get_body (env->msg, &body);
  mu_body_get_streamref (body, &input);

  if (env->bvar[B_DECODE])
    {
      mu_header_t hdr;
      char *encoding = NULL;

      mu_message_get_header (env->msg, &hdr);
      mu_header_aget_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING, &encoding);
      if (encoding)
	{
	  int rc = mu_filter_create (&dstr, input, encoding,
				     MU_FILTER_DECODE, 
				     MU_STREAM_READ);
	  if (rc == 0)
	    input = dstr;
	  free (encoding);
	}
    }
  
  while (mu_stream_readline (input, buf, sizeof buf, &n) == 0
	 && n > 0)
    {
      goto_offset (env, env->ivar[I_OFFSET]);
      print (env, buf, 0);
      nl = buf[n-1] == '\n';
    }
  mu_stream_destroy (&input);
  if (!nl && env->bvar[B_NEWLINE])
    newline (env);
  return 0;
}

int
eval_extras (struct eval_env *env)
{
  mu_header_t hdr;
  size_t i, num;
  char *str;

  print_component_name (env);
  mu_message_get_header (env->msg, &hdr);
  mu_header_get_field_count (hdr, &num);
  for (i = 1; i <= num; i++)
    {
      if (mu_header_aget_field_name (hdr, i, &str))
	break;
      if (want_header (env, str)
	  && !header_is_printed (env, str))
	{
	  goto_offset (env, env->ivar[I_OFFSET]);
	  print (env, str, 0);
	  print (env, ": ", 0);
	  free (str);
	  mu_header_aget_field_value (hdr, i, &str);
	  print_header_value (env, str);
	  if (i < num && env->bvar[B_NEWLINE])
	    newline (env);
	}
      free (str);
    }
  if (env->bvar[B_NEWLINE])
    newline (env);
  return 0;
}

int
eval_comp (struct eval_env *env, char *compname, mu_list_t format)
{
  struct eval_env lenv = *env;
  
  mu_list_foreach (format, eval_stmt, &lenv);

  goto_offset (&lenv, lenv.ivar[I_OFFSET]);

  if (!lenv.svar[S_COMPONENT])
    lenv.svar[S_COMPONENT] = compname;
  
  if (strcmp (compname, "extras") == 0)
    eval_extras (&lenv);
  else if (strcmp (compname, "body") == 0)
    eval_body (&lenv);
  else
    eval_component (&lenv, compname);
  
  env->pos = lenv.pos;
  env->nlines = lenv.nlines;
  
  return 0;
}

static int
eval_stmt (void *item, void *data)
{
  mhl_stmt_t *stmt = item;
  struct eval_env *env = data;

  switch (stmt->type)
    {
    case stmt_cleartext:
      print (env, stmt->v.cleartext, 0);
      newline (env);
      break;
      
    case stmt_component:
      eval_comp (env, stmt->v.component.name, stmt->v.component.format);
      break;
      
    case stmt_variable:
      eval_var (env, &stmt->v.variable);
      break;
      
    default:
      abort ();
    }

  return 0;
}

int
mhl_format_run (mu_list_t fmt,
		int width, int length, int flags,
		mu_message_t msg, mu_stream_t output)
{
  int rc;
  struct eval_env env;

  /* Initialize the environment */
  memset (&env, 0, sizeof (env));

  env.bvar[B_NEWLINE] = 1;
  mu_list_create (&env.printed_fields);
  env.ivar[I_WIDTH] = width;
  env.ivar[I_LENGTH] = length;
  env.bvar[B_CLEARSCREEN] = flags & MHL_CLEARSCREEN;
  env.bvar[B_BELL] = flags & MHL_BELL;
  env.bvar[B_DECODE] = flags & MHL_DECODE;
  env.bvar[B_DISABLE_BODY] = flags & MHL_DISABLE_BODY;
  env.pos = 0;
  env.nlines = 0;
  env.msg = msg;
  env.output = output;
  rc = mu_list_foreach (fmt, eval_stmt, &env);
  mu_list_destroy (&env.printed_fields);
  return rc;
}

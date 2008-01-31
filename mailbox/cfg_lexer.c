/* cfg_lexer.c -- default lexer for Mailutils configuration files
   Copyright (C) 2007, 2008 Free Software Foundation, Inc.

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
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/argcv.h>
#include <mailutils/nls.h>
#include <mailutils/cfg.h>
#include <mailutils/list.h>

#include "cfg_parser.h"

struct lexer_data
{
  char *buffer;
  char *curp;
  mu_list_t mpool;
  char *cbuf;
  size_t cbufsize;
  size_t cbuflevel;
};

#define CBUFINCR 256

static void
cbuf_grow (struct lexer_data *datp, const char *str, size_t len)
{
  if (datp->cbufsize - datp->cbuflevel < len)
    {
      size_t n = ((datp->cbuflevel + len + CBUFINCR - 1) / CBUFINCR);
      datp->cbufsize = n * CBUFINCR;
      datp->cbuf = realloc (datp->cbuf, datp->cbufsize);
      if (!datp->cbuf)
	{
	  mu_error ("%s", mu_strerror (ENOMEM));
	  abort ();
	}
    }
  memcpy (datp->cbuf + datp->cbuflevel, str, len);
  datp->cbuflevel += len;
}

static void
cbuf_1grow (struct lexer_data *datp, char c)
{
  cbuf_grow (datp, &c, 1);
}

static void
_mpool_destroy_item (void *p)
{
  free (p);
}

static char *
cbuf_finish (struct lexer_data *datp)
{
  char *p = malloc (datp->cbuflevel);
  if (!p)
    {
      mu_error ("%s", mu_strerror (ENOMEM));
      abort ();
    }
  memcpy (p, datp->cbuf, datp->cbuflevel);
  datp->cbuflevel = 0;
  if (!datp->mpool)
    {
      mu_list_create (&datp->mpool);
      mu_list_set_destroy_item (datp->mpool, _mpool_destroy_item);
    }
  mu_list_append (datp->mpool, p);
  return p;
}

static void
skipws (struct lexer_data *p)
{
  while (*p->curp && isspace (*p->curp))
    {
      if (*p->curp == '\n')
	mu_cfg_locus.line++;
      p->curp++;
    }
}

static void
skipline (struct lexer_data *p)
{
  while (*p->curp && *p->curp != '\n')
    p->curp++;
}

static int
continuation_line_p (struct lexer_data *p, int quote)
{
  skipws (p);
  return *p->curp == quote;
}

static void
copy_string0 (struct lexer_data *p, int unquote)
{
  int quote;
  do
    {
      quote = *p->curp++;

      while (*p->curp)
	{
	  if (*p->curp == '\\')
	    {
	      char c;
	      if (*++p->curp == 0)
		{
		  cbuf_1grow (p, '\\');
		  break;
		}
	      if (*p->curp == '\n')
		{
		  p->curp++;
		  continue;
		}
	      if (!unquote)
		c = *p->curp;
	      else
		c = mu_argcv_unquote_char (*p->curp);
	      cbuf_1grow (p, c);
	      p->curp++;
	    }
	  else if (*p->curp == quote)
	    {
	      p->curp++;
	      break;
	    }
	  else
	    {
	      cbuf_1grow (p, *p->curp);
	      p->curp++;
	    }
	}
    }
  while (continuation_line_p (p, quote));
}
  
static char *
copy_string (struct lexer_data *p)
{
  copy_string0 (p, 1);
  cbuf_1grow (p, 0);
  return cbuf_finish (p);
}

static char *
copy_to (struct lexer_data *p, const char *delim)
{
  while (*p->curp)
    {
      if (*p->curp == '"' || *p->curp == '\'')
	{
	  int quote = *p->curp;
	  cbuf_1grow (p, quote);
	  copy_string0 (p, 0);
	  cbuf_1grow (p, quote);
	  continue;
	}
      
      if (strchr (delim, *p->curp))
	break;
      if (*p->curp == '\n')
	mu_cfg_locus.line++;
      cbuf_1grow (p, *p->curp);
      p->curp++;
    } 
  cbuf_1grow (p, 0);
  return cbuf_finish (p);
}

static int
isword (int c)
{
  if (mu_cfg_tie_in)
    return c && c != ';' && c != '{';
  
  return isalnum (c) || c == '_' || c == '-' || c == '.';
}

static char *
copy_alpha (struct lexer_data *p)
{
  do
    {
      if (mu_cfg_tie_in && (*p->curp == '"' || *p->curp == '\''))
	{
	  int quote = *p->curp;
	  cbuf_1grow (p, quote);
	  copy_string0 (p, 0);
	  cbuf_1grow (p, quote);
	  continue;
	}

      if (*p->curp == '\n')
	mu_cfg_locus.line++;
      cbuf_1grow (p, *p->curp);
      p->curp++;
    } while (*p->curp && isword (*p->curp));
  cbuf_1grow (p, 0);
  return cbuf_finish (p);
}

#define LEX_DEBUG(tok, arg1, arg2) \
 do \
   { \
     if (mu_debug_check_level (dbg, MU_DEBUG_TRACE2)) \
       { \
	 mu_debug_set_locus (dbg, \
			     mu_cfg_locus.file, mu_cfg_locus.line); \
	 mu_cfg_format_error (dbg, MU_DEBUG_TRACE2, "TOKEN %s \"%s\" \"%s\"", \
			      tok, \
			      arg1 ? arg1 : "", \
			      arg2 ? arg2 : ""); \
       } \
   } \
 while (0)

static void
rtrim (char *arg)
{
  int len = strlen (arg);
  while (len > 0 && strchr (" \t\n\r", arg[len-1]))
    len--;
  arg[len] = 0;
}

int
default_lexer (void *dp, mu_debug_t dbg)
{
  struct lexer_data *p = dp;
  char *save_start;
  char *tag, *label;
  extern int mu_cfg_yydebug;
  
again:
  skipws (p);

  if (*p->curp == '#')
    {
      const char *start = ++p->curp;
      skipline (p);
      if (strncmp (start, "debug=", 6) == 0)
	{
	  mu_log_level_t lev;
	  if (p->curp[0] == '\n')
	    {
	      mu_cfg_locus.line++;
	      *p->curp++ = 0;
	    }
	  if (mu_debug_level_from_string (start + 6, &lev, dbg) == 0)
	    {
	      mu_debug_set_level (dbg, lev);
	      mu_cfg_yydebug = lev & MU_DEBUG_LEVEL_MASK (MU_DEBUG_TRACE1);
	    }
	}
      goto again;
    }
    
  if (*p->curp == '/' && p->curp[1] == '/')
    {
      skipline (p);
      goto again;
    }
	
  if (*p->curp == '/' && p->curp[1] == '*')
    {
      int keep_line = mu_cfg_locus.line;

      p->curp += 2;
      do
	{
	  while (*p->curp != '*')
	    {
	      if (*p->curp == 0)
		{
		  mu_cfg_perror (&mu_cfg_locus,
                       _("unexpected EOF in comment started at line %d"),
				 keep_line);
		  return 0;
		}
	      else if (*p->curp == '\n')
		mu_cfg_locus.line++;
	      ++p->curp;
	    }
	} while (*++p->curp != '/');
      ++p->curp;
      goto again;
    }

  if (*p->curp == 0)
    {
      LEX_DEBUG ("EOF", NULL, NULL);
      return 0;
    }

  if (*p->curp == '"' || *p->curp == '\'')
    {
      mu_cfg_yylval.string = copy_string (p);
      LEX_DEBUG ("STRING", mu_cfg_yylval.string, NULL); 
      return MU_CFG_STRING_TOKEN;
    }

  if (mu_cfg_tie_in)
    {
      mu_cfg_yylval.string = copy_alpha (p);
      LEX_DEBUG ("STRING", mu_cfg_yylval.string, NULL); 
      return MU_CFG_STRING_TOKEN;
    }
	
  if (*p->curp == '}')
    {
      p->curp++;
      memset (&mu_cfg_yylval.node, 0, sizeof mu_cfg_yylval.node);
      mu_cfg_yylval.node.locus = mu_cfg_locus;
      LEX_DEBUG ("END", NULL, NULL);
      return MU_CFG_END_TOKEN;
    }

  if (*p->curp == ';')
    {
      p->curp++;
      LEX_DEBUG ("EOL", NULL, NULL);
      return MU_CFG_EOL_TOKEN;
    }

  tag = copy_alpha (p);
  skipws (p);
  
  if (*tag == '"')
    {
      mu_cfg_yylval.string = tag;
      LEX_DEBUG ("STRING", mu_cfg_yylval.string, NULL); 
      return MU_CFG_STRING_TOKEN;
    }
	
  save_start = p->curp;
  if (*p->curp != '{')
    {
      label = copy_to (p, ";{");
      rtrim (label);
    }
  else
    label = NULL;
  if (*p->curp == '{')
    {
      p->curp++;
      mu_cfg_yylval.node.tag_name = tag;
      mu_cfg_yylval.node.locus = mu_cfg_locus;
      mu_cfg_yylval.node.tag_label = label;
      LEX_DEBUG ("START", tag, label); 
      return MU_CFG_START_TOKEN;
    }
  else
    {
      p->curp = save_start;
      mu_cfg_yylval.string = tag;
    }
  LEX_DEBUG ("STRING", mu_cfg_yylval.string, NULL); 
  return MU_CFG_STRING_TOKEN; 
}


int
mu_get_config (const char *file, const char *progname,
               struct mu_cfg_param *progparam, int flags, void *target_ptr)
{
  struct lexer_data data;
  struct stat st;
  int fd;
  int rc;
  mu_cfg_tree_t *parse_tree;
  
  if (stat (file, &st))
    {
      if (errno != ENOENT)
	mu_error (_("can't stat `%s'"), file);
      return -1;
    }
  fd = open (file, O_RDONLY);
  if (fd == -1)
    {
      mu_error (_("cannot open config file `%s'"), file);
      return -1;
    }

  if (flags & MU_PARSE_CONFIG_VERBOSE)
    mu_error (_("Info: parsing file `%s'"), file);
  
  memset (&data, 0, sizeof data);
  data.buffer = malloc (st.st_size+1);
        
  read (fd, data.buffer, st.st_size);
  data.buffer[st.st_size] = 0;
  close (fd);
  data.curp = data.buffer;

  /* Parse configuration */
  mu_cfg_locus.file = (char*) file;
  mu_cfg_locus.line = 1;
  rc = mu_cfg_parse (&parse_tree,
		     &data,
		     default_lexer,
		     NULL,
		     NULL,
		     NULL);

  if (rc == 0)
    rc = mu_cfg_tree_reduce (parse_tree, progname, progparam, flags,
			     target_ptr);

  mu_cfg_destroy_tree (&parse_tree);
  mu_list_destroy (&data.mpool);
  free (data.cbuf);
  free (data.buffer);

  if (flags & MU_PARSE_CONFIG_VERBOSE)
    mu_error (_("Info: finished parsing file `%s'"), file);
  
  return rc;
}


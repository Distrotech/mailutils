/* cfg_lexer.c -- default lexer for Mailutils configuration files
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <mailutils/argcv.h>
#include <mailutils/nls.h>
#include <mailutils/cfg.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/mutil.h>

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free
#include "obstack.h"
#include "cfg_parser.h"

struct lexer_data
{
  char *buffer;
  char *curp;
  struct obstack stk;
};

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
isword (int c)
{
  if (mu_cfg_tie_in)
    return c && c != ';';
  
  return isalnum (c) || c == '_' || c == '-';
}

static char *
copy_alpha (struct lexer_data *p)
{
  do
    {
      if (*p->curp == '\n')
	mu_cfg_locus.line++;
      obstack_1grow (&p->stk, *p->curp);
      p->curp++;
    } while (*p->curp && isword(*p->curp));
  obstack_1grow (&p->stk, 0);
  return obstack_finish (&p->stk);
}

static char *
copy_string(struct lexer_data *p)
{
  int quote = *p->curp++;

  while (*p->curp)
    {
      if (*p->curp == '\\')
	{
	  char c;
	  if (*++p->curp == 0)
	    {
	      obstack_1grow (&p->stk, '\\');
	      break;
	    }
	  c = mu_argcv_unquote_char (*p->curp);
	  if (c == *p->curp)
	    {
	      obstack_1grow (&p->stk, '\\');
	      obstack_1grow (&p->stk, *p->curp);
	    }
	  else
	    obstack_1grow (&p->stk, c);
	  p->curp++;
	}
      else if (*p->curp == quote)
	{
	  p->curp++;
	  break;
	}
      else
	{
	  obstack_1grow (&p->stk, *p->curp);
	  p->curp++;
	}
    }
  obstack_1grow (&p->stk, 0);
  return obstack_finish (&p->stk);
}

int
default_lexer (void *dp)
{
  struct lexer_data *p = dp;
  char *save_start;
  char *tag, *label;
  
again:
  skipws (p);

  if (*p->curp == '#')
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
		  mu_cfg_perror (p,
				 &mu_cfg_locus,
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
    return 0;

  if (*p->curp == '\"')
    {
      mu_cfg_yylval.string = copy_string (p);
      return MU_CFG_STRING_TOKEN;
    }

  if (mu_cfg_tie_in)
    {
      mu_cfg_yylval.string = copy_alpha (p);
      return MU_CFG_STRING_TOKEN;
    }
	
  if (*p->curp == '}')
    {
      p->curp++;
      memset (&mu_cfg_yylval.node, 0, sizeof mu_cfg_yylval.node);
      mu_cfg_yylval.node.locus = mu_cfg_locus;
      return MU_CFG_END_TOKEN;
    }

  if (*p->curp == ';')
    {
      p->curp++;
      return MU_CFG_EOL_TOKEN;
    }
	
  tag = copy_alpha (p);
  skipws (p);
  if (*p->curp == '"')
    {
      mu_cfg_yylval.string = tag;
      return MU_CFG_STRING_TOKEN;
    }
	
  save_start = p->curp;
  if (*p->curp != '{')
    {
      label = copy_alpha(p);
      skipws(p);
    }
  else
    label = NULL;
  if (*p->curp == '{')
    {
      p->curp++;
      mu_cfg_yylval.node.tag_name = tag;
      mu_cfg_yylval.node.locus = mu_cfg_locus;
      mu_cfg_yylval.node.tag_label = label;
      return MU_CFG_START_TOKEN;
    }
  else
    {
      p->curp = save_start;
      mu_cfg_yylval.string = tag;
    }
  return MU_CFG_STRING_TOKEN; 
}

static struct mu_cfg_section *root_section;

int
mu_config_register_section (const char *parent_path,
			    const char *ident,
			    mu_cfg_section_fp parser,
			    void *data,
			    struct mu_cfg_param *param)
{
  struct mu_cfg_section *parent;
  size_t size = 0; 

  if (!root_section)
    {
      root_section = calloc (1, sizeof (root_section[0]));
      if (!root_section)
	return MU_ERR_NOENT;
    }
  
  if (parent_path)
    {
      if (mu_cfg_find_section (root_section, parent_path, &parent))
	return MU_ERR_NOENT;
    }
  else  
    parent = root_section;

  if (ident)
    {
      struct mu_cfg_section *s;

      if (parent->subsec)
	for (s = parent->subsec; s->ident; s++)
	  size++;
  
      s = realloc (parent->subsec, (size + 2) * sizeof parent->subsec[0]);
      if (!s)
	return ENOMEM;
      parent->subsec = s;
      s = parent->subsec + size;
      s[1].ident = NULL;

      s->ident = strdup (ident);
      s->parser = parser;
      s->data = data;
      s->subsec = NULL;
      s->param = param;
    }
  else
    {
      size_t orig_size = 0;
      struct mu_cfg_param *p;
      if (parent->param)
	for (p = parent->param; p->ident; p++)
	  orig_size++;
      size = 0;
      for (p = param; p->ident; p++)
	size++;
      parent->param = realloc (parent->param,
			       (size + orig_size + 1)
			         * sizeof (parent->param[0]));
      memcpy (parent->param + orig_size, param,
	      size * sizeof (parent->param[0]));
      if (!parent->parser)
	parent->parser = parser;
      if (!parent->data)
	parent->data = data;
    }
  return 0;
}
  
int
mu_config_register_plain_section (const char *parent_path, const char *ident,
				  struct mu_cfg_param *params)
{
  return mu_config_register_section (parent_path, ident, NULL, NULL, params);
}

static int
_mu_parse_config (char *file, char *progname)
{
  struct lexer_data data;
  struct stat st;
  int fd;
  extern int mu_cfg_yydebug;
  int rc;
  mu_cfg_node_t *parse_tree;
  
  mu_cfg_locus.file = file;
  mu_cfg_locus.line = 1;

  if (stat (mu_cfg_locus.file, &st))
    {
      mu_error (_("can't stat `%s'"), mu_cfg_locus.file);
      return -1;
    }
  fd = open (mu_cfg_locus.file, O_RDONLY);
  if (fd == -1)
    {
      if (errno != ENOENT)
	mu_error (_("can't open config file `%s'"),
		 mu_cfg_locus.file);
      return -1;
    }
  data.buffer = malloc (st.st_size+1);
        
  read (fd, data.buffer, st.st_size);
  data.buffer[st.st_size] = 0;
  close (fd);
  data.curp = data.buffer;

  mu_cfg_yydebug = strncmp (data.curp, "#debug", 6) == 0;

  obstack_init (&data.stk);
	
  /* Parse configuration */
  rc = mu_cfg_parse (&parse_tree,
		     &data,
		     default_lexer,
		     NULL,
		     NULL,
		     NULL);

  if (rc == 0)
    {
      rc = mu_cfg_scan_tree (parse_tree, root_section,
			     progname, NULL, NULL, NULL);
    }

  mu_cfg_destroy_tree (&parse_tree);
  obstack_free (&data.stk, NULL);
  free (data.buffer);
  
  return rc;
}

int
mu_parse_config (char *file, char *progname)
{
  int rc;
  char *full_name = mu_tilde_expansion (file, "/", NULL);
  if (full_name)
    {
      if (access (full_name, R_OK) == 0)
	{
	  rc = _mu_parse_config (full_name, progname);
	  free (full_name);
	}
      else
	rc = ENOENT;
    }
  else
    rc = ENOMEM;
  return rc;
}

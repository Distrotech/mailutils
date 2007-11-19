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
#include <mailutils/monitor.h>
#include <mailutils/refcount.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>

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
isword (int c)
{
  if (mu_cfg_tie_in)
    return c && c != ';';
  
  return isalnum (c) || c == '_' || c == '-' || c == '.';
}

static char *
copy_alpha (struct lexer_data *p)
{
  do
    {
      if (*p->curp == '\n')
	mu_cfg_locus.line++;
      cbuf_1grow (p, *p->curp);
      p->curp++;
    } while (*p->curp && isword (*p->curp));
  cbuf_1grow (p, 0);
  return cbuf_finish (p);
}

static char *
copy_string (struct lexer_data *p)
{
  int quote = *p->curp++;

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
	  c = mu_argcv_unquote_char (*p->curp);
	  if (c == *p->curp)
	    {
	      cbuf_1grow (p, '\\');
	      cbuf_1grow (p, *p->curp);
	    }
	  else
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
  cbuf_1grow (p, 0);
  return cbuf_finish (p);
}

int
default_lexer (void *dp)
{
  struct lexer_data *p = dp;
  char *save_start;
  char *tag, *label;
  
again:
  skipws (p);

  if (*p->curp == '#'
      || (*p->curp == '/' && p->curp[1] == '/'))
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


static struct mu_cfg_cont *root_container;

int
mu_config_create_container (struct mu_cfg_cont **pcont,
			    enum mu_cfg_cont_type type)
{
  struct mu_cfg_cont *cont;
  int rc;
  
  cont = malloc (sizeof (*cont));
  if (!cont)
    return ENOMEM;
  rc = mu_refcount_create (&cont->refcount);
  if (rc)
    free (cont);
  else
    {
      cont->type = type;
      *pcont = cont;
    }
  return rc; 
}  


#define DUP_SHALLOW 0
#define DUP_DEEP    1

struct dup_data
{
  int duptype;
  struct mu_cfg_cont *cont;
  struct mu_cfg_section **endsect;
};

static int dup_container (struct mu_cfg_cont **pcont,
			  struct mu_cfg_section **endsect);

static int
_dup_section_action (void *item, void *cbdata)
{
  int rc;
  struct mu_cfg_cont *cont = item;
  struct dup_data *pdd = cbdata;

  switch (pdd->duptype)
    {
    case DUP_DEEP:
      rc = dup_container (&cont, pdd->endsect);
      if (rc)
	return rc;
      break;
      
    case DUP_SHALLOW:
      break;
    }
  if (!pdd->cont->v.section.subsec)
    {
      int rc = mu_list_create (&pdd->cont->v.section.subsec);
      if (rc)
	return rc;
    }
  return mu_list_append (pdd->cont->v.section.subsec, cont);
}

static int
_dup_param_action (void *item, void *cbdata)
{
  int rc;
  struct mu_cfg_cont *cont = item;
  struct dup_data *pdd = cbdata;
  rc = dup_container (&cont, pdd->endsect);
  if (rc)
    return rc;
  if (!pdd->cont->v.section.param)
    {
      rc = mu_list_create (&pdd->cont->v.section.param);
      if (rc)
	return rc;
    }
  return mu_list_append (pdd->cont->v.section.param, cont);
}
    
static int
dup_container (struct mu_cfg_cont **pcont, struct mu_cfg_section **endsect)
{
  int rc;
  struct mu_cfg_cont *newcont, *oldcont = *pcont;
  struct dup_data dd;

  rc = mu_config_create_container (&newcont, oldcont->type);
  if (rc)
    return rc;

  dd.cont = newcont;
  dd.endsect = endsect;
  switch (oldcont->type)
    {
    case mu_cfg_cont_section:
      newcont->v.section.ident = oldcont->v.section.ident;
      newcont->v.section.parser = oldcont->v.section.parser;
      newcont->v.section.data = oldcont->v.section.data;
      newcont->v.section.subsec = NULL;
      newcont->v.section.param = NULL;
      if (endsect && &oldcont->v.section == *endsect)
	dd.duptype = DUP_SHALLOW;
      else
	dd.duptype = DUP_DEEP;
      mu_list_do (oldcont->v.section.subsec, _dup_section_action, &dd);
      mu_list_do (oldcont->v.section.param, _dup_param_action, &dd);
      if (dd.duptype == DUP_SHALLOW)
	*endsect = &newcont->v.section;
      break;

    case mu_cfg_cont_param:
      newcont->v.param = oldcont->v.param;
      break;
    }
  *pcont = newcont;
  return 0;
}


static int
_destroy_container_action (void *item, void *cbdata)
{
  struct mu_cfg_cont *cont = item;
  mu_config_destroy_container (&cont);
  return 0;
}

void
mu_config_destroy_container (struct mu_cfg_cont **pcont)
{
  struct mu_cfg_cont *cont = *pcont;
  unsigned refcount = mu_refcount_dec (cont->refcount);
  switch (cont->type)
    {
    case mu_cfg_cont_section:
      mu_list_do (cont->v.section.subsec, _destroy_container_action, NULL);
      mu_list_destroy (&cont->v.section.subsec);
      mu_list_do (cont->v.section.param, _destroy_container_action, NULL);
      mu_list_destroy (&cont->v.section.param);
      break;

    case mu_cfg_cont_param:
      break;
    }

  if (refcount == 0)
    {
      free (cont);
      *pcont = 0;
    }
}
     

static int
add_parameters (struct mu_cfg_section *sect, struct mu_cfg_param *param)
{
  if (!param)
    return 0;
  if (!sect->param)
    mu_list_create (&sect->param);
  for (; param->ident; param++)
    {
      int rc;
      struct mu_cfg_cont *container;

      rc = mu_config_create_container (&container, mu_cfg_cont_param);
      if (rc)
	return rc;
      container->v.param = *param;
      mu_list_append (sect->param, container);
    }
  return 0;
}

static int
_clone_action (void *item, void *cbdata)
{
  struct mu_cfg_cont *cont = item;
  return mu_config_clone_container (cont);
}

int
mu_config_clone_container (struct mu_cfg_cont *cont)
{
  mu_refcount_inc (cont->refcount);
  switch (cont->type)
    {
    case mu_cfg_cont_section:
      mu_list_do (cont->v.section.subsec, _clone_action, NULL);
      mu_list_do (cont->v.section.param, _clone_action, NULL);
      break;

    case mu_cfg_cont_param:
      break;
    }
  return 0;
}  


int
_mu_config_register_section (struct mu_cfg_cont **proot,
			     const char *parent_path,
			     const char *ident,
			     mu_cfg_section_fp parser,
			     void *data,
			     struct mu_cfg_param *param,
			     struct mu_cfg_section **psection)
{
  int rc;
  struct mu_cfg_section *root_section;
  struct mu_cfg_section *parent;
  
  if (!*proot)
    {
      rc = mu_config_create_container (proot, mu_cfg_cont_section);
      if (rc)
	return rc;
      memset (&(*proot)->v.section, 0, sizeof (*proot)->v.section);
    }
  
  root_section = &(*proot)->v.section;
  
  if (parent_path)
    {
      if (mu_cfg_find_section (root_section, parent_path, &parent))
	return MU_ERR_NOENT;
    }
  else  
    parent = root_section;

  if (mu_refcount_value ((*proot)->refcount) > 1)
    {
      /* It is a clone, do copy-on-write */
      rc = dup_container (proot, &parent);
      if (rc)
	return rc;
    }

  if (ident)
    {
      struct mu_cfg_cont *container;
      struct mu_cfg_section *s;
      
      if (!parent->subsec)
	mu_list_create (&parent->subsec);
      mu_config_create_container (&container, mu_cfg_cont_section);
      mu_list_append (parent->subsec, container); 
      s = &container->v.section;

      s->ident = strdup (ident);
      s->parser = parser;
      s->data = data;
      s->subsec = NULL;
      s->param = NULL;
      add_parameters (s, param);
      if (psection)
	*psection = s;
    }
  else
    {
      add_parameters (parent, param);
      if (!parent->parser)
	parent->parser = parser;
      if (!parent->data)
	parent->data = data;
      if (psection)
	*psection = parent;
    }
  return 0;
}
  
int
mu_config_register_section (const char *parent_path,
			    const char *ident,
			    mu_cfg_section_fp parser,
			    void *data,
			    struct mu_cfg_param *param)
{
  return _mu_config_register_section (&root_container,
				      parent_path,
				      ident, parser, data, param, NULL);
}

int
mu_config_register_plain_section (const char *parent_path, const char *ident,
				  struct mu_cfg_param *params)
{
  return mu_config_register_section (parent_path, ident, NULL, NULL, params);
}

static int
prog_parser (enum mu_cfg_section_stage stage,
	     const mu_cfg_node_t *node,
	     void *section_data, void *call_data)
{
  if (stage == mu_cfg_section_start)
    {
      return strcmp (node->tag_label, call_data);
    }
  return 0;
}

static char *
make_file_name (const char *dir, const char *file)
{
  char *tmp;
  size_t len = strlen (dir) + 1 + strlen (file);
  tmp = malloc (len + 1);
  if (!tmp)
    {
      mu_error ("%s", mu_strerror (errno));
      exit (1);
    }
  strcpy (tmp, dir);
  strcat (tmp, "/");
  strcat (tmp, file);
  return tmp;
}

struct include_data
{
  const char *progname;
  struct mu_cfg_param *progparam;
  int global;
};

static int _mu_parse_config (char *file, char *progname,
			     struct mu_cfg_param *progparam, int global);

static int
_cb_include (mu_cfg_locus_t *locus, void *data, char *arg)
{
  int ret = 0;
  struct stat sb;
  char *dirname = arg;
  struct include_data *idp = data;
  char *tmp = NULL;
  
  if (dirname[0] != '/')
    dirname = tmp = make_file_name (SYSCONFDIR, dirname);
  
  if (stat (dirname, &sb) == 0)
    {
      if (S_ISDIR (sb.st_mode))
	{
	  char *file = make_file_name (dirname, idp->progname);
	  ret = _mu_parse_config (file, idp->progname, idp->progparam, 0);
	}
      else
	ret = _mu_parse_config (dirname, idp->progname, idp->progparam,
				idp->global);
    }
  else if (errno == ENOENT)
    {
      mu_cfg_perror (NULL, locus,
		     _("include directory does not exist"));
      ret = 1;
    }
  else
    {
      mu_cfg_perror (NULL, locus,
		     _("cannot stat include directory: %s"),
		     mu_strerror (errno));
      ret = 1;
    }
  free (tmp);
  return ret;
}

static int
_mu_parse_config (char *file, char *progname,
		  struct mu_cfg_param *progparam, int global)
{
  struct lexer_data data;
  struct stat st;
  int fd;
  extern int mu_cfg_yydebug;
  int rc;
  mu_cfg_node_t *parse_tree;
  
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

  if (mu_cfg_parser_verbose)
    mu_error (_("Info: parsing file `%s'"), file);
  
  memset (&data, 0, sizeof data);
  data.buffer = malloc (st.st_size+1);
        
  read (fd, data.buffer, st.st_size);
  data.buffer[st.st_size] = 0;
  close (fd);
  data.curp = data.buffer;

  if (mu_cfg_parser_verbose > 1
      || strncmp (data.curp, "#debug", 6) == 0)
    mu_cfg_yydebug = 1;
  else
    mu_cfg_yydebug = 0;
  
  /* Parse configuration */
  mu_cfg_locus.file = file;
  mu_cfg_locus.line = 1;
  rc = mu_cfg_parse (&parse_tree,
		     &data,
		     default_lexer,
		     NULL,
		     NULL,
		     NULL);

  if (rc == 0 && root_container)
    {
      struct mu_cfg_cont *cont = root_container;
      struct include_data idata;
      struct mu_cfg_param mu_include_param[] = {
	{ "include", mu_cfg_callback, &idata, _cb_include },
	{ NULL }
      };

      mu_config_clone_container (cont);
      idata.progname = progname;
      idata.progparam = progparam;
      idata.global = global;
      _mu_config_register_section (&cont, NULL, NULL, NULL,
				   progname, mu_include_param, NULL);
      if (global)
	{
	  mu_iterator_t iter;
	  struct mu_cfg_section *prog_sect;
	  struct mu_cfg_cont *old_root = root_container;
	  static struct mu_cfg_param empty_param = { NULL };
	  if (!progparam)
	    progparam = &empty_param;
	  _mu_config_register_section (&cont, NULL, "program", prog_parser,
				       progname,
				       progparam, &prog_sect);

	  mu_config_clone_container (old_root);

	  if (old_root->v.section.subsec)
	    {
	      if (!prog_sect->subsec)
		mu_list_create (&prog_sect->subsec);
	      mu_list_get_iterator (old_root->v.section.subsec, &iter);
	      for (mu_iterator_first (iter); !mu_iterator_is_done (iter);
		   mu_iterator_next (iter))
		{
		  struct mu_cfg_section *s;
		  mu_iterator_current (iter, (void**)&s);
		  mu_list_append (prog_sect->subsec, s);
		}
	      mu_iterator_destroy (&iter);
	    }
	  
	  if (old_root->v.section.param)
	    {
	      if (!prog_sect->param)
		mu_list_create (&prog_sect->param);
	      mu_list_get_iterator (old_root->v.section.param, &iter);
	      for (mu_iterator_first (iter); !mu_iterator_is_done (iter);
		   mu_iterator_next (iter))
		{
		  struct mu_cfg_param *p;
		  mu_iterator_current (iter, (void**)&p);
		  mu_list_append (prog_sect->param, p);
		}
	      mu_iterator_destroy (&iter);
	    }
	}
      else if (progparam)
	_mu_config_register_section (&cont, NULL, NULL, NULL, NULL,
				     progparam, NULL);
      
      rc = mu_cfg_scan_tree (parse_tree, &cont->v.section,
			     progname, NULL, NULL, NULL);
      mu_config_destroy_container (&cont);
    }

  mu_cfg_destroy_tree (&parse_tree);
  mu_list_destroy (&data.mpool);
  free (data.cbuf);
  free (data.buffer);

  if (mu_cfg_parser_verbose)
    mu_error (_("Info: finished parsing file `%s'"), file);
  
  return rc;
}

int
mu_parse_config (char *file, char *progname,
		 struct mu_cfg_param *progparam, int global)
{
  int rc;
  char *full_name = mu_tilde_expansion (file, "/", NULL);
  if (full_name)
    {
      if (access (full_name, R_OK) == 0)
	{
	  rc = _mu_parse_config (full_name, progname, progparam, global);
	  free (full_name);
	}
      else
	rc = ENOENT;
    }
  else
    rc = ENOMEM;
  return rc;
}

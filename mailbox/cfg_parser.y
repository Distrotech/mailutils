%{
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
#include <mailutils/list.h>
#include <mailutils/iterator.h>  
#include <mailutils/debug.h>
  
int mu_cfg_parser_verbose;
static mu_cfg_node_t *parse_tree;
mu_cfg_locus_t mu_cfg_locus;
int mu_cfg_tie_in;
 
static int _mu_cfg_errcnt;
static mu_cfg_lexer_t _mu_cfg_lexer;
static void *_mu_cfg_lexer_data;
mu_cfg_alloc_t _mu_cfg_alloc;
mu_cfg_free_t _mu_cfg_free;
static mu_debug_t _mu_cfg_debug;
 
static int
yyerror (char *s)
{
  mu_cfg_perror (&mu_cfg_locus, "%s", s);
  return 0;
}
 
static int
yylex ()
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
      mu_cfg_perror (&mu_cfg_locus, _("Not enough memory"));
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

void
mu_cfg_format_error (mu_debug_t debug, size_t level, const char *fmt, ...)
{
  va_list ap;
  
  va_start (ap, fmt);
  mu_debug_vprintf (debug, 0, fmt, ap);
  mu_debug_printf (debug, 0, "\n");
  va_end (ap);
}  

static void
_mu_cfg_vperror (mu_debug_t debug, const mu_cfg_locus_t *loc,
		 const char *fmt, va_list ap)
{
  mu_debug_set_locus (_mu_cfg_debug,
		      loc->file ? loc->file : _("unknown file"),
		      loc->line);
  mu_debug_vprintf (_mu_cfg_debug, 0, fmt, ap);
  mu_debug_printf (_mu_cfg_debug, 0, "\n");
}

static void
_mu_cfg_perror (mu_debug_t debug, const mu_cfg_locus_t *loc,
		const char *fmt, ...)
{
  va_list ap;
  
  va_start (ap, fmt);
  _mu_cfg_vperror (debug, loc, fmt, ap);
  va_end (ap);
}

void
mu_cfg_perror (const mu_cfg_locus_t *loc, const char *fmt, ...)
{
  va_list ap;
  
  va_start (ap, fmt);
  _mu_cfg_vperror (_mu_cfg_debug, loc, fmt, ap);
  va_end (ap);
}
  
%}

%token MU_CFG_EOL_TOKEN
%token <node> MU_CFG_START_TOKEN MU_CFG_END_TOKEN
%token <string> MU_CFG_STRING_TOKEN

%type <pnode> tag
%type <nodelist> taglist

%union
{
  mu_cfg_node_t node;
  mu_cfg_node_t *pnode;
  struct
  {
    mu_cfg_node_t *head, *tail;
  } nodelist;
  char *string;
};

%%

input   : taglist
          {
	    parse_tree = $1.head;
          }
        ;

taglist : tag
          {
	    $$.head = $$.tail = $1;
	  }
        | taglist tag
          {
	    $$ = $1;
	    $$.tail->next = $2;
	    $$.tail = $2;
	  }
        ;

opt_eol : /* empty */
        | eol
        ;

eol     : MU_CFG_EOL_TOKEN
        | eol MU_CFG_EOL_TOKEN
        ;

tag     : MU_CFG_START_TOKEN opt_eol taglist MU_CFG_END_TOKEN MU_CFG_EOL_TOKEN
          {
	    if ($4.tag_name && strcmp ($4.tag_name, $1.tag_name))
	      {
		mu_cfg_perror (&$1.locus,
			       _("Tag %s not closed"),
			       $1.tag_name);
		mu_cfg_perror (&$4.locus,
			       _("Found closing %s tag instead"),
			       $4.tag_name);
		_mu_cfg_errcnt++;
	      }
	    $$ = mu_cfg_alloc_node (mu_cfg_node_tag, &$1.locus,
				    $1.tag_name, $1.tag_label, $3.head);
	  }
        | MU_CFG_STRING_TOKEN { mu_cfg_tie_in++; }
          MU_CFG_STRING_TOKEN { mu_cfg_tie_in = 0; } MU_CFG_EOL_TOKEN
          {
	    $$ = mu_cfg_alloc_node (mu_cfg_node_param, &mu_cfg_locus, $1, $3,
				    NULL);
	  }
        ;

%%

static int	  
_cfg_default_printer (void *unused, size_t level, const char *str)
{
  fprintf (stderr, "%s", str);
  return 0;
}
	  
int
mu_cfg_parse (mu_cfg_tree_t **ptree,
	      void *data, mu_cfg_lexer_t lexer,
	      mu_debug_t debug,
	      mu_cfg_alloc_t palloc, mu_cfg_free_t pfree)
{
  int rc;
  mu_cfg_tree_t *tree;
  
  _mu_cfg_lexer = lexer;
  _mu_cfg_lexer_data = data;
  if (debug)
    _mu_cfg_debug = debug;
  else
    {
      mu_debug_create (&_mu_cfg_debug, NULL);
      mu_debug_set_print (_mu_cfg_debug, _cfg_default_printer, NULL);
    }
  _mu_cfg_alloc = palloc ? palloc : malloc;
  _mu_cfg_free = pfree ? pfree : free;
  _mu_cfg_errcnt = 0;
  mu_cfg_tie_in = 0;
  rc = yyparse ();
  if (rc == 0 && _mu_cfg_errcnt)
    rc = 1;
  /* FIXME if (rc) free_memory; else */

  tree = _mu_cfg_alloc (sizeof (*tree));
  tree->debug = _mu_cfg_debug;
  tree->alloc = _mu_cfg_alloc;
  tree->free = _mu_cfg_free;
  tree->node = parse_tree;
  parse_tree = NULL;
  *ptree = tree;
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
mu_cfg_destroy_tree (mu_cfg_tree_t **ptree)
{
  if (ptree && *ptree)
    {
      mu_cfg_tree_t *tree = *ptree;
      mu_cfg_postorder (tree->node, free_param, free_section, tree->free);
      mu_debug_destroy (&tree->debug, NULL);
      *ptree = NULL;
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
  mu_cfg_tree_t *tree;
  int error;
};

static struct mu_cfg_cont *
find_container (mu_list_t list, const char *ident, size_t len)
{
  mu_iterator_t iter;
  struct mu_cfg_cont *ret = NULL;
      
  if (len == 0)
    len = strlen (ident);

  mu_list_get_iterator (list, &iter);
  for (mu_iterator_first (iter); !mu_iterator_is_done (iter);
       mu_iterator_next (iter))
    {
      struct mu_cfg_cont *cont;
      mu_iterator_current (iter, (void**) &cont);

      if (strlen (cont->v.ident) == len
	  && memcmp (cont->v.ident, ident, len) == 0)
	{
	  ret = cont;
	  break;
	}
    }
  mu_iterator_destroy (&iter);
  return ret;
}

static struct mu_cfg_section *
find_subsection (struct mu_cfg_section *sec, const char *ident, size_t len)
{
  if (sec)
    {
      if (sec->subsec)
	{
	  struct mu_cfg_cont *cont = find_container (sec->subsec, ident, len);
	  if (cont)
	    return &cont->v.section;
	}
    }
  return NULL;
}

static struct mu_cfg_param *
find_param (struct mu_cfg_section *sec, const char *ident, size_t len)
{
  if (sec)
    {
      if (sec->param)
	{
	  struct mu_cfg_cont *cont = find_container (sec->param, ident, len);
	  if (cont)
	    return &cont->v.param;
	}
    }
  return NULL;
}

static int
push_section (struct scan_tree_data *dat, struct mu_cfg_section *sec)
{
  struct mu_cfg_section_list *p = dat->tree->alloc (sizeof *p);
  if (!p)
    {
      _mu_cfg_perror (dat->tree->debug, NULL, _("not enough memory"));
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
  dat->tree->free (p);
  return sec;
}

#define STRTONUM(s, type, base, res, limit, d)				      \
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
	  _mu_cfg_perror (d, &node->locus,                           	      \
			  _("numeric overflow"));                 	      \
	  return 1;					      		      \
	}								      \
      else if (limit && x > limit)					      \
	{			      					      \
	  _mu_cfg_perror (d, &node->locus,                           	      \
		 	  _("value out of allowed range"));       	      \
	  return 1;					      		      \
	}							      	      \
      sum = x;						      		      \
      *s++;							      	      \
    }								      	      \
  res = sum;                                                                  \
}

#define STRxTONUM(s, type, res, limit, d)			              \
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
  STRTONUM (s, type, base, res, limit, d);		                      \
}

#define GETUNUM(str, type, res, d)					      \
{									      \
  type tmpres;							      	      \
  const char *s = str;                                                        \
  STRxTONUM (s, type, tmpres, 0, d);					      \
  if (*s)								      \
    {							      		      \
      _mu_cfg_perror (d, &node->locus,                                 	      \
	 	      _("not a number (stopped near `%s')"),          	      \
		      s);    					      	      \
      return 1;					      	      		      \
    }								      	      \
  res = tmpres;							              \
}

#define GETSNUM(str, type, res, d)             				      \
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
  STRxTONUM (s, unsigned type, tmpres, limit, d);      		      	      \
  if (*s)								      \
    {							      		      \
      _mu_cfg_perror (d, &node->locus,                                 	      \
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
      _mu_cfg_perror (sdata->tree->debug, &node->locus, _("not an IPv4"));
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
  else if (inet_aton (node->tag_label, &addr) == 0)
    {
      _mu_cfg_perror (sdata->tree->debug, &node->locus,
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
	_mu_cfg_perror (sdata->tree->debug, &node->locus,
			_("not a valid IPv4 address in CIDR"));
	return 1;
      }
      memcpy (astr, node->tag_label, len);
      astr[len] = 0;
      if (inet_aton (astr, &addr) == 0)
	{
	  _mu_cfg_perror (sdata->tree->debug, &node->locus,
			  _("not a valid IPv4 address in CIDR"));
	  return 1;
	}
      addr.s_addr = ntohl (addr.s_addr);

      p++;
      s = p;
      STRxTONUM (s, unsigned long, mask, 0, sdata->tree->debug);
      if (*s == '.')
	{
	  struct in_addr a;
	  if (inet_aton (p, &a) == 0)
	    {
	      _mu_cfg_perror (sdata->tree->debug, &node->locus,
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
	  _mu_cfg_perror (sdata->tree->debug, &node->locus,
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
	  STRxTONUM(p, unsigned short, x, 255, sdata->tree->debug);
	  if (*p != '.')
	    break;
	  addr.s_addr = (addr.s_addr << 8) + x;
	}
		
      if (*p)
	{
	  _mu_cfg_perror (sdata->tree->debug, &node->locus,
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

int
mu_cfg_parse_boolean (const char *str, int *res)
{
  if (strcmp (str, "yes") == 0
      || strcmp (str, "on") == 0
      || strcmp (str, "t") == 0
      || strcmp (str, "true") == 0
      || strcmp (str, "1") == 0)
    *res = 1;
  else if (strcmp (str, "no") == 0
	   || strcmp (str, "off") == 0
	   || strcmp (str, "nil") == 0
	   || strcmp (str, "false") == 0
	   || strcmp (str, "0") == 0)
    *res = 0;
  else
    return 1;
  return 0;
}

static int
parse_bool (struct scan_tree_data *sdata, const mu_cfg_node_t *node, int *res)
{
  if (mu_cfg_parse_boolean (node->tag_label, res))
    {
      _mu_cfg_perror (sdata->tree->debug, &node->locus, _("not a boolean"));
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
      _mu_cfg_perror (sdata->tree->debug, &node->locus,
		      _("unknown keyword `%s'"),
		      node->tag_name);
      return 1;
    }

  switch (param->type)
    {
    case mu_cfg_string:
      {
	size_t len = strlen (node->tag_label);
	char *s = sdata->tree->alloc (len + 1);
	if (!s)
	  {
	    _mu_cfg_perror (sdata->tree->debug, &node->locus,
			    _("not enough memory"));
	    return 1;
	  }
	strcpy (s, node->tag_label);
	/* FIXME: free param->data? */
	*(char**)param->data = s;
	break;
      }
		
    case mu_cfg_short:
      GETSNUM (node->tag_label, short, *(short*)param->data,
	       sdata->tree->debug);
      break;
		
    case mu_cfg_ushort:
      GETUNUM (node->tag_label, unsigned short, *(unsigned short*)param->data,
	       sdata->tree->debug);
      break;
		
    case mu_cfg_int:
      GETSNUM (node->tag_label, int, *(int*)param->data, sdata->tree->debug);
      break;
		
    case mu_cfg_uint:
      GETUNUM (node->tag_label, unsigned int, *(unsigned int*)param->data,
	       sdata->tree->debug);
      break;
            
    case mu_cfg_long:
      GETSNUM (node->tag_label, long, *(long*)param->data,
	       sdata->tree->debug);
      break;
      
    case mu_cfg_ulong:
      GETUNUM (node->tag_label, unsigned long, *(unsigned long*)param->data,
	       sdata->tree->debug);
      break;
		
    case mu_cfg_size:
      GETUNUM (node->tag_label, size_t, *(size_t*)param->data,
	       sdata->tree->debug);
      break;
		
    case mu_cfg_off:
      _mu_cfg_perror (sdata->tree->debug, &node->locus,
		      _("not implemented yet"));
      /* GETSNUM(node->tag_label, off_t, *(off_t*)param->data); */
      return 1;

    case mu_cfg_time:
      GETUNUM (node->tag_label, time_t, *(time_t*)param->data,
	       sdata->tree->debug);
      break;
      
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
      mu_debug_set_locus (sdata->tree->debug, node->locus.file,
			  node->locus.line);
      if (param->callback (sdata->tree->debug, param->data, node->tag_label))
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
	  if (mu_cfg_parser_verbose)
	    {
	      _mu_cfg_perror (sdata->tree->debug, &node->locus,
			      _("unknown section `%s'"),
			      node->tag_name);
	    }
	  return MU_CFG_ITER_SKIP;
	}
      if (!sec->subsec && !sec->param)
	return MU_CFG_ITER_SKIP;
      if (sec->parser &&
	  sec->parser (mu_cfg_section_start, node,
		       sec->data, sdata->call_data, sdata->tree))
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
			   sdata->call_data, sdata->tree))
	    {
	      sdata->error++;
	      return MU_CFG_ITER_SKIP;
	    }
	}
    }
  return MU_CFG_ITER_OK;
}

int
mu_cfg_scan_tree (mu_cfg_tree_t *tree, struct mu_cfg_section *sections,
		  void *data)
{
  struct scan_tree_data dat;
  dat.tree = tree;
  dat.list = NULL;
  dat.error = 0;
  dat.call_data = data;
  if (push_section (&dat, sections))
    return 1;
  mu_cfg_preorder (tree->node, _scan_tree_helper, _scan_tree_end_helper, &dat);
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





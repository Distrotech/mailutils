/* cfg_driver.c -- Main driver for Mailutils configuration files
   Copyright (C) 2007-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   GNU Mailutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.
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
#include <mailutils/argcv.h>
#include <mailutils/nls.h>
#define MU_CFG_COMPATIBILITY /* This source uses deprecated cfg interfaces */
#include <mailutils/cfg.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/util.h>
#include <mailutils/monitor.h>
#include <mailutils/refcount.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/stream.h>
#include <mailutils/assoc.h>
#include <mailutils/alloc.h>


static mu_assoc_t section_tab;

static void
alloc_section_tab ()
{
  if (!section_tab)
    mu_assoc_create (&section_tab, sizeof (struct mu_cfg_cont **),
		     MU_ASSOC_COPY_KEY);
}

int
mu_create_canned_section (char *name, struct mu_cfg_section **psection)
{
  int rc;
  struct mu_cfg_cont **pcont;
  alloc_section_tab ();
  rc = mu_assoc_ref_install (section_tab, name, (void **)&pcont);
  if (rc == 0)
    {
      mu_config_create_container (pcont, mu_cfg_cont_section);
      *psection = &(*pcont)->v.section;
      (*psection)->ident = name;
    }
  else if (rc == MU_ERR_EXISTS)
    *psection = &(*pcont)->v.section;
  return rc;
}

int
mu_create_canned_param (char *name, struct mu_cfg_param **pparam)
{
  int rc;
  struct mu_cfg_cont **pcont;
  alloc_section_tab ();
  rc = mu_assoc_ref_install (section_tab, name, (void **)&pcont);
  if (rc == 0)
    {
      mu_config_create_container (pcont, mu_cfg_cont_param);
      *pparam = &(*pcont)->v.param;
      (*pparam)->ident = name;
    }
  else if (rc == MU_ERR_EXISTS)
    *pparam = &(*pcont)->v.param;
  return rc;
}

struct mu_cfg_cont *
mu_get_canned_container (const char *name)
{
  struct mu_cfg_cont **pcont = mu_assoc_ref (section_tab, name);
  return pcont ? *pcont : NULL;
}


static struct mu_cfg_cont *root_container;

int
mu_config_create_container (struct mu_cfg_cont **pcont,
			    enum mu_cfg_cont_type type)
{
  struct mu_cfg_cont *cont;
  int rc;
  
  cont = calloc (1, sizeof (*cont));
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


struct dup_data
{
  struct mu_cfg_cont *cont;
};

static int dup_container (struct mu_cfg_cont **pcont);

static int
_dup_cont_action (void *item, void *cbdata)
{
  int rc;
  struct mu_cfg_cont *cont = item;
  struct dup_data *pdd = cbdata;

  rc = dup_container (&cont);
  if (rc)
    return rc;

  if (!pdd->cont->v.section.children)
    {
      int rc = mu_list_create (&pdd->cont->v.section.children);
      if (rc)
	return rc;
    }
  return mu_list_append (pdd->cont->v.section.children, cont);
}

static int
dup_container (struct mu_cfg_cont **pcont)
{
  int rc;
  struct mu_cfg_cont *newcont, *oldcont = *pcont;
  struct dup_data dd;

  rc = mu_config_create_container (&newcont, oldcont->type);
  if (rc)
    return rc;

  dd.cont = newcont;
  switch (oldcont->type)
    {
    case mu_cfg_cont_section:
      newcont->v.section.ident = oldcont->v.section.ident;
      newcont->v.section.label = oldcont->v.section.label;
      newcont->v.section.parser = oldcont->v.section.parser;
      newcont->v.section.target = oldcont->v.section.target;
      newcont->v.section.offset = oldcont->v.section.offset;
      newcont->v.section.docstring = oldcont->v.section.docstring;
      newcont->v.section.children = NULL;
      mu_list_foreach (oldcont->v.section.children, _dup_cont_action, &dd);
      break;

    case mu_cfg_cont_param:
      newcont->v.param = oldcont->v.param;
      break;
    }
  *pcont = newcont;
  return 0;
}


static void
destroy_list (mu_list_t *plist)
{
  mu_list_t list = *plist;
  mu_iterator_t itr = NULL;
  
  if (!list)
    return;

  mu_list_get_iterator (list, &itr);
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      struct mu_cfg_cont *cont, *p;
      mu_iterator_current (itr, (void**)&cont);
      p = cont;
      mu_config_destroy_container (&p);
      if (!p)
	mu_list_remove (list, cont);
    }
  mu_iterator_destroy (&itr);
  if (mu_list_is_empty (list))
    mu_list_destroy (plist);
}

void
mu_config_destroy_container (struct mu_cfg_cont **pcont)
{
  struct mu_cfg_cont *cont = *pcont;
  unsigned refcount = mu_refcount_dec (cont->refcount);
  /* printf ("destr %p-%s: %d\n", cont, cont->v.section.ident, refcount); */
  switch (cont->type)
    {
    case mu_cfg_cont_section:
      destroy_list (&cont->v.section.children);
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
     

int
mu_cfg_section_add_container (struct mu_cfg_section *sect,
			      struct mu_cfg_cont *cont)
{
  if (!cont)
    return 0;
  if (!sect->children)
    mu_list_create (&sect->children);
  return mu_list_append (sect->children, cont);
}

int
mu_cfg_section_add_params (struct mu_cfg_section *sect,
			   struct mu_cfg_param *param)
{
  if (!param)
    return 0;

  for (; param->ident; param++)
    {
      int rc;
      struct mu_cfg_cont *container;

      if (param->type == mu_cfg_section)
	{
	  container = mu_get_canned_container (param->ident);
	  if (!container)
	    {
	      mu_error (_("INTERNAL ERROR: Requested unknown canned "
			  "section %s"),
			param->ident);
	      abort ();
	    }
	  if (param->ident[0] == '.')
	    {
	      mu_iterator_t itr;
	      mu_list_get_iterator (container->v.section.children, &itr);
	      for (mu_iterator_first (itr);
		   !mu_iterator_is_done (itr);
		   mu_iterator_next (itr))
		{
		  struct mu_cfg_cont *c;
		  mu_iterator_current (itr, (void**)&c);
		  mu_config_clone_container (c);
		  if (mu_refcount_value (c->refcount) > 1)
		    dup_container (&c);
		  switch (c->type)
		    {
		    case mu_cfg_cont_section:
		      c->v.section.offset += param->offset;
		      break;

		    case mu_cfg_cont_param:
		      container->v.param.offset += param->offset;
		      break;
		    }
		  mu_cfg_section_add_container (sect, c);
		}
	      mu_iterator_destroy (&itr);
	      continue;
	    }
	  else
	    {
	      mu_config_clone_container (container);
	      if (mu_refcount_value (container->refcount) > 1)
		dup_container (&container);
	      container->v.section.target = param->data;
	      container->v.section.offset = param->offset;
	    }
	}
      else
	{
	  rc = mu_config_create_container (&container, mu_cfg_cont_param);
	  if (rc)
	    return rc;
	  container->v.param = *param;
	}
      mu_cfg_section_add_container (sect, container);
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
  if (!cont)
    return 0;
  mu_refcount_inc (cont->refcount);
  /* printf("clone %p-%s: %d\n", cont, cont->v.section.ident, n); */
  switch (cont->type)
    {
    case mu_cfg_cont_section:
      mu_list_foreach (cont->v.section.children, _clone_action, NULL);
      break;

    case mu_cfg_cont_param:
      break;
    }
  return 0;
}  


int
mu_config_container_register_section (struct mu_cfg_cont **proot,
				      const char *parent_path,
				      const char *ident,
				      const char *label,
				      mu_cfg_section_fp parser,
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
      rc = dup_container (proot);
      if (rc)
	return rc;

      root_section = &(*proot)->v.section;
      
      if (parent_path)
	{
	  if (mu_cfg_find_section (root_section, parent_path, &parent))
	    return MU_ERR_NOENT;
	}
      else  
	parent = root_section;
    }

  if (ident)
    {
      struct mu_cfg_cont *container;
      struct mu_cfg_section *s;
      
      if (!parent->children)
	mu_list_create (&parent->children);
      mu_config_create_container (&container, mu_cfg_cont_section);
      mu_list_append (parent->children, container); 
      s = &container->v.section;

      s->ident = strdup (ident);
      s->label = label ? strdup (label) : NULL;
      s->parser = parser;
      s->children = NULL;
      mu_cfg_section_add_params (s, param);
      if (psection)
	*psection = s;
    }
  else
    {
      mu_cfg_section_add_params (parent, param);
      /* FIXME: */
      if (!parent->parser)
	parent->parser = parser;
      if (psection)
	*psection = parent;
    }
  return 0;
}
  
int
mu_config_root_register_section (const char *parent_path,
				 const char *ident,
				 const char *label,
				 mu_cfg_section_fp parser,
				 struct mu_cfg_param *param)
{
  return mu_config_container_register_section (&root_container,
					       parent_path,
					       ident, label,
					       parser, param, NULL);
}

int
mu_config_register_plain_section (const char *parent_path, const char *ident,
				  struct mu_cfg_param *params)
{
  return mu_config_root_register_section (parent_path, ident, NULL, NULL,
					  params);
}

struct mu_cfg_cont *
mu_config_clone_root_container (void)
{
  struct mu_cfg_cont *cont = root_container;
  mu_config_clone_container (cont);
  return cont;
}

static struct mu_cfg_cont *
mu_build_container (struct mu_cfg_param *param)
{
  struct mu_cfg_cont *cont = mu_config_clone_root_container ();
  mu_config_container_register_section (&cont, NULL, NULL, NULL, NULL,
					param, NULL);
  return cont;
}

int
mu_cfg_tree_reduce (mu_cfg_tree_t *parse_tree,
		    struct mu_cfg_parse_hints *hints,
		    struct mu_cfg_param *progparam,
		    void *target_ptr)
{
  int rc = 0;

  if (!parse_tree)
    return 0;
  if (hints && (hints->flags & MU_PARSE_CONFIG_DUMP))
    {
      int yes = 1;
      mu_stream_t stream;
      
      mu_stdio_stream_create (&stream, MU_STDERR_FD, 0);
      mu_stream_ioctl (stream, MU_IOCTL_FD, MU_IOCTL_FD_SET_BORROW, &yes);
      mu_cfg_format_parse_tree (stream, parse_tree, MU_CFG_FMT_LOCUS);
      mu_stream_destroy (&stream);
    }

  if (root_container)
    {
      struct mu_cfg_cont *cont = mu_build_container (progparam);
      rc = mu_cfg_scan_tree (parse_tree, &cont->v.section, target_ptr,
			     NULL);
      mu_config_destroy_container (&cont);
    }

  return rc;
}

void
mu_format_config_tree (mu_stream_t stream, struct mu_cfg_param *progparam)
{
  struct mu_cfg_cont *cont = mu_build_container (progparam);
  mu_cfg_format_container (stream, cont);
  mu_config_destroy_container (&cont);
}

int
mu_parse_config (const char *file, const char *progname,
		 struct mu_cfg_param *progparam, int flags,
		 void *target_ptr)
{
  int rc;
  char *full_name = mu_tilde_expansion (file, MU_HIERARCHY_DELIMITER, NULL);
  if (full_name)
    {
      if (access (full_name, R_OK) == 0)
	{
	  rc = mu_get_config (full_name, progname, progparam, flags,
			      target_ptr);
	}
      else
	rc = ENOENT;
      free (full_name);
    }
  else
    rc = ENOMEM;
  return rc;
}

static const char *
_first_value_ptr (mu_config_value_t *val)
{
  switch (val->type)
    {
    case MU_CFG_STRING:
      return val->v.string;
      
    case MU_CFG_ARRAY:
      return _first_value_ptr (val->v.arg.v);
      
    case MU_CFG_LIST:
      mu_list_get (val->v.list, 0, (void**) &val);
      return _first_value_ptr (val);
    }
  return "";  
}

int
mu_cfg_assert_value_type (mu_config_value_t *val, int type)
{
  if (!val)
    { 
      mu_error (_("required argument missing"));
      return 1;
    }

  if (type == MU_CFG_ARRAY)
    {
      if (val->type == MU_CFG_STRING)
	{
	  mu_config_value_t *arr = mu_calloc (1, sizeof arr[0]);
	  arr[0] = *val;
	  val->v.arg.c = 1;
	  val->v.arg.v = arr;
	  val->type = MU_CFG_ARRAY;
	}
    }
  
  if (val->type != type)
    {
      /* FIXME */
      mu_error (_("unexpected value: %s"), _first_value_ptr (val));
      return 1;
    }
  return 0;
}

int
mu_cfg_string_value_cb (mu_config_value_t *val,
			int (*fun) (const char *, void *),
			void *data)
{
  int rc = 0;
  
  switch (val->type)
    {
    case MU_CFG_STRING:
      return fun (val->v.string, data);
      break;

    case MU_CFG_ARRAY:
      {
	int i;

	for (i = 0; i < val->v.arg.c; i++)
	  {
	    if (mu_cfg_assert_value_type (&val->v.arg.v[i],
					  MU_CFG_STRING))
	      return 1;
	    fun (val->v.arg.v[i].v.string, data);
	  }
      }
      break;

    case MU_CFG_LIST:
      {
	mu_iterator_t itr;
	mu_list_get_iterator (val->v.list, &itr);
	for (mu_iterator_first (itr);
	     !mu_iterator_is_done (itr); mu_iterator_next (itr))
	  {
	    mu_config_value_t *pval;
	    mu_iterator_current (itr, (void*) &pval);
	    if (mu_cfg_assert_value_type (pval, MU_CFG_STRING))
	      {
		rc = 1;
		break;
	      }
	    fun (pval->v.string, data);
	  }
	mu_iterator_destroy (&itr);
      }
    }
  return rc;
}

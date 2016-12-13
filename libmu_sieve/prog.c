/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2012, 2014-2016 Free Software
   Foundation, Inc.

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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sieve-priv.h>

void
mu_i_sv_code (struct mu_sieve_machine *mach, sieve_op_t op)
{
  if (mach->pc >= mach->progsize)
    {
      mu_i_sv_2nrealloc (mach, (void**) &mach->prog, &mach->progsize,
			 sizeof mach->prog[0]);
    }
  mach->prog[mach->pc++] = op;
}

static int
file_eq (char const *a, char const *b)
{
  if (a)
    return b ? (strcmp (a, b) == 0) : 1;
  return b ? 0 : 1;
}
      
/* FIXME: 1. Only beg is stored
          2. mu_col is not used
 */
int
mu_i_sv_locus (struct mu_sieve_machine *mach, struct mu_locus_range *lr)
{
  if (!file_eq (mach->locus.mu_file, lr->beg.mu_file))
    {
      mu_i_sv_code (mach, (sieve_op_t) _mu_i_sv_instr_source);
      mu_i_sv_code (mach, (sieve_op_t) mu_i_sv_id_num (mach, lr->beg.mu_file));
    }
  if (mach->locus.mu_line != lr->beg.mu_line)
    {
      mu_i_sv_code (mach, (sieve_op_t) _mu_i_sv_instr_line);
      mu_i_sv_code (mach, (sieve_op_t) lr->beg.mu_line);
    }

  mach->locus = lr->beg;
  return 0;
}

mu_sieve_tag_def_t *
find_tag (mu_sieve_tag_group_t *taglist, char *tagname,
	  mu_sieve_tag_checker_t *checker)
{
  *checker = NULL;
  
  if (!taglist)
    return NULL;
  
  for (; taglist->tags; taglist++)
    {
      mu_sieve_tag_def_t *def;
      for (def = taglist->tags; def->name; def++)
	if (strcmp (def->name, tagname) == 0)
	  {
	    *checker = taglist->checker;
	    return def;
	  }
    }
  return NULL;
}

static int
_compare_ptr (void *item, void *data)
{
  return item == data;
}

struct check_arg
{
  struct mu_sieve_machine *mach;
  struct mu_sieve_node *node;
};

static int
_run_checker (void *item, void *data)
{
  struct check_arg *arg = data;
  mu_sieve_machine_t mach = arg->mach;
  struct mu_sieve_node *node = arg->node;
  mu_sieve_tag_checker_t checker = item;
  int rc;

  mach->comparator = node->v.command.comparator;
  mach->argstart = node->v.command.argstart;
  mach->argcount = node->v.command.argcount;
  mach->tagcount = node->v.command.tagcount;
  mach->identifier = node->v.command.reg->name;
  
  rc = checker (arg->mach);

  /* checker is allowed to alter these values */
  node->v.command.comparator = mach->comparator;
  node->v.command.argcount = mach->argcount;
  node->v.command.tagcount = mach->tagcount;
  
  mach->argstart = 0;
  mach->argcount = 0;
  mach->tagcount = 0;
  mach->identifier = NULL;

  return rc;
}

void
mu_i_sv_lint_command (struct mu_sieve_machine *mach,
		      struct mu_sieve_node *node)
{
  size_t i;
  mu_sieve_registry_t *reg = node->v.command.reg;
  
  mu_sieve_value_t *start = mach->valspace + node->v.command.argstart;
  
  mu_list_t chk_list = NULL;
  mu_sieve_data_type *exp_arg;
  int opt_args = 0;
  int rc, err = 0;
  static mu_sieve_data_type empty[] = { SVT_VOID };

  if (!reg)
    return;
  
  exp_arg = reg->v.command.req_args ? reg->v.command.req_args : empty;

  /* Pass 1: tag consolidation and argument checking */
  for (i = 0; i < node->v.command.argcount; i++)
    {
      mu_sieve_value_t *val = start + i;
	  
      if (val->type == SVT_TAG)
	{
	  mu_sieve_tag_checker_t cf;
	  mu_sieve_tag_def_t *tag = find_tag (reg->v.command.tags,
					      val->v.string, &cf);
	      
	  if (!tag)
	    {
	      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				_("invalid tag name `%s' for `%s'"),
				val->v.string, reg->name);
	      mu_i_sv_error (mach);
	      err = 1;
	      break;
	    }

	  node->v.command.tagcount++;
	  
	  if (tag->argtype == SVT_VOID)
	    {
	      val->type = SVT_VOID;
	      val->tag = val->v.string;
	      val->v.string = NULL;
	    }
	  else
	    {
	      if (i + 1 == node->v.command.argcount)
		{
		  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				    _("required argument for tag %s is missing"),
				    tag->name);
		  mu_i_sv_error (mach);
		  err = 1;
		  break;
		}

	      val[1].tag = val->v.string;
	      *val = val[1];
	      memmove (val + 1, val + 2,
		       (node->v.command.argcount - i - 2) * sizeof (val[0]));
	      mach->valcount--;
	      node->v.command.argcount--;
	      
	      if (val->type != tag->argtype)
		{
		  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				    _("type mismatch in argument to "
				      "tag `%s'"),
				    tag->name);
		  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				    _("expected %s but passed %s"),
				    mu_sieve_type_str (tag->argtype),
				    mu_sieve_type_str (val->type));
		  mu_i_sv_error (mach);
		  err = 1;
		  break;
		}
	    }

	  if (cf)
	    {
	      if (!chk_list && (rc = mu_list_create (&chk_list)))
		{
		  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				    _("cannot create check list: %s"),
				    mu_strerror (rc));
		  mu_i_sv_error (mach);
		  err = 1;
		  break;
		}
	      if (mu_list_foreach (chk_list, _compare_ptr, cf) == 0)
		{
		  rc = mu_list_append (chk_list, cf);
		  if (rc)
		    {
		      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus,
					"mu_list_append: %s",
					mu_strerror (rc));
		      mu_i_sv_error (mach);
		      err = 1;
		      break;
		    }
		}
	    }
	}
      else
	{
	  if (*exp_arg == SVT_VOID)
	    {
	      if (reg->v.command.opt_args)
		{
		  exp_arg = reg->v.command.opt_args;
		  opt_args = 1;
		}
	      else
		{
		  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				    _("too many arguments in call to `%s'"),
				    reg->name);
		  mu_i_sv_error (mach);
		  err = 1;
		  break;
		}
	    }
	      
	  if (*exp_arg != val->type)
	    {
	      if (*exp_arg == SVT_STRING_LIST && val->type == SVT_STRING)
		/* compatible types */;
	      else
		{
		  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				    _("type mismatch in argument %lu to `%s'"),
				    (unsigned long) (exp_arg - reg->v.command.req_args + 1),
				    reg->name);
		  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				    _("expected %s but passed %s"),
				    mu_sieve_type_str (*exp_arg),
				    mu_sieve_type_str (val->type));
		  mu_i_sv_error (mach);
		  err = 1;
		  break;
		}
	    }
	  exp_arg++;
	}
    }

  if (!err && !opt_args && *exp_arg != SVT_VOID)
    {
      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
			_("too few arguments in call to `%s'"),
			reg->name);
      mu_i_sv_error (mach);
      err = 1;
    }
  
  if (err)
    {
      mu_list_destroy (&chk_list);
      return;
    }

  if (node->v.command.tagcount)
    {
      /* Pass 2: Move tags to the end of the list */
      for (i = 1; i < node->v.command.argcount; i++)
	{
	  size_t j;
	  mu_sieve_value_t tmp = start[i];
	  for (j = i; j > 0; j--)
	    {
	      if (!tmp.tag && start[j - 1].tag)
		start[j] = start[j - 1];
	      else
		break;
	    }
	  start[j] = tmp;
	}
    }

  node->v.command.argcount -= node->v.command.tagcount;
  
  if (chk_list)
    {
      struct check_arg chk_arg;

      chk_arg.mach = mach;
      chk_arg.node = node;
      err = mu_list_foreach (chk_list, _run_checker, &chk_arg);
    }
}  

static void
sv_code_command (struct mu_sieve_machine *mach,
		 struct mu_sieve_node *node)
{
  mu_i_sv_code (mach, (sieve_op_t) node->v.command.reg->v.command.handler);
  mu_i_sv_code (mach, (sieve_op_t) node->v.command.argstart);
  mu_i_sv_code (mach, (sieve_op_t) node->v.command.argcount);
  mu_i_sv_code (mach, (sieve_op_t) node->v.command.tagcount);
  mu_i_sv_code (mach, (sieve_op_t) (char*) node->v.command.reg->name);
  mu_i_sv_code (mach, (sieve_op_t) node->v.command.comparator);
}

void
mu_i_sv_code_action (struct mu_sieve_machine *mach,
		     struct mu_sieve_node *node)
{
  mu_i_sv_code (mach, (sieve_op_t) _mu_i_sv_instr_action);
  sv_code_command (mach, node);
}

void
mu_i_sv_code_test (struct mu_sieve_machine *mach, struct mu_sieve_node *node)
{
  mu_i_sv_code (mach, (sieve_op_t) _mu_i_sv_instr_test);
  sv_code_command (mach, node);
}


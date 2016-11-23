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

int
mu_i_sv_code (struct mu_sieve_machine *mach, sieve_op_t op)
{
  if (mach->pc >= mach->progsize)
    {
      size_t newsize = mach->progsize + SIEVE_CODE_INCR;
      sieve_op_t *newprog =
	mu_sieve_mrealloc (mach, mach->prog, newsize * sizeof mach->prog[0]);
      if (!newprog)
	{
	  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, _("not enough memory"));
	  mu_i_sv_error (mach);
	  return 1;
	}
      mach->prog = newprog;
      mach->progsize = newsize;
    }
  mach->prog[mach->pc++] = op;
  return 0;
}

static int
file_eq (char const *a, char const *b)
{
  if (a)
    return b ? (strcmp (a, b) == 0) : 1;
  return b ? -1 : 0;
}
      
/* FIXME: 1. Only beg is stored
          2. mu_col is not used
 */
int
mu_i_sv_locus (struct mu_sieve_machine *mach, struct mu_locus_range *lr)
{
  if (!file_eq (mach->locus.mu_file, lr->beg.mu_file))
    {
      mu_i_sv_code (mach, (sieve_op_t) _mu_sv_instr_source);
      mu_i_sv_code (mach, (sieve_op_t) lr->beg.mu_file);
    }
  if (mach->locus.mu_line != lr->beg.mu_line)
    {
      mu_i_sv_code (mach, (sieve_op_t) _mu_sv_instr_line);
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
  const char *name;
  mu_list_t args;
  mu_list_t tags;
};

static int
_run_checker (void *item, void *data)
{
  struct check_arg *arg = data;
  return (*(mu_sieve_tag_checker_t)item) (arg->mach, arg->name,
					  arg->tags, arg->args);
}

static int
sv_code_command (struct mu_sieve_machine *mach,
		 mu_sieve_register_t *reg, mu_list_t arglist)
{
  mu_iterator_t itr;
  mu_list_t arg_list = NULL;
  mu_list_t tag_list = NULL;
  mu_list_t chk_list = NULL;
  mu_sieve_data_type *exp_arg;
  int opt_args = 0;
  int rc, err = 0;
  static mu_sieve_data_type empty[] = { SVT_VOID };
  
  if (mu_i_sv_code (mach, (sieve_op_t) reg->handler))
    return 1;

  exp_arg = reg->req_args ? reg->req_args : empty;

  if (arglist)
    {
      rc = mu_list_get_iterator (arglist, &itr);

      if (rc)
	{
	  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus,
			    _("cannot create iterator: %s"),
			    mu_strerror (rc));
	  mu_i_sv_error (mach);
	  return 1;
	}
  
      for (mu_iterator_first (itr);
	   !mu_iterator_is_done (itr); mu_iterator_next (itr))
	{
	  mu_sieve_value_t *val;
	  mu_sieve_runtime_tag_t tagrec, *tagptr;
	  
	  mu_iterator_current (itr, (void **)&val);
	  
	  if (val->type == SVT_TAG)
	    {
	      mu_sieve_tag_checker_t cf;
	      mu_sieve_tag_def_t *tag = find_tag (reg->tags, val->v.string,
						  &cf);
	      if (!tag)
		{
		  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				    _("invalid tag name `%s' for `%s'"),
				    val->v.string, reg->name);
		  mu_i_sv_error (mach);
		  err = 1;
		  break;
		}
	      
	      if (!tag_list && (rc = mu_list_create (&tag_list)))
		{
		  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				    _("cannot create tag list: %s"),
				    mu_strerror (rc));
		  mu_i_sv_error (mach);
		  err = 1;
		  break;
		}
	      
	      tagrec.tag = tag->name;
	      if (tag->argtype != SVT_VOID)
		{
		  mu_iterator_next (itr);
		  if (mu_iterator_is_done (itr))
		    {
		      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
			   _("required argument for tag %s is missing"),
					tag->name);
		      mu_i_sv_error (mach);
		      err = 1;
		      break;
		    }
		  mu_iterator_current (itr, (void **)&tagrec.arg);
		  if (tagrec.arg->type != tag->argtype)
		    {
		      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
					     _("type mismatch in argument to "
					       "tag `%s'"),
					     tag->name);
		      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
					     _("expected %s but passed %s"),
					     mu_sieve_type_str (tag->argtype),
					     mu_sieve_type_str (tagrec.arg->type));
		      mu_i_sv_error (mach);
		      err = 1;
		      break;
		    }
		}
	      else
		tagrec.arg = NULL;
	      
	      tagptr = mu_sieve_malloc (mach, sizeof (*tagptr));
	      *tagptr = tagrec;
	      mu_list_append (tag_list, tagptr);

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
		    mu_list_append (chk_list, cf);
		}
	    }
	  else
	    {
	      if (*exp_arg == SVT_VOID)
		{
		  if (reg->opt_args)
		    {
		      exp_arg = reg->opt_args;
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
		    {
		      mu_list_t list;

		      mu_list_create (&list);
		      mu_list_append (list, val->v.string);
		      mu_sieve_mfree (mach, val);
		      val = mu_sieve_value_create (SVT_STRING_LIST, list);
		    }
		  else
		    {
		      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
                                   _("type mismatch in argument %lu to `%s'"),
				   (unsigned long) (exp_arg - reg->req_args + 1),
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

	      if (!arg_list && (rc = mu_list_create (&arg_list)))
		{
		  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				    _("cannot create arg list: %s"),
				    mu_strerror (rc));
		  mu_i_sv_error (mach);
		  err = 1;
		  break;
		}
	      
	      mu_list_append (arg_list, val);
	      exp_arg++;
	    }	    
	}
      mu_iterator_destroy (&itr);
    }

  if (!err)
    {
      if (!opt_args && *exp_arg != SVT_VOID)
	{
	  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
			    _("too few arguments in call to `%s'"),
			    reg->name);
	  mu_i_sv_error (mach);
	  err = 1;
	}

      if (chk_list)
	{
	  struct check_arg chk_arg;

	  chk_arg.mach = mach;
	  chk_arg.name = reg->name;
	  chk_arg.tags = tag_list;
	  chk_arg.args = arg_list;
	  err = mu_list_foreach (chk_list, _run_checker, &chk_arg);
	}
    }
  
  if (!err)
    err = mu_i_sv_code (mach, (sieve_op_t) arg_list)
      || mu_i_sv_code (mach, (sieve_op_t) tag_list)
      || mu_i_sv_code (mach, (sieve_op_t) (char*) reg->name);

  if (err)
    {
      mu_list_destroy (&arg_list);
      mu_list_destroy (&tag_list);
      mu_list_destroy (&chk_list);
    }

  return err;
}

int
mu_i_sv_code_action (struct mu_sieve_machine *mach,
		     mu_sieve_register_t *reg, mu_list_t arglist)
{
  return mu_i_sv_code (mach, (sieve_op_t) _mu_sv_instr_action)
         || sv_code_command (mach, reg, arglist);
}

int
mu_i_sv_code_test (struct mu_sieve_machine *mach,
		   mu_sieve_register_t *reg, mu_list_t arglist)
{
  return mu_i_sv_code (mach, (sieve_op_t) _mu_sv_instr_test)
         || sv_code_command (mach, reg, arglist);
}


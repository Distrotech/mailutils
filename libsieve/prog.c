/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sieve.h>

int
sieve_code (sieve_op_t *op)
{
  if (sieve_machine->pc >= sieve_machine->progsize)
    {
      size_t newsize = sieve_machine->progsize + SIEVE_CODE_INCR;
      sieve_op_t *newprog = sieve_prealloc (&sieve_machine->memory_pool,
					    sieve_machine->prog,
					    newsize *
					    sizeof sieve_machine->prog[0]);
      if (!newprog)
	{
	  sieve_error ("%s:%d: out of memory!",
		       sieve_filename, sieve_line_num);
	  return 1;
	}
      sieve_machine->prog = newprog;
      sieve_machine->progsize = newsize;
    }
  sieve_machine->prog[sieve_machine->pc++] = *op;
  return 0;
}

int
sieve_code_instr (sieve_instr_t instr)
{
  sieve_op_t op;

  op.instr = instr;
  return sieve_code (&op);
}

int
sieve_code_handler (sieve_handler_t handler)
{
  sieve_op_t op;

  op.handler = handler;
  return sieve_code (&op);
}

int
sieve_code_list (list_t list)
{
  sieve_op_t op;

  op.list = list;
  return sieve_code (&op);
}

int
sieve_code_number (long num)
{
  sieve_op_t op;

  op.number = num;
  return sieve_code (&op);
}

int
sieve_code_string (char *string)
{
  sieve_op_t op;

  op.string = string;
  return sieve_code (&op);
}

sieve_tag_def_t *
find_tag (sieve_tag_def_t *taglist, char *tagname)
{
  if (!taglist)
    return NULL;
  
  for (; taglist->name; taglist++)
    if (strcmp (taglist->name, tagname) == 0)
      return taglist;
  return NULL;
}

int
sieve_code_command (sieve_register_t *reg, list_t arglist)
{
  iterator_t itr;
  list_t arg_list = NULL;
  list_t tag_list = NULL;
  sieve_data_type *exp_arg;
  int rc, err = 0;
  static sieve_data_type empty[] = { SVT_VOID };
  
  if (sieve_code_handler (reg->handler))
    return 1;

  exp_arg = reg->req_args ? reg->req_args : empty;

  if (arglist)
    {
      rc = iterator_create (&itr, arglist);

      if (rc)
	{
	  sieve_error ("%s:%d: can't create iterator: %s",
		       sieve_filename, sieve_line_num,
		       mu_errstring (rc));
	  return 1;
	}
  
      for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
	{
	  sieve_value_t *val;
	  sieve_runtime_tag_t tagrec, *tagptr;
	  
	  iterator_current (itr, (void **)&val);
	  
	  if (val->type == SVT_TAG)
	    {
	      sieve_tag_def_t *tag = find_tag (reg->tags, val->v.string);
	      if (!tag)
		{
		  sieve_error ("%s:%d: invalid tag name `%s' for `%s'",
			       sieve_filename, sieve_line_num,
			       val->v.string, reg->name);
		  err = 1;
		  break;
		}
	      
	      if (!tag_list && (rc = list_create (&tag_list)))
		{
		  sieve_error ("%s:%d: can't create tag list: %s",
			       sieve_filename, sieve_line_num,
			       mu_errstring (rc));
		  err = 1;
		  break;
		}
	      
	      tagrec.tag = tag->num;
	      if (tag->argtype != SVT_VOID)
		{
		  iterator_next (itr);
		  iterator_current (itr, (void **)&tagrec.arg);
		}
	      else
		tagrec.arg = NULL;
	      
	      tagptr = sieve_palloc (&sieve_machine->memory_pool,
				     sizeof (*tagptr));
	      *tagptr = tagrec;
	      list_append (tag_list, tagptr);
	    }
	  else if (*exp_arg == SVT_VOID)
	    {
	      sieve_error ("%s:%d: too many arguments in call to `%s'",
			   sieve_filename, sieve_line_num,
			   reg->name);
	      err = 1;
	      break;
	    }
	  else if (*exp_arg != val->type)
	    {
	      sieve_error ("%s:%d: type mismatch in argument %d to `%s'",
			   sieve_filename, sieve_line_num,
			   exp_arg - reg->req_args + 1,
			   reg->name);
	      err = 1;
	      break;
	    }
	  else
	    {
	      if (!arg_list && (rc = list_create (&arg_list)))
		{
		  sieve_error ("%s:%d: can't create arg list: %s",
			       sieve_filename, sieve_line_num,
			       mu_errstring (rc));
		  err = 1;
		  break;
		}
	      
	      list_append (arg_list, val);
	      exp_arg++;
	    }	    
	}
      iterator_destroy (&itr);
    }

  if (!err)
    {
      if (*exp_arg != SVT_VOID)
	{
	  sieve_error ("%s:%d: too few arguments in call to `%s'",
			   sieve_filename, sieve_line_num,
			   reg->name);
	  err = 1;
	}
    }
  
  if (!err)
    err = sieve_code_list (arg_list)
          || sieve_code_list (tag_list)
          || sieve_code_string (reg->name);

  if (err)
    {
      list_destroy (&arg_list);
      list_destroy (&tag_list);
    }

  return err;
}

int
sieve_code_action (sieve_register_t *reg, list_t arglist)
{
  return sieve_code_instr (instr_action)
         || sieve_code_command (reg, arglist);
}

int
sieve_code_test (sieve_register_t *reg, list_t arglist)
{
  return sieve_code_instr (instr_test)
         || sieve_code_command (reg, arglist);
}


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

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
      sieve_op_t *newprog = sieve_mrealloc (sieve_machine, sieve_machine->prog,
					    newsize *
					    sizeof sieve_machine->prog[0]);
      if (!newprog)
	{
	  sieve_compile_error (sieve_filename, sieve_line_num, 
                               _("out of memory!"));
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
find_tag (sieve_tag_group_t *taglist, char *tagname,
	  sieve_tag_checker_t *checker)
{
  *checker = NULL;
  
  if (!taglist)
    return NULL;
  
  for (; taglist->tags; taglist++)
    {
      sieve_tag_def_t *def;
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

struct check_arg {
  const char *name;
  list_t args;
  list_t tags;
};

static int
_run_checker (void *item, void *data)
{
  struct check_arg *arg = data;
  return (*(sieve_tag_checker_t)item) (arg->name, arg->tags, arg->args);
}

int
sieve_code_command (sieve_register_t *reg, list_t arglist)
{
  iterator_t itr;
  list_t arg_list = NULL;
  list_t tag_list = NULL;
  list_t chk_list = NULL;
  sieve_data_type *exp_arg;
  int rc, err = 0;
  static sieve_data_type empty[] = { SVT_VOID };
  
  if (sieve_code_handler (reg->handler))
    return 1;

  exp_arg = reg->req_args ? reg->req_args : empty;

  if (arglist)
    {
      rc = list_get_iterator (arglist, &itr);

      if (rc)
	{
	  sieve_compile_error (sieve_filename, sieve_line_num,
                               _("cannot create iterator: %s"),
  		               mu_strerror (rc));
	  return 1;
	}
  
      for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
	{
	  sieve_value_t *val;
	  sieve_runtime_tag_t tagrec, *tagptr;
	  
	  iterator_current (itr, (void **)&val);
	  
	  if (val->type == SVT_TAG)
	    {
	      sieve_tag_checker_t cf;
	      sieve_tag_def_t *tag = find_tag (reg->tags, val->v.string, &cf);
	      if (!tag)
		{
		  sieve_compile_error (sieve_filename, sieve_line_num,
				       _("invalid tag name `%s' for `%s'"),
				       val->v.string, reg->name);
		  err = 1;
		  break;
		}
	      
	      if (!tag_list && (rc = list_create (&tag_list)))
		{
		  sieve_compile_error (sieve_filename, sieve_line_num,
                                       _("%s:%d: cannot create tag list: %s"),
			               mu_strerror (rc));
		  err = 1;
		  break;
		}
	      
	      tagrec.tag = tag->name;
	      if (tag->argtype != SVT_VOID)
		{
		  iterator_next (itr);
		  iterator_current (itr, (void **)&tagrec.arg);
		}
	      else
		tagrec.arg = NULL;
	      
	      tagptr = sieve_malloc (sieve_machine, sizeof (*tagptr));
	      *tagptr = tagrec;
	      list_append (tag_list, tagptr);

	      if (cf)
		{
		  if (!chk_list && (rc = list_create (&chk_list)))
		    {
		      sieve_compile_error (sieve_filename, sieve_line_num,
			  	         _("%s:%d: cannot create check list: %s"),
					   mu_strerror (rc));
		      err = 1;
		      break;
		    }
		  if (list_do (chk_list, _compare_ptr, cf) == 0)
		    list_append (chk_list, cf);
		}
	    }
	  else if (*exp_arg == SVT_VOID)
	    {
	      sieve_compile_error (sieve_filename, sieve_line_num,
                                   _("too many arguments in call to `%s'"),
 			           reg->name);
	      err = 1;
	      break;
	    }
	  else
	    {
	      if (*exp_arg != val->type)
		{
		  if (*exp_arg == SVT_STRING_LIST && val->type == SVT_STRING)
		    {
		      list_t list;

		      list_create (&list);
		      list_append (list, val->v.string);
		      sieve_mfree (sieve_machine, val);
		      val = sieve_value_create (SVT_STRING_LIST, list);
		    }
		  else
		    {
		      sieve_compile_error (sieve_filename, sieve_line_num,
                                      _("type mismatch in argument %d to `%s'"),
				      exp_arg - reg->req_args + 1,
				      reg->name);
		      sieve_compile_error (sieve_filename, sieve_line_num,
					   _("expected %s but passed %s"),
					   sieve_type_str (*exp_arg),
					   sieve_type_str (val->type));
		      err = 1;
		      break;
		    }
		}

	      if (!arg_list && (rc = list_create (&arg_list)))
		{
		  sieve_compile_error (sieve_filename, sieve_line_num,
                                       _("cannot create arg list: %s"),
			               mu_strerror (rc));
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
	  sieve_compile_error (sieve_filename, sieve_line_num, 
                               _("too few arguments in call to `%s'"),
			       reg->name);
	  err = 1;
	}

      if (chk_list)
	{
	  struct check_arg chk_arg;
      
	  chk_arg.name = reg->name;
	  chk_arg.tags = tag_list;
	  chk_arg.args = arg_list;
	  err = list_do (chk_list, _run_checker, &chk_arg);
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
      list_destroy (&chk_list);
    }

  return err;
}

int
sieve_code_source (const char *name)
{
  char *s;
  
  if (list_locate (sieve_machine->source_list, (void*) name, (void **) &s))
    {
      s = sieve_mstrdup (sieve_machine, name);
      list_append (sieve_machine->source_list, s);
    }
  
  return sieve_code_instr (instr_source)
	 || sieve_code_string (s);
}

int
sieve_code_line (size_t line)
{
  sieve_op_t op;

  op.line = line;
  return sieve_code_instr (instr_line)
	 || sieve_code (&op);
}

int
sieve_code_action (sieve_register_t *reg, list_t arglist)
{
  return sieve_code_line (sieve_line_num)
         || sieve_code_instr (instr_action)
         || sieve_code_command (reg, arglist);
}

int
sieve_code_test (sieve_register_t *reg, list_t arglist)
{
  return sieve_code_line (sieve_line_num)
         || sieve_code_instr (instr_test)
         || sieve_code_command (reg, arglist);
}

void
sieve_code_anyof (size_t start)
{
  size_t end = sieve_machine->pc;
  while (sieve_machine->prog[start+1].pc != 0)
    {
      size_t next = sieve_machine->prog[start+1].pc;
      sieve_machine->prog[start].instr = instr_brnz;
      sieve_machine->prog[start+1].pc = end - start - 2;
      start = next;
    }
  sieve_machine->prog[start].instr = instr_nop;
  sieve_machine->prog[start+1].instr = instr_nop;
}

void
sieve_code_allof (size_t start)
{
  size_t end = sieve_machine->pc;
  
  while (sieve_machine->prog[start+1].pc != 0)
    {
      size_t next = sieve_machine->prog[start+1].pc;
      sieve_machine->prog[start+1].pc = end - start - 2;
      start = next;
    }
  sieve_machine->prog[start].instr = instr_nop;
  sieve_machine->prog[start+1].instr = instr_nop;
}
		

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

#define SIEVE_ARG(m,n,t) ((m)->prog[(m)->pc+(n)].t)
#define SIEVE_ADJUST(m,n) (m)->pc+=(n)

#define INSTR_DEBUG(m) ((m)->debug_level == 100 && (m)->debug_printer)

static int
instr_run (sieve_machine_t *mach)
{
  sieve_handler_t han = SIEVE_ARG (mach, 0, handler);
  list_t arg_list = SIEVE_ARG (mach, 1, list);
  list_t tag_list = SIEVE_ARG (mach, 2, list);
  int rc;
  
  if (INSTR_DEBUG (mach))
    {
      sieve_debug (mach, "Arguments: ");
      sieve_print_value_list (arg_list, mach->debug_printer, mach->data);
      sieve_debug (mach, "\nTags:");
      sieve_print_tag_list (tag_list, mach->debug_printer, mach->data);
      sieve_debug (mach, "\n");
    }

  rc = han (mach, arg_list, tag_list);
  SIEVE_ADJUST(mach, 4);
  return rc;
}

void
instr_action (sieve_machine_t *mach)
{
  if (INSTR_DEBUG (mach))
    sieve_debug (mach, "ACTION: %s\n", SIEVE_ARG (mach, 3, string));
  instr_run (mach);
}

void
instr_test (sieve_machine_t *mach)
{
  if (INSTR_DEBUG (mach))
    sieve_debug (mach, "TEST: %s\n", SIEVE_ARG (mach, 3, string));
  mach->reg = instr_run (mach);
}

void
instr_push (sieve_machine_t *mach)
{
  if (INSTR_DEBUG (mach))
    sieve_debug (mach, "PUSH\n");
  if (!mach->stack && list_create (&mach->stack))
    {
      sieve_error ("can't create stack");
      sieve_abort (mach);
    }
  list_prepend (mach->stack, (void*) mach->reg);
}

void
instr_pop (sieve_machine_t *mach)
{
  if (INSTR_DEBUG (mach))
    sieve_debug (mach, "POP\n");
  if (!mach->stack || list_is_empty (mach->stack))
    {
      sieve_error ("stack underflow");
      sieve_abort (mach);
    }
  list_get (mach->stack, 0, (void **)&mach->reg);
  list_remove (mach->stack, (void *)mach->reg);
}

void
instr_allof (sieve_machine_t *mach)
{
  int num = SIEVE_ARG (mach, 0, number);
  int val = 1;

  if (INSTR_DEBUG (mach))
    sieve_debug (mach, "ALLOF %d\n", num);
  SIEVE_ADJUST(mach, 1);
  while (num-- > 0)
    {
      instr_pop (mach);
      val &= mach->reg;
    }
  mach->reg = val;
}

void
instr_anyof (sieve_machine_t *mach)
{
  int num = SIEVE_ARG (mach, 0, number);
  int val = 0;

  if (INSTR_DEBUG (mach))
    sieve_debug (mach, "ANYOF %d\n", num);
  SIEVE_ADJUST(mach, 1);
  while (num-- > 0)
    {
      instr_pop (mach);
      val &= mach->reg;
    }
  mach->reg = val;
}

void
instr_not (sieve_machine_t *mach)
{
  if (INSTR_DEBUG (mach))
    sieve_debug (mach, "NOT");
  mach->reg = !mach->reg;
}

void
sieve_abort (sieve_machine_t *mach)
{
  longjmp (mach->errbuf, 1);
}

int
sieve_run (sieve_machine_t *mach)
{
  if (setjmp (mach->errbuf))
    return 1;
  
  for (mach->pc = 1; mach->prog[mach->pc].handler; )
    (*mach->prog[mach->pc++].instr) (mach);

  return 0;
}

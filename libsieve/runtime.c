/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

#define SIEVE_ARG(m,n,t) ((m)->prog[(m)->pc+(n)].t)
#define SIEVE_ADJUST(m,n) (m)->pc+=(n)

#define INSTR_DEBUG(m) \
 (((m)->debug_level & (MU_SIEVE_DEBUG_INSTR|MU_SIEVE_DEBUG_DISAS)) \
  && (m)->debug_printer)
#define INSTR_DISASS(m) ((m)->debug_level & MU_SIEVE_DEBUG_DISAS)

void
instr_nop (sieve_machine_t mach)
{
  if (INSTR_DEBUG (mach))
    sieve_debug (mach, "%4lu: NOP\n",
		 (unsigned long) (mach->pc - 1));
}

static int
instr_run (sieve_machine_t mach)
{
  sieve_handler_t han = SIEVE_ARG (mach, 0, handler);
  list_t arg_list = SIEVE_ARG (mach, 1, list);
  list_t tag_list = SIEVE_ARG (mach, 2, list);
  int rc = 0;
  
  if (INSTR_DEBUG (mach))
    {
      sieve_debug (mach, "Arguments: ");
      sieve_print_value_list (arg_list, mach->debug_printer, mach->data);
      sieve_debug (mach, "\nTags:");
      sieve_print_tag_list (tag_list, mach->debug_printer, mach->data);
      sieve_debug (mach, "\n");
    }

  if (!INSTR_DISASS(mach))
    rc = han (mach, arg_list, tag_list);
  SIEVE_ADJUST(mach, 4);
  return rc;
}

void
instr_action (sieve_machine_t mach)
{
  if (INSTR_DEBUG (mach))
    sieve_debug (mach, "%4lu: ACTION: %s\n",
		 (unsigned long) (mach->pc - 1),
		 SIEVE_ARG (mach, 3, string));
  mach->action_count++;
  instr_run (mach);
}

void
instr_test (sieve_machine_t mach)
{
  if (INSTR_DEBUG (mach))
    sieve_debug (mach, "%4lu: TEST: %s\n",
		 (unsigned long) (mach->pc - 1),
		 SIEVE_ARG (mach, 3, string));
  mach->reg = instr_run (mach);
}

void
instr_push (sieve_machine_t mach)
{
  if (INSTR_DEBUG (mach))
    {
      sieve_debug (mach, "%4lu: PUSH\n", (unsigned long)(mach->pc - 1));
      if (INSTR_DISASS (mach))
	return;
    }
  
  if (!mach->stack && list_create (&mach->stack))
    {
      sieve_error (mach, _("can't create stack"));
      sieve_abort (mach);
    }
  list_prepend (mach->stack, (void*) mach->reg);
}

void
instr_pop (sieve_machine_t mach)
{
  if (INSTR_DEBUG (mach))
    {
      sieve_debug (mach, "%4lu: POP\n", (unsigned long)(mach->pc - 1));
      if (INSTR_DISASS (mach))
	return;
    }

  if (!mach->stack || list_is_empty (mach->stack))
    {
      sieve_error (mach, _("stack underflow"));
      sieve_abort (mach);
    }
  list_get (mach->stack, 0, (void **)&mach->reg);
  list_remove (mach->stack, (void *)mach->reg);
}

void
instr_not (sieve_machine_t mach)
{
  if (INSTR_DEBUG (mach))
    {
      sieve_debug (mach, "%4lu: NOT\n", (unsigned long)(mach->pc - 1));
      if (INSTR_DISASS (mach))
	return;
    }
  mach->reg = !mach->reg;
}

void
instr_branch (sieve_machine_t mach)
{
  long num = SIEVE_ARG (mach, 0, number);

  SIEVE_ADJUST (mach, 1);
  if (INSTR_DEBUG (mach))
    {
      sieve_debug (mach, "%4lu: BRANCH %lu\n",
		   (unsigned long)(mach->pc-2),
		   (unsigned long)(mach->pc + num));
      if (INSTR_DISASS (mach))
	return;
    }

  mach->pc += num;
}

void
instr_brz (sieve_machine_t mach)
{
  long num = SIEVE_ARG (mach, 0, number);
  SIEVE_ADJUST (mach, 1);

  if (INSTR_DEBUG (mach))
    {
      sieve_debug (mach, "%4lu: BRZ %lu\n",
		   (unsigned long)(mach->pc-2),
		   (unsigned long)(mach->pc + num));
      if (INSTR_DISASS (mach))
	return;
    }
  
  if (!mach->reg)
    mach->pc += num;
}
  
void
instr_brnz (sieve_machine_t mach)
{
  long num = SIEVE_ARG (mach, 0, number);
  SIEVE_ADJUST (mach, 1);

  if (INSTR_DEBUG (mach))
    {
      sieve_debug (mach, "%4lu: BRNZ %lu\n",
		   (unsigned long)(mach->pc-2),
		   (unsigned long)(mach->pc + num));
      if (INSTR_DISASS (mach))
	return;
    }
  
  if (mach->reg)
    mach->pc += num;
}
  
void
sieve_abort (sieve_machine_t mach)
{
  longjmp (mach->errbuf, 1);
}

void *
sieve_get_data (sieve_machine_t mach)
{
  return mach->data;
}

message_t
sieve_get_message (sieve_machine_t mach)
{
  if (!mach->msg)
    mailbox_get_message (mach->mailbox, mach->msgno, &mach->msg);
  return mach->msg;
}

size_t
sieve_get_message_num (sieve_machine_t mach)
{
  return mach->msgno;
}

int
sieve_get_debug_level (sieve_machine_t mach)
{
  return mach->debug_level;
}

int
sieve_is_dry_run (sieve_machine_t mach)
{
  return mach->debug_level & MU_SIEVE_DRY_RUN;
}

int
sieve_run (sieve_machine_t mach)
{
  if (setjmp (mach->errbuf))
    return 1;

  mach->action_count = 0;
  
  for (mach->pc = 1; mach->prog[mach->pc].handler; )
    (*mach->prog[mach->pc++].instr) (mach);

  if (mach->action_count == 0)
    sieve_log_action (mach, "IMPLICIT KEEP", NULL);
  
  if (INSTR_DEBUG (mach))
    sieve_debug (mach, "%4lu: STOP\n", mach->pc);
  
  return 0;
}

int
sieve_disass (sieve_machine_t mach)
{
  int level = mach->debug_level;
  int rc;

  mach->debug_level = MU_SIEVE_DEBUG_INSTR | MU_SIEVE_DEBUG_DISAS;
  rc = sieve_run (mach);
  mach->debug_level = level;
  return rc;
}
  
static int
_sieve_action (observer_t obs, size_t type)
{
  sieve_machine_t mach;
  
  if (type != MU_EVT_MESSAGE_ADD)
    return 0;

  mach = observer_get_owner (obs);
  mach->msgno++;
  mailbox_get_message (mach->mailbox, mach->msgno, &mach->msg);
  sieve_run (mach);
  return 0;
}

int
sieve_mailbox (sieve_machine_t mach, mailbox_t mbox)
{
  int rc;
  size_t total;
  observer_t observer;
  observable_t observable;
  
  if (!mach || !mbox)
    return EINVAL;

  observer_create (&observer, mach);
  observer_set_action (observer, _sieve_action, mach);
  mailbox_get_observable (mbox, &observable);
  observable_attach (observable, MU_EVT_MESSAGE_ADD, observer);
  
  mach->mailbox = mbox;
  mach->msgno = 0;
  rc = mailbox_scan (mbox, 1, &total);
  if (rc)
    sieve_error (mach, _("mailbox_scan: %s"), mu_strerror (errno));

  observable_detach (observable, observer);
  observer_destroy (&observer, mach);

  mach->mailbox = NULL;
  
  return rc;
}

int
sieve_message (sieve_machine_t mach, message_t msg)
{
  int rc;
  
  if (!mach || !msg)
    return EINVAL;

  mach->msgno = 1;
  mach->msg = msg;
  mach->mailbox = NULL;
  rc = sieve_run (mach);
  mach->msg = NULL;
  
  return rc;
}

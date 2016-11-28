/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007-2012, 2014-2016 Free Software
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

#define SIEVE_ARG(m,n,t) ((m)->prog[(m)->pc+(n)].t)
#define SIEVE_ADJUST(m,n) (m)->pc+=(n)

#define INSTR_DISASS(m) ((m)->state == mu_sieve_state_disass)
#define INSTR_DEBUG(m) \
  (INSTR_DISASS(m) || mu_debug_level_p (mu_sieve_debug_handle, MU_DEBUG_TRACE9)) 

void
_mu_i_sv_instr_source (mu_sieve_machine_t mach)
{
  mach->locus.mu_file = (char*) SIEVE_ARG (mach, 0, string);
  mu_stream_ioctl (mach->errstream, MU_IOCTL_LOGSTREAM,
                   MU_IOCTL_LOGSTREAM_SET_LOCUS,
		   &mach->locus);
  if (INSTR_DEBUG (mach))
    mu_i_sv_debug (mach, mach->pc - 1, "SOURCE %s", mach->locus.mu_file);
  SIEVE_ADJUST (mach, 1);
}
		 
void
_mu_i_sv_instr_line (mu_sieve_machine_t mach)
{
  mach->locus.mu_line = SIEVE_ARG (mach, 0, line);
  mu_stream_ioctl (mach->errstream, MU_IOCTL_LOGSTREAM,
                   MU_IOCTL_LOGSTREAM_SET_LOCUS,
		   &mach->locus);
  if (INSTR_DEBUG (mach))
    mu_i_sv_debug (mach, mach->pc - 1, "LINE %u",
		   mach->locus.mu_line);
  SIEVE_ADJUST (mach, 1);
}
		 
static int
instr_run (mu_sieve_machine_t mach, char const *what)
{
  int rc = 0;
  mu_sieve_handler_t han = SIEVE_ARG (mach, 0, handler);
  mach->arg_list = SIEVE_ARG (mach, 1, list);
  mach->tag_list = SIEVE_ARG (mach, 2, list);
  mach->identifier = SIEVE_ARG (mach, 3, string);
  
  SIEVE_ADJUST (mach, 4);

  if (INSTR_DEBUG (mach))
    mu_i_sv_debug_command (mach, mach->pc - 1, what);
  else
    mu_i_sv_trace (mach, what);
  
  if (!INSTR_DISASS (mach))
    rc = han (mach);
  mach->arg_list = NULL;
  mach->tag_list = NULL;
  mach->identifier = NULL;
  return rc;
}

void
_mu_i_sv_instr_action (mu_sieve_machine_t mach)
{
  mach->action_count++;
  instr_run (mach, "ACTION");
}

void
_mu_i_sv_instr_test (mu_sieve_machine_t mach)
{
  mach->reg = instr_run (mach, "TEST");
}

void
_mu_i_sv_instr_push (mu_sieve_machine_t mach)
{
  if (INSTR_DEBUG (mach))
    mu_i_sv_debug (mach, mach->pc - 1, "PUSH");
  if (INSTR_DISASS (mach))
    return;
  
  if (!mach->stack && mu_list_create (&mach->stack))
    {
      mu_sieve_error (mach, _("cannot create stack"));
      mu_sieve_abort (mach);
    }
  mu_list_push (mach->stack, (void*) mach->reg);
}

void
_mu_i_sv_instr_pop (mu_sieve_machine_t mach)
{
  if (INSTR_DEBUG (mach))
    mu_i_sv_debug (mach, mach->pc - 1, "POP");
  if (INSTR_DISASS (mach))
    return;

  if (!mach->stack || mu_list_is_empty (mach->stack))
    {
      mu_sieve_error (mach, _("stack underflow"));
      mu_sieve_abort (mach);
    }
  mu_list_pop (mach->stack, (void **)&mach->reg);
}

void
_mu_i_sv_instr_not (mu_sieve_machine_t mach)
{
  if (INSTR_DEBUG (mach))
    mu_i_sv_debug (mach, mach->pc - 1, "NOT");
  if (INSTR_DISASS (mach))
    return;
  mach->reg = !mach->reg;
}

void
_mu_i_sv_instr_branch (mu_sieve_machine_t mach)
{
  long num = SIEVE_ARG (mach, 0, number);

  SIEVE_ADJUST (mach, 1);
  if (INSTR_DEBUG (mach))
    mu_i_sv_debug (mach, mach->pc - 2, "BRANCH %lu",
		   (unsigned long)(mach->pc + num));
  if (INSTR_DISASS (mach))
    return;

  mach->pc += num;
}

void
_mu_i_sv_instr_brz (mu_sieve_machine_t mach)
{
  long num = SIEVE_ARG (mach, 0, number);
  SIEVE_ADJUST (mach, 1);

  if (INSTR_DEBUG (mach))
    mu_i_sv_debug (mach, mach->pc - 2, "BRZ %lu",
		   (unsigned long)(mach->pc + num));
  if (INSTR_DISASS (mach))
    return;
  
  if (!mach->reg)
    mach->pc += num;
}
  
void
_mu_i_sv_instr_brnz (mu_sieve_machine_t mach)
{
  long num = SIEVE_ARG (mach, 0, number);
  SIEVE_ADJUST (mach, 1);

  if (INSTR_DEBUG (mach))
    mu_i_sv_debug (mach, mach->pc - 2, "BRNZ %lu",
		   (unsigned long)(mach->pc + num));
  if (INSTR_DISASS (mach))
    return;
  
  if (mach->reg)
    mach->pc += num;
}
  
void
mu_sieve_abort (mu_sieve_machine_t mach)
{
  longjmp (mach->errbuf, 1);
}

void
mu_sieve_set_data (mu_sieve_machine_t mach, void *data)
{
  mach->data = data;
}

void *
mu_sieve_get_data (mu_sieve_machine_t mach)
{
  return mach->data;
}

int
mu_sieve_get_locus (mu_sieve_machine_t mach, struct mu_locus *loc)
{
  if (mach->source_list)
    {
      *loc = mach->locus;
      return 0;
    }
  return 1;
}

mu_message_t
mu_sieve_get_message (mu_sieve_machine_t mach)
{
  if (!mach->msg)
    mu_mailbox_get_message (mach->mailbox, mach->msgno, &mach->msg);
  return mach->msg;
}

size_t
mu_sieve_get_message_num (mu_sieve_machine_t mach)
{
  return mach->msgno;
}

const char *
mu_sieve_get_identifier (mu_sieve_machine_t mach)
{
  return mach->identifier;
}

int
mu_sieve_is_dry_run (mu_sieve_machine_t mach)
{
  return mach->dry_run;
}

int
mu_sieve_set_dry_run (mu_sieve_machine_t mach, int val)
{
  if (mach->state != mu_sieve_state_compiled)
    return EINVAL; //FIXME: another error code
  return mach->dry_run = val;
}

int
sieve_run (mu_sieve_machine_t mach)
{
  int rc;

  mu_sieve_stream_save (mach);
  
  rc = setjmp (mach->errbuf);
  if (rc == 0)
    {
      mach->action_count = 0;
  
      for (mach->pc = 1; mach->prog[mach->pc].handler; )
	(*mach->prog[mach->pc++].instr) (mach);

      if (mach->action_count == 0)
	mu_sieve_log_action (mach, "IMPLICIT KEEP", NULL);
  
      if (INSTR_DEBUG (mach))
	mu_i_sv_debug (mach, mach->pc, "STOP");
    }
  
  mu_sieve_stream_restore (mach);
  
  return rc;
}

int
mu_sieve_disass (mu_sieve_machine_t mach)
{
  int rc;

  if (mach->state != mu_sieve_state_compiled)
    return EINVAL; /* FIXME: Error code */
  mach->state = mu_sieve_state_disass;
  rc = sieve_run (mach);
  mach->state = mu_sieve_state_compiled;
  return rc;
}
  
static int
_sieve_action (mu_observer_t obs, size_t type, void *data, void *action_data)
{
  mu_sieve_machine_t mach;
  
  if (type != MU_EVT_MESSAGE_ADD)
    return 0;

  mach = mu_observer_get_owner (obs);
  mach->msgno++;
  mu_mailbox_get_message (mach->mailbox, mach->msgno, &mach->msg);
  sieve_run (mach);
  return 0;
}

int
mu_sieve_mailbox (mu_sieve_machine_t mach, mu_mailbox_t mbox)
{
  int rc;
  size_t total;
  mu_observer_t observer;
  mu_observable_t observable;
  
  if (!mach || !mbox)
    return EINVAL;

  if (mach->state != mu_sieve_state_compiled)
    return EINVAL; /* FIXME: Error code */
  mach->state = mu_sieve_state_running;
  
  mu_observer_create (&observer, mach);
  mu_observer_set_action (observer, _sieve_action, mach);
  mu_mailbox_get_observable (mbox, &observable);
  mu_observable_attach (observable, MU_EVT_MESSAGE_ADD, observer);
  
  mach->mailbox = mbox;
  mach->msgno = 0;
  rc = mu_mailbox_scan (mbox, 1, &total);
  if (rc)
    mu_sieve_error (mach, _("mu_mailbox_scan: %s"), mu_strerror (errno));

  mu_observable_detach (observable, observer);
  mu_observer_destroy (&observer, mach);

  mach->state = mu_sieve_state_compiled;
  mach->mailbox = NULL;
  
  return rc;
}

int
mu_sieve_message (mu_sieve_machine_t mach, mu_message_t msg)
{
  int rc;
  
  if (!mach || !msg)
    return EINVAL;

  if (mach->state != mu_sieve_state_compiled)
    return EINVAL; /* FIXME: Error code */
  mach->state = mu_sieve_state_running;

  mach->msgno = 1;
  mach->msg = msg;
  mach->mailbox = NULL;
  rc = sieve_run (mach);
  mach->state = mu_sieve_state_compiled;
  mach->msg = NULL;
  
  return rc;
}

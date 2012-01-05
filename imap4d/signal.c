/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2007-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "imap4d.h"

jmp_buf child_jmp;
static int __critical_section;
static int __got_signal;

void
imap4d_enter_critical ()
{
  __critical_section = 1;
  if (__got_signal)
    __critical_section++;
}

void
imap4d_leave_critical ()
{
  if (__got_signal && __critical_section != 2)
    longjmp (child_jmp, __got_signal);
  __critical_section = 0;
}

RETSIGTYPE
imap4d_child_signal (int signo)
{
  imap4d_child_signal_setup (SIG_IGN);
  if (__critical_section)
    __got_signal = signo;
  else
    longjmp (child_jmp, signo);
}

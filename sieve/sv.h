/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef SV_H
#define SV_H

#include <mailutils/types.h>

#include "sieve_interface.h"

#include "svfield.h"

#include "sieve.h"

/** sieve context structures

The object relationship diagram is this, with the names in ""
being the argument name when the contexts are provided as
arguments to callback functions.

  sieve_execute_script()  --> sv_msg_ctx_t, "mc"

       |
       |
       V

  sieve_script_t  --->  sv_script_ctx_t, "sc"

       |
       |
       V

  sieve_interp_t  --->  sv_interp_ctx_t, "ic"


*/

typedef struct sv_interp_ctx_t
{
  /* The sieve interpreter for our context. */
  sieve_interp_t *interp;

  /* Callback functions. */
  sv_parse_error_t   parse_error;
  sv_execute_error_t execute_error;
  sv_action_log_t    action_log;

} sv_interp_ctx_t;

typedef struct sv_script_ctx_t
{
  /* The file the script was parsed from. */
  char* file;

  /* The sieve script for our context. */
  sieve_script_t* script;

  /* The sieve interpreter context for our script. */
  sv_interp_ctx_t* ic;

} sv_script_ctx_t;

typedef struct sv_msg_ctx_t
{
  /* The mailutils return code - the real error. */
  int rc;

  /* The field cache. */
  int cache_filled;
  sv_field_cache_t cache;

  sv_script_ctx_t* sc;

  message_t msg;

  /* Ticket for use by mailbox URLs for implicit authentication. */
  ticket_t ticket;

  /* Debug used for debug output. */
  mu_debug_t debug;

  /* Mailer for redirecting messages. */
  mailer_t mailer;

  /* Flags controlling execution of script. */
  int svflags;

  /* The uid of the message, for debug output. */
  size_t uid;
} sv_msg_ctx_t;


/*
  svcb.c: sieve callbacks
*/

int sv_register_callbacks (sieve_interp_t * i);

/*
  sv?.c: sv print wrappers

  These should call print callbacks supplied by the user of the mailutils
  sieve implementation.
*/

extern void sv_printv (sv_interp_ctx_t* ic, int level, const char *fmt, va_list ap);
extern void sv_print (sv_interp_ctx_t* ic, int level, const char* fmt, ...);

/*
  svutil.c: utility functions and mailutil wrapper functions
*/

/* Converts a mailutils errno to the equivalent sieve return code. */

extern int sv_mu_errno_to_rc (int eno);

extern int sv_mu_debug_print (mu_debug_t d, const char *fmt, va_list ap);

extern int sv_mu_mark_deleted (message_t msg, int deleted);

extern int sv_mu_copy_debug_level (const mailbox_t from, mailbox_t to);

#endif


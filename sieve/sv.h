#ifndef SV_H
#define SV_H

#include <mailutils/mailbox.h>
#include <mailutils/address.h>
#include <mailutils/registrar.h>

#include "sieve_interface.h"

#include "svfield.h"

/** sieve context structures

The object relationship diagram is this, with the names in ""
being the argument name when the context's are provided as
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
  /* options */
  int   opt_no_actions;
  int   opt_verbose;
  int   opt_no_run;
  int   opt_watch;
  char* opt_mbox;
  char* opt_tickets;
  char* opt_script;

  int   print_mask;
  FILE* print_stream;

  /* Ticket for use by mailbox URLs for implicit authentication. */
  ticket_t ticket;

  /* mailutils debug handle, we need to destroy it */
  mu_debug_t debug;
} sv_interp_ctx_t;

typedef struct sv_script_ctx_t
{
    sv_interp_ctx_t* ic;
} sv_script_ctx_t;

typedef struct sv_msg_ctx_t
{
  int rc;			/* the mailutils return code */
  int cache_filled;
  sv_field_cache_t cache;
  message_t msg;
  mailbox_t mbox;
  char *summary;

  sv_interp_ctx_t* ic;
} sv_msg_ctx_t;

enum /* print level masks */
{
    SV_PRN_MU  = 0x01,
    SV_PRN_ACT = 0x02,
    SV_PRN_QRY = 0x04,
    SV_PRN_ERR = 0x08,
    SV_PRN_NOOP
};

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

extern int sv_mu_mark_deleted (message_t msg);

extern int sv_mu_copy_debug_level (const mailbox_t from, mailbox_t to);

extern int sv_mu_save_to (const char *toname, message_t mesg, ticket_t ticket, const char **errmsg);

#endif


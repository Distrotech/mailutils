/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005-2012, 2014-2016 Free Software
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

#include <mailutils/sieve.h>
#include <setjmp.h>
#include <string.h>

#define SIEVE_CODE_INCR 128

typedef void (*sieve_instr_t) (mu_sieve_machine_t mach);

typedef union
{
  sieve_instr_t instr;
  mu_sieve_handler_t handler;
  mu_sieve_value_t *val;
  mu_list_t list;
  long number;
  size_t pc;
  size_t line;
  int inum;
  char *string;
} sieve_op_t;

struct mu_locus_range
{
  struct mu_locus beg;
  struct mu_locus end;
};
  
#define YYLTYPE struct mu_locus_range

enum mu_sieve_state
  {
    mu_sieve_state_init,
    mu_sieve_state_error,
    mu_sieve_state_compiled,
    mu_sieve_state_running
  };

struct mu_sieve_machine
{
  /* Static data */
  struct mu_locus locus;     /* Approximate location in the code */

  mu_list_t memory_pool;     /* Pool of allocated memory objects */
  mu_list_t destr_list;      /* List of destructor functions */

  /* Symbol space: */
  mu_opool_t string_pool;    /* String constants */
  mu_list_t test_list;       /* Tests */
  mu_list_t action_list;     /* Actions */
  mu_list_t comp_list;       /* Comparators */
  mu_list_t source_list;     /* Source names (for diagnostics) */
  
  size_t progsize;           /* Number of allocated program cells */
  sieve_op_t *prog;          /* Compiled program */

  /* Runtime data */
  enum mu_sieve_state state;
  size_t pc;                 /* Current program counter */
  long reg;                  /* Numeric register */
  mu_list_t stack;           /* Runtime stack */

  int debug_level;           /* Debugging level */
  jmp_buf errbuf;            /* Target location for non-local exits */
  const char *identifier;    /* Name of action or test being executed */
  
  mu_mailbox_t mailbox;      /* Mailbox to operate upon */
  size_t    msgno;           /* Current message number */
  mu_message_t msg;          /* Current message */
  int action_count;          /* Number of actions executed over this message */
			    
  /* User supplied data */
  mu_stream_t errstream;
  
  mu_sieve_action_log_t logger;
  
  mu_mailer_t mailer;
  char *daemon_email;
  void *data;
};

enum mu_sieve_node_type
  {
    mu_sieve_node_noop,
    mu_sieve_node_false,
    mu_sieve_node_true,
    mu_sieve_node_test,
    mu_sieve_node_action,
    mu_sieve_node_cond,
    mu_sieve_node_anyof,
    mu_sieve_node_allof,
    mu_sieve_node_not,
  };

struct mu_sieve_node
{
  struct mu_sieve_node *prev, *next;
  enum mu_sieve_node_type type;
  struct mu_locus_range locus;
  union
  {
    mu_sieve_value_t *value;
    mu_list_t list;
    struct mu_sieve_node *node;
    struct
    {
      struct mu_sieve_node *expr;
      struct mu_sieve_node *iftrue;
      struct mu_sieve_node *iffalse;
    } cond;
    struct
    {
      mu_sieve_register_t *reg;
      mu_list_t arg;
    } command;
  } v;
};

int mu_sieve_yyerror (const char *s);
int mu_sieve_yylex (void);

int mu_i_sv_lex_begin (const char *name);
int mu_i_sv_lex_begin_string (const char *buf, int bufsize,
			      const char *fname, int line);
void mu_i_sv_lex_finish (struct mu_sieve_machine *mach);

extern mu_sieve_machine_t mu_sieve_machine;

#define TAG_COMPFUN "__compfun__"

int mu_i_sv_code (struct mu_sieve_machine *mach, sieve_op_t op);

void mu_i_sv_compile_error (struct mu_sieve_machine *mach,
			    const char *fmt, ...) MU_PRINTFLIKE(2,3);

int mu_i_sv_locus (struct mu_sieve_machine *mach, struct mu_locus_range *lr);
int mu_i_sv_code_action (struct mu_sieve_machine *mach,
			 mu_sieve_register_t *reg, mu_list_t arglist);
int mu_i_sv_code_test (struct mu_sieve_machine *mach,
		       mu_sieve_register_t *reg, mu_list_t arglist);

/* Opcodes */
void _mu_sv_instr_action (mu_sieve_machine_t mach);
void _mu_sv_instr_test (mu_sieve_machine_t mach);
void _mu_sv_instr_push (mu_sieve_machine_t mach);
void _mu_sv_instr_pop (mu_sieve_machine_t mach);
void _mu_sv_instr_not (mu_sieve_machine_t mach);
void _mu_sv_instr_branch (mu_sieve_machine_t mach);
void _mu_sv_instr_brz (mu_sieve_machine_t mach);
void _mu_sv_instr_brnz (mu_sieve_machine_t mach);
void _mu_sv_instr_nop (mu_sieve_machine_t mach);
void _mu_sv_instr_source (mu_sieve_machine_t mach);
void _mu_sv_instr_line (mu_sieve_machine_t mach);

int mu_sv_load_add_dir (mu_sieve_machine_t mach, const char *name);

void mu_sv_register_standard_actions (mu_sieve_machine_t mach);
void mu_sv_register_standard_tests (mu_sieve_machine_t mach);
void mu_sv_register_standard_comparators (mu_sieve_machine_t mach);

void mu_sv_print_value_list (mu_list_t list, mu_stream_t str);
void mu_sv_print_tag_list (mu_list_t list, mu_stream_t str);

void mu_i_sv_error (mu_sieve_machine_t mach);


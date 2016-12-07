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
#include <mailutils/assoc.h>
#include <setjmp.h>
#include <string.h>
#include <regex.h>

typedef void (*sieve_instr_t) (mu_sieve_machine_t mach);

typedef union
{
  sieve_instr_t instr;
  mu_sieve_handler_t handler;
  mu_sieve_value_t *val;
  mu_sieve_comparator_t comp;
  long number;
  size_t pc;
  size_t line;
  int inum;
  char *string;
  unsigned unum;
} sieve_op_t;

struct mu_locus_range
{
  struct mu_locus beg;
  struct mu_locus end;
};
  
#define YYLTYPE struct mu_locus_range

#define MU_SV_SAVED_ERR_STATE 0x01
#define MU_SV_SAVED_DBG_STATE 0x02
#define MU_SV_SAVED_STATE     0x80

enum mu_sieve_state
  {
    mu_sieve_state_init,
    mu_sieve_state_error,
    mu_sieve_state_compiled,
    mu_sieve_state_running,
    mu_sieve_state_disass
  };

struct mu_sieve_machine
{
  /* Static data */
  struct mu_locus locus;     /* Approximate location in the code */

  mu_list_t memory_pool;     /* Pool of allocated memory objects */
  mu_list_t destr_list;      /* List of destructor functions */

  /* Symbol space: */
  mu_opool_t string_pool;    /* String constants */
  mu_list_t registry;        /* Tests, Actions, Comparators */

  char **idspace;            /* Source and identifier names */
  size_t idcount;
  size_t idmax;
  
  mu_sieve_string_t *stringspace;
  size_t stringcount;
  size_t stringmax;
  
  mu_sieve_value_t *valspace;
  size_t valcount;
  size_t valmax;
  
  size_t progsize;           /* Number of allocated program cells */
  sieve_op_t *prog;          /* Compiled program */

  /* Runtime data */
  enum mu_sieve_state state; /* Machine state */
  size_t pc;                 /* Current program counter */
  long reg;                  /* Numeric register */

  /* Support for variables (RFC 5229) */
  mu_assoc_t vartab;         /* Table of variables */
  char *match_string;        /* The string used in the most recent match */
  regmatch_t *match_buf;     /* Offsets of parenthesized groups */
  size_t match_count;        /* Actual number of elements used in match_buf */
  size_t match_max;          /* Total number of elements available in match_buf */

  /* Call environment */
  const char *identifier;    /* Name of action or test being executed */
  size_t argstart;           /* Index of the first argument in valspace */
  size_t argcount;           /* Number of positional arguments */
  size_t tagcount;           /* Number of tagged arguments */
  mu_sieve_comparator_t comparator; /* Comparator (for tests) */

  int dry_run;               /* Dry-run mode */
  jmp_buf errbuf;            /* Target location for non-local exits */
  
  mu_mailbox_t mailbox;      /* Mailbox to operate upon */
  size_t    msgno;           /* Current message number */
  mu_message_t msg;          /* Current message */
  int action_count;          /* Number of actions executed over this message */

  /* Stream state info */
  int state_flags;
  int err_mode;
  struct mu_locus err_locus;
  int dbg_mode;
  struct mu_locus dbg_locus;
  
  /* User supplied data */
  mu_stream_t errstream;
  mu_stream_t dbgstream;
  
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
    struct mu_sieve_node *node;
    struct
    {
      struct mu_sieve_node *expr;
      struct mu_sieve_node *iftrue;
      struct mu_sieve_node *iffalse;
    } cond;
    struct
    {
      mu_sieve_registry_t *reg;
      size_t argstart;
      size_t argcount;
      size_t tagcount;
      mu_sieve_comparator_t comparator; /* Comparator (for tests) */
    } command;
  } v;
};

int mu_sieve_yyerror (const char *s);
int mu_sieve_yylex (void);

int mu_i_sv_lex_begin (const char *name);
int mu_i_sv_lex_begin_string (const char *buf, int bufsize,
			      const char *fname, int line);
void mu_i_sv_lex_finish (void);

extern mu_sieve_machine_t mu_sieve_machine;

void mu_i_sv_code (struct mu_sieve_machine *mach, sieve_op_t op);

int mu_i_sv_locus (struct mu_sieve_machine *mach, struct mu_locus_range *lr);
void mu_i_sv_code_action (struct mu_sieve_machine *mach,
			  struct mu_sieve_node *node);
void mu_i_sv_code_test (struct mu_sieve_machine *mach,
			struct mu_sieve_node *node);

/* Opcodes */
void _mu_i_sv_instr_action (mu_sieve_machine_t mach);
void _mu_i_sv_instr_test (mu_sieve_machine_t mach);
void _mu_i_sv_instr_not (mu_sieve_machine_t mach);
void _mu_i_sv_instr_branch (mu_sieve_machine_t mach);
void _mu_i_sv_instr_brz (mu_sieve_machine_t mach);
void _mu_i_sv_instr_brnz (mu_sieve_machine_t mach);
void _mu_i_sv_instr_source (mu_sieve_machine_t mach);
void _mu_i_sv_instr_line (mu_sieve_machine_t mach);

int mu_i_sv_load_add_dir (mu_sieve_machine_t mach, const char *name);

void mu_i_sv_register_standard_actions (mu_sieve_machine_t mach);
void mu_i_sv_register_standard_tests (mu_sieve_machine_t mach);
void mu_i_sv_register_standard_comparators (mu_sieve_machine_t mach);

void mu_i_sv_error (mu_sieve_machine_t mach);

void mu_i_sv_debug (mu_sieve_machine_t mach, size_t pc, const char *fmt, ...)
  MU_PRINTFLIKE(3,4);
void mu_i_sv_debug_command (mu_sieve_machine_t mach, size_t pc,
			    char const *what);
void mu_i_sv_trace (mu_sieve_machine_t mach, const char *what);

void mu_i_sv_valf (mu_sieve_machine_t mach, mu_stream_t str,
		   mu_sieve_value_t *val);

typedef int (*mu_i_sv_interp_t) (char const *, size_t, char **, void *);

int mu_i_sv_string_expand (char const *input,
			   mu_i_sv_interp_t interp, void *data, char **ret);

int mu_i_sv_expand_encoded_char (char const *input, size_t len, char **exp, void *data);

int mu_sieve_require_encoded_character (mu_sieve_machine_t mach,
					const char *name);

void mu_i_sv_2nrealloc (mu_sieve_machine_t mach,
			void **pptr, size_t *pnmemb, size_t size);


mu_sieve_value_t *mu_i_sv_mach_arg (mu_sieve_machine_t mach, size_t n);
mu_sieve_value_t *mu_i_sv_mach_tagn (mu_sieve_machine_t mach, size_t n);
void mu_i_sv_lint_command (struct mu_sieve_machine *mach,
			   struct mu_sieve_node *node);


size_t  mu_i_sv_string_create (mu_sieve_machine_t mach, char *str);

char *mu_i_sv_id_canon (mu_sieve_machine_t mach, char const *name);
size_t mu_i_sv_id_num (mu_sieve_machine_t mach, char const *name);
char *mu_i_sv_id_str (mu_sieve_machine_t mach, size_t n);
void mu_i_sv_free_idspace (mu_sieve_machine_t mach);

void mu_i_sv_copy_variables (mu_sieve_machine_t child,
			     mu_sieve_machine_t parent);
int mu_i_sv_expand_variables (char const *input, size_t len,
			      char **exp, void *data);


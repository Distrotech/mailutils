/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005  Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#include <mailutils/libsieve.h>
#include <mu_asprintf.h>
#include <setjmp.h>
#include <string.h>

#define SIEVE_CODE_INCR 128

typedef void (*sieve_instr_t) (sieve_machine_t mach);

typedef union {
  sieve_instr_t instr;
  sieve_handler_t handler;
  sieve_value_t *val;
  list_t list;
  long number;
  char *string;
  size_t pc;
  size_t line;
} sieve_op_t;

struct sieve_machine {
  /* Static data */
  sieve_locus_t locus;    /* Approximate location in the code */

  list_t memory_pool;     /* Pool of allocated memory objects */
  list_t destr_list;      /* List of destructor functions */

  /* Symbol space: */
  list_t test_list;       /* Tests */
  list_t action_list;     /* Actions */
  list_t comp_list;       /* Comparators */
  list_t source_list;     /* Source names (for diagnostics) */
  
  size_t progsize;        /* Number of allocated program cells */
  sieve_op_t *prog;       /* Compiled program */

  /* Runtime data */
  size_t pc;              /* Current program counter */
  long reg;               /* Numeric register */
  list_t stack;           /* Runtime stack */

  int debug_level;        /* Debugging level */
  jmp_buf errbuf;         /* Target location for non-local exits */
  char *identifier;       /* Name of action or test being executed */
  
  mailbox_t mailbox;      /* Mailbox to operate upon */
  size_t    msgno;        /* Current message number */
  message_t msg;          /* Current message */
  int action_count;       /* Number of actions executed over this message */
			    
  /* User supplied data */
  sieve_parse_error_t parse_error_printer;
  sieve_printf_t error_printer; 
  sieve_printf_t debug_printer;
  sieve_action_log_t logger;
  
  mailer_t mailer;
  ticket_t ticket;
  mu_debug_t mu_debug;
  char *daemon_email;
  void *data;
};

extern char *sieve_filename;
extern int sieve_line_num;
extern sieve_machine_t sieve_machine;
extern int sieve_error_count; 

#define TAG_COMPFUN "__compfun__"
#define TAG_RELFUN  "__relfun__"

void sieve_compile_error (const char *filename, int linenum,
			  const char *fmt, ...);
void sieve_debug_internal (sieve_printf_t printer, void *data,
			   const char *fmt, ...);
void sieve_print_value (sieve_value_t *val, sieve_printf_t printer,
		        void *data);
void sieve_print_value_list (list_t list, sieve_printf_t printer, void *data);
void sieve_print_tag_list (list_t list, sieve_printf_t printer, void *data);

int _sieve_default_error_printer (void *data, const char *fmt, va_list ap);
int _sieve_default_parse_error (void *unused, const char *filename, int lineno,
			        const char *fmt, va_list ap);

int sieve_lex_begin (const char *name);
void sieve_lex_finish (void);

void sieve_register_standard_actions (sieve_machine_t mach);
void sieve_register_standard_tests (sieve_machine_t mach);
void sieve_register_standard_comparators (sieve_machine_t mach);

int sieve_code (sieve_op_t *op);
int sieve_code_instr (sieve_instr_t instr);
int sieve_code_handler (sieve_handler_t handler);
int sieve_code_list (list_t list);
int sieve_code_number (long num);
int sieve_code_test (sieve_register_t *reg, list_t arglist);
int sieve_code_action (sieve_register_t *reg, list_t arglist);
void sieve_code_anyof (size_t start);
void sieve_code_allof (size_t start);
int sieve_code_source (const char *name);
int sieve_code_line (size_t line);

void instr_action (sieve_machine_t mach);
void instr_test (sieve_machine_t mach);
void instr_push (sieve_machine_t mach);
void instr_pop (sieve_machine_t mach);
void instr_not (sieve_machine_t mach);
void instr_branch (sieve_machine_t mach);
void instr_brz (sieve_machine_t mach);
void instr_brnz (sieve_machine_t mach);
void instr_nop (sieve_machine_t mach);
void instr_source (sieve_machine_t mach);
void instr_line (sieve_machine_t mach);

int sieve_mark_deleted (message_t msg, int deleted);

int sieve_match_part_checker (const char *name, list_t tags, list_t args);
int sieve_relational_checker (const char *name, list_t tags, list_t args);

int sieve_load_add_path (list_t path);
int sieve_load_add_dir (sieve_machine_t mach, const char *name);

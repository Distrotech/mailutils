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

#include <mailutils/libsieve.h>
#include <setjmp.h>

#define SIEVE_CODE_INCR 128

typedef void (*sieve_instr_t) __P((sieve_machine_t mach));

typedef union {
  sieve_instr_t instr;
  sieve_handler_t handler;
  sieve_value_t *val;
  list_t list;
  long number;
  char *string;
  size_t pc;
} sieve_op_t;

struct sieve_machine {
  /* Static data */
  char *filename;         /* Name of the source script */
  list_t memory_pool;     /* Pool of allocated memory objects */

  size_t progsize;        /* Number of allocated program cells */
  sieve_op_t *prog;       /* Compiled program */

  /* Runtime data */
  size_t pc;              /* Current program counter */
  long reg;               /* Numeric register */
  list_t stack;           /* Runtime stack */

  int debug_level;        /* Debugging level */
  jmp_buf errbuf;         /* Target location for non-local exits */

  mailbox_t mailbox;      /* Mailbox to operate upon */
  size_t    msgno;        /* Current message number */
  message_t msg;          /* Current message */

  /* User supplied data */
  sieve_parse_error_t parse_error_printer;
  sieve_printf_t error_printer; 
  sieve_printf_t debug_printer;
  sieve_action_log_t logger;

  ticket_t ticket;
  mu_debug_t mu_debug;
  void *data;
};

extern char *sieve_filename;
extern int sieve_line_num;
extern sieve_machine_t sieve_machine;
extern int sieve_error_count; 

void sieve_compile_error __P((const char *filename, int linenum,
			      const char *fmt, ...));
void sieve_debug_internal __P((sieve_printf_t printer, void *data,
			       const char *fmt, ...));
void sieve_print_value __P((sieve_value_t *val, sieve_printf_t printer,
			    void *data));
void sieve_print_value_list __P((list_t list, sieve_printf_t printer,
				 void *data));
void sieve_print_tag_list __P((list_t list, sieve_printf_t printer,
			       void *data));

int _sieve_default_error_printer __P((void *data,
				      const char *fmt, va_list ap));
int _sieve_default_parse_error __P((void *unused,
				    const char *filename, int lineno,
				    const char *fmt, va_list ap));

int sieve_lex_begin __P((const char *name));
void sieve_lex_finish __P((void));

void sieve_register_standard_actions __P((void));
void sieve_register_standard_tests __P((void));

int sieve_code __P((sieve_op_t *op));
int sieve_code_instr __P((sieve_instr_t instr));
int sieve_code_handler __P((sieve_handler_t handler));
int sieve_code_list __P((list_t list));
int sieve_code_number __P((long num));
int sieve_code_test __P((sieve_register_t *reg, list_t arglist));
int sieve_code_action __P((sieve_register_t *reg, list_t arglist));
     
void instr_action __P((sieve_machine_t mach));
void instr_test __P((sieve_machine_t mach));
void instr_push __P((sieve_machine_t mach));
void instr_pop __P((sieve_machine_t mach));
void instr_allof __P((sieve_machine_t mach));
void instr_anyof __P((sieve_machine_t mach));
void instr_not __P((sieve_machine_t mach));
void instr_branch __P((sieve_machine_t mach));
void instr_brz __P((sieve_machine_t mach));

int sieve_mark_deleted __P((message_t msg, int deleted));

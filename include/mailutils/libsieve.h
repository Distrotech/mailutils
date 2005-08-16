/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_LIBSIEVE_H
#define _MAILUTILS_LIBSIEVE_H

#include <sys/types.h>
#include <stdarg.h>
#include <mailutils/mailutils.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __s_cat3__(a,b,c) a ## b ## c
#define SIEVE_EXPORT(module,name) __s_cat3__(module,_LTX_,name)

typedef struct sieve_machine *sieve_machine_t;

typedef struct {
  const char *source_file;
  size_t source_line;
} sieve_locus_t;

typedef int (*sieve_handler_t) (sieve_machine_t mach,
			        list_t args, list_t tags);
typedef int (*sieve_printf_t) (void *data, const char *fmt, va_list ap);
typedef int (*sieve_parse_error_t) (void *data,
				    const char *filename, int lineno,
				    const char *fmt, va_list ap);
typedef void (*sieve_action_log_t) (void *data,
				    const sieve_locus_t *locus,
				    size_t msgno, message_t msg,
				    const char *action,
				    const char *fmt, va_list ap);

typedef int (*sieve_relcmp_t) (int, int);  
typedef int (*sieve_relcmpn_t) (size_t, size_t);  
typedef int (*sieve_comparator_t) (const char *, const char *);
typedef int (*sieve_retrieve_t) (void *item, void *data, int idx, char **pval);
typedef void (*sieve_destructor_t) (void *data);
typedef int (*sieve_tag_checker_t) (const char *name,
				    list_t tags, list_t args);

typedef enum {
  SVT_VOID,
  SVT_NUMBER,
  SVT_STRING,
  SVT_STRING_LIST,
  SVT_TAG,
  SVT_IDENT,
  SVT_VALUE_LIST,
  SVT_POINTER
} sieve_data_type;

typedef struct sieve_runtime_tag sieve_runtime_tag_t;

typedef struct {
  sieve_data_type type;
  union {
    char *string;
    size_t number;
    list_t list;
    sieve_runtime_tag_t *tag;
    void *ptr;
  } v;
} sieve_value_t;

typedef struct {
  char *name;
  sieve_data_type argtype;
} sieve_tag_def_t;

typedef struct {
  sieve_tag_def_t *tags;
  sieve_tag_checker_t checker;
} sieve_tag_group_t;

struct sieve_runtime_tag {
  char *tag;
  sieve_value_t *arg;
};

typedef struct {
  const char *name;
  int required;
  sieve_handler_t handler;
  sieve_data_type *req_args;
  sieve_tag_group_t *tags;
} sieve_register_t;

#define MU_SIEVE_MATCH_IS        1
#define MU_SIEVE_MATCH_CONTAINS  2
#define MU_SIEVE_MATCH_MATCHES   3
#define MU_SIEVE_MATCH_REGEX     4
#define MU_SIEVE_MATCH_EQ        5
#define MU_SIEVE_MATCH_LAST      6

/* Debugging levels */
#define MU_SIEVE_DEBUG_TRACE  0x0001 
#define MU_SIEVE_DEBUG_INSTR  0x0002
#define MU_SIEVE_DEBUG_DISAS  0x0004
#define MU_SIEVE_DRY_RUN      0x0008

extern int sieve_yydebug;
extern list_t sieve_include_path;
extern list_t sieve_library_path;

/* Memory allocation functions */
void *sieve_alloc (size_t size);
void *sieve_palloc (list_t *pool, size_t size);
void *sieve_prealloc (list_t *pool, void *ptr, size_t size);
void sieve_pfree (list_t *pool, void *ptr);
char *sieve_pstrdup (list_t *pool, const char *str);

void *sieve_malloc (sieve_machine_t mach, size_t size);
char *sieve_mstrdup (sieve_machine_t mach, const char *str);
void *sieve_mrealloc (sieve_machine_t mach, void *ptr, size_t size);
void sieve_mfree (sieve_machine_t mach, void *ptr);

sieve_value_t *sieve_value_create (sieve_data_type type, void *data);
void sieve_slist_destroy (list_t *plist);

/* Symbol space functions */
sieve_register_t *sieve_test_lookup (sieve_machine_t mach,
				     const char *name);
sieve_register_t *sieve_action_lookup (sieve_machine_t mach,
				       const char *name);
int sieve_register_test (sieve_machine_t mach,
		         const char *name, sieve_handler_t handler,
			 sieve_data_type *arg_types,
			 sieve_tag_group_t *tags, int required);
int sieve_register_action (sieve_machine_t mach,
			   const char *name, sieve_handler_t handler,
			   sieve_data_type *arg_types,
			   sieve_tag_group_t *tags, int required);
int sieve_register_comparator (sieve_machine_t mach, const char *name,
			       int required, sieve_comparator_t is,
			       sieve_comparator_t contains,
			       sieve_comparator_t matches,
			       sieve_comparator_t regex,
			       sieve_comparator_t eq);
int sieve_require_action (sieve_machine_t mach, const char *name);
int sieve_require_test (sieve_machine_t mach, const char *name);
int sieve_require_comparator (sieve_machine_t mach, const char *name);
int sieve_require_relational (sieve_machine_t mach, const char *name);
  
sieve_comparator_t sieve_comparator_lookup (sieve_machine_t mach,
				  	    const char *name,
					    int matchtype);

sieve_comparator_t sieve_get_comparator (sieve_machine_t mach, list_t tags);
int sieve_str_to_relcmp (const char *str,
		         sieve_relcmp_t *test, sieve_relcmpn_t *stest);
sieve_relcmp_t sieve_get_relcmp (sieve_machine_t mach, list_t tags);
  
void sieve_require (list_t slist);
int sieve_tag_lookup (list_t taglist, char *name, sieve_value_t **arg);
int sieve_load_ext (sieve_machine_t mach, const char *name);
int sieve_match_part_checker (const char *name, list_t tags, list_t args);
  
/* Operations in value lists */
sieve_value_t *sieve_value_get (list_t vlist, size_t index);
int sieve_vlist_do (sieve_value_t *val, list_action_t *ac, void *data);
int sieve_vlist_compare (sieve_value_t *a, sieve_value_t *b,
			 sieve_comparator_t comp, sieve_relcmp_t test,
			 sieve_retrieve_t ac,
			 void *data,
			 size_t *count);

/* Functions to create and destroy sieve machine */
int sieve_machine_init (sieve_machine_t *mach, void *data);
void sieve_machine_destroy (sieve_machine_t *pmach);
int sieve_machine_add_destructor (sieve_machine_t mach,
				  sieve_destructor_t destr,
				  void *ptr);

/* Functions for accessing sieve machine internals */
void *sieve_get_data (sieve_machine_t mach);
message_t sieve_get_message (sieve_machine_t mach);
size_t sieve_get_message_num (sieve_machine_t mach);
int sieve_get_debug_level (sieve_machine_t mach);
ticket_t sieve_get_ticket (sieve_machine_t mach);
mailer_t sieve_get_mailer (sieve_machine_t mach);
int sieve_get_locus (sieve_machine_t mach, sieve_locus_t *);
char *sieve_get_daemon_email (sieve_machine_t mach);
const char *sieve_get_identifier (sieve_machine_t mach);
       
void sieve_set_error (sieve_machine_t mach, sieve_printf_t error_printer);
void sieve_set_parse_error (sieve_machine_t mach, sieve_parse_error_t p);
void sieve_set_debug (sieve_machine_t mach, sieve_printf_t debug);
void sieve_set_debug_level (sieve_machine_t mach, mu_debug_t dbg, int level);
void sieve_set_logger (sieve_machine_t mach, sieve_action_log_t logger);
void sieve_set_ticket (sieve_machine_t mach, ticket_t ticket);
void sieve_set_mailer (sieve_machine_t mach, mailer_t mailer);
void sieve_set_daemon_email (sieve_machine_t mach, const char *email);

int sieve_get_message_sender (message_t msg, char **ptext);

/* Logging and diagnostic functions */

void sieve_error (sieve_machine_t mach, const char *fmt, ...);
void sieve_debug (sieve_machine_t mach, const char *fmt, ...);
void sieve_log_action (sieve_machine_t mach, const char *action,
		       const char *fmt, ...);
void sieve_abort (sieve_machine_t mach);
int stream_printf (stream_t stream, size_t *off, const char *fmt, ...);
void sieve_arg_error (sieve_machine_t mach, int n);
  
int sieve_is_dry_run (sieve_machine_t mach);
const char *sieve_type_str (sieve_data_type type);

/* Principal entry points */

int sieve_compile (sieve_machine_t mach, const char *name);
int sieve_mailbox (sieve_machine_t mach, mailbox_t mbox);
int sieve_message (sieve_machine_t mach, message_t message);
int sieve_disass (sieve_machine_t mach);

/* Command line handling */  
  
extern void sieve_argp_init (void);
  
#ifdef __cplusplus
}
#endif

#endif

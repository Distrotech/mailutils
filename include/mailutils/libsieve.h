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

typedef int (*sieve_handler_t) __PMT((sieve_machine_t mach,
				      list_t args, list_t tags));
typedef int (*sieve_printf_t) __PMT((void *data, const char *fmt, va_list ap));
typedef int (*sieve_parse_error_t) __PMT((void *data,
					  const char *filename, int lineno,
					  const char *fmt, va_list ap));
typedef void (*sieve_action_log_t) __PMT((void *data,
					  const char *script,
					  size_t msgno, message_t msg,
					  const char *action,
					  const char *fmt, va_list ap));

typedef int (*sieve_comparator_t) __PMT((const char *, const char *));
typedef int (*sieve_retrieve_t) __PMT((void *item, void *data, int idx, char **pval));
typedef void (*sieve_destructor_t) __PMT((void *data));
typedef int (*sieve_tag_checker_t) __PMT((const char *name,
					  list_t tags, list_t args));

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
    long number;
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
  char *name;
  int required;
  sieve_handler_t handler;
  sieve_data_type *req_args;
  sieve_tag_group_t *tags;
} sieve_register_t;

#define MU_SIEVE_MATCH_IS        1
#define MU_SIEVE_MATCH_CONTAINS  2
#define MU_SIEVE_MATCH_MATCHES   3
#define MU_SIEVE_MATCH_REGEX     4
#define MU_SIEVE_MATCH_LAST      5

/* Debugging levels */
#define MU_SIEVE_DEBUG_TRACE  0x0001 
#define MU_SIEVE_DEBUG_INSTR  0x0002
#define MU_SIEVE_DEBUG_DISAS  0x0004
#define MU_SIEVE_DRY_RUN      0x0008

extern int sieve_yydebug;
extern list_t sieve_include_path;
extern list_t sieve_library_path;

/* Memory allocation functions */
void *sieve_alloc __P((size_t size));
void *sieve_palloc __P((list_t *pool, size_t size));
void *sieve_prealloc __P((list_t *pool, void *ptr, size_t size));
void sieve_pfree __P((list_t *pool, void *ptr));
char *sieve_pstrdup __P((list_t *pool, const char *str));

void *sieve_malloc __P((sieve_machine_t mach, size_t size));
char *sieve_mstrdup __P((sieve_machine_t mach, const char *str));
void *sieve_mrealloc __P((sieve_machine_t mach, void *ptr, size_t size));
void sieve_mfree __P((sieve_machine_t mach, void *ptr));

sieve_value_t *sieve_value_create __P((sieve_data_type type, void *data));
void sieve_slist_destroy __P((list_t *plist));

/* Symbol space functions */
sieve_register_t *sieve_test_lookup __P((sieve_machine_t mach,
					 const char *name));
sieve_register_t *sieve_action_lookup __P((sieve_machine_t mach,
					   const char *name));
int sieve_register_test __P((sieve_machine_t mach,
			     const char *name, sieve_handler_t handler,
			     sieve_data_type *arg_types,
			     sieve_tag_group_t *tags, int required));
int sieve_register_action __P((sieve_machine_t mach,
			       const char *name, sieve_handler_t handler,
			       sieve_data_type *arg_types,
			       sieve_tag_group_t *tags, int required));
int sieve_register_comparator __P((sieve_machine_t mach,
				   const char *name,
				   int required,
				   sieve_comparator_t is,
				   sieve_comparator_t contains,
				   sieve_comparator_t matches,
				   sieve_comparator_t regex));
int sieve_require_action __P((sieve_machine_t mach, const char *name));
int sieve_require_test __P((sieve_machine_t mach, const char *name));
int sieve_require_comparator __P((sieve_machine_t mach, const char *name));
  
sieve_comparator_t sieve_comparator_lookup __P((sieve_machine_t mach,
						const char *name,
						int matchtype));

sieve_comparator_t sieve_get_comparator __P((sieve_machine_t mach,
					     list_t tags));
  
void sieve_require __P((list_t slist));
int sieve_tag_lookup __P((list_t taglist, char *name, sieve_value_t **arg));
int sieve_load_ext __P((sieve_machine_t mach, const char *name));

/* Operations in value lists */
sieve_value_t *sieve_value_get __P((list_t vlist, size_t index));
int sieve_vlist_do __P((sieve_value_t *val, list_action_t *ac, void *data));
int sieve_vlist_compare __P((sieve_value_t *a, sieve_value_t *b,
			     sieve_comparator_t comp, sieve_retrieve_t ac,
			     void *data));

/* Functions to create and destroy sieve machine */
int sieve_machine_init __P((sieve_machine_t *mach, void *data));
void sieve_machine_destroy __P((sieve_machine_t *pmach));
int sieve_machine_add_destructor __P((sieve_machine_t mach,
				      sieve_destructor_t destr,
				      void *ptr));

/* Functions for accessing sieve machine internals */
void *sieve_get_data __P((sieve_machine_t mach));
message_t sieve_get_message __P((sieve_machine_t mach));
size_t sieve_get_message_num __P((sieve_machine_t mach));
int sieve_get_debug_level __P((sieve_machine_t mach));
ticket_t sieve_get_ticket __P((sieve_machine_t mach));
mailer_t sieve_get_mailer __P((sieve_machine_t mach));
char *sieve_get_daemon_email __P((sieve_machine_t mach));

void sieve_set_error __P((sieve_machine_t mach, sieve_printf_t error_printer));
void sieve_set_parse_error __P((sieve_machine_t mach, sieve_parse_error_t p));
void sieve_set_debug __P((sieve_machine_t mach, sieve_printf_t debug));
void sieve_set_debug_level __P((sieve_machine_t mach, mu_debug_t dbg,
				int level));
void sieve_set_logger __P((sieve_machine_t mach, sieve_action_log_t logger));
void sieve_set_ticket __P((sieve_machine_t mach, ticket_t ticket));
void sieve_set_mailer __P((sieve_machine_t mach, mailer_t mailer));
void sieve_set_daemon_email __P((sieve_machine_t mach, const char *email));

/* Logging and diagnostic functions */

void sieve_error __P((sieve_machine_t mach, const char *fmt, ...));
void sieve_debug __P((sieve_machine_t mach, const char *fmt, ...));
void sieve_log_action __P((sieve_machine_t mach, const char *action,
			   const char *fmt, ...));
void sieve_abort __P((sieve_machine_t mach));


int sieve_is_dry_run __P((sieve_machine_t mach));
const char *sieve_type_str __P((sieve_data_type type));

/* Principal entry points */

int sieve_compile __P((sieve_machine_t mach, const char *name));
int sieve_mailbox __P((sieve_machine_t mach, mailbox_t mbox));
int sieve_message __P((sieve_machine_t mach, message_t message));
int sieve_disass __P((sieve_machine_t mach));

/* Command line handling */  
  
extern void sieve_argp_init __P((void));
  
#ifdef __cplusplus
}
#endif

#endif

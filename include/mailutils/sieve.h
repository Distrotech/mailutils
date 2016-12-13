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

#ifndef _MAILUTILS_LIBSIEVE_H
#define _MAILUTILS_LIBSIEVE_H

#include <sys/types.h>
#include <stdarg.h>
#include <mailutils/mailutils.h>
#include <mailutils/cli.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __s_cat3__(a,b,c) a ## b ## c
#define SIEVE_EXPORT(module,name) __s_cat3__(module,_LTX_,name)

typedef struct mu_sieve_machine *mu_sieve_machine_t;

typedef struct mu_sieve_string
{
  unsigned constant:1;       /* String is constant */
  unsigned changed:1;        /* String value has changed */
  char *orig;                /* String original value */
  char *exp;                 /* Actual string value after expansion */
  void *rx;                  /* Pointer to the corresponding regular expr */
} mu_sieve_string_t;
  
typedef int (*mu_sieve_handler_t) (mu_sieve_machine_t mach);
typedef void (*mu_sieve_action_log_t) (mu_sieve_machine_t mach,
				       const char *action,
				       const char *fmt, va_list ap);

typedef int (*mu_sieve_relcmp_t) (int, int);
typedef int (*mu_sieve_relcmpn_t) (size_t, size_t);
typedef int (*mu_sieve_comparator_t) (mu_sieve_machine_t mach,
				      mu_sieve_string_t *, const char *);
typedef int (*mu_sieve_retrieve_t) (void *item, void *data, size_t idx,
				    char **pval);
typedef void (*mu_sieve_destructor_t) (void *data);
typedef int (*mu_sieve_tag_checker_t) (mu_sieve_machine_t mach);

typedef enum
{
  SVT_VOID,
  SVT_NUMBER,
  SVT_STRING,
  SVT_STRING_LIST,
  SVT_TAG,
}
mu_sieve_data_type;

/* Struct mu_sieve_slice represents a contiguous slice of an array of strings
   or variables */
struct mu_sieve_slice
{
  size_t first;            /* Index of the first object */
  size_t count;            /* Number of objects */
};

typedef struct mu_sieve_slice *mu_sieve_slice_t;

union mu_sieve_value_storage
{
  char *string;
  size_t number;
  struct mu_sieve_slice list;
};

typedef struct
{
  mu_sieve_data_type type;
  char *tag;
  union mu_sieve_value_storage v;
} mu_sieve_value_t;

typedef struct
{
  char *name;
  mu_sieve_data_type argtype;
} mu_sieve_tag_def_t;

typedef struct
{
  mu_sieve_tag_def_t *tags;
  mu_sieve_tag_checker_t checker;
} mu_sieve_tag_group_t;

struct mu_sieve_command          /* test or action */
{
  mu_sieve_handler_t handler;
  mu_sieve_data_type *req_args;
  mu_sieve_data_type *opt_args;
  mu_sieve_tag_group_t *tags;
};

#define MU_SIEVE_MATCH_IS        1
#define MU_SIEVE_MATCH_CONTAINS  2
#define MU_SIEVE_MATCH_MATCHES   3
#define MU_SIEVE_MATCH_REGEX     4
#define MU_SIEVE_MATCH_EQ        5
#define MU_SIEVE_MATCH_LAST      6

enum mu_sieve_record
  {
    mu_sieve_record_action,
    mu_sieve_record_test,
    mu_sieve_record_comparator
  };
  
typedef struct
{
  const char *name;
  int required;
  void *handle;
  enum mu_sieve_record type;
  union
  {
    struct mu_sieve_command command;
    mu_sieve_comparator_t comp[MU_SIEVE_MATCH_LAST];
  } v;
} mu_sieve_registry_t;

#define MU_SIEVE_CHARSET "UTF-8"

extern mu_debug_handle_t mu_sieve_debug_handle;
extern mu_list_t mu_sieve_include_path;
extern mu_list_t mu_sieve_library_path;
extern mu_list_t mu_sieve_library_path_prefix;

extern mu_sieve_tag_def_t mu_sieve_match_part_tags[];

/* Memory allocation functions */
typedef void (*mu_sieve_reclaim_t) (void *);
void mu_sieve_register_memory (mu_sieve_machine_t mach, void *ptr,
			       mu_sieve_reclaim_t reclaim);
void *mu_sieve_alloc_memory (mu_sieve_machine_t mach, size_t size,
			     mu_sieve_reclaim_t recfun);
void mu_sieve_free (mu_sieve_machine_t mach, void *ptr);
void *mu_sieve_malloc (mu_sieve_machine_t mach, size_t size);
void *mu_sieve_calloc (mu_sieve_machine_t mach, size_t nmemb, size_t size);
char *mu_sieve_strdup (mu_sieve_machine_t mach, char const *str);
void *mu_sieve_realloc (mu_sieve_machine_t mach, void *ptr, size_t size);

void mu_sieve_reclaim_default (void *p);
void mu_sieve_reclaim_value (void *p);
  
size_t mu_sieve_value_create (mu_sieve_machine_t mach,
			      mu_sieve_data_type type, void *data);

/* Symbol space functions */
mu_sieve_registry_t *mu_sieve_registry_add (mu_sieve_machine_t mach,
					    const char *name);
mu_sieve_registry_t *mu_sieve_registry_lookup (mu_sieve_machine_t mach,
					       const char *name,
					       enum mu_sieve_record type);
int mu_sieve_registry_require (mu_sieve_machine_t mach, const char *name,
			       enum mu_sieve_record type);

void mu_sieve_register_test_ext (mu_sieve_machine_t mach,
				 const char *name, mu_sieve_handler_t handler,
				 mu_sieve_data_type *req_args,
				 mu_sieve_data_type *opt_args,
				 mu_sieve_tag_group_t *tags, int required);
void mu_sieve_register_test (mu_sieve_machine_t mach,
			     const char *name, mu_sieve_handler_t handler,
			     mu_sieve_data_type *arg_types,
			     mu_sieve_tag_group_t *tags, int required);

void mu_sieve_register_action_ext (mu_sieve_machine_t mach,
				   const char *name, mu_sieve_handler_t handler,
				   mu_sieve_data_type *req_args,
				   mu_sieve_data_type *opt_args,
				   mu_sieve_tag_group_t *tags, int required);
void mu_sieve_register_action (mu_sieve_machine_t mach,
			       const char *name, mu_sieve_handler_t handler,
			       mu_sieve_data_type *arg_types,
			       mu_sieve_tag_group_t *tags, int required);
  
void mu_sieve_register_comparator (mu_sieve_machine_t mach, const char *name,
				   int required, mu_sieve_comparator_t is,
				   mu_sieve_comparator_t contains,
				   mu_sieve_comparator_t matches,
				   mu_sieve_comparator_t regex,
				   mu_sieve_comparator_t eq);

int mu_sieve_require_relational (mu_sieve_machine_t mach, const char *name);
  
int mu_sieve_require_variables (mu_sieve_machine_t mach);
int mu_sieve_has_variables (mu_sieve_machine_t mach);

int mu_sieve_require_environment (mu_sieve_machine_t mach);

void *mu_sieve_load_ext (mu_sieve_machine_t mach, const char *name);
void mu_sieve_unload_ext (void *handle);
  
int mu_sieve_match_part_checker (mu_sieve_machine_t mach);

mu_sieve_comparator_t mu_sieve_comparator_lookup (mu_sieve_machine_t mach,
						  const char *name,
						  int matchtype);

mu_sieve_comparator_t mu_sieve_get_comparator (mu_sieve_machine_t mach);
int mu_sieve_str_to_relcmp (const char *str, mu_sieve_relcmp_t *test,
			    mu_sieve_relcmpn_t *stest);
mu_sieve_relcmp_t mu_sieve_get_relcmp (mu_sieve_machine_t mach);

void mu_sieve_require (mu_sieve_machine_t mach, mu_sieve_slice_t list);

void mu_sieve_value_get (mu_sieve_machine_t mach, mu_sieve_value_t *val,
			 mu_sieve_data_type type, void *ret);
/* Tagged argument accessors */   
int mu_sieve_get_tag (mu_sieve_machine_t mach, char *name,
		      mu_sieve_data_type type, void *ret);
mu_sieve_value_t *mu_sieve_get_tag_untyped (mu_sieve_machine_t mach,
					    char const *name);
mu_sieve_value_t *mu_sieve_get_tag_n (mu_sieve_machine_t mach, size_t n);
  
/* Positional argument accessors */
mu_sieve_value_t *mu_sieve_get_arg_optional (mu_sieve_machine_t mach,
					     size_t index);
mu_sieve_value_t *mu_sieve_get_arg_untyped (mu_sieve_machine_t mach,
					    size_t index);
void mu_sieve_get_arg (mu_sieve_machine_t mach, size_t index,
		       mu_sieve_data_type type, void *ret);
/* String and string list accessors */
char *mu_sieve_string (mu_sieve_machine_t mach,
		       mu_sieve_slice_t slice,
		       size_t i);
mu_sieve_string_t *mu_sieve_string_raw (mu_sieve_machine_t mach,
					mu_sieve_slice_t slice,
					size_t i);
char *mu_sieve_string_get (mu_sieve_machine_t mach, mu_sieve_string_t *string);

/* Operations on value lists */ 
int mu_sieve_vlist_do (mu_sieve_machine_t mach,
		       mu_sieve_value_t *val, mu_list_action_t ac,
		       void *data);
int mu_sieve_vlist_compare (mu_sieve_machine_t mach,
			    mu_sieve_value_t *a, mu_sieve_value_t *b,
			    mu_sieve_retrieve_t ac, mu_list_folder_t fold,
			    void *data);

/* Functions to create and destroy sieve machine */
int mu_sieve_machine_create (mu_sieve_machine_t *mach);
int mu_sieve_machine_dup (mu_sieve_machine_t const in,
			  mu_sieve_machine_t *out);
int mu_sieve_machine_clone (mu_sieve_machine_t const in,
			      mu_sieve_machine_t *out);
void mu_sieve_machine_destroy (mu_sieve_machine_t *pmach);
void mu_sieve_machine_add_destructor (mu_sieve_machine_t mach,
				      mu_sieve_destructor_t destr, void *ptr);

/* Functions for accessing sieve machine internals */
void mu_sieve_get_diag_stream (mu_sieve_machine_t mach, mu_stream_t *pstr);
void mu_sieve_set_diag_stream (mu_sieve_machine_t mach, mu_stream_t str);

void mu_sieve_set_dbg_stream (mu_sieve_machine_t mach, mu_stream_t str);
void mu_sieve_get_dbg_stream (mu_sieve_machine_t mach, mu_stream_t *pstr);

void *mu_sieve_get_data (mu_sieve_machine_t mach);
void mu_sieve_set_data (mu_sieve_machine_t mach, void *);
mu_message_t mu_sieve_get_message (mu_sieve_machine_t mach);
size_t mu_sieve_get_message_num (mu_sieve_machine_t mach);

mu_mailbox_t mu_sieve_get_mailbox (mu_sieve_machine_t mach);

int mu_sieve_is_dry_run (mu_sieve_machine_t mach);
int mu_sieve_set_dry_run (mu_sieve_machine_t mach, int val);

void mu_sieve_get_argc (mu_sieve_machine_t mach, size_t *args, size_t *tags);

mu_mailer_t mu_sieve_get_mailer (mu_sieve_machine_t mach);
int mu_sieve_get_locus (mu_sieve_machine_t mach, struct mu_locus *);
char *mu_sieve_get_daemon_email (mu_sieve_machine_t mach);
const char *mu_sieve_get_identifier (mu_sieve_machine_t mach);

void mu_sieve_set_logger (mu_sieve_machine_t mach,
			  mu_sieve_action_log_t logger);
void mu_sieve_set_mailer (mu_sieve_machine_t mach, mu_mailer_t mailer);
void mu_sieve_set_daemon_email (mu_sieve_machine_t mach, const char *email);

int mu_sieve_get_message_sender (mu_message_t msg, char **ptext);

int mu_sieve_get_environ (mu_sieve_machine_t mach, char const *name,
			  char **retval);
int mu_sieve_set_environ (mu_sieve_machine_t mach, char const *name,
			  char const *value);

/* Stream state saving & restoring */
void mu_sieve_stream_save (mu_sieve_machine_t mach);
void mu_sieve_stream_restore (mu_sieve_machine_t mach);
  
/* Logging and diagnostic functions */

void mu_sieve_debug_init (void);  
void mu_sieve_error (mu_sieve_machine_t mach, const char *fmt, ...) 
                     MU_PRINTFLIKE(2,3);
void mu_sieve_log_action (mu_sieve_machine_t mach, const char *action,
			  const char *fmt, ...)
			  MU_PRINTFLIKE(3,4);
void mu_sieve_abort (mu_sieve_machine_t mach);

const char *mu_sieve_type_str (mu_sieve_data_type type);

/* Principal entry points */

int mu_sieve_compile (mu_sieve_machine_t mach, const char *name);
int mu_sieve_compile_buffer (mu_sieve_machine_t mach,
			     const char *buf, int bufsize,
			     const char *fname, int line);
int mu_sieve_mailbox (mu_sieve_machine_t mach, mu_mailbox_t mbox);
int mu_sieve_message (mu_sieve_machine_t mach, mu_message_t message);
int mu_sieve_disass (mu_sieve_machine_t mach);

/* Configuration functions */

#define MU_SIEVE_CLEAR_INCLUDE_PATH 0x1
#define MU_SIEVE_CLEAR_LIBRARY_PATH 0x2

extern struct mu_cli_capa mu_cli_capa_sieve;
  
#ifdef __cplusplus
}
#endif

#endif

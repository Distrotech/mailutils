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

#include <sys/types.h>
#include <stdarg.h>
#include <mailutils/mailutils.h>

typedef struct sieve_machine sieve_machine_t;

typedef int (*sieve_handler_t) __P((sieve_machine_t *mach,
				    list_t args, list_t tags));
typedef int (*sieve_printf_t) __P((void *data, const char *fmt, va_list ap));
typedef int (*sieve_vprintf_t) __P((void *data, const char *fmt, va_list ap));

typedef enum {
  SVT_VOID,
  SVT_NUMBER,
  SVT_STRING,
  SVT_STRING_LIST,
  SVT_TAG,
  SVT_IDENT,
  SVT_VALUE_LIST
} sieve_data_type;

typedef struct sieve_runtime_tag sieve_runtime_tag_t;

typedef struct {
  sieve_data_type type;
  union {
    char *string;
    long number;
    list_t list;
    sieve_runtime_tag_t *tag;
  } v;
} sieve_value_t;

typedef struct {
  char *name;
  int num;
  sieve_data_type argtype;
} sieve_tag_def_t;

struct sieve_runtime_tag {
  int tag;
  sieve_value_t *arg;
};

typedef struct {
  char *name;
  int required;
  sieve_handler_t handler;
  int num_req_args;
  sieve_data_type *req_args;
  int num_tags;
  sieve_tag_def_t *tags;
} sieve_register_t;


void *sieve_alloc __P((size_t size));
void *sieve_palloc __P((list_t *pool, size_t size));
void *sieve_prealloc __P((list_t *pool, void *ptr, size_t size));
void sieve_pfree __P((list_t *pool, void *ptr));
char *sieve_pstrdup __P((list_t *pool, const char *str));

int sieve_compile __P((sieve_machine_t *mach, const char *name, void *data,
		       sieve_printf_t errfn));
void sieve_set_debug __P((sieve_machine_t *mach, sieve_printf_t debug,
			  int level));

sieve_value_t * sieve_value_create __P((sieve_data_type type, void *data));

sieve_register_t *sieve_test_lookup __P((const char *name));
sieve_register_t *sieve_action_lookup __P((const char *name));
int sieve_register_test __P((const char *name, sieve_handler_t handler,
			     sieve_data_type *arg_types,
			     sieve_tag_def_t *tags, int required));
int sieve_register_action __P((const char *name, sieve_handler_t handler,
			       sieve_data_type *arg_types,
			       sieve_tag_def_t *tags, int required));

void sieve_slist_destroy __P((list_t *plist));
void sieve_require __P((list_t slist));

void sieve_abort __P((sieve_machine_t *mach));

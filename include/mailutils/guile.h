/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005, 2006 
   Free Software Foundation, Inc.

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

#ifndef _MU_SCM_H
#define _MU_SCM_H

#include <libguile.h>

#if GUILE_VERSION == 14

# define SCM_STRING_CHARS SCM_CHARS
# define scm_list_1 SCM_LIST1
# define scm_list_2 SCM_LIST2
# define scm_list_3 SCM_LIST3
# define scm_list_4 SCM_LIST4
# define scm_list_5 SCM_LIST5
# define scm_list_n SCM_LISTN
# define scm_c_define scm_sysintern
# define scm_primitive_eval_x scm_eval_x
# define scm_i_big2dbl scm_big2dbl
# define scm_c_eval_string scm_eval_0str
# define MU_SCM_SYMBOL_VALUE(p) scm_symbol_value0(p)

extern SCM scm_long2num (long val);

#else
# define MU_SCM_SYMBOL_VALUE(p) SCM_VARIABLE_REF(scm_c_lookup(p))
#endif

#if GUILE_VERSION < 18

# define scm_i_string_chars SCM_ROCHARS 
# define scm_i_string_length SCM_ROLENGTH 
# define scm_is_string(s) (SCM_NIMP(s) && SCM_STRINGP(s))
# define scm_from_locale_stringn(s,l) scm_makfromstr(s,l,0) 
# define scm_from_locale_string(s) scm_makfrom0str(s)

# define scm_from_locale_symbol(s) scm_str2symbol(s)

# define scm_from_int SCM_MAKINUM 
# define scm_from_size_t mu_scm_makenum

# define scm_gc_malloc scm_must_malloc 

/* FIXME */
# define scm_is_integer SCM_INUMP 
# define scm_to_int32 SCM_INUM 
# define scm_to_int SCM_INUM 

# define DECL_SCM_T_ARRAY_HANDLE(name) 
# define SCM_T_ARRAY_HANDLE_PTR(name) 0
# define scm_vector_writable_elements(res,handle,a,b) SCM_VELTS(res)
# define scm_array_handle_release(h)

#else

# define DECL_SCM_T_ARRAY_HANDLE(name) scm_t_array_handle name
# define SCM_T_ARRAY_HANDLE_PTR(name) &(name)

#endif

typedef struct
{
  int debug_guile;
  mu_mailbox_t mbox;
  char *user_name;
  int (*init) (void *data);
  SCM (*catch_body) (void *data, mu_mailbox_t mbox);
  SCM (*catch_handler) (void *data, SCM tag, SCM throw_args);
  int (*next) (void *data, mu_mailbox_t mbox);
  int (*exit) (void *data, mu_mailbox_t mbox);
  void *data;
} mu_guimb_param_t;

#ifdef __cplusplus
extern "C" {
#endif

void mu_scm_error (const char *func_name, int status,
		   const char *fmt, SCM args);
extern SCM mu_scm_makenum (unsigned long val);
extern void mu_set_variable (const char *name, SCM value);
extern void mu_scm_init (void);

extern void mu_scm_mailbox_init (void);
extern SCM mu_scm_mailbox_create (mu_mailbox_t mbox);
extern int mu_scm_is_mailbox (SCM scm);

extern void mu_scm_message_init (void);
extern SCM mu_scm_message_create (SCM owner, mu_message_t msg);
extern int mu_scm_is_message (SCM scm);
extern const mu_message_t mu_scm_message_get (SCM MESG);

extern int mu_scm_is_body (SCM scm);
extern void mu_scm_body_init (void);
extern SCM mu_scm_body_create (SCM mesg, mu_body_t body);

extern void mu_scm_address_init (void);
extern void mu_scm_logger_init (void);

extern void mu_scm_port_init (void);
extern SCM mu_port_make_from_stream (SCM msg, mu_stream_t stream, long mode);

extern void mu_scm_mime_init (void);
extern void mu_scm_message_add_owner (SCM MESG, SCM owner);

extern void mu_process_mailbox (int argc, char *argv[], mu_guimb_param_t *param);

extern void mu_scm_mutil_init (void);

#ifdef __cplusplus
}
#endif

#endif 

/* GNU Mailutils -- a suite of utilities for electronic mail -*- c -*-
   Copyright (C) 1999-2000, 2005, 2007-2008, 2010-2012 Free Software
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

#ifndef _MAILUTILS_DEBUG_H
#define _MAILUTILS_DEBUG_H

#include <stdarg.h>

#include <mailutils/types.h>
#include <mailutils/error.h>
#include <mailutils/sys/debcat.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int mu_debug_line_info;

#define MU_DEBUG_ERROR  0
#define MU_DEBUG_TRACE0 1
#define MU_DEBUG_TRACE MU_DEBUG_TRACE0
#define MU_DEBUG_TRACE1 2
#define MU_DEBUG_TRACE2 3
#define MU_DEBUG_TRACE3 4
#define MU_DEBUG_TRACE4 5
#define MU_DEBUG_TRACE5 6
#define MU_DEBUG_TRACE6 7
#define MU_DEBUG_TRACE7 8
#define MU_DEBUG_TRACE8 9
#define MU_DEBUG_TRACE9 10
  
#define MU_DEBUG_PROT   11

#define MU_DEBUG_LEVEL_MASK(lev) (1 << (lev))
#define MU_DEBUG_LEVEL_UPTO(lev) ((1 << ((lev)+1)) - 1)
#define MU_DEBUG_LEVEL_RANGE(a, b)					\
  ((a) == 0 ? MU_DEBUG_LEVEL_UPTO (b) :					\
              MU_DEBUG_LEVEL_UPTO (b) & ~MU_DEBUG_LEVEL_UPTO ((a) - 1))


struct sockaddr;
char *mu_sockaddr_to_astr (const struct sockaddr *sa, int salen);


size_t mu_debug_register_category (char *name);
size_t mu_debug_next_handle (void);
int mu_debug_level_p (mu_debug_handle_t catn, mu_debug_level_t level);
void mu_debug_enable_category (const char *catname, size_t catlen,
			       mu_debug_level_t level);
void mu_debug_disable_category (const char *catname, size_t catlen);
int mu_debug_category_level (const char *catname, size_t catlen,
			     mu_debug_level_t *plev);
void mu_debug_parse_spec (const char *spec);  
int mu_debug_format_spec(mu_stream_t str, const char *names, int showunset);

int mu_debug_get_category_level (mu_debug_handle_t catn,
				 mu_debug_level_t *plev);
int mu_debug_set_category_level (mu_debug_handle_t catn,
				 mu_debug_level_t level);
  
void mu_debug_clear_all (void);
  
void mu_debug_log (const char *fmt, ...) MU_PRINTFLIKE(1,2);
void mu_debug_log_begin (const char *fmt, ...) MU_PRINTFLIKE(1,2);
void mu_debug_log_cont (const char *fmt, ...) MU_PRINTFLIKE(1,2);
void mu_debug_log_end (const char *fmt, ...) MU_PRINTFLIKE(1,2);
void mu_debug_log_nl (void);

  int mu_debug_get_iterator (mu_iterator_t *piterator, int skipunset);

  
#define MU_ASSERT(expr)						\
 do								\
  {								\
    int rc = expr;						\
    if (rc)							\
      {								\
	mu_error ("%s:%d: " #expr " failed: %s",                \
                  __FILE__, __LINE__, mu_strerror (rc));	\
	abort ();						\
      }								\
  }                                                             \
 while (0)

  

#define mu_debug(cat,lev,s)					         \
  do                                                                     \
    if (mu_debug_level_p (cat, lev))			                 \
      {								         \
	if (mu_debug_line_info)					         \
	  {								 \
	    mu_debug_log_begin ("\033X<%d>%s:%d: ",	                 \
				MU_LOGMODE_LOCUS, __FILE__, __LINE__);	 \
	    mu_debug_log_end s;						 \
	  }								 \
	else								 \
	  mu_debug_log s;						 \
      }                                                                  \
  while (0)

  /* Debugging hooks. */
  /* Dump a stack trace and terminate the program. */
void mu_gdb_bt (void);
  /* Sleep till attached to by gdb. */
void mu_wd (unsigned to);
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_DEBUG_H */

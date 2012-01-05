/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2010-2012 Free Software
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

#ifndef _ARGCV_H
#define _ARGCV_H 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void mu_argcv_free (size_t argc, char **argv);
void mu_argv_free (char **argv);

enum mu_argcv_escape
  {
    mu_argcv_escape_no,
    mu_argcv_escape_c
    /*    mu_argcv_escape_sh */
  };

int mu_argcv_join (int argc, char **argv, char *delim,
		   enum mu_argcv_escape esc,
		   char **pstring);
void mu_argcv_remove (int *pargc, char ***pargv,
		      int (*sel) (const char *, void *), void *);
int mu_argcv_string (int argc, char **argv, char **string);

  
/* Deprecated interfaces */  

#define MU_ARGCV_RETURN_DELIMS 0x01

#ifndef MU_ARCGV_DEPRECATED
# define MU_ARCGV_DEPRECATED MU_DEPRECATED
#endif
  
int mu_argcv_get    (const char *command, const char *delim,
		     const char *cmnt,
		     int *argc, char ***argv) MU_ARCGV_DEPRECATED;
int mu_argcv_get_n (const char *command, int len,
		    const char *delim, const char *cmnt,
		    int *argc, char ***argv) MU_ARCGV_DEPRECATED;
int mu_argcv_get_np (const char *command, int len,
		     const char *delim, const char *cmnt,
		     int flags,
		     int *pargc, char ***pargv, char **endp)
               MU_ARCGV_DEPRECATED;
  
int mu_argcv_unquote_char (int c) MU_ARCGV_DEPRECATED;
int mu_argcv_quote_char   (int c) MU_ARCGV_DEPRECATED;
size_t mu_argcv_quoted_length (const char *str, int *quote)
                           MU_ARCGV_DEPRECATED;
void mu_argcv_unquote_copy (char *dst, const char *src, size_t n)
                           MU_ARCGV_DEPRECATED;
void mu_argcv_quote_copy (char *dst, const char *src)
                           MU_ARCGV_DEPRECATED;

#ifdef __cplusplus
}
#endif

#endif /* _ARGCV_H */

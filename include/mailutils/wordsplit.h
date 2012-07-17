/* wordsplit - a word splitter
   Copyright (C) 2009, 2010 Sergey Poznyakoff

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program. If not, see <http://www.gnu.org/licenses/>. */

#ifndef __MAILUTILS_WORDSPLIT_H
#define __MAILUTILS_WORDSPLIT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mu_wordsplit
{
  size_t ws_wordc;
  char **ws_wordv;
  size_t ws_offs;
  size_t ws_wordn;
  int ws_flags;
  const char *ws_delim;
  const char *ws_comment;
  const char *ws_escape;
  void (*ws_alloc_die) (struct mu_wordsplit *wsp);
  void (*ws_error) (const char *, ...)
		    __attribute__ ((__format__ (__printf__, 1, 2)));
  void (*ws_debug) (const char *, ...)
		    __attribute__ ((__format__ (__printf__, 1, 2)));
	
  const char **ws_env;
  const char *(*ws_getvar) (const char *, size_t, void *); 
  void *ws_closure;
  
  const char *ws_input;
  size_t ws_len;
  size_t ws_endp;
  int ws_errno;
  struct mu_wordsplit_node *ws_head, *ws_tail;
};

/* Wordsplit flags.  Only 2 bits of a 32-bit word remain unused.
   It is getting crowded... */
/* Append the words found to the array resulting from a previous
   call. */
#define MU_WRDSF_APPEND            0x00000001
/* Insert we_offs initial NULLs in the array ws_wordv.
   (These are not counted in the returned ws_wordc.) */
#define MU_WRDSF_DOOFFS            0x00000002
/* Don't do command substitution. Reserved for future use. */
#define MU_WRDSF_NOCMD             0x00000004
/* The parameter p resulted from a previous call to
   mu_wordsplit(), and mu_wordsplit_free() was not called. Reuse the
   allocated storage. */
#define MU_WRDSF_REUSE             0x00000008
/* Print errors */
#define MU_WRDSF_SHOWERR           0x00000010
/* Consider it an error if an undefined shell variable
   is expanded. */
#define MU_WRDSF_UNDEF             0x00000020

/* Don't do variable expansion. */
#define MU_WRDSF_NOVAR             0x00000040
/* Abort on ENOMEM error */
#define MU_WRDSF_ENOMEMABRT        0x00000080
/* Trim off any leading and trailind whitespace */
#define MU_WRDSF_WS                0x00000100
/* Handle single quotes */
#define MU_WRDSF_SQUOTE            0x00000200
/* Handle double quotes */
#define MU_WRDSF_DQUOTE            0x00000400
/* Handle quotes and escape directives */
#define MU_WRDSF_QUOTE             (MU_WRDSF_SQUOTE|MU_WRDSF_DQUOTE)
/* Replace each input sequence of repeated delimiters with a single
   delimiter */
#define MU_WRDSF_SQUEEZE_DELIMS    0x00000800
/* Return delimiters */
#define MU_WRDSF_RETURN_DELIMS     0x00001000
/* Treat sed expressions as words */
#define MU_WRDSF_SED_EXPR          0x00002000
/* ws_delim field is initialized */
#define MU_WRDSF_DELIM             0x00004000
/* ws_comment field is initialized */
#define MU_WRDSF_COMMENT           0x00008000
/* ws_alloc_die field is initialized */
#define MU_WRDSF_ALLOC_DIE         0x00010000
/* ws_error field is initialized */
#define MU_WRDSF_ERROR             0x00020000
/* ws_debug field is initialized */
#define MU_WRDSF_DEBUG             0x00040000
/* ws_env field is initialized */
#define MU_WRDSF_ENV               0x00080000
/* ws_getvar field is initialized */
#define MU_WRDSF_GETVAR            0x00100000
/* enable debugging */
#define MU_WRDSF_SHOWDBG           0x00200000
/* Don't split input into words.  Useful for side effects. */
#define MU_WRDSF_NOSPLIT           0x00400000
/* Keep undefined variables in place, instead of expanding them to
   empty string */
#define MU_WRDSF_KEEPUNDEF         0x00800000
/* Warn about undefined variables */
#define MU_WRDSF_WARNUNDEF         0x01000000
/* Handle C escapes */
#define MU_WRDSF_CESCAPES          0x02000000

/* ws_closure is set */
#define MU_WRDSF_CLOSURE           0x04000000
/* ws_env is a Key/Value environment, i.e. the value of a variable is
   stored in the element that follows its name. */
#define MU_WRDSF_ENV_KV            0x08000000

/* ws_escape is set */
#define MU_WRDSF_ESCAPE            0x10000000

/* Incremental mode */
#define MU_WRDSF_INCREMENTAL       0x20000000

#define MU_WRDSF_DEFFLAGS	       \
  (MU_WRDSF_NOVAR | MU_WRDSF_NOCMD | \
   MU_WRDSF_QUOTE | MU_WRDSF_SQUEEZE_DELIMS | MU_WRDSF_CESCAPES)

#define MU_WRDSE_EOF        0
#define MU_WRDSE_QUOTE      1
#define MU_WRDSE_NOSPACE    2
#define MU_WRDSE_NOSUPP     3
#define MU_WRDSE_USAGE      4
#define MU_WRDSE_CBRACE     5
#define MU_WRDSE_UNDEF      6
#define MU_WRDSE_NOINPUT    7

int mu_wordsplit (const char *s, struct mu_wordsplit *p, int flags);
int mu_wordsplit_len (const char *s, size_t len,
		      struct mu_wordsplit *p, int flags);
void mu_wordsplit_free (struct mu_wordsplit *p);
void mu_wordsplit_free_words (struct mu_wordsplit *ws);

int mu_wordsplit_c_unquote_char (int c);
int mu_wordsplit_c_quote_char (int c);
size_t mu_wordsplit_c_quoted_length (const char *str, int quote_hex,
				     int *quote);
void mu_wordsplit_general_unquote_copy (char *dst, const char *src, size_t n,
					const char *escapable);
void mu_wordsplit_sh_unquote_copy (char *dst, const char *src, size_t n);
void mu_wordsplit_c_unquote_copy (char *dst, const char *src, size_t n);
void mu_wordsplit_c_quote_copy (char *dst, const char *src, int quote_hex);

void mu_wordsplit_perror (struct mu_wordsplit *ws);
const char *mu_wordsplit_strerror (struct mu_wordsplit *ws);

#ifdef __cplusplus
}
#endif

#endif

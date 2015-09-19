/* wordsplit - a word splitter
   Copyright (C) 2009-2015 Sergey Poznyakoff

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

typedef struct mu_wordsplit mu_wordsplit_t;

/* Structure used to direct the splitting.  Members marked with [Input]
   can be defined before calling mu_wordsplit(), those marked with [Output]
   provide return values when the function returns.  If neither mark is
   used, the member is internal and must not be used by the caller.

   In the comments below, the
   identifiers in parentheses indicate bits that must be set (or unset, if
   starting with !) in the ws_flags to initialize or use the given member.
   If not redefined explicitly, most of them are set to some reasonable
   default value upon entry to mu_wordsplit(). */
struct mu_wordsplit            
{
  size_t ws_wordc;          /* [Output] Number of words in ws_wordv. */
  char **ws_wordv;          /* [Output] Array of parsed out words. */
  size_t ws_offs;           /* [Input] (MU_WRDSF_DOOFFS) Number of initial
			       elements in ws_wordv to fill with NULLs. */
  size_t ws_wordn;          /* Number of elements ws_wordv can accomodate. */ 
  int ws_flags;             /* [Input] Flags passed to mu_wordsplit. */
  int ws_options;           /* [Input] (MU_WRDSF_PATHEXPAND)
			       Additional options. */
  const char *ws_delim;     /* [Input] (MU_WRDSF_DELIM) Word delimiters. */
  const char *ws_comment;   /* [Input] (MU_WRDSF_COMMENT) Comment characters. */
  const char *ws_escape[2]; /* [Input] (MU_WRDSF_ESCAPE) Characters to be escaped
			       with backslash. */
  void (*ws_alloc_die) (mu_wordsplit_t *wsp);
                            /* [Input] (MU_WRDSF_ALLOC_DIE) Function called when
			       out of memory.  Must not return. */
  void (*ws_error) (const char *, ...)
                   __attribute__ ((__format__ (__printf__, 1, 2)));
                            /* [Input] (MU_WRDSF_ERROR) Function used for error
			       reporting */
  void (*ws_debug) (const char *, ...)
                   __attribute__ ((__format__ (__printf__, 1, 2)));
                            /* [Input] (MU_WRDSF_DEBUG) Function used for debug
			       output. */
  const char **ws_env;      /* [Input] (MU_WRDSF_ENV, !MU_WRDSF_NOVAR) Array of
			       environment variables. */

  char **ws_envbuf;
  size_t ws_envidx;
  size_t ws_envsiz;
  
  int (*ws_getvar) (char **ret, const char *var, size_t len, void *clos);
                            /* [Input] (MU_WRDSF_GETVAR, !MU_WRDSF_NOVAR) Looks up
			       the name VAR (LEN bytes long) in the table of
			       variables and, if found returns the value of
			       that variable in memory location pointed to
			       by RET .  Returns WRDSE_OK (0) on success,
			       and an error code (see WRDSE_* defines below)
			       on error.  User-specific errors can be returned
			       by storing the error diagnostic string in RET
			       and returning WRDSE_USERERR.
                               Whatever is stored in RET, it must be allocated
			       using malloc(3). */
  void *ws_closure;         /* [Input] (MU_WRDSF_CLOSURE) Passed as the CLOS
			       argument to ws_getvar and ws_command. */
  int (*ws_command) (char **ret, const char *cmd, size_t len, char **argv,
                     void *clos);
	                    /* [Input] (!MU_WRDSF_NOCMD) Returns in the memory
			       location pointed to by RET the expansion of
			       the command CMD (LEN bytes nong).  If MU_WRDSF_ARGV
			       flag is set, ARGV contains CMD split out to
			       words.  Otherwise ARGV is NULL.

			       See ws_getvar for a discussion of possible
			       return values. */
	
  const char *ws_input;     /* Input string (the S argument to mu_wordsplit. */  
  size_t ws_len;            /* Length of ws_input. */
  size_t ws_endp;           /* Points past the last processed byte in
			       ws_input. */
  int ws_errno;             /* [Output] Error code, if an error occurred. */
  char *ws_usererr;         /* Points to textual description of
			       the error, if ws_errno is WRDSE_USERERR.  Must
			       be allocated with malloc(3). */
  struct mu_wordsplit_node *ws_head, *ws_tail;
                            /* Doubly-linked list of parsed out nodes. */
  int ws_lvl;               /* Invocation nesting level. */
};

/* Initial size for ws_env, if allocated automatically */
#define MU_WORDSPLIT_ENV_INIT 16

/* Mu_Wordsplit flags. */
/* Append the words found to the array resulting from a previous
   call. */
#define MU_WRDSF_APPEND            0x00000001
/* Insert ws_offs initial NULLs in the array ws_wordv.
   (These are not counted in the returned ws_wordc.) */
#define MU_WRDSF_DOOFFS            0x00000002
/* Don't do command substitution. */
#define MU_WRDSF_NOCMD             0x00000004
/* The parameter p resulted from a previous call to
   mu_wordsplit(), and mu_wordsplit_free() was not called. Reuse the
   allocated storage. */
#define MU_WRDSF_REUSE             0x00000008
/* Print errors */
#define MU_WRDSF_SHOWERR           0x00000010
/* Consider it an error if an undefined variable is expanded. */
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
/* Handle single and double quotes */
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
   empty strings. */
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
/* Perform pathname and tilde expansion */
#define MU_WRDSF_PATHEXPAND        0x40000000
/* ws_options is initialized */
#define MU_WRDSF_OPTIONS           0x80000000

#define MU_WRDSF_DEFFLAGS	       \
  (MU_WRDSF_NOVAR | MU_WRDSF_NOCMD | \
   MU_WRDSF_QUOTE | MU_WRDSF_SQUEEZE_DELIMS | MU_WRDSF_CESCAPES)

/* Remove the word that produces empty string after path expansion */
#define MU_WRDSO_NULLGLOB        0x00000001
/* Print error message if path expansion produces empty string */
#define MU_WRDSO_FAILGLOB        0x00000002
/* Allow a leading period to be matched by metacharacters. */
#define MU_WRDSO_DOTGLOB         0x00000004
/* ws_command needs argv parameter */
#define MU_WRDSO_ARGV            0x00000008
/* Keep backslash in unrecognized escape sequences in words */
#define MU_WRDSO_BSKEEP_WORD     0x00000010
/* Handle octal escapes in words */
#define MU_WRDSO_OESC_WORD       0x00000020
/* Handle hex escapes in words */
#define MU_WRDSO_XESC_WORD       0x00000040

/* Keep backslash in unrecognized escape sequences in quoted strings */
#define MU_WRDSO_BSKEEP_QUOTE    0x00000100
/* Handle octal escapes in quoted strings */
#define MU_WRDSO_OESC_QUOTE      0x00000200
/* Handle hex escapes in quoted strings */
#define MU_WRDSO_XESC_QUOTE      0x00000400

#define MU_WRDSO_BSKEEP          MU_WRDSO_BSKEEP_WORD     
#define MU_WRDSO_OESC            MU_WRDSO_OESC_WORD       
#define MU_WRDSO_XESC            MU_WRDSO_XESC_WORD       

/* Indices into ws_escape */
#define MU_WRDSX_WORD  0
#define MU_WRDSX_QUOTE 1    
  
/* Set escape option F in WS for words (Q==0) or quoted strings (Q==1) */
#define MU_WRDSO_ESC_SET(ws,q,f) ((ws)->ws_options |= ((f) << 4*(q)))
/* Test WS for escape option F for words (Q==0) or quoted strings (Q==1) */
#define MU_WRDSO_ESC_TEST(ws,q,f) ((ws)->ws_options & ((f) << 4*(q)))

#define MU_WRDSE_OK         0
#define MU_WRDSE_EOF        MU_WRDSE_OK
#define MU_WRDSE_QUOTE      1
#define MU_WRDSE_NOSPACE    2
#define MU_WRDSE_USAGE      3
#define MU_WRDSE_CBRACE     4
#define MU_WRDSE_UNDEF      5
#define MU_WRDSE_NOINPUT    6
#define MU_WRDSE_PAREN      7
#define MU_WRDSE_GLOBERR    8
#define MU_WRDSE_USERERR    9

int mu_wordsplit (const char *s, mu_wordsplit_t *ws, int flags);
int mu_wordsplit_len (const char *s, size_t len, mu_wordsplit_t *ws, int flags);
void mu_wordsplit_free (mu_wordsplit_t *ws);
void mu_wordsplit_free_words (mu_wordsplit_t *ws);
void mu_wordsplit_free_envbuf (mu_wordsplit_t *ws);

int mu_wordsplit_get_words (mu_wordsplit_t *ws, size_t *wordc, char ***wordv);

int mu_wordsplit_c_unquote_char (int c);
int mu_wordsplit_c_quote_char (int c);
size_t mu_wordsplit_c_quoted_length (const char *str, int quote_hex, int *quote);
void mu_wordsplit_c_quote_copy (char *dst, const char *src, int quote_hex);

void mu_wordsplit_perror (mu_wordsplit_t *ws);
const char *mu_wordsplit_strerror (mu_wordsplit_t *ws);

void mu_wordsplit_clearerr (mu_wordsplit_t *ws);

#ifdef __cplusplus
}
#endif
  
#endif

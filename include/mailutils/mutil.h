/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _MAILUTILS_MUTIL_H
#define _MAILUTILS_MUTIL_H

/*
   Collection of useful utility routines that are worth sharing,
   but don't have a natural home somewhere else.
*/

#include <time.h>

#include <mailutils/list.h>
#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned long mu_hex2ul __P ((char hex));
extern size_t mu_hexstr2ul __P ((unsigned long* ul, const char* hex, size_t len));

struct mu_timezone
{
  int utc_offset;
    /* Seconds east of UTC. */

  const char *tz_name;
    /* Nickname for this timezone, if known. It is always considered
     * to be a pointer to static string, so will never be freed. */
};

typedef struct mu_timezone mu_timezone;

int mu_parse_date __P ((const char *p, time_t *rettime, const time_t *now));
extern int mu_parse_imap_date_time __P ((const char **p, struct tm * tm,
					 mu_timezone * tz));
extern int mu_parse_ctime_date_time __P ((const char **p, struct tm * tm,
					 mu_timezone * tz));

extern time_t mu_utc_offset __P ((void));
extern time_t mu_tm2time __P ((struct tm * timeptr, mu_timezone * tz));
extern char * mu_get_homedir __P ((void));
extern char * mu_tilde_expansion __P ((const char *ref, const char *delim, const char *homedir));

extern size_t mu_cpystr __P ((char *dst, const char *src, size_t size));

/* Get the host name, doing a gethostbyname() if possible.
 *
 * It is the caller's responsibility to free host.
 */
extern int mu_get_host_name __P((char **host));

/* Set the default user email address.
 *
 * Subsequent calls to mu_get_user_email() with a NULL name will return this
 * email address.  email is parsed to determine that it consists of a a valid
 * rfc822 address, with one valid addr-spec, i.e, the address must be
 * qualified.
 */
extern int mu_set_user_email __P ((const char *email));

/* Set the default user email address domain.
 *
 * Subsequent calls to mu_get_user_email() with a non-null name will return
 * email addresses in this domain (name@domain). It should be fully
 * qualified, but this isn't (and can't) be enforced.
 */
extern int mu_set_user_email_domain __P ((const char *domain));

/* Return the currently set user email domain, or NULL if not set. */
extern int mu_get_user_email_domain __P ((const char** domain));

/*
 * Get the default email address for user name. A NULL name is taken
 * to mean the current user.
 *
 * The result must be freed by the caller after use.
 */
extern char *mu_get_user_email __P ((const char *name));

extern char *mu_normalize_path __P ((char *path, const char *delim));
extern char *mu_normalize_maildir __P ((const char *dir));
extern int mu_tempfile __P ((const char *tmpdir, char **namep));
extern char *mu_tempname __P ((const char *tmpdir));

extern char * mu_get_full_path __P((const char *file));
extern char * mu_getcwd __P((void));
  
extern int mu_spawnvp(const char* prog, const char* const av[], int* stat);

typedef void *(*mu_retrieve_fp) __PMT((void *));
extern void mu_register_retriever __P((list_t *pflist, mu_retrieve_fp fun));
extern void * mu_retrieve __P((list_t flist, void *data));

extern int mu_unroll_symlink __P((char *out, size_t outsz, const char *in));

extern char * mu_expand_path_pattern __P((const char *pattern,
					  const char *username));

extern int mu_rfc2822_references __P((message_t msg, char **pstr));
extern int mu_rfc2822_in_reply_to __P((message_t msg, char **pstr));

/* Find NEEDLE in the HAYSTACK. Case insensitive comparison */
extern char *mu_strcasestr __P((const char *haystack, const char *needle));

extern int mu_string_unfold __P((char *text, size_t *plen));

extern int munre_set_regex __P((char *str, int caseflag, char **errp));
extern int munre_subject  __P((const char *subject, const char **new_subject));

extern char *mu_charset_lookup __P((char *lang, char *terr));

extern int mu_true_answer_p __P((const char *p));
extern int mu_scheme_autodetect_p __P((const char *scheme, const char **path));

struct timeval; 
  
extern int mu_fd_wait __P((int fd, int *pflags, struct timeval *tvp));

extern int mu_decode_filter __P((stream_t *pfilter, stream_t input,
				 const char *filter_type,
				 const char *fromcode, const char *tocode));

extern enum mu_iconv_fallback_mode mu_default_fallback_mode;
extern int mu_set_default_fallback __P((const char *str));
  
#ifdef __cplusplus
}
#endif

#endif


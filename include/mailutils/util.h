/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2007, 2009-2012 Free Software
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

#ifndef _MAILUTILS_UTIL_H
#define _MAILUTILS_UTIL_H

/* A collection of utility routines that don't belong somewhere else. */

#include <mailutils/list.h>
#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  /* ----------------------- */
  /* String manipulation     */
  /* ----------------------- */
unsigned long mu_hex2ul (char hex);
size_t mu_hexstr2ul (unsigned long* ul, const char* hex, size_t len);
size_t mu_cpystr (char *dst, const char *src, size_t size);
char *mu_stpcpy (char *p, char *q);
int mu_string_unfold (char *text, size_t *plen);
int mu_true_answer_p (const char *p);
int mu_unre_set_regex (const char *str, int caseflag, char **errp);
int mu_unre_subject  (const char *subject, const char **new_subject);
int mu_is_proto (const char *p);
int mu_mh_delim (const char *str);
void mu_str_url_decode_inline (char *str);
int mu_str_url_decode (char **ptr, const char *s);

  /* ----------------------- */
  /* File & path names.      */
  /* ----------------------- */
  
#define MU_HIERARCHY_DELIMITER '/'
  
char *mu_get_homedir (void);
char *mu_get_full_path (const char *file);
char *mu_normalize_path (char *path);
char *mu_expand_path_pattern (const char *pattern, const char *username);
char *mu_tilde_expansion (const char *ref, int delim,
			  const char *homedir);
int mu_readlink (const char *name, char **pbuf, size_t *psize, size_t *plen);
int mu_unroll_symlink (const char *name, char **pout);
char *mu_getcwd (void);
char *mu_make_file_name_suf (const char *dir, const char *file,
			     const char *suf);
#define mu_make_file_name(dir, file) mu_make_file_name_suf (dir, file, NULL)

  /* ------------------------ */
  /* Temporary file creation. */
  /* ------------------------ */
#define MU_TEMPFILE_TMPDIR  0x01   /* tmpdir is set */
#define MU_TEMPFILE_SUFFIX  0x02   /* suffix is set */  
#define MU_TEMPFILE_MKDIR   0x04   /* create a directory, not a file */
  
struct mu_tempfile_hints
{
  char *tmpdir;
  char *suffix;
};

  /*int mu_tempfile (const char *tmpdir, char **namep);*/
int mu_tempfile (struct mu_tempfile_hints *hints, int flags,
		 int *pfd, char **namep);
char *mu_tempname (const char *tmpdir);
  
  /* ----------------------- */
  /* Current user email.     */
  /* ----------------------- */
/* Set the default user email address.
 *  
 * Subsequent calls to mu_get_user_email() with a NULL name will return this
 * email address.  email is parsed to determine that it consists of a a valid
 * rfc822 address, with one valid addr-spec, i.e, the address must be
 * qualified.
 */
int mu_set_user_email (const char *email);

/* Set the default user email address domain.
 *  
 * Subsequent calls to mu_get_user_email() with a non-null name will return
 * email addresses in this domain (name@domain). It should be fully
 * qualified, but this isn't (and can't) be enforced.
 */
int mu_set_user_email_domain (const char *domain);

/* Return the currently set user email domain, or NULL if not set. */
int mu_get_user_email_domain (const char** domain);

/* Same, but allocates memory */
int mu_aget_user_email_domain (char **pdomain);

/*
 * Get the default email address for user name. A NULL name is taken
 * to mean the current user.
 *  
 * The result must be freed by the caller after use.
 */
char *mu_get_user_email (const char *name);

  /* ----------------------- */
  /* Message ID support.     */
  /* ----------------------- */

int mu_rfc2822_msg_id (int subpart, char **pstr);
int mu_rfc2822_references (mu_message_t msg, char **pstr);
int mu_rfc2822_in_reply_to (mu_message_t msg, char **pstr);

  /* ----------------------- */
  /* Filter+iconv            */
  /* ----------------------- */
int mu_decode_filter (mu_stream_t *pfilter, mu_stream_t input,
		      const char *filter_type,
		      const char *fromcode, const char *tocode);
int mu_decode_filter_args (mu_stream_t *pfilter, mu_stream_t input,
			   const char *filter_name,
			   int argc, const char **argv,
			   const char *fromcode, const char *tocode);

extern enum mu_iconv_fallback_mode mu_default_fallback_mode;
int mu_set_default_fallback (const char *str);

  /* ----------------------- */
  /* Stream flags conversion */
  /* ----------------------- */
int mu_stream_flags_to_mode (int flags, int isdir);
int mu_parse_stream_perm_string (int *pmode, const char *str,
				 const char **endp);
  
  
  /* ----------------------- */
  /* Stream-based getpass    */
  /* ----------------------- */
int mu_getpass (mu_stream_t in, mu_stream_t out, const char *prompt,
		char **passptr);

  /* ----------------------- */
  /* Assorted functions.     */
  /* ----------------------- */
int mu_getmaxfd (void);
/* Get the host name, doing a gethostbyname() if possible. */
int mu_get_host_name (char **host);
int mu_spawnvp (const char *prog, char *av[], int *stat);
int mu_scheme_autodetect_p (mu_url_t);
int mu_set_user_privileges (uid_t uid, gid_t *gidv, size_t gidc);
int mu_switch_to_privs (uid_t uid, gid_t gid, mu_list_t retain_groups);

struct timeval; 
int mu_fd_wait (int fd, int *pflags, struct timeval *tvp);


int mutil_parse_field_map (const char *map, mu_assoc_t *passoc_tab,
			   int *perr);

/* Run the onexit list */
void mu_onexit_run (void);
/* Reset the onexit list. */
void mu_onexit_reset (void);
/* Register the onexit function and associated data */
int mu_onexit (mu_onexit_t func, void *data);

#define MU_FILE_SAFETY_NONE           0x00
#define MU_FILE_SAFETY_OWNER_MISMATCH 0x01
#define MU_FILE_SAFETY_GROUP_WRITABLE 0x02
#define MU_FILE_SAFETY_WORLD_WRITABLE 0x04
#define MU_FILE_SAFETY_GROUP_READABLE 0x08
#define MU_FILE_SAFETY_WORLD_READABLE 0x10  
#define MU_FILE_SAFETY_LINKED_WRDIR   0x20 
#define MU_FILE_SAFETY_DIR_IWGRP      0x40
#define MU_FILE_SAFETY_DIR_IWOTH      0x80

#define MU_FILE_SAFETY_ALL      (		\
  MU_FILE_SAFETY_OWNER_MISMATCH |		\
  MU_FILE_SAFETY_GROUP_WRITABLE |		\
  MU_FILE_SAFETY_WORLD_WRITABLE |		\
  MU_FILE_SAFETY_GROUP_READABLE |		\
  MU_FILE_SAFETY_WORLD_READABLE |		\
  MU_FILE_SAFETY_LINKED_WRDIR   |		\
  MU_FILE_SAFETY_DIR_IWGRP      |		\
  MU_FILE_SAFETY_DIR_IWOTH      )
  

int mu_file_safety_check (const char *filename, int mode,
			  uid_t uid,
			  mu_list_t idlist);
const char *mu_file_safety_code_to_name (int code);
int mu_file_safety_name_to_code (const char *name, int *pcode);
int mu_file_safety_name_to_error (const char *name, int *pcode);  
int mu_file_safety_compose (int *res, const char *name, int defval);

int mu_file_mode_to_safety_criteria (int mode);
int mu_safety_criteria_to_file_mode (int crit);
  
  
#ifdef __cplusplus
}
#endif

#endif


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007-2012 Free Software Foundation,
   Inc.

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

#ifndef _MAILUTILS_URL_H
#define _MAILUTILS_URL_H	1

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_URL_SCHEME 0x0001
#define MU_URL_USER   0x0002 /* Has a user part */
#define MU_URL_SECRET 0x0004 /* Has a secret (password) part */
#define MU_URL_AUTH   0x0008 /* Has auth part */
#define MU_URL_HOST   0x0010 /* Has host part */
#define MU_URL_PORT   0x0020 /* Has port part */  
#define MU_URL_PATH   0x0040 /* Has path */
#define MU_URL_PARAM  0x0080 /* Has parameters */
#define MU_URL_QUERY  0x0100 /* Has query */
#define MU_URL_IPV6   0x0200 /* Host part is bracketed (IPv6) */
  
#define MU_URL_CRED (MU_URL_USER | MU_URL_SECRET | MU_URL_AUTH)  
  /* Has some of authentication credentials */
#define MU_URL_INET (MU_URL_HOST | MU_URL_PORT)
  /* Has Inet address */
#define MU_URL_ALL \
  (MU_URL_CRED | \
   MU_URL_HOST | \
   MU_URL_PATH | \
   MU_URL_PARAM | \
   MU_URL_QUERY)

  /* Parser flags */
#define MU_URL_PARSE_HEXCODE      0x0001  /* Decode % notations (RFC 1738,
					     section 2.2) */
#define MU_URL_PARSE_HIDEPASS     0x0002  /* Hide password in the URL */
#define MU_URL_PARSE_PORTSRV      0x0004  /* Use getservbyname to determine
					     port number */
#define MU_URL_PARSE_PORTWC       0x0008  /* Allow wildcard (*) as a port
					     number (for tickets) */
#define MU_URL_PARSE_PIPE         0x0010  /* Translate "| ..." to
					     "prog://..." */
#define MU_URL_PARSE_SLASH        0x0020  /* Translate "/..." to
					     "file:///..." */
#define MU_URL_PARSE_DSLASH_OPTIONAL 0x0040 /* Double-slash after scheme:
					       part is optional */
#define MU_URL_PARSE_LOCAL        0x0080  /* Local URL: no host part */   
  
#define MU_URL_PARSE_DEFAULT \
  (MU_URL_PARSE_HEXCODE|MU_URL_PARSE_HIDEPASS|MU_URL_PARSE_PORTSRV|\
   MU_URL_PARSE_PIPE|MU_URL_PARSE_SLASH)
#define MU_URL_PARSE_ALL (MU_URL_PARSE_DEFAULT|MU_URL_PARSE_PORTWC)  
  
int mu_url_create_hint (mu_url_t *purl, const char *str, int flags,
			mu_url_t hint);
int mu_url_copy_hints (mu_url_t url, mu_url_t hint);
  
int  mu_url_create    (mu_url_t *, const char *name);
int  mu_url_dup       (mu_url_t old_url, mu_url_t *new_url);
int  mu_url_uplevel   (mu_url_t url, mu_url_t *upurl);

int mu_url_get_flags (mu_url_t, int *);
int mu_url_has_flag (mu_url_t, int);  
  
void mu_url_destroy   (mu_url_t *);

int mu_url_sget_name (const mu_url_t, const char **);
int mu_url_aget_name (const mu_url_t, char **);
int mu_url_get_name (const mu_url_t, char *, size_t, size_t *);
  
int mu_url_sget_scheme  (const mu_url_t, const char **);
int mu_url_aget_scheme  (const mu_url_t, char **);  
int mu_url_get_scheme  (const mu_url_t, char *, size_t, size_t *);

int mu_url_sget_user  (const mu_url_t, const char **);
int mu_url_aget_user  (const mu_url_t, char **);  
int mu_url_get_user  (const mu_url_t, char *, size_t, size_t *);

int mu_url_get_secret (const mu_url_t url, mu_secret_t *psecret);

int mu_url_sget_auth  (const mu_url_t, const char **);
int mu_url_aget_auth  (const mu_url_t, char **);  
int mu_url_get_auth  (const mu_url_t, char *, size_t, size_t *);

int mu_url_sget_host  (const mu_url_t, const char **);
int mu_url_aget_host  (const mu_url_t, char **);  
int mu_url_get_host  (const mu_url_t, char *, size_t, size_t *);

int mu_url_sget_path  (const mu_url_t, const char **);
int mu_url_aget_path  (const mu_url_t, char **);  
int mu_url_get_path  (const mu_url_t, char *, size_t, size_t *);

int mu_url_sget_query (const mu_url_t url, size_t *qc, char ***qv);
int mu_url_aget_query (const mu_url_t url, size_t *qc, char ***qv);

int mu_url_sget_portstr  (const mu_url_t, const char **);
int mu_url_aget_portstr  (const mu_url_t, char **);  
int mu_url_get_portstr   (const mu_url_t, char *, size_t, size_t *);
  
int mu_url_get_port    (const mu_url_t, unsigned *);

int mu_url_sget_fvpairs (const mu_url_t url, size_t *fvc, char ***fvp);
int mu_url_aget_fvpairs (const mu_url_t url, size_t *pfvc, char ***pfvp);

int mu_url_sget_param (const mu_url_t url, const char *param, const char **val);
int mu_url_aget_param (const mu_url_t url, const char *param, char **val);
  
int mu_url_expand_path (mu_url_t url);
const char *mu_url_to_string   (const mu_url_t);

int mu_url_is_scheme   (mu_url_t, const char *scheme);

int mu_url_is_same_scheme (mu_url_t, mu_url_t);
int mu_url_is_same_user   (mu_url_t, mu_url_t);
int mu_url_is_same_path   (mu_url_t, mu_url_t);
int mu_url_is_same_host   (mu_url_t, mu_url_t);
int mu_url_is_same_port   (mu_url_t, mu_url_t);

int mu_url_matches_ticket   (mu_url_t ticket, mu_url_t url, int *wcn);  

int mu_url_decode (mu_url_t url);

int mu_url_invalidate (mu_url_t url);
  
int mu_url_create_null (mu_url_t *purl);
int mu_url_set_user (mu_url_t url, const char *user);
int mu_url_set_path (mu_url_t url, const char *path);
int mu_url_set_scheme (mu_url_t url, const char *scheme);
int mu_url_set_host (mu_url_t url, const char *host);
int mu_url_set_port (mu_url_t url, unsigned port);
int mu_url_set_service (mu_url_t url, const char *str);
int mu_url_set_auth (mu_url_t url, const char *auth);
int mu_url_set_secret (mu_url_t url, mu_secret_t secret);
int mu_url_add_param (mu_url_t url, size_t pc, const char **pv);
int mu_url_clear_param (mu_url_t url);
int mu_url_add_query (mu_url_t url, size_t pc, const char **pv);
int mu_url_clear_query (mu_url_t url);  
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_URL_H */

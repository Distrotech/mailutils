/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2009-2012 Free Software
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

#ifndef _MAILUTILS_AUTH_H
#define _MAILUTILS_AUTH_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int  mu_ticket_create          (mu_ticket_t *, void *);
int mu_ticket_ref              (mu_ticket_t);
int mu_ticket_unref            (mu_ticket_t);
void mu_ticket_destroy         (mu_ticket_t *);
int  mu_ticket_set_destroy     (mu_ticket_t,
				void (*) (mu_ticket_t), void *);
void *mu_ticket_get_owner      (mu_ticket_t);

int mu_ticket_get_cred         (mu_ticket_t ticket,
				mu_url_t url, const char *challenge,
				char **pplain, mu_secret_t *psec);

int mu_ticket_set_get_cred     (mu_ticket_t,
				int  (*) (mu_ticket_t, mu_url_t,
					  const char *,
					  char **, mu_secret_t *),
				void *);

int mu_ticket_set_data         (mu_ticket_t, void *, void *owner);
void *mu_ticket_get_data       (mu_ticket_t);

int mu_ticket_set_secret (mu_ticket_t ticket, mu_secret_t secret);
int mu_ticket_set_plain (mu_ticket_t ticket, const char *text);

int mu_authority_create           (mu_authority_t *, mu_ticket_t, void *);
void mu_authority_destroy         (mu_authority_t *, void *);
void *mu_authority_get_owner      (mu_authority_t);
int mu_authority_set_ticket       (mu_authority_t, mu_ticket_t);
int mu_authority_get_ticket       (mu_authority_t, mu_ticket_t *);
int mu_authority_authenticate     (mu_authority_t);
int mu_authority_set_authenticate (mu_authority_t,
				   int (*_authenticate) (mu_authority_t),
				   void *);

int mu_authority_create_null      (mu_authority_t *pauthority, void *owner);

int mu_wicket_create       (mu_wicket_t *);
int mu_wicket_get_ticket (mu_wicket_t wicket, const char *user,
				 mu_ticket_t *pticket);
int mu_wicket_ref (mu_wicket_t wicket);
int mu_wicket_unref (mu_wicket_t wicket);
void mu_wicket_destroy (mu_wicket_t *pwicket);
int mu_wicket_set_destroy (mu_wicket_t wicket,
				  void (*_destroy) (mu_wicket_t));
int mu_wicket_set_data (mu_wicket_t wicket, void *data);
void *mu_wicket_get_data (mu_wicket_t wicket);
int mu_wicket_set_get_ticket (mu_wicket_t wicket,
			      int (*_get_ticket) (mu_wicket_t, void *,
						 const char *, mu_ticket_t *));
int mu_file_wicket_create (mu_wicket_t *pwicket, const char *filename);

struct mu_debug_locus;  
int mu_wicket_stream_match_url (mu_stream_t stream, struct mu_locus *loc,
				mu_url_t url, int parse_flags,
				mu_url_t *pticket_url);
int mu_wicket_file_match_url (const char *name, mu_url_t url,
			      int parse_flags,
			      mu_url_t *pticket_url);

  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_AUTH_H */

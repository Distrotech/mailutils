/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

/**
* Parses syntatic elements defined in RFC 822.
*/

#ifndef _MAILUTILS_PARSE822_H
#define _MAILUTILS_PARSE822_H

#include <mailutils/mu_features.h>
#include <mailutils/address.h>
#include <mailutils/mutil.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* Reads an RFC822 defined lexical token from an input. All names are
* as close as possible to those used in the extended BNF of the RFC.
*/

/* From RFC 822, 3.3 Lexical Tokens */

extern int parse822_is_char        __P ((char c));
extern int parse822_is_digit       __P ((char c));
extern int parse822_is_ctl         __P ((char c));
extern int parse822_is_space       __P ((char c));
extern int parse822_is_htab        __P ((char c));
extern int parse822_is_lwsp_char   __P ((char c));
extern int parse822_is_special     __P ((char c));
extern int parse822_is_atom_char   __P ((char c));
extern int parse822_is_q_text      __P ((char c));
extern int parse822_is_d_text      __P ((char c));
extern int parse822_is_smtp_q      __P ((char c));

extern int parse822_skip_crlf      __P ((const char** p, const char* e));
extern int parse822_skip_lwsp_char __P ((const char** p, const char* e));
extern int parse822_skip_lwsp      __P ((const char** p, const char* e));
extern int parse822_skip_comments  __P ((const char** p, const char* e));
extern int parse822_skip_nl        __P ((const char** p, const char* e));

extern int parse822_digits         __P ((const char** p, const char* e, int min, int max, int* digits));
extern int parse822_special        __P ((const char** p, const char* e, char c));
extern int parse822_comment        __P ((const char** p, const char* e, char** comment));
extern int parse822_atom           __P ((const char** p, const char* e, char** atom));
extern int parse822_quoted_pair    __P ((const char** p, const char* e, char** qpair));
extern int parse822_quoted_string  __P ((const char** p, const char* e, char** qstr));
extern int parse822_word           __P ((const char** p, const char* e, char** word));
extern int parse822_phrase         __P ((const char** p, const char* e, char** phrase));
extern int parse822_d_text         __P ((const char** p, const char* e, char** dtext));

/* From RFC 822, 6.1 Address Specification Syntax */

extern int parse822_address_list   __P ((address_t* a, const char* s));
extern int parse822_mail_box       __P ((const char** p, const char* e, address_t* a));
extern int parse822_group          __P ((const char** p, const char* e, address_t* a));
extern int parse822_address        __P ((const char** p, const char* e, address_t* a));
extern int parse822_route_addr     __P ((const char** p, const char* e, address_t* a));
extern int parse822_route          __P ((const char** p, const char* e, char** route));
extern int parse822_addr_spec      __P ((const char** p, const char* e, address_t* a));
extern int parse822_unix_mbox      __P ((const char** p, const char* e, address_t* a));
extern int parse822_local_part     __P ((const char** p, const char* e, char** local_part));
extern int parse822_domain         __P ((const char** p, const char* e, char** domain));
extern int parse822_sub_domain     __P ((const char** p, const char* e, char** sub_domain));
extern int parse822_domain_ref     __P ((const char** p, const char* e, char** domain_ref));
extern int parse822_domain_literal __P ((const char** p, const char* e, char** domain_literal));

/* RFC 822 Quoting Functions
* Various elements must be quoted if then contain non-safe characters. What
* characters are allowed depend on the element. The following functions will
* allocate a quoted version of the raw element, it may not actually be
* quoted if no unsafe characters were in the raw string.
*/

extern int parse822_quote_string     __P ((char** quoted, const char* raw));
extern int parse822_quote_local_part __P ((char** quoted, const char* raw));

extern int parse822_field_body       __P ((const char** p, const char *e, char** fieldbody));
extern int parse822_field_name       __P ((const char** p, const char *e, char** fieldname));

/***** From RFC 822, 5.1 Date and Time Specification Syntax *****/

extern int parse822_day       __P ((const char** p, const char* e, int* day));
extern int parse822_date      __P ((const char** p, const char* e, int* day, int* mon, int* year));
extern int parse822_time      __P ((const char** p, const char* e, int* h, int* m, int* s, int* tz, const char** tz_name));
extern int parse822_date_time __P ((const char** p, const char* e, struct tm* tm, mu_timezone* tz));


#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_PARSE822_H */


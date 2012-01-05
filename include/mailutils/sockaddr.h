/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_SOCKADDR_H
#define _MAILUTILS_SOCKADDR_H

#include <sys/types.h>
#include <sys/socket.h>
#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mu_sockaddr
{
  struct mu_sockaddr *prev, *next;
  struct sockaddr *addr;            /* Address */
  socklen_t        addrlen;         /* Size of addr */
  char *str;                        /* string representation of addr,
				       filled up by mu_sockaddr_str */
};

#define MU_AH_PASSIVE       0x01
#define MU_AH_DETECT_FAMILY 0x02
  
struct mu_sockaddr_hints
{
  int flags;
  int family;
  int socktype;
  int protocol;
  unsigned short port;
};

int mu_sockaddr_create (struct mu_sockaddr **res,
			struct sockaddr *addr, socklen_t len);
int mu_sockaddr_copy (struct mu_sockaddr **pnew, struct mu_sockaddr *old);
void mu_sockaddr_free (struct mu_sockaddr *addr);
struct mu_sockaddr *mu_sockaddr_unlink (struct mu_sockaddr *addr);
void mu_sockaddr_free_list (struct mu_sockaddr *addr);
struct mu_sockaddr *mu_sockaddr_insert (struct mu_sockaddr *anchor,
					struct mu_sockaddr *addr,
					int before);
const char *mu_sockaddr_str (struct mu_sockaddr *addr);

int mu_sockaddr_format (char **pbuf, const struct sockaddr *sa,
			socklen_t salen);

int mu_sockaddr_from_node (struct mu_sockaddr **retval, const char *node,
			   const char *serv, struct mu_sockaddr_hints *hints); 
int mu_sockaddr_from_url (struct mu_sockaddr **retval, mu_url_t url,
			  struct mu_sockaddr_hints *hints);

int mu_str_is_ipv4 (const char *addr);
int mu_str_is_ipv6 (const char *addr);
int mu_str_is_ipaddr (const char *addr);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SOCKADDR_H */
	

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

#ifndef _MAILUTILS_CIDR_H
#define _MAILUTILS_CIDR_H

#include <sys/types.h>
#include <sys/socket.h>
#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_INADDR_BYTES 16

struct mu_cidr
{
  int family;
  int len;
  unsigned char address[MU_INADDR_BYTES];
  unsigned char netmask[MU_INADDR_BYTES];
};

#define MU_CIDR_MAXBUFSIZE 81
  
int mu_cidr_from_sockaddr (struct mu_cidr *cp, const struct sockaddr *sa);
int mu_cidr_from_string (struct mu_cidr *cp, const char *str);

#define MU_CIDR_FMT_ADDRONLY 0x01
#define MU_CIDR_FMT_SIMPLIFY 0x02
  
int mu_cidr_to_string (struct mu_cidr *cidr, int flags, char *buf, size_t size,
		       size_t *pret);
int mu_cidr_format (struct mu_cidr *, int flags, char **pbuf);
int mu_cidr_to_sockaddr (struct mu_cidr *, struct sockaddr **sa);

int mu_cidr_match (struct mu_cidr *a, struct mu_cidr *b);

int _mu_inaddr_to_bytes (int af, void *buf, unsigned char *bytes);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_CIDR_H */
	
  

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _ADDRESS0_H
#define _ADDRESS0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/address.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The data-structure representing an RFC822 MAILBOX. It may be
 * one MAILBOX or a list of them, as found in an ADDRESS or
 * a MAILBOX list (as found in a GROUP).
 *  
 * Capitalized names are from RFC 822, section 6.1 (Address Syntax).
 */
struct _mu_address
{
  char *addr;
  	/* the original string that this list of addresses was created
	 * from, only present at the head of the list */

  char *comments;
  	/* the collection of comments stripped during parsing this MAILBOX */
  char *personal;
  	/* the PHRASE portion of a MAILBOX, called the DISPLAY-NAME in drums */
  char *email;
  	/* the ADDR-SPEC, the LOCAL-PART@DOMAIN */
  char *local_part;
  	/* the LOCAL-PART of a MAILBOX */
  char *domain;
  	/* the DOMAIN of a MAILBOX */
  char *route;
  	/* the optional ROUTE in the ROUTE-ADDR form of MAILBOX */

  struct _mu_address *next;
};

#ifdef __cplusplus
}
#endif

#endif /* _ADDRESS0_H */

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

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
struct _address
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

/*  size_t num; this didn't appear to be used anywhere... so I commented
 it out, is that ok? -sam */

  struct _address *next;
};

#ifdef __cplusplus
}
#endif

#endif /* _ADDRESS0_H */

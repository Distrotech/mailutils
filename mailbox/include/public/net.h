/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _NET_H
#define _NET_H

#include <sys/types.h>

#include <io.h>

#ifdef _cplusplus
extern "C" {
#endif

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

struct _net;
typedef struct _net *net_t;

extern int net_api_create 		__P((net_t *, net_t, const char *type));
extern int net_api_set_option	__P((net_t net, const char *name, const char *value));
extern int net_api_destroy		__P((net_t *));

struct _netinstance;
typedef struct _netinstance *netinstance_t;

extern int net_new				__P((net_t, netinstance_t *));
extern int net_connect			__P((netinstance_t, const char *host, int port));
extern int net_get_stream		__P((netinstance_t, stream_t *iostr));
extern int net_close			__P((netinstance_t));
extern int net_free				__P((netinstance_t *));

#ifdef _cplusplus
}
#endif

#endif /* NET_H */

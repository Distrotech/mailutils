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

#ifndef _NET0_H
#define _NET0_H

#include <sys/types.h>

#include <io.h>
#include <net.h>

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


struct _net_api {
	int (*new)(void *netdata, net_t parent, void **data);
	int (*connect)(void *data, const char *host, int port);
	int (*get_stream)(void *data, stream_t *iostr);
	int (*close)(void *data);
	int (*free)(void **data);
};

struct _netregistrar {
	const char 	*type;

	int (*create)(void **netdata, struct _net_api **api);
	int (*set_option)(void *netdata, const char *name, const char *value);
	int (*destroy)(void **netdata);
};

struct _net {
	struct _net_api		*api;
	void				*data;
	struct _net			*parent;

	struct _netregistrar *net_reg;
};

struct _netinstance {
	struct _net_api	*api;
	void			*data;
};

int _tcp_create(void **data, struct _net_api **api);
int _tcp_set_option(void *data, const char *name, const char *value);
int _tcp_destroy(void **data);

#ifdef _cplusplus
}
#endif

#endif /* NET0_H */

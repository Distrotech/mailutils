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

#ifndef _TCP0_H
#define _TCP0_H

#define TCP_STATE_INIT 			1
#define TCP_STATE_RESOLVE		2
#define TCP_STATE_RESOLVING		3
#define TCP_STATE_CONNECTING 	4
#define TCP_STATE_CONNECTED		5

struct _tcp_instance {
	int 			fd;
	char 			*host;
	int 			port;
	int				state;
	unsigned long	address;
};

#endif /* _TCP0_H */

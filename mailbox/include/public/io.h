/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _IO_H
# define _IO_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" { /*}*/
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

struct _stream;
typedef struct _stream *stream_t;

extern int stream_create __P ((stream_t *, void *owner));
extern void stream_destroy __P ((stream_t *, void *owner));

extern int stream_set_fd __P ((stream_t,
			       int (*_get_fd)(stream_t, int *),
			       void *owner));
extern int stream_set_read __P ((stream_t,
				 int (*_read) __P ((stream_t, char *,
						    size_t, off_t, size_t *)),
				 void *owner));
extern int stream_set_write __P ((stream_t,
				  int (*_write) __P ((stream_t, const char *,
						      size_t, off_t,
						      size_t *)),
				  void *owner));
extern int stream_get_fd __P ((stream_t , int *));
extern int stream_read __P ((stream_t, char *, size_t, off_t, size_t *));
extern int stream_write __P ((stream_t, const char *, size_t,
			       off_t, size_t *));


#ifdef __cplusplus
}
#endif

#endif /* _IO_H */

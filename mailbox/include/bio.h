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

#ifndef _BIO_H
# define _BIO_H

#include <sys/types.h>
#include <mailutils/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

typedef struct _bio * bio_t;

/* bufferred I/O */
extern ssize_t bio_create __P ((bio_t *pbio, stream_t));
extern void bio_destroy __P ((bio_t *pbio));
extern ssize_t bio_read __P ((bio_t bio, char *vptr,
			      size_t maxlen, size_t *pnread));
extern ssize_t bio_readline __P ((bio_t bio, char *vptr,
				  size_t maxlen, size_t *pnread));
extern ssize_t bio_write __P ((bio_t bio, const char *vptr,
			       size_t maxlen, size_t *pnwrite));

#ifdef __cplusplus
}
#endif

#endif /* _BIO_H */

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

#ifndef _MAILUTILS_TRANSCODE_H
#define _MAILUTILS_TRANSCODE_H

#include <sys/types.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /* __P */

#ifdef _cplusplus
extern "C" {
#endif

/* forward declaration */
struct _transcoder;
typedef struct _transcoder *transcoder_t;

struct _transcoder
{
	stream_t 	ustream;	 /* user reads/writes decoded/encoded data from here */
	stream_t 	stream;    	 /* encoder/decoder read/writes data to here */
	void 		(*destroy)(transcoder_t tc);
	void 		*tcdata; 
};

extern int transcode_create			__P ((transcoder_t *, char *encoding));
extern void transcode_destroy		__P ((transcoder_t *));
extern int transcode_get_stream		__P ((transcoder_t tc, stream_t *pis));
extern int transcode_set_stream		__P ((transcoder_t tc, stream_t is));

#ifdef _cplusplus
}           
#endif

#endif /* _MAILUTILS_TRANSCODE_H */

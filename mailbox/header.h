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

#ifndef _HEADER_H
#define _HEADER_H

#include <sys/types.h>

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

#ifdef _cpluscplus
extern "C" {
#endif

#define MU_HDR_RFC822                0

#define MU_HDR_UNIX_FROM             "From "
#define MU_HDR_RETURN_PATH           "Return-Path"
#define MU_HDR_RECEIVED              "Received"
#define MU_HDR_DATE                  "Date"
#define MU_HDR_FROM                  "From"
#define MU_HDR_RESENT_FROM           "Resent-From"
#define MU_HDR_SUBJECT               "Subject"
#define MU_HDR_SENDER                "Sender"
#define MU_HDR_RESENT_SENDER         "Resent-SENDER"
#define MU_HDR_TO                    "To"
#define MU_HDR_RESENT_TO             "Resent-To"
#define MU_HDR_CC                    "Cc"
#define MU_HDR_RESENT_CC             "Resent-Cc"
#define MU_HDR_BCC                   "Bcc"
#define MU_HDR_RESENT_BCC            "Resent-Bcc"
#define MU_HDR_REPLY_TO              "Reply-To"
#define MU_HDR_RESENT_REPLY_TO       "Resent-Reply-To"
#define MU_HDR_MESSAGE_ID            "Message-ID"
#define MU_HDR_RESENT_MESSAGE_ID     "Resent-Message-ID"
#define MU_HDR_IN_REPLY_TO           "In-Reply-To"
#define MU_HDR_ENCRYPTED             "Encrypted"
#define MU_HDR_PRECEDENCE            "Precedence"
#define MU_HDR_STATUS                "Status"
#define MU_HDR_CONTENT_LENGTH        "Content-Length"
#define MU_HDR_CONTENT_TYPE          "Content-Type"
#define MU_HDR_MIME_VERSION          "MIME-Version"

/* Mime support header attribute */

/* forward declaration */
struct _header;
typedef struct _header * header_t;

struct _header
{
  /* Data */
  void *data;

  /* Functions */
  int (*_set_value)  __P ((header_t, const char *fn, const char *fv,
			   size_t n, int replace));
  int (*_get_value)  __P ((header_t, const char *fn, char *fv,
			   size_t len, size_t *n));
  int (*_get_mvalue) __P ((header_t, const char *fn, char **fv, size_t *n));
  int (*_parse)      __P ((header_t, const char *blurb, size_t len));
} ;

extern int header_init    __P ((header_t *, const char *blurb,
				size_t ln, int flag));
extern void header_destroy __P ((header_t *));

extern int header_gvalue __P ((const char *blurb, size_t bl, const char *fn,
			       char *fv, size_t len, size_t *n));
extern int header_gmvalue __P ((const char *blurb, size_t bl, const char *fn,
			       char **fv, size_t *nv));

#undef INLINE
#ifdef __GNUC__
#define INLINE __inline__
#else
#define INLINE
#endif

extern INLINE int header_set_value __P ((header_t, const char *fn,
					 const char *fv, size_t n,
					 int replace));
extern INLINE int header_get_value __P ((header_t, const char *fn, char *fv,
					size_t len, size_t *n));
extern INLINE int header_get_mvalue __P ((header_t, const char *fn,
					 char **fv, size_t *n));

#ifdef USE_MACROS
#define header_set_value(h, fn, fv, n, r)  h->_set_value (h, fn, fv, n, r)
#define header_get_value(h, fn, fv, v, ln, n)  h->_get_value (h, fn, fv, ln, n)
#define header_get_mvalue(h, fn, fv, v, n) h->_get_mvalue (h, fn, fv, n)
#endif

#ifdef _cpluscplus
}
#endif

#endif /* HEADER_H */

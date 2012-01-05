/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2010-2012 Free Software
   Foundation, Inc.

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

#ifndef _MAILUTILS_FILTER_H
#define _MAILUTILS_FILTER_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Mode */
#define MU_FILTER_DECODE 0
#define MU_FILTER_ENCODE 1

/* Direction.  */
#define MU_FILTER_READ  MU_STREAM_READ
#define MU_FILTER_WRITE MU_STREAM_WRITE
#define MU_FILTER_RDWR  MU_STREAM_RDWR

struct mu_filter_io
{
  const char *input;
  size_t isize;
  char *output;
  size_t osize;
  int errcode;
  int eof;
};

enum mu_filter_command
  {
    mu_filter_init,
    mu_filter_done,
    mu_filter_xcode,
    mu_filter_lastbuf,
    mu_filter_flush
  };
  
enum mu_filter_result
  {
    mu_filter_ok,
    mu_filter_failure,
    mu_filter_moreinput,
    mu_filter_moreoutput,
    mu_filter_again
  };

/* An xcode function is not allowed to return mu_filter_again more
   than MU_FILTER_MAX_AGAIN times in a sequence. */
#define MU_FILTER_MAX_AGAIN 5

typedef int (*mu_filter_new_data_t) (void **, int, int argc,
				     const char **argv);
typedef enum mu_filter_result
       (*mu_filter_xcode_t) (void *data, enum mu_filter_command cmd,
			     struct mu_filter_io *iobuf);

int mu_filter_create_args (mu_stream_t *pstream, mu_stream_t stream,
			   const char *name, int argc, const char **argv,
			   int mode, int flags);
  
int mu_filter_stream_create (mu_stream_t *pflt,
			     mu_stream_t str,
			     int mode, 
			     mu_filter_xcode_t xcode,
			     void *xdata, 
			     int flags);

int mu_filter_chain_create_pred (mu_stream_t *pret, mu_stream_t transport,
				 int defmode, int flags,
				 size_t argc, char **argv,
				 int (*pred) (void *, mu_stream_t,
					      const char *),
				 void *closure);
int mu_filter_chain_create (mu_stream_t *pret, mu_stream_t transport,
			    int defmode, int flags,
			    size_t argc, char **argv);

struct _mu_filter_record
{
  const char *name;
  mu_filter_new_data_t newdata;
  mu_filter_xcode_t encoder;
  mu_filter_xcode_t decoder;
};
  
extern int mu_filter_create (mu_stream_t *, mu_stream_t, const char*,
			     int, int);
extern int mu_filter_get_list (mu_list_t *);

/* List of defaults.  */
extern mu_filter_record_t mu_crlf_filter;  
extern mu_filter_record_t mu_rfc822_filter;
extern mu_filter_record_t mu_crlfdot_filter;
extern mu_filter_record_t mu_dot_filter;
extern mu_filter_record_t mu_qp_filter; /* quoted-printable.  */
extern mu_filter_record_t mu_base64_filter;
extern mu_filter_record_t mu_binary_filter;
extern mu_filter_record_t mu_bit8_filter;
extern mu_filter_record_t mu_bit7_filter;
extern mu_filter_record_t mu_rfc_2047_Q_filter;
extern mu_filter_record_t mu_rfc_2047_B_filter;
extern mu_filter_record_t mu_from_filter;
extern mu_filter_record_t mu_inline_comment_filter;
extern mu_filter_record_t mu_header_filter;
extern mu_filter_record_t mu_linecon_filter;
extern mu_filter_record_t mu_linelen_filter;
extern mu_filter_record_t mu_iconv_filter;
extern mu_filter_record_t mu_c_escape_filter;
  
enum mu_iconv_fallback_mode
  {
    mu_fallback_none,
    mu_fallback_copy_pass,
    mu_fallback_copy_octal
  };

extern int mu_filter_iconv_create (mu_stream_t *s, mu_stream_t transport,
				   const char *fromcode, const char *tocode,
				   int flags,
				   enum mu_iconv_fallback_mode fallback_mode)
                                   MU_DEPRECATED;
  

extern int mu_linelen_filter_create (mu_stream_t *pstream, mu_stream_t stream,
				     size_t limit, int flags);

  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_FILTER_H */

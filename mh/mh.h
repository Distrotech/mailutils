/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <argp.h>
#include <mh_getopt.h>

#include <string.h>

#include <mailutils/parse822.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/registrar.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/address.h>
#include <mailutils/mutil.h>
#include <mailutils/stream.h>
#include <mailutils/filter.h>
#include <mailutils/url.h>
#include <mailutils/attribute.h>

#define MH_FMT_RALIGN 0x1000
#define MH_FMT_ZEROPAD 0x2000
#define MH_WIDTH_MASK  0x0fff

#define MH_SEQUENCES_FILE ".mh_sequences"
#define MH_USER_PROFILE ".mh_profile"
#define MH_GLOBAL_PROFILE "mh-profile"

enum mh_opcode
{
  /* 0. Stop. Format: mhop_stop */
  mhop_stop,
  /* 1. Branch.
     Format: mhop_branch offset */
  mhop_branch,
  /* 2. Assign to numeric register
     Format: mhop_num_asgn number */
  mhop_num_asgn,
  /* 3. Assign to string register
     Format: mhop_str_asgn length string */
  mhop_str_asgn,
  /* 4. Numeric arg follows.
     Format: mhop_num_arg number */
  mhop_num_arg,
  /* 5. String arg follows.
     Format: mhop_str_arg length string */
  mhop_str_arg,
  /* 6. Branch if reg_num is zero.
     Format: mhop_num_branch dest-off */
  mhop_num_branch,
  /* 7. Branch if reg_str is zero.
     Format: mhop_str_branch dest-off */
  mhop_str_branch,
  /* 8. Set str to the value of the header component
     Format: mhop_header */
  mhop_header,

  /* 9. Read message body contents into str.
     Format: mhop_body */
  mhop_body,
  
  /* 10. Call a function.
     Format: mhop_call function-pointer */
  mhop_call,

  /* 11. assign reg_num to arg_num */
  mhop_num_to_arg,

  /* 12. assign reg_str to arg_str */
  mhop_str_to_arg,

  /* 13. Convert arg_str to arg_num */
  mhop_str_to_num,
  
  /* 14. Convert arg_num to arg_str */
  mhop_num_to_str,

  /* 15. Print reg_num */
  mhop_num_print,

  /* 16. Print reg_str */
  mhop_str_print,

  /* 17. Set format specification.
     Format: mhop_fmtspec number */
  mhop_fmtspec
};    

enum mh_type
{
  mhtype_none,
  mhtype_num,
  mhtype_str
};

typedef int mh_opcode_t;
typedef struct mh_format mh_format_t;
struct mh_format
{
  size_t progsize;          /* Size of allocated program*/
  mh_opcode_t *prog;        /* Program itself */
};
struct mh_machine;
typedef void (*mh_builtin_fp)(struct mh_machine *);

typedef struct mh_builtin mh_builtin_t;

struct mh_builtin
{
  char *name;
  mh_builtin_fp fun;
  int type;
  int argtype;
  int optarg;
};

extern char *current_folder;
extern size_t current_message;
extern char mh_list_format[];
extern header_t ctx_header;
extern header_t profile_header;

void mh_init (void);
void mh_read_profile (void);
void mh_save_context (void);
int mh_read_context_file (char *path, header_t *header);
int mh_write_context_file (char *path, header_t header);
int mh_read_formfile (char *name, char **pformat);

char * mh_profile_value (char *name, char *defval);

int mh_getyn (const char *fmt, ...);
int mh_check_folder (char *pathname);

int mh_format (mh_format_t *fmt, message_t msg, size_t msgno,
	       char *buffer, size_t bufsize);
int mh_format_parse (char *format_str, mh_format_t *fmt);
void mh_format_free (mh_format_t *fmt);
mh_builtin_t *mh_lookup_builtin (char *name, int *rest);

void mh_error(const char *fmt, ...);

FILE *mh_audit_open (char *name, mailbox_t mbox);
void mh_audit_close (FILE *fp);

void *xmalloc (size_t);
void *xrealloc (void *, size_t);
     

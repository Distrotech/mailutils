/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <unistd.h>
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
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/argcv.h>

#include <mu_asprintf.h>
#include <getline.h>

#define MH_FMT_RALIGN 0x1000
#define MH_FMT_ZEROPAD 0x2000
#define MH_WIDTH_MASK  0x0fff

#define MH_SEQUENCES_FILE ".mh_sequences"
#define MH_USER_PROFILE ".mh_profile"
#define MH_GLOBAL_PROFILE "mh-profile"
#define MH_CONTEXT_FILE "context"

#define is_true(arg) ((arg)==NULL||(arg)[0] == 'y')

enum mh_opcode
{
  /* 0. Stop. Format: mhop_stop */
  mhop_stop,
  /* 1. Branch.
     Format: mhop_branch offset */
  mhop_branch,
  /* 2. Assign to numeric register
     Format: mhop_num_asgn  */
  mhop_num_asgn,
  /* 3. Assign to string register
     Format: mhop_str_asgn */
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
  mhop_fmtspec,

};    

enum mh_type
{
  mhtype_none,
  mhtype_num,
  mhtype_str
};

typedef enum mh_opcode mh_opcode_t;

struct mh_machine;
typedef void (*mh_builtin_fp) __P((struct mh_machine *));

typedef union {
  mh_opcode_t opcode;
  mh_builtin_fp builtin;
  int num;
  void *ptr;
  char str[1]; /* Any number of characters follows */
} mh_instr_t;

#define MHI_OPCODE(m) (m).opcode
#define MHI_BUILTIN(m) (m).builtin
#define MHI_NUM(m) (m).num
#define MHI_PTR(m) (m).ptr
#define MHI_STR(m) (m).str

typedef struct mh_format mh_format_t;

struct mh_format
{
  size_t progsize;          /* Size of allocated program*/
  mh_instr_t *prog;         /* Program itself */
};

typedef struct mh_builtin mh_builtin_t;

struct mh_builtin
{
  char *name;
  mh_builtin_fp fun;
  int type;
  int argtype;
  int optarg;
};

typedef struct
{
  char *name;
  header_t header;
} mh_context_t;

typedef struct
{
  size_t count;
  size_t *list;
} mh_msgset_t;

typedef void (*mh_iterator_fp) __P((mailbox_t mbox, message_t msg,
				    size_t num, void *data));

/* Recipient masks */
#define RCPT_NONE 0
#define RCPT_TO   0x0001
#define RCPT_CC   0x0002
#define RCPT_ME   0x0004
#define RCPT_ALL  (RCPT_TO|RCPT_CC|RCPT_ME)

#define RCPT_DEFAULT RCPT_NONE

struct mh_whatnow_env {   /* An environment for whatnow shell */
  char *file;             /* The file being processed */
  char *msg;              /* The original message (if any) */
  char *draftfile;        /* File to preserve the draft into */
  char *draftfolder;
  char *draftmessage;
  char *editor;
  char *prompt;
};

#define DISP_QUIT 0
#define DISP_USE 1
#define DISP_REPLACE 2

extern char *current_folder;
extern size_t current_message;
extern char mh_list_format[];
extern int rcpt_mask;

void mh_init __P((void));
void mh_init2 __P((void));
void mh_read_profile __P((void));
int mh_read_formfile __P((char *name, char **pformat));

char *mh_global_profile_get __P((char *name, char *defval));
int mh_global_profile_set __P((const char *name, const char *value));
char *mh_global_context_get __P((const char *name, const char *defval));
int mh_global_context_set __P((const char *name, const char *value));
char *mh_current_folder __P((void));
char *mh_global_sequences_get __P((const char *name, const char *defval));
int mh_global_sequences_set __P((const char *name, const char *value));
void mh_global_save_state __P((void));

int mh_getyn __P((const char *fmt, ...));
int mh_check_folder __P((char *pathname, int confirm));

int mh_format __P((mh_format_t *fmt, message_t msg, size_t msgno,
		   char *buffer, size_t bufsize));
void mh_format_dump __P((mh_format_t *fmt));
int mh_format_parse __P((char *format_str, mh_format_t *fmt));
void mh_format_free __P((mh_format_t *fmt));
mh_builtin_t *mh_lookup_builtin __P((char *name, int *rest));

void mh_error __P((const char *fmt, ...));

FILE *mh_audit_open __P((char *name, mailbox_t mbox));
void mh_audit_close __P((FILE *fp));

mh_context_t *mh_context_create __P((char *name, int copy));
int mh_context_read __P((mh_context_t *ctx));
int mh_context_write __P((mh_context_t *ctx));
char *mh_context_get_value __P((mh_context_t *ctx, const char *name,
				const char *defval));
int mh_context_set_value __P((mh_context_t *ctx, const char *name,
			      const char *value));

int mh_message_number __P((message_t msg, size_t *pnum));

mailbox_t mh_open_folder __P((const char *folder, int create));

int mh_msgset_parse __P((mailbox_t mbox, mh_msgset_t *msgset,
			 int argc, char **argv, char *def));
int mh_msgset_member __P((mh_msgset_t *msgset, size_t num));
void mh_msgset_reverse __P((mh_msgset_t *msgset));
void mh_msgset_negate __P((mailbox_t mbox, mh_msgset_t *msgset));
int mh_msgset_current __P((mailbox_t mbox, mh_msgset_t *msgset, int index));
void mh_msgset_free __P((mh_msgset_t *msgset));

char *mh_get_dir __P((void));
char *mh_expand_name __P((const char *base, const char *name, int is_folder));

int mh_is_my_name __P((char *name));
char * mh_my_email __P((void));

int mh_iterate __P((mailbox_t mbox, mh_msgset_t *msgset,
		    mh_iterator_fp itr, void *data));

size_t mh_get_message __P((mailbox_t mbox, size_t seqno, message_t *mesg));

int mh_decode_rcpt_flag __P((const char *arg));

void *xmalloc __P((size_t));
void *xrealloc __P((void *, size_t));
     
int mh_spawnp __P((const char *prog, const char *file));
int mh_whatnow __P((struct mh_whatnow_env *wh, int initial_edit));
int mh_disposition __P((const char *filename));

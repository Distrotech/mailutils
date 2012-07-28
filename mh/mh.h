/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2007, 2009-2012 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <mh_getopt.h>
#include <mailutils/io.h>

#include <string.h>

#include <mailutils/alloc.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/parse822.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/registrar.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/address.h>
#include <mailutils/util.h>
#include <mailutils/stream.h>
#include <mailutils/filter.h>
#include <mailutils/url.h>
#include <mailutils/attribute.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/argcv.h>
#include <mailutils/wordsplit.h>
#include <mailutils/debug.h>
#include <mailutils/mailer.h>
#include <mailutils/envelope.h>
#include <mailutils/mime.h>
#include <mailutils/io.h>
#include <mailutils/property.h>
#include <mailutils/prog.h>
#include <mailutils/mh.h>
#include <mailutils/stdstream.h>
#include <mailutils/datetime.h>
#include <mailutils/msgset.h>

#include <mu_umaxtostr.h>

#define MH_FMT_RALIGN  0x1000
#define MH_FMT_ZEROPAD 0x2000
#define MH_FMT_COMPWS  0x4000
#define MH_WIDTH_MASK  0x0fff

#define MH_SEQUENCES_FILE ".mh_sequences"
#define MH_USER_PROFILE ".mh_profile"
#define MH_GLOBAL_PROFILE "mh-profile"
#define MH_CONTEXT_FILE "context"
#define DEFAULT_ALIAS_FILE MHLIBDIR "/MailAliases"

#define is_true(arg) ((arg) == NULL||mu_true_answer_p (arg) == 1)

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

  /* 18. Noop */
  mhop_nop
};    

enum mh_type
{
  mhtype_none,
  mhtype_num,
  mhtype_str
};

typedef enum mh_opcode mh_opcode_t;

struct mh_machine;
typedef void (*mh_builtin_fp) (struct mh_machine *);

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

#define MHA_REQUIRED       0
#define MHA_OPTARG         1
#define MHA_OPT_CLEAR      2

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
  const char *name;
  mu_header_t header;
} mh_context_t;

/* Recipient masks */
#define RCPT_NONE 0
#define RCPT_TO   0x0001
#define RCPT_CC   0x0002
#define RCPT_ME   0x0004
#define RCPT_ALL  (RCPT_TO|RCPT_CC|RCPT_ME)

#define RCPT_DEFAULT RCPT_NONE

struct mh_whatnow_env     /* whatnow shell environment */
{  
  char *file;             /* The file being processed */
  char *msg;              /* File name of the original message (if any) */
  char *draftfile;        /* File to preserve the draft into */
  const char *editor;
  char *prompt;
  char *anno_field;       /* Annotate field to be used */
  mu_list_t anno_list;    /* List of messages (mu_message_t) to annotate */
  mu_mailbox_t mbox;
};

#define DISP_QUIT 0
#define DISP_USE 1
#define DISP_REPLACE 2

#define SEQ_PRIVATE 1
#define SEQ_ZERO    2

extern char mh_list_format[];
extern int rcpt_mask;
extern int mh_mailbox_cur_default;

void mh_init (void);
void mh_init2 (void);
void mh_read_profile (void);
int mh_read_formfile (char *name, char **pformat);
mu_message_t mh_file_to_message (const char *folder, const char *file_name);
mu_message_t mh_stream_to_message (mu_stream_t stream);
void mh_install (char *name, int automode);

mu_property_t mh_read_property_file (char *name, int ro);
void mh_property_merge (mu_property_t dst, mu_property_t src);

#define mh_global_profile_get(name, defval) \
  mu_mhprop_get_value (mu_mh_profile, name, defval)
#define mh_global_profile_set(name, value) \
  mu_property_set_value (mu_mh_profile, name, value, 1))
#define mh_global_profile_iterate(fp, data) \
  mu_mhprop_iterate (mu_mh_profile, fp, data)

#define mh_global_context_get(name, defval) \
  mu_mhprop_get_value (mu_mh_context, name, defval)
#define mh_global_context_set(name, value) \
  mu_property_set_value (mu_mh_context, name, value, 1)
#define mh_global_context_iterate(fp, data) \
  mu_mhprop_iterate (mu_mh_context, fp, data)

const char *mh_set_current_folder (const char *val);
const char *mh_current_folder (void);

mu_property_t mh_mailbox_get_property (mu_mailbox_t mbox);
const char *mh_global_sequences_get (mu_mailbox_t mbox,
				     const char *name, const char *defval);
void mh_global_sequences_set (mu_mailbox_t mbox,
			      const char *name, const char *value);
void mh_global_sequences_iterate (mu_mailbox_t mbox,
				  mu_mhprop_iterator_t fp, void *data);
void mh_global_sequences_drop (mu_mailbox_t mbox);

void mh_global_save_state (void);

int mh_interactive_mode_p (void);
int mh_getyn (const char *fmt, ...) MU_PRINTFLIKE(1,2);
int mh_getyn_interactive (const char *fmt, ...) MU_PRINTFLIKE(1,2);
int mu_vgetans (const char *variants, const char *fmt, va_list ap);
int mu_getans (const char *variants, const char *fmt, ...)
  MU_PRINTFLIKE(2,3);
int mh_check_folder (const char *pathname, int confirm);
int mh_makedir (const char *p);

int mh_format (mh_format_t *fmt, mu_message_t msg, size_t msgno,
	       size_t width, char **pret);
int mh_format_str (mh_format_t *fmt, char *str, size_t width, char **pret);
void mh_format_dump (mh_format_t *fmt);
int mh_format_parse (char *format_str, mh_format_t *fmt);
void mh_format_debug (int val);
void mh_format_free (mh_format_t *fmt);
mh_builtin_t *mh_lookup_builtin (char *name, int *rest);

void mh_error (const char *fmt, ...) MU_PRINTFLIKE(1,2);
void mh_err_memory (int fatal);

FILE *mh_audit_open (char *name, mu_mailbox_t mbox);
void mh_audit_close (FILE *fp);

int mh_message_number (mu_message_t msg, size_t *pnum);

mu_mailbox_t mh_open_folder (const char *folder, int flags);

void mh_msgset_parse (mu_msgset_t *msgset, mu_mailbox_t mbox, 
		      int argc, char **argv, char *def);
void mh_msgset_parse_string (mu_msgset_t *msgset, mu_mailbox_t mbox, 
			     const char *string, char *def);
void mh_msgset_first_current (mu_mailbox_t mbox, mu_msgset_t msgset);
size_t mh_msgset_first (mu_msgset_t msgset);
size_t mh_msgset_first_uid (mu_msgset_t msgset);
int mh_msgset_single_message (mu_msgset_t msgset);

char *mh_get_dir (void);
int mh_find_file (const char *name, char **resolved_name);
char *mh_expand_name (const char *base, const char *name, int is_folder);
void mh_quote (const char *in, char **out);
void mh_expand_aliases (mu_message_t msg, mu_address_t *addr_to,
			mu_address_t *addr_cc,
			mu_address_t *addr_bcc);

int mh_is_my_name (const char *name);
char * mh_my_email (void);

size_t mh_get_message (mu_mailbox_t mbox, size_t seqno, mu_message_t *mesg);

int mh_decode_rcpt_flag (const char *arg);

int mh_draft_message (const char *name, const char *msgspec, char **pname);
     
int mh_spawnp (const char *prog, const char *file);
int mh_whatnow (struct mh_whatnow_env *wh, int initial_edit);
int mh_whatnowproc (struct mh_whatnow_env *wh, int initial_edit,
		    const char *prog);

int mh_disposition (const char *filename);
int mh_usedraft (const char *filename);
int mh_file_copy (const char *from, const char *to);
char *mh_draft_name (void);
char *mh_create_message_id (int);
int mh_whom (const char *filename, int check);
void mh_set_reply_regex (const char *str);
int mh_decode_2047 (char *text, char **decoded_text);
const char *mh_charset (const char *);

int mh_alias_read (char *name, int fail);
int mh_alias_get (const char *name, mu_list_t *return_list);
int mh_alias_get_address (const char *name, mu_address_t *addr, int *incl);
int mh_alias_get_alias (const char *uname, mu_list_t *return_list);
int mh_read_aliases (void);
int mh_alias_expand (const char *str, mu_address_t *paddr, int *incl);

typedef int (*mh_alias_enumerator_t) (char *alias, mu_list_t names, void *data);
void mh_alias_enumerate (mh_alias_enumerator_t fun, void *data);


void mh_annotate (mu_message_t msg, const char *field, const char *text,
		  int date);

#define MHL_DECODE       1
#define MHL_CLEARSCREEN  2
#define MHL_BELL         4
#define MHL_DISABLE_BODY 8

mu_list_t mhl_format_compile (char *name);

int mhl_format_run (mu_list_t fmt, int width, int length, int flags,
		    mu_message_t msg, mu_stream_t output);
void mhl_format_destroy (mu_list_t *fmt);

void mh_seq_add (mu_mailbox_t mbox, const char *name, mu_msgset_t mset,
		 int flags);
int mh_seq_delete (mu_mailbox_t mbox, const char *name, mu_msgset_t mset,
		   int flags);
const char *mh_seq_read (mu_mailbox_t mbox, const char *name, int flags);

void mh_comp_draft (const char *formfile, const char *draftfile);
int check_draft_disposition (struct mh_whatnow_env *wh, int use_draft);

void ali_parse_error (const char *fmt, ...) MU_PRINTFLIKE(1,2); 
void ali_verbatim (int enable);

char *mh_safe_make_file_name (const char *dir, const char *file);
  
void mh_mailbox_get_cur (mu_mailbox_t mbox, size_t *pcur);
void mh_mailbox_set_cur (mu_mailbox_t mbox, size_t cur);

void mh_whatnow_env_from_environ (struct mh_whatnow_env *wh);
void mh_whatnow_env_to_environ (struct mh_whatnow_env *wh);

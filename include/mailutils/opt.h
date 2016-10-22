/* opt.h -- general-purpose command line option parser 
   Copyright (C) 2016 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   GNU Mailutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MAILUTILS_OPT_H
#define _MAILUTILS_OPT_H
#include <stdio.h>
#include <mailutils/types.h>
#include <mailutils/list.h>
#include <mailutils/util.h>
#include <mailutils/cctype.h>

extern char *mu_program_name;
extern char *mu_full_program_name;

void mu_set_program_name (char const *arg);

#define MU_OPTION_DEFAULT        0
#define MU_OPTION_ARG_OPTIONAL   0x01
#define MU_OPTION_HIDDEN         0x02
#define MU_OPTION_ALIAS          0x04
#define MU_OPTION_IMMEDIATE      0x08

struct mu_parseopt;

struct mu_option
{
  char *opt_long;         /* Long option name */
  int opt_short;          /* Short option character */
  char *opt_arg;          /* Argument name */
  int opt_flags;          /* Flags (see above) */  
  char *opt_doc;          /* Human-readable documentation */
  mu_c_type_t opt_type;   /* Option type */
  void *opt_ptr;          /* Data pointer */
  void (*opt_set) (struct mu_parseopt *, struct mu_option *, char const *);
                          /* Function to set the option */
  char const *opt_default;/* Default value */
};

#define MU_OPTION_GROUP(text) { NULL, 0, NULL, 0, text }
#define MU_OPTION_END { NULL, 0, NULL, 0, NULL }

#define MU_OPTION_IS_END(opt)					\
  (!(opt)->opt_long && !(opt)->opt_short && !(opt)->opt_doc)

#define MU_OPTION_IS_OPTION(opt) \
  ((opt)->opt_short || (opt)->opt_long)
#define MU_OPTION_IS_GROUP_HEADER(opt)			\
  (!MU_OPTION_IS_OPTION(opt) && (opt)->opt_doc)
#define MU_OPTION_IS_VALID_SHORT_OPTION(opt) \
  ((opt)->opt_short > 0 && (opt)->opt_short < 127 && \
   (mu_isalnum ((opt)->opt_short) || ((opt)->opt_short == '?')))
#define MU_OPTION_IS_VALID_LONG_OPTION(opt) \
  ((opt)->opt_long != NULL)

typedef struct mu_option_cache *mu_option_cache_ptr_t;

struct mu_option_cache
{
  struct mu_option *cache_opt;
  char const *cache_arg;
};

#define MU_PARSEOPT_DEFAULT        0
/* Don't ignore the first element of ARGV.  By default it is the program
   name */
#define MU_PARSEOPT_ARGV0          0x00000001
/* Ignore command line errors. */
#define MU_PARSEOPT_IGNORE_ERRORS  0x00000002
/* Don't order arguments so that options come first. */
#define MU_PARSEOPT_IN_ORDER       0x00000004
/* Don't provide standard options: -h, --help, --usage, --version */
#define MU_PARSEOPT_NO_STDOPT      0x00000008
/* Don't exit on errors */
#define MU_PARSEOPT_NO_ERREXIT     0x00000010
/* Apply all options immediately */
#define MU_PARSEOPT_IMMEDIATE      0x00000020

/* Don't sort options */
#define MU_PARSEOPT_NO_SORT        0x00001000

#define MU_PARSEOPT_PROG_NAME      0x00002000
#define MU_PARSEOPT_PROG_DOC       0x00004000
#define MU_PARSEOPT_PROG_ARGS      0x00008000
#define MU_PARSEOPT_BUG_ADDRESS    0x00010000
#define MU_PARSEOPT_PACKAGE_NAME   0x00020000
#define MU_PARSEOPT_PACKAGE_URL    0x00040000
#define MU_PARSEOPT_EXTRA_INFO     0x00080000
#define MU_PARSEOPT_EXIT_ERROR     0x00100000
#define MU_PARSEOPT_HELP_HOOK      0x00200000
#define MU_PARSEOPT_DATA           0x00400000
#define MU_PARSEOPT_VERSION_HOOK   0x00800000
#define MU_PARSEOPT_PROG_DOC_HOOK  0x01000000
/* Long options start with single dash.  Disables recognition of traditional
   short options */
#define MU_PARSEOPT_SINGLE_DASH    0x02000000
/* Negation prefix is set */
#define MU_PARSEOPT_NEGATION       0x04000000

/* Reuse mu_parseopt struct initialized previously */
#define MU_PARSEOPT_REUSE          0x80000000
/* Mask for immutable flag bits */
#define MU_PARSEOPT_IMMUTABLE_MASK 0xFFFFF000

struct mu_parseopt
{
  /* Input data: */
  int po_argc;                     /* Number of argiments */
  char **po_argv;                  /* Array of arguments */
  size_t po_optc;                  /* Number of elements in optv */
  struct mu_option **po_optv;      /* Array of ptrs to option structures */ 
  int po_flags;                        

  char *po_negation;               /* Negation prefix for boolean options */
  void *po_data;                   /* Call-specific data */

  int po_exit_error;               /* Exit on error with this code */
  
  /* Informational: */
  char const *po_prog_name;
  char const *po_prog_doc;	
  char const **po_prog_args;
  char const *po_bug_address;
  char const *po_package_name;
  char const *po_package_url;
  char const *po_extra_info;

  void (*po_prog_doc_hook) (struct mu_parseopt *po, mu_stream_t stream);
  void (*po_help_hook) (struct mu_parseopt *po, mu_stream_t stream); 
  void (*po_version_hook) (struct mu_parseopt *po, mu_stream_t stream);
  
  /* Output data */
  int po_ind;                      /* Index of the next option */
  int po_opterr;                   /* Index of the element in po_argv that
				      caused last error, or -1 if no errors */
  mu_list_t po_optlist;
  
  /* Auxiliary data */
  char *po_cur;                    /* Points to the next character */
  int po_chr;                      /* Single-char option */

  char *po_long_opt_start;         /* Character sequence that starts
				      long option */
  
  /* The following two keep the position of the first non-optional argument
     and the number of contiguous non-optional arguments after it.
     Obviously, the following holds true:

         arg_start + arg_count == opt_ind

     If permutation is not allowed (MU_OPTION_PARSE_IN_ORDER flag is set),
     arg_count is always 0.
  */
  int po_arg_start;
  int po_arg_count;

  unsigned po_permuted:1;           /* Whether the arguments were permuted */
};


int mu_parseopt (struct mu_parseopt *p,
		 int argc, char **argv, struct mu_option **optv,
		 int flags);
void mu_parseopt_error (struct mu_parseopt *po, char const *fmt, ...);

int mu_parseopt_apply (struct mu_parseopt *p);
void mu_parseopt_free (struct mu_parseopt *p);

unsigned mu_parseopt_getcolumn (const char *name);

void mu_option_describe_options (mu_stream_t str, struct mu_parseopt *p);
void mu_program_help (struct mu_parseopt *p, mu_stream_t str);
void mu_program_usage (struct mu_parseopt *p, int optsummary, mu_stream_t str);
void mu_program_version (struct mu_parseopt *po, mu_stream_t str);

void mu_option_set_value (struct mu_parseopt *po, struct mu_option *opt,
			  char const *arg);

int mu_option_possible_negation (struct mu_parseopt *po, struct mu_option *opt);

#endif

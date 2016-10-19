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

#ifndef _MAILUTILS_CLI_H
#define _MAILUTILS_CLI_H
#include <stdio.h>
#include <mailutils/types.h>
#include <mailutils/cfg.h>
#include <mailutils/opt.h>

typedef void (*mu_cli_capa_commit_fp) (void *);

struct mu_cli_capa
{
  char *name;
  struct mu_option *opt;
  struct mu_cfg_param *cfg;
  mu_cfg_section_fp parser;
  mu_cli_capa_commit_fp commit;
};

void mu_cli_capa_init (void);
void mu_cli_capa_register (struct mu_cli_capa *capa);
void mu_cli_capa_extend_settings (char const *name, mu_list_t opts,
				  mu_list_t commits);

struct mu_cli_setup
{
  struct mu_option **optv;     /* Command-line options */
  struct mu_cfg_param *cfg;    /* Configuration parameters */
  char *prog_doc;              /* Program documentation string */
  char *prog_args;             /* Program arguments string */
  char const **prog_alt_args;  /* Alternative arguments string */
  char *prog_extra_doc;        /* Extra documentation.  This will be
				  displayed after options. */
  int ex_usage;                /* If not 0, exit code on usage errors */
  int ex_config;               /* If not 0, exit code on configuration
				  errors */
  int inorder;
  void (*prog_doc_hook) (mu_stream_t);
};

void mu_version_func (struct mu_parseopt *po, mu_stream_t stream);
void mu_cli (int argc, char **argv, struct mu_cli_setup *setup,
	     char **capa, void *data,
	     int *ret_argc, char ***ret_argv);

char *mu_site_config_file (void);

void mu_acl_cfg_init (void);

#endif

/* cfg.h -- general-purpose configuration file parser 
   Copyright (C) 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MAILUTILS_CFG_H
#define _MAILUTILS_CFG_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef enum mu_cfg_node_type mu_cfg_node_type_t;
typedef struct mu_cfg_node mu_cfg_node_t;
typedef struct mu_cfg_locus mu_cfg_locus_t;

typedef int (*mu_cfg_lexer_t) (void *ptr);
typedef void (*mu_cfg_perror_t) (void *ptr, const mu_cfg_locus_t *loc,
				 const char *fmt, ...);
typedef void *(*mu_cfg_alloc_t) (size_t size);
typedef void (*mu_cfg_free_t) (void *ptr);

enum mu_cfg_node_type
  {
    mu_cfg_node_undefined,
    mu_cfg_node_tag,
    mu_cfg_node_param
  };

struct mu_cfg_locus
{
  char *file;
  size_t line;
};

struct mu_cfg_node
{
  mu_cfg_node_t *next;
  mu_cfg_locus_t locus;
  enum mu_cfg_node_type type;
  char *tag_name;
  char *tag_label;
  mu_cfg_node_t *node;
};

int mu_cfg_parse (mu_cfg_node_t **ptree,
		  void *data,
		  mu_cfg_lexer_t lexer,
		  mu_cfg_perror_t perror,
		  mu_cfg_alloc_t alloc,
		  mu_cfg_free_t free);

extern mu_cfg_locus_t mu_cfg_locus;
extern int mu_cfg_tie_in;
extern mu_cfg_perror_t mu_cfg_perror; 

#define MU_CFG_ITER_OK   0
#define MU_CFG_ITER_SKIP 1
#define MU_CFG_ITER_STOP 2

typedef int (*mu_cfg_iter_func_t) (const mu_cfg_node_t *node, void *data);

void mu_cfg_destroy_tree (mu_cfg_node_t **tree);

int mu_cfg_preorder (mu_cfg_node_t *node,
		     mu_cfg_iter_func_t fun, mu_cfg_iter_func_t endfun,
		     void *data);
int mu_cfg_postorder (mu_cfg_node_t *node,
		      mu_cfg_iter_func_t fun, mu_cfg_iter_func_t endfun,
		      void *data);

int mu_cfg_find_node (mu_cfg_node_t *tree, const char *path,
		      mu_cfg_node_t **pval);
int mu_cfg_find_node_label (mu_cfg_node_t *tree, const char *path,
			    const char **pval);

/* Table-driven parsing */
enum mu_cfg_param_data_type
  {
    mu_cfg_string,
    mu_cfg_short,
    mu_cfg_ushort,
    mu_cfg_int,
    mu_cfg_uint,
    mu_cfg_long,
    mu_cfg_ulong,
    mu_cfg_size,
    mu_cfg_off,
    mu_cfg_bool,
    mu_cfg_ipv4,
    mu_cfg_cidr,
    mu_cfg_host,
    mu_cfg_callback
  };

typedef int (*mu_cfg_callback_t) (mu_cfg_locus_t *, void *, char *);

struct mu_cfg_param
{
  const char *ident;
  enum mu_cfg_param_data_type type;
  void *data;
  mu_cfg_callback_t callback;
};

enum mu_cfg_section_stage
  {
    mu_cfg_section_start,
    mu_cfg_section_end
  };

typedef int (*mu_cfg_section_fp) (enum mu_cfg_section_stage stage,
				  const mu_cfg_node_t *node,
				  void *section_data, void *call_data);

struct mu_cfg_section
{
  const char *ident;
  mu_cfg_section_fp parser;
  void *data;
  struct mu_cfg_section *subsec;
  struct mu_cfg_param *param;
};

typedef struct mu_cfg_cidr mu_cfg_cidr_t;

struct mu_cfg_cidr
{
  struct in_addr addr;
  unsigned long mask;
};

int mu_cfg_scan_tree (mu_cfg_node_t *node,
		      struct mu_cfg_section *sections,
		      void *data, mu_cfg_perror_t perror,
		      mu_cfg_alloc_t palloc, mu_cfg_free_t pfree);

int mu_cfg_find_section (struct mu_cfg_section *root_sec,
			 const char *path, struct mu_cfg_section **retval);

int mu_config_register_section (const char *parent_path,
				const char *ident,
				mu_cfg_section_fp parser,
				void *data,
				struct mu_cfg_param *param);
int mu_config_register_plain_section (const char *parent_path,
				      const char *ident,
				      struct mu_cfg_param *params);

#endif

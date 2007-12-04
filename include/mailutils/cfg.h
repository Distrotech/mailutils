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

#include <mailutils/list.h>
#include <mailutils/debug.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif  

typedef enum mu_cfg_node_type mu_cfg_node_type_t;
typedef struct mu_cfg_node mu_cfg_node_t;
typedef struct mu_cfg_locus mu_cfg_locus_t;
typedef struct mu_cfg_tree mu_cfg_tree_t;

typedef int (*mu_cfg_lexer_t) (void *ptr, mu_debug_t dbg);
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

struct mu_cfg_tree
{
  mu_cfg_node_t *node;
  mu_debug_t debug;
  mu_cfg_alloc_t alloc;
  mu_cfg_free_t free;
};

int mu_cfg_parse (mu_cfg_tree_t **ptree,
		  void *data,
		  mu_cfg_lexer_t lexer,
		  mu_debug_t debug,
		  mu_cfg_alloc_t alloc,
		  mu_cfg_free_t free);

extern mu_cfg_locus_t mu_cfg_locus;
extern int mu_cfg_tie_in;

void mu_cfg_perror (const mu_cfg_locus_t *, const char *, ...);
void mu_cfg_format_error (mu_debug_t debug, size_t, const char *fmt, ...);

#define MU_CFG_ITER_OK   0
#define MU_CFG_ITER_SKIP 1
#define MU_CFG_ITER_STOP 2

typedef int (*mu_cfg_iter_func_t) (const mu_cfg_node_t *node, void *data);

void mu_cfg_destroy_tree (mu_cfg_tree_t **tree);

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
    mu_cfg_time,
    mu_cfg_bool,
    mu_cfg_ipv4,
    mu_cfg_cidr,
    mu_cfg_host,
    mu_cfg_callback
  };

typedef int (*mu_cfg_callback_t) (mu_debug_t, void *, char *);

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
				  void *section_data,
				  void *call_data,
				  mu_cfg_tree_t *tree);

struct mu_cfg_section
{
  const char *ident;
  mu_cfg_section_fp parser;
  void *data;
  mu_list_t /* of mu_cfg_cont/mu_cfg_section */ subsec;
  mu_list_t /* of mu_cfg_cont/mu_cfg_param */ param;
};

enum mu_cfg_cont_type
  {
    mu_cfg_cont_section,
    mu_cfg_cont_param
  };

struct mu_cfg_cont
{
  enum mu_cfg_cont_type type;
  mu_refcount_t refcount;
  union
  {
    const char *ident;
    struct mu_cfg_section section;
    struct mu_cfg_param param;
  } v;
};

typedef struct mu_cfg_cidr mu_cfg_cidr_t;

struct mu_cfg_cidr
{
  struct in_addr addr;
  unsigned long mask;
};

int mu_config_create_container (struct mu_cfg_cont **pcont,
				enum mu_cfg_cont_type type);
int mu_config_clone_container (struct mu_cfg_cont *cont);
void mu_config_destroy_container (struct mu_cfg_cont **pcont);

int mu_cfg_scan_tree (mu_cfg_tree_t *tree, struct mu_cfg_section *sections,
		      void *call_data);

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

#define MU_PARSE_CONFIG_GLOBAL  0x1
#define MU_PARSE_CONFIG_VERBOSE 0x2
#define MU_PARSE_CONFIG_DUMP    0x4

int mu_parse_config (const char *file, const char *progname,
		     struct mu_cfg_param *progparam, int flags);

int mu_cfg_parse_boolean (const char *str, int *res);

extern int mu_cfg_parser_verbose;

void mu_cfg_format_parse_tree (mu_stream_t stream, struct mu_cfg_tree *tree);
void mu_cfg_format_container (mu_stream_t stream, struct mu_cfg_cont *cont);
void mu_format_config_tree (mu_stream_t stream, const char *progname,
			    struct mu_cfg_param *progparam, int flags);
int mu_parse_config_tree (mu_cfg_tree_t *parse_tree, const char *progname,
			  struct mu_cfg_param *progparam, int flags);

int mu_cfg_tree_create (struct mu_cfg_tree **ptree);
void mu_cfg_tree_set_debug (struct mu_cfg_tree *tree, mu_debug_t debug);
void mu_cfg_tree_set_alloc (struct mu_cfg_tree *tree,
			    mu_cfg_alloc_t alloc, mu_cfg_free_t free);
void *mu_cfg_tree_alloc (struct mu_cfg_tree *tree, size_t size);
void mu_cfg_tree_free (struct mu_cfg_tree *tree, void *mem);
mu_cfg_node_t *mu_cfg_tree_create_node (struct mu_cfg_tree *tree,
					enum mu_cfg_node_type type,
					const mu_cfg_locus_t *loc,
					const char *tag, const char *label,
					mu_cfg_node_t *node);
void mu_cfg_tree_add_node (mu_cfg_tree_t *tree, mu_cfg_node_t *node);

#ifdef __cplusplus
}
#endif

#endif

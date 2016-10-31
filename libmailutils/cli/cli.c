/* cli.c -- Command line interface for GNU Mailutils
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
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <sysexits.h>
#include <mailutils/cfg.h>
#include <mailutils/opt.h>
#include <mailutils/cli.h>
#include <mailutils/list.h>
#include <mailutils/alloc.h>
#include <mailutils/nls.h>
#include <mailutils/errno.h>
#include <mailutils/version.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>
#include <mailutils/io.h>
#include <mailutils/syslog.h>
#include <mailutils/mu_auth.h>

#ifndef MU_SITE_CONFIG_FILE
# define MU_SITE_CONFIG_FILE SYSCONFDIR "/mailutils.rc"
#endif

char *
mu_site_config_file (void)
{
  char *p = getenv ("MU_SITE_CONFIG_FILE");
  if (p)
    return p;
  return MU_SITE_CONFIG_FILE;
}


const char mu_version_copyright[] =
  /* Do *not* mark this string for translation.  %s is a copyright
     symbol suitable for this locale, and %d is the copyright
     year.  */
  "Copyright %s 2007-2016 Free Software Foundation, inc.";

void
mu_version_hook (struct mu_parseopt *po, mu_stream_t stream)
{
#ifdef GIT_DESCRIBE
  mu_stream_printf (stream, "%s (%s) %s [%s]\n",
		    mu_program_name, PACKAGE_NAME, PACKAGE_VERSION,
		    GIT_DESCRIBE);
#else
  mu_stream_printf (stream, "%s (%s) %s\n", mu_program_name,
		    PACKAGE_NAME, PACKAGE_VERSION);
#endif
  /* TRANSLATORS: Translate "(C)" to the copyright symbol
     (C-in-a-circle), if this symbol is available in the user's
     locale.  Otherwise, do not translate "(C)"; leave it as-is.  */
  mu_stream_printf (stream, mu_version_copyright, _("(C)"));
  mu_stream_printf (stream, _("\
\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\nThis is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
"));
}

static char gnu_general_help_url[] =
  N_("General help using GNU software: <http://www.gnu.org/gethelp/>");

static void
extra_help_hook (struct mu_parseopt *po, mu_stream_t stream)
{
  struct mu_cfg_parse_hints *hints = po->po_data;
  struct mu_cli_setup *setup = hints->data;
  mu_stream_printf (stream, "%s\n", _(setup->prog_extra_doc));
}

static void
prog_doc_hook (struct mu_parseopt *po, mu_stream_t stream)
{
  struct mu_cfg_parse_hints *hints = po->po_data;
  struct mu_cli_setup *setup = hints->data;
  setup->prog_doc_hook (stream);
}

static void
change_progname (struct mu_parseopt *po, struct mu_option *opt,
		 char const *arg)
{
  po->po_prog_name = mu_strdup (arg);
}

static void
no_user_config (struct mu_parseopt *po, struct mu_option *opt,
		char const *arg)
{
  struct mu_cfg_parse_hints *hints = po->po_data;
  hints->flags &= ~MU_CFG_PARSE_PROGRAM;
}

static void
no_site_config (struct mu_parseopt *po, struct mu_option *opt,
		char const *arg)
{
  struct mu_cfg_parse_hints *hints = po->po_data;
  hints->flags &= ~MU_CFG_PARSE_SITE_RCFILE;
}

static void
config_file (struct mu_parseopt *po, struct mu_option *opt,
	     char const *arg)
{
  struct mu_cfg_parse_hints *hints = po->po_data;
  hints->flags |= MU_CFG_PARSE_CUSTOM_RCFILE;
  hints->custom_rcfile = mu_strdup (arg);
}

static void
config_verbose (struct mu_parseopt *po, struct mu_option *opt,
		char const *arg)
{
  struct mu_cfg_parse_hints *hints = po->po_data;
  if (hints->flags & MU_PARSE_CONFIG_VERBOSE)
    hints->flags |= MU_PARSE_CONFIG_DUMP;
  else
    hints->flags |= MU_PARSE_CONFIG_VERBOSE;
}

static void
config_lint (struct mu_parseopt *po, struct mu_option *opt,
		char const *arg)
{
  struct mu_cfg_parse_hints *hints = po->po_data;
  hints->flags |= MU_PARSE_CONFIG_VERBOSE|MU_PARSE_CONFIG_LINT;
}

static void
param_set (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  struct mu_cfg_parse_hints *hints = po->po_data;
  mu_cfg_node_t *node;
  int rc = mu_cfg_create_subtree (arg, &node);
  if (rc)
    mu_parseopt_error (po, "%s: cannot create node: %s",
		       arg, mu_strerror (rc));
  if (!(hints->flags & MU_CFG_APPEND_TREE))
    {
      mu_cfg_tree_create (&hints->append_tree);
      hints->flags |= MU_CFG_APPEND_TREE;
    }
  mu_cfg_tree_add_node (hints->append_tree, node);
}

struct mu_option mu_common_options[] = {
  MU_OPTION_GROUP(N_("Common options")),
  { "program-name",   0,  N_("NAME"),  MU_OPTION_IMMEDIATE|MU_OPTION_HIDDEN,
    N_("set program name"),
    mu_c_string, NULL, change_progname },

  { "no-user-config", 0,  NULL,        MU_OPTION_IMMEDIATE,
    N_("do not load user configuration file"),
    mu_c_string, NULL, no_user_config },
  { "no-user-rcfile", 0,  NULL,        MU_OPTION_ALIAS },
  
  { "no-site-config", 0,  NULL,        MU_OPTION_IMMEDIATE,
    N_("do not load site-wide configuration file"),
    mu_c_string, NULL, no_site_config },
  { "no-site-rcfile", 0,  NULL,        MU_OPTION_ALIAS },
  
  { "config-file",    0,  N_("FILE"),  MU_OPTION_IMMEDIATE,
    N_("load this configuration file"),
    mu_c_string, NULL, config_file },
  { "rcfile",         0,  NULL,        MU_OPTION_ALIAS },
  
  { "config-verbose", 0,  NULL,        MU_OPTION_IMMEDIATE,
    N_("verbosely log parsing of the configuration files"),
    mu_c_string, NULL, config_verbose },
  { "rcfile-verbose", 0,  NULL,        MU_OPTION_ALIAS },
  
  { "config-lint",    0,  NULL,        MU_OPTION_IMMEDIATE,
    N_("check configuration file syntax and exit"),
    mu_c_string, NULL, config_lint },
  { "rcfile-lint",    0,  NULL,        MU_OPTION_ALIAS },

  { "set",            0,  N_("PARAM=VALUE"), MU_OPTION_IMMEDIATE,
    N_("set configuration parameter"),
    mu_c_string, NULL, param_set },
  
  MU_OPTION_END
};

static void
show_comp_defaults (struct mu_parseopt *po, struct mu_option *opt,
		    char const *unused)
{
  mu_print_options ();
  exit (0);
}

static void
show_config_help (struct mu_parseopt *po, struct mu_option *opt,
		  char const *unused)
{
  struct mu_cfg_parse_hints *hints = po->po_data;

  char *comment;
  mu_stream_t stream;
  struct mu_cfg_cont *cont;
  static struct mu_cfg_param dummy_include_param[] = {
    { "include", mu_c_string, NULL, 0, NULL,
      N_("Include contents of the given file.  If a directory is given, "
	 "include contents of the file <file>/<program>, where "
	 "<program> is the name of the program.  This latter form is "
	 "allowed only in the site-wide configuration file."),
      N_("file-or-directory") },
    { NULL }
  };

  mu_stdio_stream_create (&stream, MU_STDOUT_FD, 0);

  mu_asprintf (&comment,
	       "Configuration file structure for %s utility.",
	       po->po_prog_name);
  mu_cfg_format_docstring (stream, comment, 0);
  free (comment);
  
  mu_asprintf (&comment,
	       "For use in global configuration file (%s), enclose it "
	       "in `program %s { ... };",
	       mu_site_config_file (),
	       po->po_prog_name);		   
  mu_cfg_format_docstring (stream, comment, 0);
  free (comment);

  /* FIXME: %s should be replaced by the canonical utility name */
  mu_asprintf (&comment, "For more information, use `info %s'.",
	       po->po_prog_name);
  mu_cfg_format_docstring (stream, comment, 0);
  free (comment);

  cont = mu_config_clone_root_container ();
  mu_config_container_register_section (&cont, NULL, NULL, NULL, NULL,
					dummy_include_param, NULL);
  if (hints->data)
    {
      struct mu_cli_setup *setup = hints->data;
      mu_config_container_register_section (&cont, NULL, NULL, NULL, NULL,
					    setup->cfg, NULL);
    }
  
  mu_cfg_format_container (stream, cont);
  mu_config_destroy_container (&cont);
      
  mu_stream_destroy (&stream);
  exit (0);
}  

struct mu_option mu_extra_help_options[] = {
  MU_OPTION_GROUP (N_("Informational options")),
  { "show-config-options",   0,  NULL,  MU_OPTION_IMMEDIATE,
    N_("show compilation options"),
    mu_c_string, NULL, show_comp_defaults },
  { "config-help",           0,  NULL,  MU_OPTION_IMMEDIATE,
    N_("show configuration file summary"),
    mu_c_string, NULL, show_config_help },
  MU_OPTION_END
};


static int
add_opt_group (void *item, void *data)
{
  struct mu_parseopt *po = data;
  struct mu_option *opt = item;
  po->po_optv[po->po_optc++] = opt;
  return 0;
}

/* Build the list of option groups and configuration sections */
static struct mu_option **
init_options (char **capa, struct mu_cli_setup *setup,
	      mu_list_t *ret_comlist)
{
  size_t i, s;
  mu_list_t oplist;
  mu_list_t comlist;
  struct mu_parseopt po;
  
  mu_list_create (&oplist);
  if (setup->optv)
    {
      for (i = 0; setup->optv[i]; i++)
	mu_list_append (oplist, setup->optv[i]);
    }

  mu_list_create (&comlist);
  mu_auth_extend_settings (oplist, comlist);
  if (capa)
    {
      for (i = 0; capa[i]; i++)
	mu_cli_capa_extend_settings (capa[i], oplist, comlist);
    }

  *ret_comlist = comlist;
  
  mu_list_append (oplist, mu_common_options);
  mu_list_append (oplist, mu_extra_help_options);
  
  mu_list_count (oplist, &s);

  po.po_optv = mu_calloc (s + 1, sizeof (po.po_optv[0]));
  po.po_optc = 0;
  mu_list_foreach (oplist, add_opt_group, &po);
  if (po.po_optc != s)
    abort ();
  po.po_optv[po.po_optc] = NULL;
  mu_list_destroy (&oplist);
  return po.po_optv;
}

static int
run_commit (void *item, void *data)
{
  mu_cli_capa_commit_fp commit = item;
  commit (data);
  return 0;
}

#define PRESERVE_FLAGS \
  ( MU_PARSEOPT_NO_SORT					\
    | MU_PARSEOPT_SINGLE_DASH				\
    | MU_PARSEOPT_PACKAGE_NAME				\
    | MU_PARSEOPT_PACKAGE_URL				\
    | MU_PARSEOPT_BUG_ADDRESS				\
    | MU_PARSEOPT_EXTRA_INFO				\
    | MU_PARSEOPT_VERSION_HOOK				\
    | MU_PARSEOPT_NEGATION)

void
mu_cli_ext (int argc, char **argv,
	    struct mu_cli_setup *setup,
	    struct mu_parseopt *pohint,
	    struct mu_cfg_parse_hints *cfhint,
	    char **capa,
	    void *data,
	    int *ret_argc, char ***ret_argv)
{
  struct mu_parseopt po;
  int flags = 0;
  struct mu_cfg_tree *parse_tree = NULL;
  struct mu_cfg_parse_hints hints;
  struct mu_option **optv;
  mu_list_t com_list;
#define DFLARGC 2
  char const *dfl_args[DFLARGC];
  char **args = NULL;
  size_t argcnt;
  
  /* Set up defaults */
  if (setup->ex_usage == 0)
    setup->ex_usage = EX_USAGE;
  if (setup->ex_config == 0)
    setup->ex_config = EX_CONFIG;

  hints = *cfhint;
  hints.data = setup;
  
  /* Set program name */
  if (hints.flags & MU_CFG_PARSE_PROGRAM)
    {
      if (!mu_log_tag)
	mu_log_tag = (char*)hints.program;
    }
  else
    {
      mu_set_program_name (argv[0]);
      if (!mu_log_tag)
	mu_log_tag = (char*)mu_program_name;
      hints.program = (char*) mu_program_name;
      hints.flags |= MU_CFG_PARSE_PROGRAM;
    }

  /* Initialize standard streams */
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);

  /* Initialize standard capabilities */
  mu_cli_capa_init ();

  /* Initialize po */
  
  if (setup->prog_doc)
    {
      po.po_prog_doc = setup->prog_doc;
      flags |= MU_PARSEOPT_PROG_DOC;
    }
  else if (pohint->po_flags & MU_PARSEOPT_PROG_DOC)
    {
      po.po_prog_doc = pohint->po_prog_doc;
      flags |= MU_PARSEOPT_PROG_DOC;
    }

  if (setup->prog_args)
    {
      size_t i;
      argcnt = 1;

      if (setup->prog_alt_args)
	{
	  for (i = 0; setup->prog_alt_args[i]; i++)
	    argcnt++;
	}

      if (argcnt < DFLARGC)
	po.po_prog_args = dfl_args;
      else
	{
	  args = mu_calloc (argcnt + 1, sizeof (args[0]));
	  po.po_prog_args = (char const **) args;
	}
      
      po.po_prog_args[0] = setup->prog_args;
      for (i = 1; i < argcnt; i++)
	po.po_prog_args[i] = setup->prog_alt_args[i-1];
      po.po_prog_args[i] = NULL;

      flags |= MU_PARSEOPT_PROG_ARGS;
    }
  else if (pohint->po_flags & MU_PARSEOPT_PROG_ARGS)
    {
      po.po_prog_args = pohint->po_prog_args;
      flags |= MU_PARSEOPT_PROG_ARGS;
    }

  if (setup->prog_extra_doc)
    {
      po.po_help_hook = extra_help_hook;
      flags |= MU_PARSEOPT_HELP_HOOK;
    }

  if (setup->prog_doc_hook)
    {
      po.po_prog_doc_hook = prog_doc_hook;
      flags |= MU_PARSEOPT_PROG_DOC_HOOK;
    }
  else if (pohint->po_flags & MU_PARSEOPT_PROG_DOC_HOOK)
    {
      po.po_prog_doc_hook = pohint->po_prog_doc_hook;
      flags |= MU_PARSEOPT_PROG_DOC_HOOK;
    }
  
  if (setup->inorder)
    flags |= MU_PARSEOPT_IN_ORDER;
  
  flags |= pohint->po_flags & PRESERVE_FLAGS;

  if (flags & MU_PARSEOPT_PACKAGE_NAME)
    po.po_package_name = pohint->po_package_name;
  if (flags & MU_PARSEOPT_PACKAGE_URL)
    po.po_package_url = pohint->po_package_url;
  if (flags & MU_PARSEOPT_BUG_ADDRESS)
    po.po_bug_address = pohint->po_bug_address;
  if (flags & MU_PARSEOPT_EXTRA_INFO)
    po.po_extra_info = pohint->po_extra_info;
  if (flags & MU_PARSEOPT_VERSION_HOOK)
    po.po_version_hook = pohint->po_version_hook;
  if (flags & MU_PARSEOPT_NEGATION)
    po.po_negation = pohint->po_negation;

  po.po_data = &hints;
  flags |= MU_PARSEOPT_DATA;

  po.po_exit_error = setup->ex_usage;

  optv = init_options (capa, setup, &com_list);

  if (mu_parseopt (&po, argc, argv, optv, flags))
    exit (po.po_exit_error);

  argc -= po.po_arg_start;
  argv += po.po_arg_start;
  
  if (ret_argc)
    {
      *ret_argc = argc;
      *ret_argv = argv;
    }
  else if (argc)
    mu_parseopt_error (&po, "%s", _("unexpected arguments"));

  if (mu_cfg_parse_config (&parse_tree, &hints))
    exit (setup->ex_config);

  if (mu_cfg_tree_reduce (parse_tree, &hints, setup->cfg, data))
    exit (setup->ex_config);

  if (mu_cfg_error_count)
    exit (setup->ex_config);
  
  mu_parseopt_apply (&po);

  mu_list_foreach (com_list, run_commit, NULL);
  mu_list_destroy (&com_list);

  if (hints.flags & MU_PARSE_CONFIG_LINT)
    exit (0);

  mu_cfg_destroy_tree (&parse_tree);
  free (optv);
  
  free (args);

  mu_parseopt_free (&po);  
}

void
mu_cli (int argc, char **argv, struct mu_cli_setup *setup, char **capa,
	void *data,
	int *ret_argc, char ***ret_argv)
{
  struct mu_parseopt pohint;
  struct mu_cfg_parse_hints cfhint;

  pohint.po_flags = 0;
  
  pohint.po_package_name = PACKAGE_NAME;
  pohint.po_flags |= MU_PARSEOPT_PACKAGE_NAME;

  pohint.po_package_url = PACKAGE_URL;
  pohint.po_flags |= MU_PARSEOPT_PACKAGE_URL;

  pohint.po_bug_address = PACKAGE_BUGREPORT;
  pohint.po_flags |= MU_PARSEOPT_BUG_ADDRESS;

  pohint.po_extra_info = gnu_general_help_url;
  pohint.po_flags |= MU_PARSEOPT_EXTRA_INFO;

  pohint.po_version_hook = mu_version_hook;
  pohint.po_flags |= MU_PARSEOPT_VERSION_HOOK;

  cfhint.site_rcfile = mu_site_config_file ();
  cfhint.flags = MU_CFG_PARSE_SITE_RCFILE;
  
  mu_cli_ext (argc, argv, setup, &pohint, &cfhint, capa, data,
	      ret_argc, ret_argv);
}

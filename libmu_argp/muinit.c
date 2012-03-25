/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "cmdline.h"
#include <unistd.h>
#include <stdlib.h>
#include <mailutils/alloc.h>
#include <mailutils/stream.h>
#include <mailutils/io.h>
#include <mailutils/stdstream.h>
#include <string.h>
#ifdef MU_ALPHA_RELEASE
# include <git-describe.h>
#endif
struct mu_cfg_tree *mu_argp_tree;

const char version_etc_copyright[] =
  /* Do *not* mark this string for translation.  %s is a copyright
     symbol suitable for this locale, and %d is the copyright
     year.  */
  "Copyright %s 2010 Free Software Foundation, inc.";

void
mu_program_version_hook (FILE *stream, struct argp_state *state)
{
#ifdef GIT_DESCRIBE
  fprintf (stream, "%s (%s) %s [%s]\n",
	   mu_program_name, PACKAGE_NAME, PACKAGE_VERSION, GIT_DESCRIBE);
#else
  fprintf (stream, "%s (%s) %s\n", mu_program_name,
	   PACKAGE_NAME, PACKAGE_VERSION);
#endif
  /* TRANSLATORS: Translate "(C)" to the copyright symbol
     (C-in-a-circle), if this symbol is available in the user's
     locale.  Otherwise, do not translate "(C)"; leave it as-is.  */
  fprintf (stream, version_etc_copyright, _("(C)"));

  fputs (_("\
\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\nThis is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
"),
	 stream);
}

void
mu_argp_init (const char *vers, const char *bugaddr)
{
  if (vers)
    argp_program_version = vers;
  else
    argp_program_version_hook = mu_program_version_hook;
  argp_program_bug_address = bugaddr ? bugaddr : "<" PACKAGE_BUGREPORT ">";
}

static char *
get_canonical_name ()
{
  char *name;
  size_t len;
  char *p;

  if (!argp_program_version ||
      !(p = strchr (argp_program_version, ' ')))
    return strdup (mu_program_name);
  len = p - argp_program_version;
  name = mu_alloc (len + 1);
  memcpy (name, argp_program_version, len);
  name[len] = 0;
  return name;
}

int mu_help_config_mode;
int mu_rcfile_lint;

int (*mu_app_cfg_verifier) (void) = NULL;

int
mu_app_init (struct argp *myargp, const char **capa,
	     struct mu_cfg_param *cfg_param,
	     int argc, char **argv, int flags, int *pindex, void *data)
{
  int rc, i;
  struct argp *argp;
  struct argp argpnull = { 0 };
  char **excapa;
  struct mu_cfg_tree *parse_tree = NULL;
  
  if (!mu_program_name)
    mu_set_program_name (argv[0]);
  if (!mu_log_tag)
    mu_log_tag = (char*)mu_program_name;
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);
  
  mu_libargp_init ();
  if (capa)
    for (i = 0; capa[i]; i++)
      mu_gocs_register_std (capa[i]); /*FIXME*/
  if (!myargp)
    myargp = &argpnull;
  argp = mu_argp_build (myargp, &excapa);

  mu_cfg_tree_create (&mu_argp_tree);
  rc = argp_parse (argp, argc, argv, flags, pindex, data);
  mu_argp_done (argp);
  if (rc)
    return rc;

  /* Reset program name, it may have been changed using the `--program-name'
     option. */
  mu_set_program_name (program_invocation_name);
  
  mu_libcfg_init (excapa);
  free (excapa);

  if (mu_help_config_mode)
    {
      char *comment;
      char *canonical_name = get_canonical_name ();
      mu_stream_t stream;
      struct mu_cfg_cont *cont;
      static struct mu_cfg_param dummy_include_param[] = {
	{ "include", mu_cfg_string, NULL, 0, NULL,
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
		   mu_program_name);
      mu_cfg_format_docstring (stream, comment, 0);
      free (comment);
      mu_asprintf (&comment,
		   "For use in global configuration file (%s), enclose it "
		   "in `program %s { ... };",
		   mu_site_rcfile,
		   mu_program_name);		   
      mu_cfg_format_docstring (stream, comment, 0);
      free (comment);
      mu_asprintf (&comment, "For more information, use `info %s'.",
		   canonical_name);
      mu_cfg_format_docstring (stream, comment, 0);
      free (comment);

      cont = mu_config_clone_root_container ();
      mu_config_container_register_section (&cont, NULL, NULL, NULL, NULL,
					    dummy_include_param, NULL);
      mu_config_container_register_section (&cont, NULL, NULL, NULL, NULL,
					    cfg_param, NULL);
      mu_cfg_format_container (stream, cont);
      mu_config_destroy_container (&cont);
      
      mu_stream_destroy (&stream);
      exit (0);
    }

  rc = mu_libcfg_parse_config (&parse_tree);
  if (rc == 0)
    {
      struct mu_cfg_parse_hints hints = { MU_PARSE_CONFIG_PLAIN };

      hints.flags |= MU_CFG_PARSE_PROGRAM;
      hints.program = (char*)mu_program_name;

      if (mu_cfg_parser_verbose)
	hints.flags |= MU_PARSE_CONFIG_VERBOSE;
      if (mu_cfg_parser_verbose > 1)
	hints.flags |= MU_PARSE_CONFIG_DUMP;
      mu_cfg_tree_postprocess (mu_argp_tree, &hints);
      mu_cfg_tree_union (&parse_tree, &mu_argp_tree);
      rc = mu_cfg_tree_reduce (parse_tree, &hints, cfg_param, data);
    }
  
  if (mu_rcfile_lint)
    {
      if (rc || mu_cfg_error_count)
	exit (1);
      if (mu_app_cfg_verifier)
	rc = mu_app_cfg_verifier ();
      exit (rc ? 1 : 0);
    }
  
  mu_gocs_flush ();
  mu_cfg_destroy_tree (&mu_argp_tree);
  mu_cfg_destroy_tree (&parse_tree);

  return !!(rc || mu_cfg_error_count);
}


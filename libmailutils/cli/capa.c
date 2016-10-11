/* capa.c -- CLI capabilities for GNU Mailutils
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
#include <string.h>
#include <mailutils/cli.h>
#include <mailutils/list.h>
#include <mailutils/alloc.h>
#include <mailutils/nls.h>

static mu_list_t capa_list;

static void
capa_free (void *ptr)
{
  struct mu_cli_capa *cp = ptr;
  free (cp->name);
  free (cp);
}   

void
mu_cli_capa_register (struct mu_cli_capa *capa)
{
  struct mu_cli_capa *cp = mu_alloc (sizeof (*cp));
  cp->name = mu_strdup (capa->name);
  cp->opt = capa->opt;
  cp->cfg = capa->cfg;
  cp->parser = capa->parser;
  cp->commit = capa->commit;
  if (!capa_list)
    {
      mu_list_create (&capa_list);
      mu_list_set_destroy_item (capa_list, capa_free);
    }
  mu_list_append (capa_list, cp);
}

struct capa_apply
{
  char const *name;
  mu_list_t opts;
  mu_list_t commits;
  int found;
};

static int
capa_apply (void *item, void *data)
{
  struct mu_cli_capa *cp = item;
  struct capa_apply *ap = data;

  if (strcmp (cp->name, ap->name) == 0)
    {
      ap->found = 1;
      if (cp->opt)
        mu_list_append (ap->opts, cp->opt);
      if (cp->commit)
        mu_list_append (ap->commits, cp->commit);
      if (cp->parser || cp->cfg)
        mu_config_root_register_section (NULL, cp->name, NULL,
				         cp->parser, cp->cfg);
    }
  return 0;
}

void
mu_cli_capa_apply (char const *name, mu_list_t opts, mu_list_t commits)
{
  struct capa_apply app;
  app.name = name;
  app.opts = opts;
  app.commits = commits;
  app.found = 0;
  mu_list_foreach (capa_list, capa_apply, &app);
  if (!app.found)
    mu_error (_("INTERNAL ERROR at %s:%d: unknown standard capability `%s'"),
	      __FILE__, __LINE__, name);
}

    

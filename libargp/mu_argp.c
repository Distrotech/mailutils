/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2004, 2005, 2006,
   2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "cmdline.h"

char *mu_license_text =
 N_("   GNU Mailutils is free software; you can redistribute it and/or modify\n"
    "   it under the terms of the GNU General Public License as published by\n"
    "   the Free Software Foundation; either version 3 of the License, or\n"
    "   (at your option) any later version.\n"
    "\n"
    "   GNU Mailutils is distributed in the hope that it will be useful,\n"
    "   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "   GNU General Public License for more details.\n"
    "\n"
    "   You should have received a copy of the GNU General Public License along\n"
    "   with GNU Mailutils; if not, write to the Free Software Foundation,\n"
    "   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n"
    "\n"
    "\n"
);

static char *mu_conf_option[] = {
  "VERSION=" VERSION,
#ifdef USE_LIBPAM
  "USE_LIBPAM",
#endif
#ifdef HAVE_LIBLTDL
  "HAVE_LIBLTDL",
#endif
#ifdef WITH_BDB2
  "WITH_BDB2",
#endif
#ifdef WITH_NDBM
  "WITH_NDBM",
#endif
#ifdef WITH_OLD_DBM
  "WITH_OLD_DBM",
#endif
#ifdef WITH_GDBM
  "WITH_GDBM",
#endif
#ifdef WITH_GNUTLS
  "WITH_GNUTLS",
#endif
#ifdef WITH_GSASL
  "WITH_GSASL",
#endif
#ifdef WITH_GSSAPI
  "WITH_GSSAPI",
#endif
#ifdef WITH_GUILE
  "WITH_GUILE",
#endif
#ifdef WITH_PTHREAD
  "WITH_PTHREAD",
#endif
#ifdef WITH_READLINE
  "WITH_READLINE",
#endif
#ifdef HAVE_MYSQL
  "HAVE_MYSQL",
#endif
#ifdef HAVE_PGSQL
  "HAVE_PGSQL",
#endif
#ifdef ENABLE_VIRTUAL_DOMAINS
  "ENABLE_VIRTUAL_DOMAINS",
#endif
#ifdef ENABLE_IMAP
  "ENABLE_IMAP",
#endif
#ifdef ENABLE_POP
  "ENABLE_POP", 
#endif
#ifdef ENABLE_MH
  "ENABLE_MH",
#endif
#ifdef ENABLE_MAILDIR
  "ENABLE_MAILDIR",
#endif
#ifdef ENABLE_SMTP
  "ENABLE_SMTP",
#endif
#ifdef ENABLE_SENDMAIL
  "ENABLE_SENDMAIL",
#endif
#ifdef ENABLE_NNTP
  "ENABLE_NNTP",
#endif
#ifdef ENABLE_RADIUS
  "ENABLE_RADIUS",
#endif
#ifdef WITH_INCLUDED_LIBINTL
  "WITH_INCLUDED_LIBINTL",
#endif
  NULL
};

void
mu_print_options ()
{
  int i;
  
  for (i = 0; mu_conf_option[i]; i++)
    printf ("%s\n", mu_conf_option[i]);
}

const char *
mu_check_option (char *name)
{
  int i;
  
  for (i = 0; mu_conf_option[i]; i++)
    {
      int len;
      char *q, *p = strchr (mu_conf_option[i], '=');
      if (p)
	len = p - mu_conf_option[i];
      else
	len = strlen (mu_conf_option[i]);

      if (strncasecmp (mu_conf_option[i], name, len) == 0) 
	return mu_conf_option[i];
      else if ((q = strchr (mu_conf_option[i], '_')) != NULL
	       && strncasecmp (q + 1, name,
			       len - (q - mu_conf_option[i]) - 1) == 0)
	return mu_conf_option[i];
    }
  return NULL;
}  


/* ************************************************************************* */
/* Capability array and auxiliary functions.                                 */
/* ************************************************************************* */

#define MU_MAX_CAPA 24

struct argp_capa {
  char *capability;
  struct argp_child *child;
} mu_argp_capa[MU_MAX_CAPA] = {
  {NULL,}
};

int
mu_register_argp_capa (const char *name, struct argp_child *child)
{
  int i;
  
  for (i = 0; i < MU_MAX_CAPA; i++)
    if (mu_argp_capa[i].capability == NULL)
      {
	mu_argp_capa[i].capability = strdup (name);
	mu_argp_capa[i].child = child;
	return 0;
      }
  return 1;
}

static struct argp_capa *
find_capa (const char *name)
{
  int i;
  for (i = 0; mu_argp_capa[i].capability; i++)
    if (strcmp (mu_argp_capa[i].capability, name) == 0)
      return &mu_argp_capa[i];
  return NULL;
}

static struct argp *
mu_build_argp (const struct argp *template, const char *capa[])
{
  int n;
  int nchild;
  struct argp_child *ap;
  const struct argp_option *opt;
  struct argp *argp;
  int group = 0;

  /* Count the capabilities. */
  for (n = 0; capa && capa[n]; n++)
    ;
  if (template->children)
    for (; template->children[n].argp; n++)
      ;

  ap = calloc (n + 1, sizeof (*ap));
  if (!ap)
    {
      mu_error (_("Out of memory"));
      abort ();
    }

  /* Copy the template's children. */
  nchild = 0;
  if (template->children)
    for (n = 0; template->children[n].argp; n++, nchild++)
      ap[nchild] = template->children[n];

  /* Find next group number */
  for (opt = template->options;
       opt && ((opt->name && opt->key) || opt->doc); opt++)
    if (opt->group > group)
      group = opt->group;

  group++;
    
  /* Append any capabilities to the children or options, as appropriate. */
  for (n = 0; capa && capa[n]; n++)
    {
      struct argp_capa *cp = find_capa (capa[n]);
      if (!cp)
	{
	  mu_error (_("INTERNAL ERROR: requested unknown argp "
		      "capability %s (please report)"),
		    capa[n]);
	  abort ();
	}
      ap[nchild] = *cp->child;
      ap[nchild].group = group++;
      nchild++;

    }
  ap[nchild].argp = NULL;

  /* Copy the template, and give it the expanded children. */
  argp = malloc (sizeof (*argp));
  if (!argp)
    {
      mu_error (_("Out of memory"));
      abort ();
    }

  memcpy (argp, template, sizeof (*argp));

  argp->children = ap;

  return argp;
}

struct cap_buf
{
  const char **capa;
  size_t numcapa;
  size_t maxcapa;
};

static void
cap_buf_init (struct cap_buf *bp)
{
  bp->numcapa = 0;
  bp->maxcapa = 2;
  bp->capa = calloc (bp->maxcapa, sizeof bp->capa[0]);
  if (!bp->capa)
    {
      mu_error ("%s", mu_strerror (errno));
      abort ();
    }
  bp->capa[0] = NULL;
}

static void
cap_buf_add (struct cap_buf *bp, const char *str)
{
  if (bp->numcapa == bp->maxcapa)
    {
      bp->maxcapa *= 2;
      bp->capa = realloc (bp->capa, bp->maxcapa * sizeof bp->capa[0]);
      if (!bp->capa)
	{
	  mu_error ("%s", mu_strerror (errno));
	  abort ();
	}
    }
  bp->capa[bp->numcapa] = str;
  if (str)
    bp->numcapa++;
}

static void
cap_buf_free (struct cap_buf *bp)
{
  free (bp->capa);
}

static int
argp_reg_action (void *item, void *data)
{
  struct cap_buf *bp = data;
  cap_buf_add (bp, item);
  return 0;
}

struct argp *
mu_argp_build (const struct argp *init_argp)
{
  struct cap_buf cb;
  struct argp *argp;

  cap_buf_init (&cb);
  mu_gocs_enumerate (argp_reg_action, &cb);
  cap_buf_add (&cb, NULL);
  mu_libargp_init ();
  argp = mu_build_argp (init_argp, cb.capa);
  cap_buf_free (&cb);
  return argp;
}

void
mu_argp_done (struct argp *argp)
{
  free ((void*) argp->children);
  free ((void*) argp);
}

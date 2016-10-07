/* help.c -- general-purpose command line option parser
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
#include <mailutils/alloc.h>
#include <mailutils/opt.h>
#include <mailutils/cctype.h>
#include <mailutils/nls.h>

#define LMARGIN 2
#define DESCRCOLUMN 30
#define RMARGIN 79
#define GROUPCOLUMN 2
#define USAGECOLUMN 13

static void
indent (size_t start, size_t col)
{
  for (; start < col; start++)
    putchar (' ');
}

static void
print_option_descr (const char *descr, size_t lmargin, size_t rmargin)
{
  while (*descr)
    {
      size_t s = 0;
      size_t i;
      size_t width = rmargin - lmargin;
      
      for (i = 0; ; i++)
	{
	  if (descr[i] == 0 || descr[i] == ' ' || descr[i] == '\t')
	    {
	      if (i > width)
		break;
	      s = i;
	      if (descr[i] == 0)
		break;
	    }
	}
      fwrite (descr, 1, s, stdout);
      fputc ('\n', stdout);
      descr += s;
      if (*descr)
	{
	  indent (0, lmargin);
	  descr++;
	}
    }
}


static size_t
print_option (struct mu_option **optbuf, size_t optcnt, size_t num,
	      int *argsused)
{
  struct mu_option *opt = optbuf[num];
  size_t next, i;
  int delim;
  int w;
  
  if (MU_OPTION_IS_GROUP_HEADER (opt))
    {
      if (num)
	putchar ('\n');
      indent (0, GROUPCOLUMN);
      print_option_descr (gettext (opt->opt_doc), GROUPCOLUMN, RMARGIN);
      putchar ('\n');
      return num + 1;
    }

  /* count aliases */
  for (next = num + 1;
       next < optcnt && optbuf[next]->opt_flags & MU_OPTION_ALIAS;
       next++);

  if (opt->opt_flags & MU_OPTION_HIDDEN)
    return next;

  w = 0;
  for (i = num; i < next; i++)
    {
      if (MU_OPTION_IS_VALID_SHORT_OPTION (optbuf[i]))
	{
	  if (w == 0)
	    {
	      indent (0, LMARGIN);
	      w = LMARGIN;
	    }
	  else
	    w += printf (", ");
	  w += printf ("-%c", optbuf[i]->opt_short);
	  delim = ' ';
	}
    }
  
  for (i = num; i < next; i++)
    {
      if (MU_OPTION_IS_VALID_LONG_OPTION (optbuf[i]))
	{
	  if (w == 0)
	    {
	      indent (0, LMARGIN);
	      w = LMARGIN;
	    }
	  else
	    w += printf (", ");
	  w += printf ("--%s", optbuf[i]->opt_long);
	  delim = '=';
	}
    }
  
  if (opt->opt_arg)
    {
      *argsused = 1;
      w += printf ("%c%s", delim, gettext (opt->opt_arg));
    }
  if (w >= DESCRCOLUMN)
    {
      putchar ('\n');
      w = 0;
    }
  indent (w, DESCRCOLUMN);
  print_option_descr (gettext (opt->opt_doc), DESCRCOLUMN, RMARGIN);

  return next;
}

void
mu_option_describe_options (struct mu_option **optbuf, size_t optcnt)
{
  unsigned i;
  int argsused = 0;

  for (i = 0; i < optcnt; )
    i = print_option (optbuf, optcnt, i, &argsused);
  putchar ('\n');

  if (argsused)
    {
      print_option_descr (_("Mandatory or optional arguments to long options are also mandatory or optional for any corresponding short options."), 0, RMARGIN);
      putchar ('\n');
    }
}

void
mu_program_help (struct mu_parseopt *po)
{
  printf ("%s", _("Usage:"));
  if (po->po_prog_name)
    printf (" %s", po->po_prog_name);
  printf (" [%s]...", _("OPTION"));
  if (po->po_prog_args)
    printf (" %s", gettext (po->po_prog_args));
  putchar ('\n');
  
  if (po->po_prog_doc)
    print_option_descr (gettext (po->po_prog_doc), 0, RMARGIN);
  putchar ('\n');

  mu_option_describe_options (po->po_optv, po->po_optc);

  if (po->po_help_hook)
    po->po_help_hook (stdout);

  if (po->po_bug_address)
    /* TRANSLATORS: The placeholder indicates the bug-reporting address
       for this package.  Please add _another line_ saying
       "Report translation bugs to <...>\n" with the address for translation
       bugs (typically your translation team's web or email address).  */
    printf (_("Report bugs to %s.\n"), po->po_bug_address);

  if (po->po_package_name && po->po_package_url)
    printf (_("%s home page: <%s>\n"),
	    po->po_package_name, po->po_package_url);
}

static struct mu_option **option_tab;

static int
cmpidx_short (const void *a, const void *b)
{
  unsigned const *ai = (unsigned const *)a;
  unsigned const *bi = (unsigned const *)b;

  return option_tab[*ai]->opt_short - option_tab[*bi]->opt_short;
}
  
static int
cmpidx_long (const void *a, const void *b)
{
  unsigned const *ai = (unsigned const *)a;
  unsigned const *bi = (unsigned const *)b;
  struct mu_option const *ap = option_tab[*ai];
  struct mu_option const *bp = option_tab[*bi];
  return strcmp (ap->opt_long, bp->opt_long);
}

void
mu_program_usage (struct mu_parseopt *po)
{
  unsigned i;
  unsigned n;
  char buf[RMARGIN+1];
  unsigned *idxbuf;
  unsigned nidx;

  struct mu_option **optbuf = po->po_optv;
  size_t optcnt = po->po_optc;
  
#define FLUSH								\
  do									\
    {									\
      buf[n] = 0;							\
      printf ("%s\n", buf);						\
      n = USAGECOLUMN;							\
      memset (buf, ' ', n);						\
    }									\
  while (0)
#define ADDC(c)							        \
  do									\
    {									\
      if (n == RMARGIN) FLUSH;						\
      buf[n++] = c;							\
    }									\
  while (0)

  option_tab = optbuf;
  
  idxbuf = mu_calloc (optcnt, sizeof (idxbuf[0]));

  n = snprintf (buf, sizeof buf, "%s %s ", _("Usage:"),	mu_progname);
  
  /* Print a list of short options without arguments. */
  for (i = nidx = 0; i < optcnt; i++)
    if (MU_OPTION_IS_VALID_SHORT_OPTION (optbuf[i]) && !optbuf[i]->opt_arg)
      idxbuf[nidx++] = i;

  if (nidx)
    {
      qsort (idxbuf, nidx, sizeof (idxbuf[0]), cmpidx_short);

      ADDC ('[');
      ADDC ('-');
      for (i = 0; i < nidx; i++)
	{
	  ADDC (optbuf[idxbuf[i]]->opt_short);
	}
      ADDC (']');
    }

  /* Print a list of short options with arguments. */
  for (i = nidx = 0; i < optcnt; i++)
    {
      if (MU_OPTION_IS_VALID_SHORT_OPTION (optbuf[i]) && optbuf[i]->opt_arg)
	idxbuf[nidx++] = i;
    }

  if (nidx)
    {
      qsort (idxbuf, nidx, sizeof (idxbuf[0]), cmpidx_short);
    
      for (i = 0; i < nidx; i++)
	{
	  struct mu_option *opt = optbuf[idxbuf[i]];
	  const char *arg = gettext (opt->opt_arg);
	  size_t len = 5 + strlen (arg) + 1;
	  
	  if (n + len > RMARGIN) FLUSH;
	  buf[n++] = ' ';
	  buf[n++] = '[';
	  buf[n++] = '-';
	  buf[n++] = opt->opt_short;
	  buf[n++] = ' ';
	  strcpy (&buf[n], arg);
	  n += strlen (arg);
	  buf[n++] = ']';
	}
    }
  
  /* Print a list of long options */
  for (i = nidx = 0; i < optcnt; i++)
    {
      if (MU_OPTION_IS_VALID_LONG_OPTION (optbuf[i]))
	idxbuf[nidx++] = i;
    }

  if (nidx)
    {
      qsort (idxbuf, nidx, sizeof (idxbuf[0]), cmpidx_long);
	
      for (i = 0; i < nidx; i++)
	{
	  struct mu_option *opt = optbuf[idxbuf[i]];
	  const char *arg = opt->opt_arg ? gettext (opt->opt_arg) : NULL;
	  size_t len = 3 + strlen (opt->opt_long)
	                 + (arg ? 1 + strlen (arg) : 0);
	  if (n + len > RMARGIN) FLUSH;
	  buf[n++] = ' ';
	  buf[n++] = '[';
	  buf[n++] = '-';
	  buf[n++] = '-';
	  strcpy (&buf[n], opt->opt_long);
	  n += strlen (opt->opt_long);
	  if (opt->opt_arg)
	    {
	      buf[n++] = '=';
	      strcpy (&buf[n], arg);
	      n += strlen (arg);
	    }
	  buf[n++] = ']';
	}
    }

  FLUSH;
  free (idxbuf);
}


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
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <mailutils/alloc.h>
#include <mailutils/opt.h>
#include <mailutils/cctype.h>
#include <mailutils/nls.h>
#include <mailutils/wordsplit.h>

unsigned short_opt_col = 2;
unsigned long_opt_col = 6;
/*FIXME: doc_opt_col? */
unsigned header_col = 1;
unsigned opt_doc_col = 29;
unsigned usage_indent = 12;
unsigned rmargin = 79;

unsigned dup_args = 0;
unsigned dup_args_note = 1;

enum usage_var_type
  {
    usage_var_column,
    usage_var_bool
  };

static struct usage_var
{
  char *name;
  unsigned *valptr;
  enum usage_var_type type;
} usage_var[] = {
  { "short-opt-col", &short_opt_col, usage_var_column },
  { "header-col",    &header_col,    usage_var_column },
  { "opt-doc-col",   &opt_doc_col,   usage_var_column },
  { "usage-indent",  &usage_indent,  usage_var_column },
  { "rmargin",       &rmargin,       usage_var_column },
  { "dup-args",      &dup_args,      usage_var_bool },
  { "dup-args-note", &dup_args_note, usage_var_bool },
  { "long-opt-col",  &long_opt_col,  usage_var_column },
  { "doc_opt_col",   NULL,           usage_var_column },
  { NULL }
};

static void
set_usage_var (struct mu_parseopt *po, char const *id)
{
  struct usage_var *p;
  size_t len;
  int boolval = 1;
  
  if (strlen (id) > 3 && memcmp (id, "no-", 3) == 0)
    {
      id += 3;
      boolval = 0;
    }
  len = strcspn (id, "=");

  for (p = usage_var; p->name; p++)
    {
      if (strlen (p->name) == len && memcmp (p->name, id, len) == 0)
	{
	  if (!p->valptr)
	    return;

	  if (p->type == usage_var_bool)
	    {
	      if (id[len])
		{
		  if (po->po_prog_name)
		    fprintf (stderr, "%s: ", po->po_prog_name);
		  fprintf (stderr,
			   "error in ARGP_HELP_FMT: improper usage of [no-]%s\n",
			   id);
		  return;
		}
	      *p->valptr = boolval;
	      return;
	    }
	  
	  if (id[len])
	    {
	      char *endp;
	      unsigned long val;
	      
	      errno = 0;
	      val = strtoul (id + len + 1, &endp, 10);
	      if (errno || *endp)
		{
		  if (po->po_prog_name)
		    fprintf (stderr, "%s: ", po->po_prog_name);
		  fprintf (stderr,
			   "error in ARGP_HELP_FMT: bad value for %s\n",
			   id);
		}
	      else if (val > UINT_MAX)
		{
		  if (po->po_prog_name)
		    fprintf (stderr, "%s: ", po->po_prog_name);
		  fprintf (stderr,
			   "error in ARGP_HELP_FMT: %s value is out of range\n",
			   id);
		}
	      else
		*p->valptr = val;
	    }
	  else
	    {
	      if (po->po_prog_name)
		fprintf (stderr, "%s: ", po->po_prog_name);
	      fprintf (stderr,
		       "%s: ARGP_HELP_FMT parameter requires a value\n",
		       id);
	      return;
	    }
	  return;
	}
    }

  if (po->po_prog_name)
    fprintf (stderr, "%s: ", po->po_prog_name);
  fprintf (stderr,
	   "%s: Unknown ARGP_HELP_FMT parameter\n",
	   id);
}

static void
init_usage_vars (struct mu_parseopt *po)
{
  char *fmt;
  struct mu_wordsplit ws;
  size_t i;
  
  fmt = getenv ("ARGP_HELP_FMT");
  if (!fmt)
    return;
  ws.ws_delim=",";
  if (mu_wordsplit (fmt, &ws, 
		    MU_WRDSF_DELIM | MU_WRDSF_NOVAR | MU_WRDSF_NOCMD
		    | MU_WRDSF_WS | MU_WRDSF_SHOWERR))
    return;
  for (i = 0; i < ws.ws_wordc; i++)
    {
      set_usage_var (po, ws.ws_wordv[i]);
    }

  mu_wordsplit_free (&ws);
}

static int
indent (int start, int col)
{
  for (; start < col; start++)
    putchar (' ');
  return start;
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
print_opt_arg (struct mu_option *opt, int delim)
{
  int w = 0;
  if (opt->opt_flags & MU_OPTION_ARG_OPTIONAL)
    {
      if (delim == '=')
	w = printf ("[=%s]", gettext (opt->opt_arg));
      else
	w = printf ("[%s]", gettext (opt->opt_arg));
    }
  else
    w = printf ("%c%s", delim, gettext (opt->opt_arg));
  return w;
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
      if (opt->opt_doc[0])
	{
	  indent (0, header_col);
	  print_option_descr (gettext (opt->opt_doc), header_col, rmargin);
	}
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
	      indent (0, short_opt_col);
	      w = short_opt_col;
	    }
	  else
	    w += printf (", ");
	  w += printf ("-%c", optbuf[i]->opt_short);
	  delim = ' ';
	  if (opt->opt_arg && dup_args)
	    w += print_opt_arg (opt, delim);
	}
    }

  for (i = num; i < next; i++)
    {
      if (MU_OPTION_IS_VALID_LONG_OPTION (optbuf[i]))
	{
	  if (w == 0)
	    {
	      indent (0, short_opt_col);
	      w = short_opt_col;
	    }
	  else
	    w += printf (", ");
	  if (i == num)
	    w = indent (w, long_opt_col);
  	  w += printf ("--%s", optbuf[i]->opt_long);
	  delim = '=';
	  if (opt->opt_arg && dup_args)
	    w += print_opt_arg (opt, delim);
	}
    }
  
  if (opt->opt_arg)
    {
      *argsused = 1;
      if (!dup_args)
	w += print_opt_arg (opt, delim);
    }
  if (w >= opt_doc_col)
    {
      putchar ('\n');
      w = 0;
    }
  indent (w, opt_doc_col);
  print_option_descr (gettext (opt->opt_doc), opt_doc_col, rmargin);

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

  if (argsused && dup_args_note)
    {
      print_option_descr (_("Mandatory or optional arguments to long options are also mandatory or optional for any corresponding short options."), 0, rmargin);
      putchar ('\n');
    }
}

void
mu_program_help (struct mu_parseopt *po)
{
  init_usage_vars (po);

  printf ("%s", _("Usage:"));
  if (po->po_prog_name)
    printf (" %s", po->po_prog_name);
  printf (" [%s]...", _("OPTION"));
  if (po->po_prog_args)
    printf (" %s", gettext (po->po_prog_args));
  putchar ('\n');
  
  if (po->po_prog_doc)
    print_option_descr (gettext (po->po_prog_doc), 0, rmargin);
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
  if (po->po_flags & MU_PARSEOPT_EXTRA_INFO)
    print_option_descr (po->po_extra_info, 0, rmargin);
}

static struct mu_option **option_tab;

static int
cmpidx_short (const void *a, const void *b)
{
  unsigned const *ai = (unsigned const *)a;
  unsigned const *bi = (unsigned const *)b;
  int ac = option_tab[*ai]->opt_short;
  int bc = option_tab[*bi]->opt_short;
  int d;
  
  if (mu_isalpha (ac))
    {
      if (!mu_isalpha (bc))
	return -1;
    }
  else if (mu_isalpha (bc))
    return 1;

  d = mu_tolower (ac) - mu_tolower (bc);
  if (d == 0)
    d =  mu_isupper (ac) ? 1 : -1;
  return d;
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
  char buf[rmargin+1];
  unsigned *idxbuf;
  unsigned nidx;

  struct mu_option **optbuf = po->po_optv;
  size_t optcnt = po->po_optc;
  
#define FLUSH								\
  do									\
    {									\
      buf[n] = 0;							\
      printf ("%s\n", buf);						\
      n = usage_indent;						\
      if (n) memset (buf, ' ', n);					\
    }									\
  while (0)
#define ADDC(c)							        \
  do									\
    {									\
      if (n == rmargin) FLUSH;						\
      buf[n++] = c;							\
    }									\
  while (0)

  init_usage_vars (po);

  option_tab = optbuf;
  
  idxbuf = mu_calloc (optcnt, sizeof (idxbuf[0]));

  n = snprintf (buf, sizeof buf, "%s %s ", _("Usage:"),	po->po_prog_name);
  
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
	  
	  if (n + len >= rmargin)
	    FLUSH;
	  else
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
	  size_t len = 5 + strlen (opt->opt_long)
	                 + (arg ? 1 + strlen (arg) : 0);
	  if (n + len >= rmargin)
	    FLUSH;
	  else
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

  if (po->po_flags & MU_PARSEOPT_PROG_ARGS)
    {
      char const *p = po->po_prog_args;

      if (n + 1 >= rmargin)
	FLUSH;
      buf[n++] = ' ';
      
      while (*p)
	{
	  size_t len = strcspn (p, "  \t\n");
	  if (len == 0)
	    len = 1;
	  if (n + len >= rmargin)
	    FLUSH;
	  else
	    {
	      memcpy (buf + n, p, len);
	      p += len;
	      n += len;
	    }
	}
    }

  FLUSH;
  free (idxbuf);
}


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
#include <mailutils/stream.h>

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

unsigned
mu_parseopt_getcolumn (const char *name)
{
  struct usage_var *p;
  unsigned retval = 0;
  for (p = usage_var; p->name; p++)
    {
      if (strcmp (p->name, name) == 0)
	{
	  if (p->valptr)
	    retval = *p->valptr;
	  break;
	}
    }
  return retval;
}

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
  ws.ws_delim = ",";
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

static void
set_margin (mu_stream_t str, unsigned margin)
{
  mu_stream_ioctl (str, MU_IOCTL_WORDWRAPSTREAM,
		   MU_IOCTL_WORDWRAP_SET_MARGIN,
		   &margin);
}
static void
set_next_margin (mu_stream_t str, unsigned margin)
{
  mu_stream_ioctl (str, MU_IOCTL_WORDWRAPSTREAM,
		   MU_IOCTL_WORDWRAP_SET_NEXT_MARGIN,
		   &margin);
}
static void
get_offset (mu_stream_t str, unsigned *offset)
{
  mu_stream_ioctl (str, MU_IOCTL_WORDWRAPSTREAM,
		   MU_IOCTL_WORDWRAP_GET_COLUMN,
		   offset);
}

static void
print_opt_arg (mu_stream_t str, struct mu_option *opt, int delim)
{
  if (opt->opt_flags & MU_OPTION_ARG_OPTIONAL)
    {
      if (delim == '=')
	mu_stream_printf (str, "[=%s]", gettext (opt->opt_arg));
      else
	mu_stream_printf (str, "[%s]", gettext (opt->opt_arg));
    }
  else
    mu_stream_printf (str, "%c%s", delim, gettext (opt->opt_arg));
}

static size_t
print_option (mu_stream_t str,
	      struct mu_option **optbuf, size_t optcnt, size_t num,
	      int *argsused)
{
  struct mu_option *opt = optbuf[num];
  size_t next, i;
  int delim;
  int first_option = 1;
  int first_long_option = 1;
  
  if (MU_OPTION_IS_GROUP_HEADER (opt))
    {
      if (num)
	mu_stream_printf (str, "\n");
      if (opt->opt_doc[0])
	{
	  set_margin (str, header_col);
	  mu_stream_printf (str, "%s", gettext (opt->opt_doc));
	}
      return num + 1;
    }

  /* count aliases */
  for (next = num + 1;
       next < optcnt && optbuf[next]->opt_flags & MU_OPTION_ALIAS;
       next++);

  if (opt->opt_flags & MU_OPTION_HIDDEN)
    return next;

  set_margin (str, short_opt_col); 
  for (i = num; i < next; i++)
    {
      if (MU_OPTION_IS_VALID_SHORT_OPTION (optbuf[i]))
	{
	  if (first_option)
	    first_option = 0;
	  else
	    mu_stream_printf (str, ", ");
	  mu_stream_printf (str, "-%c", optbuf[i]->opt_short);
	  delim = ' ';
	  if (opt->opt_arg && dup_args)
	    print_opt_arg (str, opt, delim);
	}
    }

  for (i = num; i < next; i++)
    {
      if (MU_OPTION_IS_VALID_LONG_OPTION (optbuf[i]))
	{
	  if (first_option)
	    first_option = 0;
	  else
	    mu_stream_printf (str, ", ");

	  if (first_long_option)
	    {
	      unsigned off;
	      get_offset (str, &off);
	      if (off < long_opt_col)
		set_margin (str, long_opt_col);
	      first_long_option = 0;
	    }
	  
  	  mu_stream_printf (str, "--%s", optbuf[i]->opt_long);
	  delim = '=';
	  if (opt->opt_arg && dup_args)
	    print_opt_arg (str, opt, delim);
	}
    }
  
  if (opt->opt_arg)
    {
      *argsused = 1;
      if (!dup_args)
	print_opt_arg (str, opt, delim);
    }

  set_margin (str, opt_doc_col);
  mu_stream_printf (str, "%s\n", gettext (opt->opt_doc));

  return next;
}

void
mu_option_describe_options (mu_stream_t str,
			    struct mu_option **optbuf, size_t optcnt)
{
  unsigned i;
  int argsused = 0;

  for (i = 0; i < optcnt; )
    i = print_option (str, optbuf, optcnt, i, &argsused);
  mu_stream_printf (str, "\n");

  if (argsused && dup_args_note)
    {
      set_margin (str, 0);
      mu_stream_printf (str, "%s\n\n",
			_("Mandatory or optional arguments to long options are also mandatory or optional for any corresponding short options."));
    }
}

static void print_program_usage (struct mu_parseopt *po, int optsum,
				 mu_stream_t str);

void
mu_program_help (struct mu_parseopt *po, mu_stream_t outstr)
{
  mu_stream_t str;
  int rc;
  
  init_usage_vars (po);

  rc = mu_wordwrap_stream_create (&str, outstr, 0, rmargin);
  if (rc)
    {
      abort ();//FIXME
    }

  print_program_usage (po, 0, str);
  
  if (po->po_prog_doc)
    {
      set_margin (str, 0);
      mu_stream_printf (str, "%s\n", gettext (po->po_prog_doc));
    }
  mu_stream_printf (str, "\n");
  
  mu_option_describe_options (str, po->po_optv, po->po_optc);

  if (po->po_help_hook)
    {
      po->po_help_hook (po, str);
      mu_stream_printf (str, "\n");
    }
  
  set_margin (str, 0);
  if (po->po_bug_address)
    /* TRANSLATORS: The placeholder indicates the bug-reporting address
       for this package.  Please add _another line_ saying
       "Report translation bugs to <...>\n" with the address for translation
       bugs (typically your translation team's web or email address).  */
    mu_stream_printf (str, _("Report bugs to <%s>.\n"), po->po_bug_address);

  if (po->po_package_name && po->po_package_url)
    mu_stream_printf (str, _("%s home page: <%s>\n"),
		      po->po_package_name, po->po_package_url);
  if (po->po_flags & MU_PARSEOPT_EXTRA_INFO)
    mu_stream_printf (str, "%s\n", _(po->po_extra_info));

  mu_stream_destroy (&str);
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

static void
option_summary (struct mu_parseopt *po, mu_stream_t str)
{
  unsigned i;
  unsigned *idxbuf;
  unsigned nidx;

  struct mu_option **optbuf = po->po_optv;
  size_t optcnt = po->po_optc;
  
  option_tab = optbuf;
  
  idxbuf = mu_calloc (optcnt, sizeof (idxbuf[0]));

  /* Print a list of short options without arguments. */
  for (i = nidx = 0; i < optcnt; i++)
    if (MU_OPTION_IS_VALID_SHORT_OPTION (optbuf[i]) && !optbuf[i]->opt_arg)
      idxbuf[nidx++] = i;

  if (nidx)
    {
      qsort (idxbuf, nidx, sizeof (idxbuf[0]), cmpidx_short);
      mu_stream_printf (str, "[-");
      for (i = 0; i < nidx; i++)
	{
	  mu_stream_printf (str, "%c", optbuf[idxbuf[i]]->opt_short);
	}
      mu_stream_printf (str, "%c", ']');
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
	  if (opt->opt_flags & MU_OPTION_ARG_OPTIONAL)
	    mu_stream_printf (str, " [-%c[%s]]", opt->opt_short, arg);
	  else
	    mu_stream_printf (str, " [-%c %s]", opt->opt_short, arg);
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

	  mu_stream_printf (str, " [--%s", opt->opt_long);
	  if (opt->opt_arg)
	    {
	      if (opt->opt_flags & MU_OPTION_ARG_OPTIONAL)
		mu_stream_printf (str, "[=%s]", arg);
	      else
		mu_stream_printf (str, "=%s", arg);
	    }
	  mu_stream_printf (str, "%c", ']');
	}
    }

  free (idxbuf);
}  

static void
print_program_usage (struct mu_parseopt *po, int optsum, mu_stream_t str)
{
  char const *usage_text;
  char const **arg_text;
  size_t i;
  
  usage_text = _("Usage:");

  if (po->po_flags & MU_PARSEOPT_PROG_ARGS)
    arg_text = po->po_prog_args;
  else
    arg_text = NULL;
  i = 0;
  
  do
    {
      mu_stream_printf (str, "%s %s ", usage_text, po->po_prog_name);
      set_next_margin (str, usage_indent);

      if (optsum)
	{
	  option_summary (po, str);
	  optsum = 0;
	}
      else
	mu_stream_printf (str, "[%s]...", _("OPTION"));

      if (arg_text)
	{
	  mu_stream_printf (str, " %s\n", gettext (arg_text[i]));
	  if (i == 0)
	    usage_text = _("or: ");
	  set_margin (str, 2);
	  i++;
	}
      else
	mu_stream_flush (str);
    }
  while (arg_text && arg_text[i]);
}

void
mu_program_usage (struct mu_parseopt *po, int optsum, mu_stream_t outstr)
{
  int rc;
  mu_stream_t str;
  
  init_usage_vars (po);

  rc = mu_wordwrap_stream_create (&str, outstr, 0, rmargin);
  if (rc)
    {
      abort ();//FIXME
    }  
  print_program_usage (po, optsum, str);
  mu_stream_destroy (&str);
}

void
mu_program_version (struct mu_parseopt *po, mu_stream_t outstr)
{
  int rc;
  mu_stream_t str;
  
  init_usage_vars (po);

  rc = mu_wordwrap_stream_create (&str, outstr, 0, rmargin);
  if (rc)
    {
      abort ();//FIXME
    }  
  po->po_version_hook (po, str);

  mu_stream_destroy (&str);
}

/* opt.c -- general-purpose command line option parser
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
#include <stdarg.h>
#include <string.h>
#include <mailutils/alloc.h>
#include <mailutils/opt.h>
#include <mailutils/nls.h>
#include <mailutils/errno.h>

#define EXIT_SUCCESS 0
#define EXIT_ERROR   1

/* Compare pointers to two option structs */
static int
optcmp (const void *a, const void *b)
{
  struct mu_option const *ap = *(struct mu_option const **)a;
  struct mu_option const *bp = *(struct mu_option const **)b;

  while (ap->opt_flags & MU_OPTION_ALIAS)
    ap--;
  while (bp->opt_flags & MU_OPTION_ALIAS)
    bp--;
  
  if (MU_OPTION_IS_VALID_SHORT_OPTION (ap)
      && MU_OPTION_IS_VALID_SHORT_OPTION (bp))
    return ap->opt_short - bp->opt_short;
  if (MU_OPTION_IS_VALID_LONG_OPTION (ap)
      && MU_OPTION_IS_VALID_LONG_OPTION (bp))
    return strcmp (ap->opt_long, bp->opt_long);
  if (MU_OPTION_IS_VALID_LONG_OPTION (ap))
    return 1;
  return -1;
}

/* Sort a group of options in OPTBUF, starting at index START (first
   option slot after a group header (if any).  The group spans up to
   next group header or end of options */
static size_t
sort_group (struct mu_option **optbuf, size_t start)
{
  size_t i;
  
  for (i = start; optbuf[i] && !MU_OPTION_IS_GROUP_HEADER (optbuf[i]); i++)
    ;
  
  qsort (&optbuf[start], i - start, sizeof (optbuf[0]), optcmp);
  return i;
}

/* Print help summary and exit. */
static void
fn_help (struct mu_parseopt *po, struct mu_option *opt, char const *unused)
{
  mu_program_help (po);
  exit (EXIT_SUCCESS);
}

/* Print usage summary and exit. */
static void
fn_usage (struct mu_parseopt *po, struct mu_option *opt, char const *unused)
{
  mu_program_usage (po);
  exit (EXIT_SUCCESS);
}

static void
fn_version (struct mu_parseopt *po, struct mu_option *opt, char const *unused)
{
  po->po_version_hook (po, stdout);
  exit (EXIT_SUCCESS);
}

/* Default options */
struct mu_option mu_default_options[] = {
  MU_OPTION_GROUP(""),
  { "help",    '?', NULL, MU_OPTION_IMMEDIATE, N_("give this help list"),
    mu_c_string, NULL, fn_help },
  { "usage",   0,   NULL, MU_OPTION_IMMEDIATE, N_("give a short usage message"),
    mu_c_string, NULL, fn_usage
  },
  MU_OPTION_END
};

struct mu_option mu_version_options[] = {
  { "version", 'V', NULL, MU_OPTION_IMMEDIATE, N_("print program version"),
    mu_c_string, NULL, fn_version },
  MU_OPTION_END
};

/* Output error message */
void
mu_parseopt_error (struct mu_parseopt *po, char const *fmt, ...)
{
  va_list ap;

  if (po->po_flags & MU_PARSEOPT_IGNORE_ERRORS)
    return;

  if (po->po_prog_name)
    fprintf (stderr, "%s: ", po->po_prog_name);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
}

static void
mu_option_cache_destroy (void *ptr)
{
  struct mu_option_cache *cache = ptr;
  free (cache->cache_arg);
  free (cache);
}

static int parseopt_apply (void *item, void *data);

/* If OPT is an immediate option, evaluate it right away.  Otherwise,
   add option OPT with argument ARG to the cache in PO. */
void
add_option_cache (struct mu_parseopt *po, struct mu_option *opt,
		  char const *arg)
{
  struct mu_option_cache *cache = mu_alloc (sizeof (*cache));
  cache->cache_opt = opt;
  cache->cache_arg = arg ? mu_strdup (arg) : NULL;

  if ((po->po_flags & MU_PARSEOPT_IMMEDIATE)
       || (opt->opt_flags & MU_OPTION_IMMEDIATE))
    {
      parseopt_apply (cache, po);
      mu_option_cache_destroy (cache);
    }
  else
    {
      mu_list_append (po->po_optlist, cache);
    }
}

/* Find first option for which I is an alias */
struct mu_option *
option_unalias (struct mu_parseopt *po, int i)
{
  while (i > 0 && po->po_optv[i]->opt_flags & MU_OPTION_ALIAS)
    --i;
  return po->po_optv[i];
}

/* Find a descriptor of short option CHR */
struct mu_option *
find_short_option (struct mu_parseopt *po, int chr)
{
  size_t i;

  for (i = 0; i < po->po_optc; i++)
    {
      if (MU_OPTION_IS_VALID_SHORT_OPTION (po->po_optv[i])
	  && po->po_optv[i]->opt_short == chr)
	return option_unalias (po, i);
    }
  mu_parseopt_error (po, _("unrecognized option '-%c'"), chr);
  return NULL;
}

/* Find a descriptor of long option OPTSTR.  If it has argument, return
   it in *ARGPTR. */
struct mu_option *
find_long_option (struct mu_parseopt *po, char const *optstr,
		  char **argptr)
{
  size_t i;
  size_t optlen;
  size_t ind;
  int found = 0;
  
  optlen = strcspn (optstr, "=");
  
  for (i = 0; i < po->po_optc; i++)
    {
      if (MU_OPTION_IS_VALID_LONG_OPTION (po->po_optv[i])
	  && optlen <= strlen (po->po_optv[i]->opt_long)
	  && memcmp (po->po_optv[i]->opt_long, optstr, optlen) == 0)
	{
	  switch (found)
	    {
	    case 0:
	      ind = i;
	      found++;
	      break;

	    case 1:
	      if (po->po_flags & MU_PARSEOPT_IGNORE_ERRORS)
		return NULL;
	      mu_parseopt_error (po,
			   _("option '--%*.*s' is ambiguous; possibilities:"),
			   optlen, optlen, optstr);
	      fprintf (stderr, "--%s\n", po->po_optv[ind]->opt_long);
	      found++;

	    case 2:
	      fprintf (stderr, "--%s\n", po->po_optv[i]->opt_long);
	    }
	}
    }

  switch (found)
    {
    case 0:
      mu_parseopt_error (po, _("unrecognized option '--%s'"), optstr);
      break;
      
    case 1:
      if (optstr[optlen])
	++optlen;
      *argptr = (char *)(optstr + optlen);
      return option_unalias (po, ind);

    case 2:
      break;
    }

  return NULL;  
}

static void
permute (struct mu_parseopt *po)
{
  if (!(po->po_flags & MU_PARSEOPT_IN_ORDER) && po->po_arg_count)
    {
      /* Array to save arguments in */
      char *save[2];
      /* Number of arguments processed (at most two) */
      int n = po->po_ind - (po->po_arg_start + po->po_arg_count);
      
      if (n > 2)
	abort ();
      
      /* Store the processed elements away */
      save[0] = po->po_argv[po->po_arg_start + po->po_arg_count];
      if (n == 2)
	save[1] = po->po_argv[po->po_arg_start + po->po_arg_count + 1];

      /* Shift the array */
      memmove (po->po_argv + po->po_arg_start + n,
	       po->po_argv + po->po_arg_start,
	       po->po_arg_count * sizeof (po->po_argv[0]));
      
      /* Place stored elements in the vacating slots */
      po->po_argv[po->po_arg_start] = save[0];
      if (n == 2)
	po->po_argv[po->po_arg_start + 1] = save[1];
      
      /* Fix up start index */
      po->po_arg_start += n;
      po->po_permuted = 1;
    }
}

/* Consume next option from PO.  On success, update PO members as
   described below and return 0.  On end of options, return 1.

   If the consumed option is a short option, then 

     po_chr  keeps its option character, and
     po_cur  points to the next option character to be processed

   Otherwise, if the consumed option is a long one, then

     po_chr  is 0
     po_cur  points to the first character after --
*/   
static int
next_opt (struct mu_parseopt *po)
{
  if (!*po->po_cur)
    {
      permute (po);
      
      while (1)
	{
	  po->po_cur = po->po_argv[po->po_ind++];
	  if (!po->po_cur)
	    return 1;
	  if (po->po_cur[0] == '-' && po->po_cur[1])
	    break;
	  if (!(po->po_flags & MU_PARSEOPT_IN_ORDER))
	    {
	      if (!po->po_permuted && po->po_arg_count == 0)
		po->po_arg_start = po->po_ind - 1;
	      po->po_arg_count++;
	      continue;
	    }
	  else
	    return 1;
	}

      if (*++po->po_cur == '-')
	{
	  if (*++po->po_cur == 0)
	    {
	      /* End of options */
	      permute (po);
	      ++po->po_ind;
	      return 1;
	    }

	  /* It's a long option */
	  po->po_chr = 0;
	  return 0;
	}
    }

  po->po_chr = *po->po_cur++;

  return 0;
}

/* Parse options */
static int
parse (struct mu_parseopt *po)
{
  int rc;

  rc = mu_list_create (&po->po_optlist);
  if (rc)
    return rc;
  mu_list_set_destroy_item (po->po_optlist, mu_option_cache_destroy);
  
  po->po_ind = 0;
  if (!(po->po_flags & MU_PARSEOPT_ARGV0))
    {
      po->po_ind++;
      if (!(po->po_flags & MU_PARSEOPT_PROG_NAME))
	{
	  char *p = strrchr (po->po_argv[0], '/');
	  if (p)
	    p++;
	  else
	    p = (char*) po->po_argv[0];
	  if (strlen (p) > 3 && memcmp (p, "lt-", 3) == 0)
	    p += 3;
	  po->po_prog_name = p;
	}
    }
  else if (!(po->po_flags & MU_PARSEOPT_PROG_NAME))
    po->po_prog_name = NULL;

  po->po_arg_start = po->po_ind;
  po->po_arg_count = 0;
  po->po_permuted = 0;
  
  po->po_cur = "";

  po->po_opterr = -1;
  
  while (next_opt (po) == 0)
    {
      struct mu_option *opt;
      char *long_opt;
      
      if (po->po_chr)
	{
	  opt = find_short_option (po, po->po_chr);
	  long_opt = NULL;
	}
      else
	{
	  long_opt = po->po_cur;
	  opt = find_long_option (po, long_opt, &po->po_cur);
	}

      if (opt)
	{
	  char *arg = NULL;
	  
	  if (opt->opt_arg)
	    {
	      if (po->po_cur[0])
		{
		  arg = po->po_cur;
		  po->po_cur = "";
		}
	      else if (opt->opt_flags & MU_OPTION_ARG_OPTIONAL)
		/* ignore it */;
	      else if (po->po_ind < po->po_argc)
		arg = po->po_argv[po->po_ind++];
	      else
		{
		  if (long_opt)
		    mu_parseopt_error (po,
				 _("option '--%s' requires an argument"),
				 long_opt);
		  else
		    mu_parseopt_error (po,
				 _("option '-%c' requires an argument"),
				 po->po_chr);
		  po->po_opterr = po->po_ind;
		  if (po->po_flags & MU_PARSEOPT_NO_ERREXIT)
		    {
		      if (!(po->po_flags & MU_PARSEOPT_IN_ORDER))
			po->po_arg_count++;
		      continue;
		    }
		  exit (po->po_exit_error);
		}	
	    }
	  else
	    {
	      if (long_opt
		  && po->po_cur[0]
		  && !(po->po_flags & MU_OPTION_ARG_OPTIONAL))
		{
		  mu_parseopt_error (po,
			       _("option '--%s' doesn't allow an argument"),
			       long_opt);
		  po->po_opterr = po->po_ind;
		  if (po->po_flags & MU_PARSEOPT_NO_ERREXIT)
		    {
		      if (!(po->po_flags & MU_PARSEOPT_IN_ORDER))
			po->po_arg_count++;
		      continue;
		    }
		  exit (po->po_exit_error);
		}
	      arg = NULL;
	    }
	  
	  add_option_cache (po, opt, arg);
	}
      else
	{
	  po->po_opterr = po->po_ind;
	  if (po->po_flags & MU_PARSEOPT_NO_ERREXIT)
	    {
	      if (!(po->po_flags & MU_PARSEOPT_IN_ORDER))
		po->po_arg_count++;
	      continue;
	    }
	  exit (po->po_exit_error);
	}
    }

  if (!po->po_permuted)
    po->po_arg_start = po->po_ind - 1 - po->po_arg_count;
  return 0;
}

/* Initialize structure mu_parseopt with given options and flags. */
static int
parseopt_init (struct mu_parseopt *po, struct mu_option **options,
	       int flags)
{
  struct mu_option *opt;
  size_t i, j;

  po->po_argc = 0;
  po->po_argv = NULL;
  po->po_optc = 0;
  po->po_flags = flags;
    
  /* Fix up flags */
  if (flags & MU_PARSEOPT_IGNORE_ERRORS)
    flags |= MU_PARSEOPT_NO_ERREXIT;

  if (!(flags & MU_PARSEOPT_PROG_DOC))
    po->po_prog_doc = NULL;
  if (!(flags & MU_PARSEOPT_PROG_ARGS))
    po->po_prog_args = NULL;
  if (!(flags & MU_PARSEOPT_BUG_ADDRESS))
    po->po_bug_address = NULL;
  if (!(flags & MU_PARSEOPT_PACKAGE_NAME))
    po->po_package_name = NULL;
  if (!(flags & MU_PARSEOPT_PACKAGE_URL))
    po->po_package_url = NULL;
  if (!(flags & MU_PARSEOPT_PACKAGE_URL))
    po->po_data = NULL;
  if (!(flags & MU_PARSEOPT_EXTRA_INFO))
    po->po_extra_info = NULL;
  if (!(flags & MU_PARSEOPT_HELP_HOOK))
    po->po_help_hook = NULL;
  if (!(flags & MU_PARSEOPT_EXIT_ERROR))
    po->po_exit_error = EXIT_ERROR;
  if (!(flags & MU_PARSEOPT_VERSION_HOOK))
    po->po_version_hook = NULL;
  
  /* Count the options */
  po->po_optc = 0;
  for (i = 0; options[i]; i++)
    for (opt = options[i]; !MU_OPTION_IS_END (opt); opt++)
      ++po->po_optc;

  if (!(flags & MU_PARSEOPT_NO_STDOPT))
    for (i = 0; !MU_OPTION_IS_END (&mu_default_options[i]); i++)
      ++po->po_optc;

  if (flags & MU_PARSEOPT_VERSION_HOOK)
    for (i = 0; !MU_OPTION_IS_END (&mu_version_options[i]); i++)
      ++po->po_optc;
  
  /* Allocate the working buffer of option pointers */
  po->po_optv = mu_calloc (po->po_optc + 1, sizeof (*po->po_optv));
  if (!po->po_optv)
    return -1;

  /* Fill in the array */
  j = 0;
  for (i = 0; options[i]; i++)
    for (opt = options[i]; !MU_OPTION_IS_END (opt); opt++, j++)
      {
	if (!opt->opt_set)
	  opt->opt_set = mu_option_set_value;
	po->po_optv[j] = opt;
      }
  
  if (!(flags & MU_PARSEOPT_NO_STDOPT))
    for (i = 0; !MU_OPTION_IS_END (&mu_default_options[i]); i++, j++)
      po->po_optv[j] = &mu_default_options[i];

  if (flags & MU_PARSEOPT_VERSION_HOOK)
    for (i = 0; !MU_OPTION_IS_END (&mu_version_options[i]); i++, j++)
      po->po_optv[j] = &mu_version_options[i];
  
  po->po_optv[j] = NULL;

  /* Ensure sane start of options.  This is necessary, in particular,
     because optcmp backs up until it finds an element with cleared
     MU_OPTION_ALIAS bit. */
  po->po_optv[0]->opt_flags &= MU_OPTION_ALIAS;
  if (!(flags & MU_PARSEOPT_NO_SORT))
    {
      /* Sort the options */
      size_t start;

      for (start = 0; start < po->po_optc; )
	{
	  if (MU_OPTION_IS_GROUP_HEADER (po->po_optv[start]))
	    start = sort_group (po->po_optv, start + 1);
	  else 
	    start = sort_group (po->po_optv, start);
	}
    }

  po->po_ind = 0;
  po->po_opterr = 0;
  po->po_optlist = NULL;
  po->po_cur = NULL;
  po->po_chr = 0;
  po->po_arg_start = 0;
  po->po_arg_count = 0;
  po->po_permuted = 0;
  
  return 0;
}

/* Parse command line from ARGC/ARGV. Valid options are given in
   OPTIONS.  FLAGS control the parsing. */
int
mu_parseopt (struct mu_parseopt *po,
	     int argc, char **argv, struct mu_option **options,
	     int flags)
{
  int rc;

  if (flags & MU_PARSEOPT_REUSE)
    {
      mu_list_clear (po->po_optlist);
      po->po_flags = (po->po_flags & MU_PARSEOPT_IMMUTABLE_MASK)
	                | (flags & ~MU_PARSEOPT_IMMUTABLE_MASK);
    }
  else
    {
      rc = parseopt_init (po, options, flags);
      if (rc)
	return rc;
    }
  po->po_argc = argc;
  po->po_argv = argv;
  
  rc = parse (po);

  if (rc == 0)
    {
      if (po->po_opterr >= 0)
	rc = -1; 
      else
	{
	  if (po->po_flags & MU_PARSEOPT_IMMEDIATE)
	    rc = mu_parseopt_apply (po);
	}
    }

  return rc;
}

void
mu_parseopt_free (struct mu_parseopt *popt)
{
  free (popt->po_optv);
  mu_list_destroy (&popt->po_optlist);
}

static int
parseopt_apply (void *item, void *data)
{
  struct mu_option_cache *cp = item;
  struct mu_parseopt *popt = data;
  cp->cache_opt->opt_set (popt, cp->cache_opt, cp->cache_arg);
  return 0;
}

int
mu_parseopt_apply (struct mu_parseopt *popt)
{
  return mu_list_foreach (popt->po_optlist, parseopt_apply, popt);
}

void
mu_option_set_value (struct mu_parseopt *po, struct mu_option *opt,
		     char const *arg)
{
  if (opt->opt_ptr)
    {
      char *errmsg;
      int rc;
      
      if (arg == NULL)
	{
	  if (opt->opt_arg == NULL)
	    arg = "1";
	  else
	    {
	      *(void**)opt->opt_ptr = NULL;
	      return;
	    }
	}
      rc = mu_str_to_c (arg, opt->opt_type, opt->opt_ptr, &errmsg);
      if (rc)
	{
	  char const *errtext;
	  if (errmsg)
	    errtext = errmsg;
	  else
	    errtext = mu_strerror (rc);

	  if (opt->opt_long)
	    mu_parseopt_error (po, "--%s: %s", opt->opt_long, errtext);
	  else
	    mu_parseopt_error (po, "-%c: %s", opt->opt_short, errtext);
	  free (errmsg);

	  if (!(po->po_flags & MU_PARSEOPT_NO_ERREXIT))
	    exit (EXIT_ERROR);
	}
    }
}

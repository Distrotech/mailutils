/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005, 2007, 2009-2012 Free Software Foundation,
   Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with GNU Mailutils.  If not, see
   <http://www.gnu.org/licenses/>. */

/* Implements "list" sieve extension test. See "Syntax:" below for the
   description */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/sieve.h>



/* Auxiliary functions */
struct header_closure
{
  mu_header_t header;     /* Message header */
  int index;              /* Header index */
  char *delim;            /* List delimiter */
  char **valv;            /* Retrieved and split-out header values */
  size_t valc;            /* Number of values in valv */
  size_t vali;            /* Current index in valv */  
};

static void
cleanup (struct header_closure *hc)
{
  mu_argcv_free (hc->valc, hc->valv);
  hc->valv = NULL;
  hc->valc = hc->vali = 0;
}

static int
retrieve_next_header (struct header_closure *hc, char *name, char **pval)
{
  const char *buf;

  cleanup (hc);
  while (!mu_header_sget_field_name (hc->header, hc->index, &buf))
    {
      int i = hc->index++;
      if (mu_c_strcasecmp (buf, name) == 0)
	{
	  const char *value;
	  struct mu_wordsplit ws;
	  
	  if (mu_header_sget_field_value (hc->header, i, &value))
	    return 1;
	  ws.ws_delim = hc->delim;
	  if (mu_wordsplit (value, &ws,
			    MU_WRDSF_DELIM|MU_WRDSF_SQUEEZE_DELIMS|
			    MU_WRDSF_WS|
			    MU_WRDSF_NOVAR|MU_WRDSF_NOCMD))
	    {
	      mu_error (_("cannot split line `%s': %s"), value,
			mu_wordsplit_strerror (&ws));
	      return 1;
	    }
	  if (ws.ws_wordc == 0)
	    {
	      cleanup (hc);
	      mu_wordsplit_free (&ws);
	      return 1;
	    }
	  hc->valv = ws.ws_wordv;
	  hc->valc = ws.ws_wordc;
	  hc->vali = 0;
	  ws.ws_wordv = NULL;
	  ws.ws_wordc = 0;
	  mu_wordsplit_free (&ws);
	  *pval = hc->valv[hc->vali++];
	  return 0;
	}
    }

  return 1;
}

static int
list_retrieve_header (void *item, void *data, int idx, char **pval)
{
  struct header_closure *hc = data;
  char *p;
  
  if (idx == 0)
    hc->index = 1;

  while (1)
    {
      if (!hc->valv)
	{
	  if (retrieve_next_header (hc, (char*) item, &p))
	    return 1;
	}
      else if (hc->vali == hc->valc)
	{
	  cleanup (hc);
	  continue;
	}	  
      else
	p = hc->valv[hc->vali++];
  
      if ((*pval = strdup (p)) == NULL)
        return 1;
      return 0;
    }
  
  return 1;
}


/* The test proper */

/* Syntax: list [COMPARATOR] [MATCH-TYPE]
                [ :delim <delimiters: string> ]
                <headers: string-list> <key-list: string-list>

   The "list" test evaluates to true if any of the headers
   match any key. Each header is regarded as containing a
   list of keywords. By default, comma is assumed as list
   separator. This can be overridden by specifying ":delim"
   tag, whose value is a string consisting of valid list
   delimiter characters.

   list :matches :delim " ," [ "X-Spam-Keywords", "X-Spamd-Keywords" ]
             [ "HTML_*", "FORGED_*" ]

		
*/

static int
list_test (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_sieve_value_t *h, *v, *arg;
  mu_sieve_comparator_t comp = mu_sieve_get_comparator (mach, tags);
  struct header_closure clos;
  int result;

  if (mu_sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
    {
      mu_sieve_debug (mach, "LIST");
    }
  
  memset (&clos, 0, sizeof clos);
  if (mu_sieve_tag_lookup (tags, "delim", &arg))
    clos.delim = arg->v.string;
  else
    clos.delim = ",";
  
  h = mu_sieve_value_get (args, 0);
  if (!h)
    {
      mu_sieve_arg_error (mach, 1);
      mu_sieve_abort (mach);
    }
  v = mu_sieve_value_get (args, 1);
  if (!v)
    {
      mu_sieve_arg_error (mach, 2);
      mu_sieve_abort (mach);
    }

  mu_message_get_header (mu_sieve_get_message (mach), &clos.header);
  result = mu_sieve_vlist_compare (h, v, comp,
				   mu_sieve_get_relcmp (mach, tags),
				   list_retrieve_header,
				   &clos, NULL) > 0;
  cleanup (&clos);
  return result; 
}


/* Initialization */

/* Required arguments: */
static mu_sieve_data_type list_req_args[] = {
  SVT_STRING_LIST,
  SVT_STRING_LIST,
  SVT_VOID
};

static mu_sieve_tag_def_t match_part_tags[] = {
  { "is", SVT_VOID },
  { "contains", SVT_VOID },
  { "matches", SVT_VOID },
  { "regex", SVT_VOID },
  { "count", SVT_STRING },
  { "value", SVT_STRING },
  { "comparator", SVT_STRING },
  { NULL }
};

static mu_sieve_tag_def_t delim_part_tags[] = {
  { "delim", SVT_STRING },
  { NULL }
};

static mu_sieve_tag_group_t list_tag_groups[] = {
  { match_part_tags, mu_sieve_match_part_checker },
  { delim_part_tags, NULL },
  { NULL }
};

/* Initialization function. */
int
SIEVE_EXPORT(list,init) (mu_sieve_machine_t mach)
{
  return mu_sieve_register_test (mach, "list", list_test,
                              list_req_args, list_tag_groups, 1);
}

/* End of list.c */

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

/* Implements "list" sieve extension test. See "Syntax:" below for the
   description */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/libsieve.h>



/* Auxiliary functions */
struct header_closure {
  header_t header;        /* Message header */
  int index;              /* Header index */
  char *delim;            /* List delimiter */
  char *value;            /* Retrieved header value */
  char *save;             /* Save pointer for strtok_r */
};

static void
cleanup (struct header_closure *hc)
{
  free (hc->value);
  hc->value = hc->save = NULL;
}

static int
retrieve_next_header (struct header_closure *hc, char *name, char **pval)
{
  char buf[512];
  size_t n;

  cleanup (hc);
  while (!header_get_field_name (hc->header, hc->index, buf, sizeof(buf), &n))
    {
      int i = hc->index++;
      if (strcasecmp (buf, name) == 0)
	{
	  if (header_aget_field_value (hc->header, i, &hc->value))
	    return 1;
	  *pval = strtok_r (hc->value, hc->delim, &hc->save);
	  if (*pval == NULL)
	    {
	      cleanup (hc);
	      return 1;
	    }
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
      if (!hc->value)
	{
	  if (retrieve_next_header (hc, (char*) item, &p))
	    return 1;
	}
      else
	{
	  p = strtok_r (NULL, hc->delim, &hc->save);
	  if (!p)
	    {
	      cleanup (hc);
	      continue;
	    }
	}
  
      *pval = strdup (p);
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
list_test (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_value_t *h, *v, *arg;
  sieve_comparator_t comp = sieve_get_comparator (mach, tags);
  struct header_closure clos;
  int result;

  if (sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
    {
      sieve_locus_t locus;
      sieve_get_locus (mach, &locus);
      sieve_debug (mach, "%s:%lu: LIST\n",
		   locus.source_file,
		   (unsigned long) locus.source_line);
    }
  
  memset (&clos, 0, sizeof clos);
  if (sieve_tag_lookup (tags, "delim", &arg))
    clos.delim = arg->v.string;
  else
    clos.delim = ",";
  
  h = sieve_value_get (args, 0);
  if (!h)
    {
      sieve_arg_error (mach, 1);
      sieve_abort (mach);
    }
  v = sieve_value_get (args, 1);
  if (!v)
    {
      sieve_arg_error (mach, 2);
      sieve_abort (mach);
    }

  message_get_header (sieve_get_message (mach), &clos.header);
  result = sieve_vlist_compare (h, v, comp, sieve_get_relcmp (mach, tags),
				list_retrieve_header,
				&clos, NULL) > 0;
  cleanup (&clos);
  return result; 
}


/* Initialization */

/* Required arguments: */
static sieve_data_type list_req_args[] = {
  SVT_STRING_LIST,
  SVT_STRING_LIST,
  SVT_VOID
};

static sieve_tag_def_t match_part_tags[] = {
  { "is", SVT_VOID },
  { "contains", SVT_VOID },
  { "matches", SVT_VOID },
  { "regex", SVT_VOID },
  { "count", SVT_STRING },
  { "value", SVT_STRING },
  { "comparator", SVT_STRING },
  { NULL }
};

static sieve_tag_def_t delim_part_tags[] = {
  { "delim", SVT_STRING },
  { NULL }
};

static sieve_tag_group_t list_tag_groups[] = {
  { match_part_tags, sieve_match_part_checker },
  { delim_part_tags, NULL },
  { NULL }
};

/* Initialization function. */
int
SIEVE_EXPORT(list,init) (sieve_machine_t mach)
{
  return sieve_register_test (mach, "list", list_test,
                              list_req_args, list_tag_groups, 1);
}

/* End of list.c */

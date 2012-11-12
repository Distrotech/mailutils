/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2012 Free Software Foundation, Inc.

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

/* This module implements the Editheader Extension (RFC 5293) */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <mailutils/types.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/sieve.h>

/* Syntax: addheader [:last] <field-name: string> <value: string>
 */
int
sieve_addheader (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_sieve_value_t *val;
  const char *field_name;
  const char *field_value;
  mu_message_t msg;
  mu_header_t hdr;
  int rc;
  
  val = mu_sieve_value_get (args, 0);
  if (!val)
    {
      mu_sieve_error (mach, "%lu: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      _("cannot get field name"));
      mu_sieve_abort (mach);
    }
  field_name = val->v.string;

  val = mu_sieve_value_get (args, 1);
  if (!val)
    {
      mu_sieve_error (mach, "%lu: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      _("cannot get field value"));
      mu_sieve_abort (mach);
    }
  field_value = val->v.string;

  mu_sieve_log_action (mach, "ADDHEADER", "%s: %s", field_name, field_value);

  if (mu_sieve_is_dry_run (mach))
    return 0;

  msg = mu_sieve_get_message (mach);
  rc = mu_message_get_header (msg, &hdr);
  if (rc)
    {
      mu_sieve_error (mach, "%lu: %s: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      _("cannot get message header"),
		      mu_strerror (rc));
      mu_sieve_abort (mach);
    }

  rc = (mu_sieve_tag_lookup (tags, "last", NULL) ?
	mu_header_append : mu_header_prepend) (hdr, field_name, field_value);
  if (rc)
    {
      mu_sieve_error (mach, "%lu: %s: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      _("cannot append message header"),
		      mu_strerror (rc));
      mu_sieve_abort (mach);
    }
  return 0;
}

/* Syntax: deleteheader [:index <fieldno: number> [:last]]
                        [COMPARATOR] [MATCH-TYPE]
                        <field-name: string>
                        [<value-patterns: string-list>]
 */
int
sieve_deleteheader (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_sieve_value_t *val;
  const char *field_name;
  const char *field_pattern;
  mu_message_t msg;
  mu_header_t hdr;
  int rc;
  mu_sieve_comparator_t comp;
  mu_iterator_t itr;
  unsigned long i, idx = 0;
  
  val = mu_sieve_value_get (args, 0);
  if (!val)
    {
      mu_sieve_error (mach, "%lu: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      _("cannot get field name"));
      mu_sieve_abort (mach);
    }
  field_name = val->v.string;

  val = mu_sieve_value_get (args, 1);
  if (!val)
    {
      field_pattern = NULL;
      mu_sieve_log_action (mach, "DELETEHEADER", "%s", field_name);
    }
  else
    {
      switch (val->type)
	{
	case SVT_STRING_LIST:
	  if (mu_list_get (val->v.list, 0, (void**)&field_pattern))
	    {
	      mu_sieve_error (mach, "%lu: %s",
			      (unsigned long) mu_sieve_get_message_num (mach),
			      _("cannot get list item"));
	      mu_sieve_abort (mach);
	    }
	  mu_sieve_log_action (mach, "DELETEHEADER", "%s: (regexp)",
			       field_name);
	  break;
	  
	case SVT_STRING:
	  field_pattern = val->v.string;
	  mu_sieve_log_action (mach, "DELETEHEADER", "%s: %s", field_name,
			       field_pattern);
	  break;

	default:
	  mu_sieve_error (mach, "%lu: %s: %d",
			  (unsigned long) mu_sieve_get_message_num (mach),
			  _("unexpected value type"), val->type);
	  mu_sieve_abort (mach);
	  
	}
    }
  
  if (mu_sieve_is_dry_run (mach))
    return 0;

  msg = mu_sieve_get_message (mach);
  rc = mu_message_get_header (msg, &hdr);
  if (rc)
    {
      mu_sieve_error (mach, "%lu: %s: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      _("cannot get message header"),
		      mu_strerror (rc));
      mu_sieve_abort (mach);
    }

  mu_header_get_iterator (hdr, &itr);
  if (mu_sieve_tag_lookup (tags, "last", NULL))
    {
      int backwards = 1;
      mu_iterator_ctl (itr, mu_itrctl_set_direction, &backwards);
    }
  comp = mu_sieve_get_comparator (mach, tags);

  if (mu_sieve_tag_lookup (tags, "index", &val))
    idx = val->v.number;
  
  for (i = 0, mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      const char *fn, *fv;

      mu_iterator_current_kv (itr, (const void **)&fn, (void **)&fv);
      if (strcmp (field_name, fn))
	continue;
      if (idx && ++i < idx)
	continue;
	  
      if (field_pattern)
	{
	  if (comp (field_pattern, fv))
	    mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	}
      else
	mu_iterator_ctl (itr, mu_itrctl_delete, NULL);

      if (idx)
	break;
    }
  mu_iterator_destroy (&itr);
  return 0;
}


/* addheader tagged arguments: */
static mu_sieve_tag_def_t addheader_tags[] = {
  { "last", SVT_VOID },
  { NULL }
};

static mu_sieve_tag_group_t addheader_tag_groups[] = {
  { addheader_tags, NULL },
  { NULL }
};

/* addheader required arguments: */
static mu_sieve_data_type addheader_args[] = {
  SVT_STRING,			/* field name */
  SVT_STRING,                   /* field value */ 
  SVT_VOID
};

#if 0
/* FIXME: The checker interface should be redone. Until then this function
   is commented out.  Problems:

   1. Checkers are called per group, there's no way to call them per tag.
   2. See FIXMEs in the code.
*/
static int
index_checker (const char *name, mu_list_t tags, mu_list_t args)
{
  mu_iterator_t itr;
  mu_sieve_runtime_tag_t *match = NULL;
  int err;
  
  if (!tags || mu_list_get_iterator (tags, &itr))
    return 0;

  err = 0;
  for (mu_iterator_first (itr); !err && !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      mu_sieve_runtime_tag_t *t;
      mu_iterator_current (itr, (void **)&t);
      
      if (strcmp (t->tag, "index") == 0)
	{
	  if (match)
	    {
	      /* FIXME: 1. This function must be public.
		        2. locus should be included in t
	      */
	      mu_sv_compile_error (&mu_sieve_locus, 
			      _("index specified twice in call to `%s'"),
				   name);
	      err = 1;
	      break;
	    }    
	}
    }

  mu_iterator_destroy (&itr);

  if (err)
    return 1;

  if (match)
    {
      if (match->arg->v.number < 1)
	{
	// See FIXME above 
	  mu_sv_compile_error (&mu_sieve_locus, 
			       _("invalid index value: %s"),
			       match->arg->v.string);
	  return 1;
	}
    }
  
  return 0;
}
#endif

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

/* deleteheader tagged arguments: */
static mu_sieve_tag_def_t deleteheader_tags[] = {
  { "last", SVT_VOID },
  { "index", SVT_NUMBER },
  { NULL }
};

static mu_sieve_tag_group_t deleteheader_tag_groups[] = {
  { deleteheader_tags, NULL },
  { match_part_tags, mu_sieve_match_part_checker },
  { NULL }
};

/* deleteheader required arguments: */
static mu_sieve_data_type deleteheader_args[] = {
  SVT_STRING,			/* field name or value pattern */
  SVT_VOID
};

int
SIEVE_EXPORT (editheader, init) (mu_sieve_machine_t mach)
{
  int rc;

  /* This dummy record is required by libmu_sieve  */
  rc = mu_sieve_register_action (mach, "editheader", NULL, NULL, NULL, 1);
  if (rc)
    return rc;
  rc = mu_sieve_register_action (mach, "addheader", sieve_addheader,
				 addheader_args, addheader_tag_groups, 1);
  if (rc)
    return rc;
  rc = mu_sieve_register_action_ext (mach, "deleteheader", sieve_deleteheader,
				     deleteheader_args, deleteheader_args,
				     deleteheader_tag_groups,
				     1);
  if (rc)
    return rc;

  return rc;
}

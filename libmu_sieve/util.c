/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005-2008, 2010-2012, 2014-2016 Free
   Software Foundation, Inc.

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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sieve-priv.h>


size_t 
mu_sieve_value_create (mu_sieve_machine_t mach, mu_sieve_data_type type,
		       void *data)
{
  size_t idx;
  mu_sieve_value_t *val;
  
  if (mach->valcount == mach->valmax)
    {
      mu_i_sv_2nrealloc (mach, (void**) &mach->valspace, &mach->valmax,
			 sizeof mach->valspace[0]);
    }
  idx = mach->valcount++;
  val = &mach->valspace[idx];
  memset (val, 0, sizeof *val);
  
  val->type = type;
  switch (type)
    {
    case SVT_NUMBER:
      val->v.number = * (long *) data;
      break;
      
    case SVT_STRING:
      val->v.list.first = mu_i_sv_string_create (mach, (char *)data);
      val->v.list.count = 1;
      break;
      
    case SVT_STRING_LIST:
      val->v.list = *(mu_sieve_slice_t)data;
      break;
      
    case SVT_TAG:
      val->v.string = data;
      break;

    default:
      mu_error ("%s", _("invalid data type"));
      abort ();
    }
  return idx;
}

mu_sieve_value_t *
mu_sieve_get_arg_untyped (mu_sieve_machine_t mach, size_t index)
{
  if (index >= mach->argcount)
    {
      mu_sieve_error (mach, _("INTERNAL ERROR: %s,%zu,%zu,%zu argument index %zu out of range"),
		      mach->identifier,
		      mach->argstart,
		      mach->argcount,
		      mach->tagcount,
		      index);
      abort ();
      //FIXME      mu_sieve_abort (mach);
    }
  
  return mach->valspace + mach->argstart + index;
}

mu_sieve_value_t *
mu_sieve_get_arg_optional (mu_sieve_machine_t mach, size_t index)
{
  if (index >= mach->argcount)
    return NULL;
  return mach->valspace + mach->argstart + index;
}  

void
mu_sieve_value_get (mu_sieve_machine_t mach, mu_sieve_value_t *val,
		    mu_sieve_data_type type, void *ret)
{
  if (val->type != type)
    {
      if (val->tag)
	mu_sieve_error (mach,
			_("tag :%s has type %s, instead of expected %s"),
			val->tag,
			mu_sieve_type_str (val->type),
			mu_sieve_type_str (type));
      else
	{
	  size_t idx = val - mu_sieve_get_arg_untyped (mach, 0);
	  if (idx < mach->argcount)
	    mu_sieve_error (mach,
			    _("argument %zu has type %s, instead of expected %s"),
			    idx,
			    mu_sieve_type_str (val->type),
			    mu_sieve_type_str (type));
	  else
	    abort ();
	}
      mu_sieve_abort (mach);
    }

  switch (type)
    {
    case SVT_VOID:
      *(void**) ret = NULL;
      break;
      
    case SVT_NUMBER:
      *(size_t*) ret = val->v.number;
      break;
      
    case SVT_STRING:
      *(char**) ret = mu_sieve_string (mach, &val->v.list, 0);
      break;

    case SVT_STRING_LIST:
      *(struct mu_sieve_slice *) ret = val->v.list;
      break;

    case SVT_TAG:
      *(char**) ret = val->v.string;
      break;

    default:
      abort ();
    }
}

void
mu_sieve_get_arg (mu_sieve_machine_t mach, size_t index,
		  mu_sieve_data_type type, void *ret)
{
  mu_sieve_value_get (mach, mu_sieve_get_arg_untyped (mach, index),
		      type, ret);
}
  
mu_sieve_value_t *
mu_sieve_get_tag_untyped (mu_sieve_machine_t mach, char const *name)
{
  size_t i;
  mu_sieve_value_t *tag = mach->valspace + mach->argstart + mach->argcount;
  
  for (i = 0; i < mach->tagcount; i++)
    {
      if (strcmp (tag[i].tag, name) == 0)
	return &tag[i];
    }
  return NULL;
}

mu_sieve_value_t *
mu_sieve_get_tag_n (mu_sieve_machine_t mach, size_t n)
{
  if (n >= mach->tagcount)
    abort ();
  return &mach->valspace[mach->argstart + mach->argcount + n];
}

int
mu_sieve_get_tag (mu_sieve_machine_t mach, 
		  char *name, mu_sieve_data_type type,
		  void *ret)
{
  mu_sieve_value_t *val = mu_sieve_get_tag_untyped (mach, name);

  if (val)
    {
      if (ret)
	mu_sieve_value_get (mach, val, type, ret);
    }

  return val != NULL;
}

void
mu_sieve_error (mu_sieve_machine_t mach, const char *fmt, ...)
{
  va_list ap;
  
  va_start (ap, fmt);
  mu_stream_printf (mach->errstream, "\033s<%d>", MU_LOG_ERROR);
  if (mach->locus.mu_file)
    mu_stream_printf (mach->errstream, "\033O<%d>\033f<%u>%s\033l<%u>",
		      MU_LOGMODE_LOCUS,
		      (unsigned) strlen (mach->locus.mu_file),
		      mach->locus.mu_file,
		      mach->locus.mu_line);
  if (mach->identifier)
    mu_stream_printf (mach->errstream, "%s: ", mach->identifier);
  mu_stream_vprintf (mach->errstream, fmt, ap);
  mu_stream_write (mach->errstream, "\n", 1, NULL);
  va_end (ap);
}

const char *
mu_sieve_type_str (mu_sieve_data_type type)
{
  switch (type)
    {
    case SVT_VOID:
      return "void";
      
    case SVT_NUMBER:
      return "number";
      
    case SVT_STRING:
      return "string";

    case SVT_STRING_LIST:
      return "string-list";
      
    case SVT_TAG:
      return "tag";

    }

  return "unknown";
}

void
mu_i_sv_debug (mu_sieve_machine_t mach, size_t pc, const char *fmt, ...)
{
  va_list ap;

  if (mach->state_flags & MU_SV_SAVED_DBG_STATE)
    {
      unsigned severity = MU_LOG_DEBUG;
      mu_stream_ioctl (mach->dbgstream, MU_IOCTL_LOGSTREAM,
		       MU_IOCTL_LOGSTREAM_SET_SEVERITY, &severity);
      if (mach->locus.mu_file)
	{
	  int mode = mach->dbg_mode | MU_LOGMODE_LOCUS;
	  mu_stream_ioctl (mach->dbgstream, MU_IOCTL_LOGSTREAM,
			   MU_IOCTL_LOGSTREAM_SET_LOCUS, &mach->locus);
	  mu_stream_ioctl (mach->dbgstream, MU_IOCTL_LOGSTREAM,
			   MU_IOCTL_LOGSTREAM_SET_MODE, &mode);
	}
    }
  va_start (ap, fmt);
  mu_stream_printf (mach->dbgstream, "%4zu: ", pc);
  mu_stream_vprintf (mach->dbgstream, fmt, ap);
  mu_stream_write (mach->dbgstream, "\n", 1, NULL);
  va_end (ap);
}

void
mu_i_sv_debug_command (mu_sieve_machine_t mach,
		       size_t pc,
		       char const *what)
{
  size_t i;
  
  if (mach->state_flags & MU_SV_SAVED_DBG_STATE)
    {
      unsigned severity = MU_LOG_DEBUG;
      mu_stream_ioctl (mach->dbgstream, MU_IOCTL_LOGSTREAM,
		       MU_IOCTL_LOGSTREAM_SET_SEVERITY, &severity);
      if (mach->locus.mu_file)
	{
	  int mode = mach->dbg_mode | MU_LOGMODE_LOCUS;
	  mu_stream_ioctl (mach->dbgstream, MU_IOCTL_LOGSTREAM,
			   MU_IOCTL_LOGSTREAM_SET_LOCUS, &mach->locus);
	  mu_stream_ioctl (mach->dbgstream, MU_IOCTL_LOGSTREAM,
			   MU_IOCTL_LOGSTREAM_SET_MODE, &mode);
	}
    }
  mu_stream_printf (mach->dbgstream, "%4zu: %s: %s",
		    pc, what, mach->identifier);
  for (i = 0; i < mach->argcount; i++)
    mu_i_sv_valf (mach, mach->dbgstream, &mach->valspace[mach->argstart + i]);
  for (i = 0; i < mach->tagcount; i++)
    mu_i_sv_valf (mach, mach->dbgstream, mu_sieve_get_tag_n (mach, i));
  
  mu_stream_write (mach->dbgstream, "\n", 1, NULL);
}

void
mu_i_sv_trace (mu_sieve_machine_t mach, const char *what)
{
  size_t i;
  
  if (!mu_debug_level_p (mu_sieve_debug_handle, MU_DEBUG_TRACE4))
    return;
  
  mu_stream_printf (mach->errstream, "\033s<%d>", MU_LOG_DEBUG);
  if (mach->locus.mu_file)
    mu_stream_printf (mach->errstream, "\033O<%d>\033f<%u>%s\033l<%u>",
		      MU_LOGMODE_LOCUS,
		      (unsigned) strlen (mach->locus.mu_file),
		      mach->locus.mu_file,
		      mach->locus.mu_line);
  mu_stream_printf (mach->errstream, "%zu: %s %s", mach->msgno, what,
		    mach->identifier);
  for (i = 0; i < mach->argcount; i++)
    mu_i_sv_valf (mach, mach->errstream, mu_sieve_get_arg_untyped (mach, i));
  for (i = 0; i < mach->tagcount; i++)
    mu_i_sv_valf (mach, mach->errstream, mu_sieve_get_tag_n (mach, i));
  mu_stream_printf (mach->errstream, "\n");
}

void
mu_sieve_log_action (mu_sieve_machine_t mach, const char *action,
		     const char *fmt, ...)
{
  va_list ap;
  
  if (!mach->logger)
    return;
  
  mu_stream_ioctl (mach->errstream, MU_IOCTL_LOGSTREAM,
		   MU_IOCTL_LOGSTREAM_SET_LOCUS, &mach->locus);
  va_start (ap, fmt);
  mach->logger (mach, action, fmt, ap);
  va_end (ap);
}

int
mu_sieve_vlist_do (mu_sieve_machine_t mach, mu_sieve_value_t *val,
		   mu_list_action_t ac, void *data)
{
  size_t i;
  switch (val->type)
    {
    case SVT_STRING_LIST:
    case SVT_STRING:
      for (i = 0; i < val->v.list.count; i++)
	{
	  int rc = ac (mu_sieve_string (mach, &val->v.list, i), data);
	  if (rc)
	    return rc;
	}
      return 0;
    default:
      mu_error ("mu_sieve_vlist_do: unexpected list type %d", val->type);
      return EINVAL;
    }
}

int
mu_sieve_vlist_compare (mu_sieve_machine_t mach,
			mu_sieve_value_t *a, mu_sieve_value_t *b,
			mu_sieve_retrieve_t retr, mu_list_folder_t fold,
			void *data)
{
  int rc = 0;
  size_t i;
  mu_sieve_comparator_t comp = mu_sieve_get_comparator (mach);
  mu_sieve_relcmp_t test = mu_sieve_get_relcmp (mach);
  char *relcmp;
  mu_list_t tmp;
  
  if (!(a->type == SVT_STRING_LIST || a->type == SVT_STRING))
    abort ();

  rc = mu_list_create (&tmp);
  if (rc)
    {
      mu_sieve_error (mach, "mu_list_create: %s", mu_strerror (rc));
      mu_sieve_abort (mach);
    }
  mu_list_set_destroy_item (tmp, mu_list_free_item);
  for (i = 0; i < a->v.list.count; i++)
    {
      char *item = mu_sieve_string (mach, &a->v.list, i);
      char *sample;
      size_t j;
      
      for (j = 0; (rc = retr (item, data, j, &sample)) == 0; j++)
	{
	  rc = mu_list_append (tmp, sample);
	  if (rc)
	    {
	      free (sample);
	      mu_list_destroy (&tmp);
	      mu_sieve_error (mach, "mu_list_append: %s", mu_strerror (rc));
	      mu_sieve_abort (mach);
	    }
	}

      if (rc != MU_ERR_NOENT)
	{
	  mu_list_destroy (&tmp);
	  mu_sieve_error (mach, "retriever failure: %s", mu_strerror (rc));
	  mu_sieve_abort (mach);
	}
    }

  if (mu_sieve_get_tag (mach, "count", SVT_STRING, &relcmp))
    {
      size_t limit;
      size_t count;
      mu_sieve_relcmpn_t stest;
      struct mu_sieve_slice slice;
      char *str, *p;
      
      if (fold)
	{
	  count = 0;
	  rc = mu_list_fold (tmp, fold, data, &count, &count);
	  if (rc)
	    {
	      mu_sieve_error (mach, "mu_list_fold: %s", mu_strerror (rc));
	      mu_sieve_abort (mach);
	    }
	}
      else
	mu_list_count (tmp, &count);
      
      mu_sieve_get_arg (mach, 1, SVT_STRING_LIST, &slice);
      str = mu_sieve_string (mach, &slice, 0);
      limit = strtoul (str, &p, 10);
      if (*p)
	{
	  mu_sieve_error (mach, _("%s: not an integer"), str);
	  mu_sieve_abort (mach);
	}

      mu_sieve_str_to_relcmp (relcmp, NULL, &stest);
      rc = stest (count, limit);
    }
  else
    {
      mu_iterator_t itr;
      
      mu_list_get_iterator (tmp, &itr);
      rc = 0;
      for (mu_iterator_first (itr); rc == 0 && !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  char const *val;
	  mu_iterator_current (itr, (void**)&val);
	  
	  for (i = 0; i < b->v.list.count; i++)
	    {
	      mu_sieve_string_t *s = mu_sieve_string_raw (mach, &b->v.list, i);
	      rc = test (comp (mach, s, val), 0);
	      if (rc)
		break;
	    }
	}
      mu_iterator_destroy (&itr);
    }
  mu_list_destroy (&tmp);
  return rc;
}

void
mu_sieve_stream_save (mu_sieve_machine_t mach)
{
  if (mach->state_flags & MU_SV_SAVED_STATE)
    return;
  
  if (mu_stream_ioctl (mach->errstream, MU_IOCTL_LOGSTREAM,
		       MU_IOCTL_LOGSTREAM_GET_MODE, &mach->err_mode) == 0
      && mu_stream_ioctl (mach->errstream, MU_IOCTL_LOGSTREAM,
			  MU_IOCTL_LOGSTREAM_SET_LOCUS, &mach->err_locus) == 0)
      mach->state_flags |= MU_SV_SAVED_ERR_STATE;

  if (mu_stream_ioctl (mach->dbgstream, MU_IOCTL_LOGSTREAM,
		       MU_IOCTL_LOGSTREAM_GET_MODE, &mach->dbg_mode) == 0
      && mu_stream_ioctl (mach->dbgstream, MU_IOCTL_LOGSTREAM,
			  MU_IOCTL_LOGSTREAM_SET_LOCUS, &mach->dbg_locus) == 0)
    mach->state_flags |= MU_SV_SAVED_DBG_STATE;
  
  mach->state_flags |= MU_SV_SAVED_STATE;
}

void
mu_sieve_stream_restore (mu_sieve_machine_t mach)
{
  if (!(mach->state_flags & MU_SV_SAVED_STATE))
    return;

  if (mach->state_flags & MU_SV_SAVED_ERR_STATE)
    {
      mu_stream_ioctl (mach->errstream, MU_IOCTL_LOGSTREAM,
		       MU_IOCTL_LOGSTREAM_SET_MODE, &mach->err_mode);
      mu_stream_ioctl (mach->errstream, MU_IOCTL_LOGSTREAM,
		       MU_IOCTL_LOGSTREAM_SET_LOCUS, &mach->err_locus);
    }
  
  if (mach->dbgstream != mach->errstream
      && (mach->state_flags & MU_SV_SAVED_DBG_STATE))
    {
      mu_stream_ioctl (mach->dbgstream, MU_IOCTL_LOGSTREAM,
		       MU_IOCTL_LOGSTREAM_SET_MODE, &mach->dbg_mode);
      mu_stream_ioctl (mach->dbgstream, MU_IOCTL_LOGSTREAM,
		       MU_IOCTL_LOGSTREAM_SET_LOCUS, &mach->dbg_locus);
    }
  
  mach->state_flags = 0;
}

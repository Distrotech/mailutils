/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sieve.h>

void *
sieve_alloc (size_t size)
{
  void *p = malloc (size);
  if (!p)
    {
      mu_error ("not enough memory");
      abort ();
    }
  return p;
}

void *
sieve_palloc (list_t *pool, size_t size)
{
  void *p = malloc (size);
  if (p)
    {
      if (!*pool && list_create (pool))
	{
	  free (p);
	  return NULL;
	}
      list_append (*pool, p);
    }
  return p;
}

char *
sieve_pstrdup (list_t *pool, const char *str)
{
  size_t len;
  char *p;
  
  if (!str)
    return NULL;
  len = strlen (str);
  p = sieve_palloc (pool, len + 1);
  if (p)
    {
      memcpy (p, str, len);
      p[len] = 0;
    }
  return p;
}

void *
sieve_prealloc (list_t *pool, void *ptr, size_t size)
{
  void *newptr;
  
  if (*pool)
    list_remove (*pool, ptr);

  newptr = realloc (ptr, size);
  if (newptr)
    {
      if (!*pool && list_create (pool))
	{
	  free (newptr);
	  return NULL;
	}
      list_append (*pool, newptr);
    }
  return newptr;
}

void
sieve_pfree (list_t *pool, void *ptr)
{

  if (*pool)
    list_remove (*pool, ptr);
  free (ptr);
}  

void *
sieve_malloc (sieve_machine_t mach, size_t size)
{
  return sieve_palloc (&mach->memory_pool, size);
}

char *
sieve_mstrdup (sieve_machine_t mach, const char *str)
{
  return sieve_pstrdup (&mach->memory_pool, str);
}

void *
sieve_mrealloc (sieve_machine_t mach, void *ptr, size_t size)
{
  return sieve_prealloc (&mach->memory_pool, ptr, size);
}

void
sieve_mfree (sieve_machine_t mach, void *ptr)
{
  sieve_pfree (&mach->memory_pool, ptr);
}  

static int
_destroy_item (void *item, void *data)
{
  free (item);
  return 0;
}

void
sieve_slist_destroy (list_t *plist)
{
  if (!plist)
    return;
  list_do (*plist, _destroy_item, NULL);
  list_destroy (plist);
}

sieve_value_t *
sieve_value_create (sieve_data_type type, void *data)
{
  sieve_value_t *val = sieve_alloc (sizeof (*val));

  val->type = type;
  switch (type)
    {
    case SVT_NUMBER:
      val->v.number = * (long *) data;
      break;
      
    case SVT_STRING:
      val->v.string = data;
      break;
      
    case SVT_VALUE_LIST:
    case SVT_STRING_LIST:
      val->v.list = data;
      
    case SVT_TAG:
    case SVT_IDENT:
      val->v.string = data;
      break;

    case SVT_POINTER:
      val->v.ptr = data;
      break;
	
    default:
      sieve_compile_error (sieve_filename, sieve_line_num,
			   _("Invalid data type"));
      abort ();
    }
  return val;
}

sieve_value_t *
sieve_value_get (list_t vlist, size_t index)
{
  sieve_value_t *val = NULL;
  list_get (vlist, index, (void **)&val);
  return val;
}

void
sieve_compile_error (const char *filename, int linenum, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  sieve_error_count++;
  sieve_machine->parse_error_printer (sieve_machine->data, filename, linenum,
                                      fmt, ap);
  va_end (ap);
}

void
sieve_error (sieve_machine_t mach, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  mach->error_printer (mach->data, fmt, ap);
  va_end (ap);
}

void
sieve_debug_internal (sieve_printf_t printer, void *data, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  printer (data, fmt, ap);
  va_end (ap);
}

void
sieve_debug (sieve_machine_t mach, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  mach->debug_printer (mach->data, fmt, ap);
  va_end (ap);
}

void
sieve_log_action (sieve_machine_t mach, const char *action,
		  const char *fmt, ...)
{
  va_list ap;

  if (!mach->logger)
    return;
  va_start (ap, fmt);
  mach->logger (mach->data, mach->filename, mach->msgno, mach->msg,
		action, fmt, ap);
  va_end (ap);
}
  

int
_sieve_default_error_printer (void *unused, const char *fmt, va_list ap)
{
  return mu_verror (fmt, ap);
}

int
_sieve_default_parse_error (void *unused, const char *filename, int lineno,
			    const char *fmt, va_list ap)
{
  if (filename)
    fprintf (stderr, "%s:%d: ", filename, lineno);
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  return 0;
}

const char *
sieve_type_str (sieve_data_type type)
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

    case SVT_IDENT:
      return "ident";

    case SVT_VALUE_LIST:
      return "value-list";

    case SVT_POINTER:
      return "pointer";
    }

  return "unknown";
}

struct debug_data {
  sieve_printf_t printer;
  void *data;
};

static int
string_printer (char *s, struct debug_data *dbg)
{
  sieve_debug_internal (dbg->printer, dbg->data, "\"%s\" ", s);
  return 0;
}

static int
value_printer (sieve_value_t *val, struct debug_data *dbg)
{
  sieve_print_value (val, dbg->printer, dbg->data);
  sieve_debug_internal (dbg->printer, dbg->data, " ");
  return 0;
}

void
sieve_print_value (sieve_value_t *val, sieve_printf_t printer, void *data)
{
  struct debug_data dbg;

  dbg.printer = printer;
  dbg.data = data;

  sieve_debug_internal (printer, data, "%s(", sieve_type_str (val->type));
  switch (val->type)
    {
    case SVT_VOID:
      break;
      
    case SVT_NUMBER:
      sieve_debug_internal (printer, data, "%ld", val->v.number);
      break;
      
    case SVT_TAG:
    case SVT_IDENT:
    case SVT_STRING:
      sieve_debug_internal (printer, data, "%s", val->v.string);
      break;
      
    case SVT_STRING_LIST:
      list_do (val->v.list, (list_action_t*) string_printer, &dbg);
      break;

    case SVT_VALUE_LIST:
      list_do (val->v.list, (list_action_t*) value_printer, &dbg);

    case SVT_POINTER:
      sieve_debug_internal (printer, data, "%p", val->v.ptr);
    }
  sieve_debug_internal (printer, data, ")");
} 

void
sieve_print_value_list (list_t list, sieve_printf_t printer, void *data)
{
  sieve_value_t val;
  
  val.type = SVT_VALUE_LIST;
  val.v.list = list;
  sieve_print_value (&val, printer, data);
}

static int
tag_printer (sieve_runtime_tag_t *val, struct debug_data *dbg)
{
  sieve_debug_internal (dbg->printer, dbg->data, "%s", val->tag);
  if (val->arg)
    {
      sieve_debug_internal (dbg->printer, dbg->data, "(");
      sieve_print_value (val->arg, dbg->printer, dbg->data);
      sieve_debug_internal (dbg->printer, dbg->data, ")");
    }
  sieve_debug_internal (dbg->printer, dbg->data, " ");
  return 0;
}

void
sieve_print_tag_list (list_t list, sieve_printf_t printer, void *data)
{
  struct debug_data dbg;

  dbg.printer = printer;
  dbg.data = data;
  list_do (list, (list_action_t*) tag_printer, &dbg);
}

static int
tag_finder (void *item, void *data)
{
  sieve_runtime_tag_t *val = item;
  sieve_runtime_tag_t *target = data;

  if (strcmp (val->tag, target->tag) == 0)
    {
      target->arg = val->arg;
      return 1;
    }
  return 0;
}

int
sieve_tag_lookup (list_t taglist, char *name, sieve_value_t **arg)
{
  sieve_runtime_tag_t t;

  t.tag = name;
  if (list_do (taglist, tag_finder, &t))
    {
      if (arg)
	*arg = t.arg;
      return 1;
    }
  return 0;
}

int
sieve_mark_deleted (message_t msg, int deleted)
{
  attribute_t attr = 0;
  int rc;

  rc = message_get_attribute (msg, &attr);

  if (!rc)
    {
      if (deleted)
	rc = attribute_set_deleted (attr);
      else
	rc = attribute_unset_deleted (attr);
    }

  return rc;
}

int
sieve_vlist_do (sieve_value_t *val, list_action_t *ac, void *data)
{
  switch (val->type)
    {
    case SVT_VALUE_LIST:
    case SVT_STRING_LIST:
      return list_do (val->v.list, ac, data);
      
    default:
      return -1;
    }
}

struct comp_data {
  sieve_value_t *val;
  sieve_comparator_t comp;
  sieve_retrieve_t retr;
  void *data;
};

struct comp_data2 {
  char *sample;
  sieve_comparator_t comp;
};

int
_comp_action2 (void *item, void *data)
{
  struct comp_data2 *cp = data;
  return cp->comp (item, cp->sample);
}

int
_comp_action (void *item, void *data)
{
  struct comp_data *cp = data;
  struct comp_data2 d;
  int rc = 0;
  int i;

  d.comp = cp->comp;
  for (i = 0; cp->retr (item, cp->data, i, &d.sample) == 0; i++)
    if (d.sample)
      {	    
        rc = sieve_vlist_do (cp->val, _comp_action2, &d);
        free (d.sample);
      }
  return rc;
}

int
sieve_vlist_compare (sieve_value_t *a, sieve_value_t *b,
		     sieve_comparator_t comp, sieve_retrieve_t retr,
		     void *data)
{
  struct comp_data d;

  d.comp = comp;
  d.retr = retr;
  d.data = data;
  d.val = b;
  return sieve_vlist_do (a, _comp_action, &d);
}

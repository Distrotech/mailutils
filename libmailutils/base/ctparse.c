/* Content-Type (RFC 2045) parser for GNU Mailutils
   Copyright (C) 2016 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>

void
mu_param_free (void *data)
{
  struct mu_param *p = data;
  free (p->name);
  free (p->value);
  free (p);
} 

int
mu_param_cmp (void const *a, void const *b)
{
  struct mu_param const *p1 = a; 
  struct mu_param const *p2 = b;

  return mu_c_strcasecmp (p1->name, p2->name);
}

static int parse_param (const char **input_ptr, mu_content_type_t ct);
static int parse_params (const char *input, mu_content_type_t ct);
static int parse_subtype (const char *input, mu_content_type_t ct);
static int parse_type (const char *input, mu_content_type_t ct);

static int
parse_type (const char *input, mu_content_type_t ct)
{
  size_t i;

  for (i = 0; input[i] != '/'; i++)
    {
      if (input[i] == 0
	  || !(mu_isalnum (input[i]) || input[i] == '-' || input[i] == '_'))
	return MU_ERR_PARSE;
    }
  ct->type = malloc (i + 1);
  if (!ct->type)
    return ENOMEM;
  memcpy (ct->type, input, i);
  ct->type[i] = 0;

  return parse_subtype (input + i + 1, ct);
}

static char tspecials[] = "()<>@,;:\\\"/[]?=";

#define ISTOKEN(c) ((unsigned char)(c) > ' ' && !strchr (tspecials, c))

static int
parse_subtype (const char *input, mu_content_type_t ct)
{
  size_t i;

  for (i = 0; !(input[i] == 0 || input[i] == ';'); i++)
    {
      if (!ISTOKEN (input[i]))
	return MU_ERR_PARSE;
    }
  ct->subtype = malloc (i + 1);
  if (!ct->subtype)
    return ENOMEM;
  memcpy (ct->subtype, input, i);
  ct->subtype[i] = 0;

  return parse_params (input + i, ct);
}
    
static int
parse_params (const char *input, mu_content_type_t ct)
{
  int rc;

  rc = mu_list_create (&ct->param);
  if (rc)
    return rc;
  mu_list_set_destroy_item (ct->param, mu_param_free);
  mu_list_set_comparator (ct->param, mu_param_cmp);
  
  while (*input == ';')
    {
      input = mu_str_skip_class (input + 1, MU_CTYPE_SPACE);
      rc = parse_param (&input, ct);
      if (rc)
	return rc;
    }

  if (*input)
    {
      input = mu_str_skip_class (input, MU_CTYPE_SPACE);
      ct->trailer = strdup (input);
      if (!ct->trailer)
	return ENOMEM;
    }

  return rc;
}

static int
parse_param (const char **input_ptr, mu_content_type_t ct)
{
  const char *input = *input_ptr;
  size_t i = 0;
  size_t namelen;
  size_t valstart, vallen;
  struct mu_param *p;
  int rc;
  unsigned quotechar = 0;
  
  while (ISTOKEN (input[i]))
    i++;
  namelen = i;

  if (input[i] != '=')
    return MU_ERR_PARSE;
  i++;
  if (input[i] == '"')
    {
      i++;
      valstart = i;
      while (input[i] != '"')
	{
	  if (input[i] == '\\')
	    {
	      quotechar++;
	      i++;
	    }
	  if (!input[i])
	    return MU_ERR_PARSE;
	  i++;
	}
      vallen = i - valstart - quotechar;
      i++;
    }
  else
    {
      valstart = i;
      while (ISTOKEN (input[i]))
	i++;
      vallen = i - valstart;
    }

  p = malloc (sizeof (*p));
  if (!p)
    return ENOMEM;
  p->name = malloc (namelen + 1);
  p->value = malloc (vallen + 1);
  if (!p->name || !p->value)
    {
      mu_param_free (p);
      return ENOMEM;
    }

  memcpy (p->name, input, namelen);
  p->name[namelen] = 0;
  if (quotechar)
    {
      size_t j;
      const char *src = input + valstart;

      for (i = j = 0; j < vallen; i++, j++)
	{
	  if (src[j] == '\\')
	    j++;
	  p->value[i] = src[j];
	}
      p->value[i] = 0;
    }
  else
    {
      memcpy (p->value, input + valstart, vallen);
      p->value[vallen] = 0;
    }
  
  rc = mu_list_append (ct->param, p);
  if (rc)
    {
      mu_param_free (p);
      return rc;
    }

  *input_ptr = input + i;

  return 0;
}     
  
int
mu_content_type_parse (const char *input, mu_content_type_t *retct)
{
  int rc;
  mu_content_type_t ct;
  
  ct = calloc (1, sizeof (*ct));
  if (!ct)
    return errno;

  rc = parse_type (mu_str_skip_class (input, MU_CTYPE_SPACE), ct);
  if (rc)
    mu_content_type_destroy (&ct);
  else
    *retct = ct;

  return rc;
}

void
mu_content_type_destroy (mu_content_type_t *pptr)
{
  if (pptr && *pptr)
    {
      mu_content_type_t ct = *pptr;
      free (ct->type);
      free (ct->subtype);
      free (ct->trailer);
      mu_list_destroy (&ct->param);
      free (ct);
      *pptr = NULL;
    }
}

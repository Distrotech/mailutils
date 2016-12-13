/* Sieve variables extension (RFC 5229) 
   Copyright (C) 2016 Free Software Foundation, Inc.

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
#include <sieve-priv.h>
#include <ctype.h>

struct sieve_variable
{
  char *value;
};


/* FIXME: UTF support */
char *
mod_lower (mu_sieve_machine_t mach, char const *value)
{
  char *newval = mu_sieve_malloc (mach, strlen (value) + 1);
  char *p;

  for (p = newval; *value; p++, value++)
    *p = tolower (*value);
  *p = 0;
  return newval;
}

char *
mod_upper (mu_sieve_machine_t mach, char const *value)
{
  char *newval = mu_sieve_malloc (mach, strlen (value) + 1);
  char *p;

  for (p = newval; *value; p++, value++)
    *p = toupper (*value);
  *p = 0;
  return newval;
}

char *
mod_lowerfirst (mu_sieve_machine_t mach, char const *value)
{
  char *newval = mu_sieve_strdup (mach, value);
  *newval = tolower (*newval);
  return newval;  
}

char *
mod_upperfirst (mu_sieve_machine_t mach, char const *value)
{
  char *newval = mu_sieve_strdup (mach, value);
  *newval = toupper (*newval);
  return newval;  
}

char *
mod_quotewildcard (mu_sieve_machine_t mach, char const *value)
{
  size_t len;
  char *newval;
  char const *p;
  char *q;
  
  len = 0;
  for (p = value; *p; p++)
    {
      switch (*p)
	{
	case '*':
	case '?':
	case '\\':
	  len += 2;
	  break;
	default:
	  len++;
	}
    }

  newval = mu_sieve_malloc (mach, len + 1);
  for (p = value, q = newval; *p;)
    {
      switch (*p)
	{
	case '*':
	case '?':
	case '\\':
	  *q++ = '\\';
	}
      *q++ = *p++;
    }
  *q = 0;
  return newval;
}

char *
mod_length (mu_sieve_machine_t mach, char const *value)
{
  char *newval, *p;
  int rc = mu_asprintf (&newval, "%zu", strlen (value));
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_asprintf", NULL, rc);
      mu_sieve_abort (mach);
    }
  p = mu_sieve_strdup (mach, newval);
  free (newval);
  return p;
}

static mu_sieve_tag_def_t set_tags[] = {
  { "lower", SVT_VOID },
  { "upper", SVT_VOID },
  { "lowerfirst", SVT_VOID },
  { "upperfirst", SVT_VOID },
  { "quotewildcard", SVT_VOID },
  { "length", SVT_VOID },
  { NULL }
};

struct modprec
{
  char *name;
  unsigned prec;
  char *(*modify) (mu_sieve_machine_t mach, char const *value);
};

static struct modprec modprec[] = {
  { "lower",         40, mod_lower },
  { "upper",         40, mod_upper },
  { "lowerfirst",    30, mod_lowerfirst },
  { "upperfirst",    30, mod_upperfirst },
  { "quotewildcard", 20, mod_quotewildcard },
  { "length",        10, mod_length },
};

static struct modprec *
findprec (char const *name)
{
  int i;
  
  for (i = 0; i < MU_ARRAY_SIZE (modprec); i++)
    if (strcmp (modprec[i].name, name) == 0)
      return &modprec[i];
  mu_error (_("%s:%d: INTERNAL ERROR, please report"), __FILE__, __LINE__);
  abort ();
}

static int
sieve_action_set (mu_sieve_machine_t mach)
{
  size_t i;
  char *name;
  char *value;
  struct sieve_variable *vptr;
  int rc;
  
  mu_sieve_get_arg (mach, 0, SVT_STRING, &name);
  mu_sieve_get_arg (mach, 1, SVT_STRING, &value);

  value = mu_sieve_strdup (mach, value);
  for (i = 0; i < mach->tagcount; i++)
    {
      mu_sieve_value_t *p = mu_sieve_get_tag_n (mach, i);
      char *str = findprec (p->tag)->modify (mach, value);
      mu_sieve_free (mach, value);
      value = str;
    }

  rc = mu_assoc_ref_install (mach->vartab, name, (void **)&vptr);
  switch (rc)
    {
    case 0:
      break;

    case MU_ERR_EXISTS:
      mu_sieve_free (mach, vptr->value);
      break;

    default:
      mu_sieve_error (mach, "mu_assoc_ref_install: %s", mu_strerror (rc));
      mu_sieve_abort (mach);
    }
  vptr->value = value;
  return 0;
}  

static int
set_tag_checker (mu_sieve_machine_t mach)
{
  int i, j;
  
  /* Sort tags by decreasing priority value (RFC 5229, 4.1) */
  for (i = 1; i < mach->tagcount; i++)
    {
      mu_sieve_value_t tmp = *mu_sieve_get_tag_n (mach, i);
      int tmp_prec = findprec (tmp.tag)->prec;

      for (j = i - 1; j >= 0; j--)
	{
	  mu_sieve_value_t *t = mu_sieve_get_tag_n (mach, j);
	  int prec = findprec (t->tag)->prec;
	  if (prec < tmp_prec)
	    *mu_sieve_get_tag_n (mach, j + 1) = *t;
	  else if (prec == tmp_prec)
	    {
	      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus,
				_("%s and %s can't be used together"),
				tmp.tag, t->tag);
	      mu_i_sv_error (mach);
	      return 1;
	    }	      
	  else
	    break;
	}
      *mu_sieve_get_tag_n (mach, j + 1) = tmp;
    }
  return 0;
}

static mu_sieve_tag_group_t set_tag_groups[] = {
  { set_tags, set_tag_checker },
  { NULL }
};

static mu_sieve_data_type set_args[] = {
  SVT_STRING,
  SVT_STRING,
  SVT_VOID
};

static int
retrieve_string (void *item, void *data, size_t idx, char **pval)
{
  if (idx)
    return MU_ERR_NOENT;
  *pval = strdup ((char*)item);
  if (!*pval)
    return errno;
  return 0;
}

int
fold_string (void *item, void *data, void *prev, void **ret)
{
  char *str = item;
  size_t count = *(size_t*) prev;

  /* The "relational" extension [RELATIONAL] adds a match type called
     ":count".  The count of a single string is 0 if it is the empty
     string, or 1 otherwise.  The count of a string list is the sum of the
     counts of the member strings.
  */
  if (*str)
    ++count;
  *(size_t*)ret = count;
  return 0;
}
    
/* RFC 5229, 5. Test string */
int
sieve_test_string (mu_sieve_machine_t mach)
{
  mu_sieve_value_t *source, *key_list;
  
  source = mu_sieve_get_arg_untyped (mach, 0);
  key_list = mu_sieve_get_arg_untyped (mach, 1);

  return mu_sieve_vlist_compare (mach, source, key_list,
				 retrieve_string, fold_string, mach);
}

static mu_sieve_data_type string_args[] = {
  SVT_STRING_LIST,
  SVT_STRING_LIST,
  SVT_VOID
};

static mu_sieve_tag_group_t string_tag_groups[] = {
  { mu_sieve_match_part_tags, mu_sieve_match_part_checker },
  { NULL }
};

int
mu_i_sv_expand_variables (char const *input, size_t len,
			  char **exp, void *data)
{
  mu_sieve_machine_t mach = data;
  if (mu_isdigit (*input))
    {
      char *p;
      size_t idx = 0;
      
      while (len)
	{
	  if (mu_isdigit (*input))
	    {
	      int d = *input - '0';
	      idx = idx * 10 + d;
	      input++;
	      len--;
	    }
	  else
	    return 1;
	}

      if (idx > mach->match_count)
	{
	  *exp = NULL;
	  return 0;
	}
      
      len = mach->match_buf[idx].rm_eo - mach->match_buf[idx].rm_so;
      p = malloc (len + 1);
      if (!p)
	{
	  mu_sieve_error (mach, "%s", mu_strerror (errno));
	  mu_sieve_abort (mach);
	}
      memcpy (p, mach->match_string + mach->match_buf[idx].rm_so, len);
      p[len] = 0;
      *exp = p;
    }
  else if (mu_isalpha (*input))
    {
      size_t i;
      char *name;
      struct sieve_variable *var;

      for (i = 0; i < len; i++)
	if (!(mu_isalnum (input[i]) || input[i] == '_'))
	  return 1;

      name = malloc (len + 1);
      if (!name)
	{
	  mu_sieve_error (mach, "%s", mu_strerror (errno));
	  mu_sieve_abort (mach);
	}
      memcpy (name, input, len);
      name[len] = 0;

      var = mu_assoc_ref (mach->vartab, name);

      free (name);
      
      if (var)
	{
	  *exp = strdup (var->value);
	  if (!*exp)
	    {
	      mu_sieve_error (mach, "%s", mu_strerror (errno));
	      mu_sieve_abort (mach);
	    }
	}
      else
	*exp = NULL;
    }
  else
    return 1;
  return 0;
}

int
mu_sieve_require_variables (mu_sieve_machine_t mach)
{
  int rc;
  
  if (mach->vartab)
    return 0;

  rc = mu_assoc_create (&mach->vartab, sizeof (struct sieve_variable),
			MU_ASSOC_ICASE);
  if (rc)
    mu_sieve_error (mach, "mu_assoc_create: %s", mu_strerror (rc));

  if (rc == 0)
    {
      mu_sieve_register_action (mach, "set", sieve_action_set, 
				set_args, set_tag_groups, 1);
      mu_sieve_register_test (mach, "string", sieve_test_string,
			      string_args, string_tag_groups, 1);
    }
  return rc;
}
  
int
mu_sieve_has_variables (mu_sieve_machine_t mach)
{
  return mach->vartab != NULL;
}

void
mu_i_sv_copy_variables (mu_sieve_machine_t child, mu_sieve_machine_t parent)
{
  mu_iterator_t itr;
  
  mu_sieve_require_variables (child);
	  
  mu_assoc_get_iterator (parent->vartab, &itr);

  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      const char *name;
      struct sieve_variable const *val;
      struct sieve_variable newval;

      mu_iterator_current_kv (itr, (const void **)&name, (void**)&val);
      newval.value = mu_sieve_strdup (child, val->value);
      mu_assoc_install (child->vartab, name, &newval);
    }

  mu_iterator_destroy (&itr);	  
}

 
  


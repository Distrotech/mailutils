%{
/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include <mh.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
  
struct mh_alias
{
  char *name;
  list_t rcpt_list;
  int inclusive;
};

static list_t alias_list;

static list_t
list_create_or_die ()
{
  int status;
  list_t list;

  status = list_create (&list);
  if (status)
    {
      ali_parse_error (_("can't create list: %s"), mu_strerror (status));
      exit (1);
    }
  return list;
}

static char *
ali_list_to_string (list_t *plist)
{
  size_t n;
  char *string;
  
  list_count (*plist, &n);
  if (n == 1)
    {
      list_get (*plist, 0, (void **)&string);
    }
  else
    {
      char *p;
      size_t length = 0;
      iterator_t itr;
      list_get_iterator (*plist, &itr);
      for (iterator_first (itr); !iterator_is_done (itr); iterator_next(itr))
	{
	  char *s;
	  iterator_current (itr, (void**) &s);
	  length += strlen (s) + 1;
	}
  
      string = xmalloc (length + 1);
      p = string;
      for (iterator_first (itr); !iterator_is_done (itr); iterator_next(itr))
	{
	  char *s;
	  iterator_current (itr, (void**) &s);
	  strcpy (p, s);
	  p += strlen (s);
	  *p++ = ' ';
	}
      *--p = 0;
      iterator_destroy (&itr);
    }
  list_destroy (plist);
  return string;
}

static list_t unix_group_to_list __P((char *name));
static list_t unix_gid_to_list __P((char *name));
static list_t unix_passwd_to_list __P((void));

int yyerror __P((char *s));
int yylex __P((void));

%}

%union {
  char *string;
  list_t list;
  struct mh_alias *alias;
}

%token <string> STRING
%type <list>  address_list address_group string_list
%type <string> address
%type <alias> alias

%%

input        : /* empty */
             | alias_list
             | alias_list nl
             | nl alias_list
             | nl alias_list nl
             ;

alias_list   : alias
               {
		 if (!alias_list)
		   alias_list = list_create_or_die ();
		 list_append (alias_list, $1);
	       }
             | alias_list nl alias
               {
		 list_append (alias_list, $3);
	       }
             ;

nl           : '\n'
             | nl '\n'
             ;

alias        : STRING ':' { ali_verbatim (1); } address_group
               {
		 ali_verbatim (0);
		 $$ = xmalloc (sizeof (*$$));
		 $$->name = $1;
		 $$->rcpt_list = $4;
		 $$->inclusive = 0;
	       }
             | STRING ';' { ali_verbatim (1); } address_group
               {
		 ali_verbatim (0);
		 $$ = xmalloc (sizeof (*$$));
		 $$->name = $1;
		 $$->rcpt_list = $4;
		 $$->inclusive = 1;
	       }
             ;

address_group: address_list
             | '=' STRING
               {
		 $$ = unix_group_to_list ($2);
		 free ($2);
	       }
             | '+' STRING
               {
		 $$ = unix_gid_to_list ($2);
		 free ($2);
	       }
             | '*'
               {
		 $$ = unix_passwd_to_list ();
	       }
             ;

address_list : address
               {
		 $$ = list_create_or_die ();
		 list_append ($$, $1);
	       }
             | address_list ',' address
               {
		 list_append ($1, $3);
		 $$ = $1;
	       }
             ;

address      : string_list
               {
		 $$ = ali_list_to_string (&$1);
	       }
             ;

string_list  : STRING
               {
		 list_create(&$$);
		 list_append($$, $1);
	       }
             | string_list STRING
               {
		 list_append($1, $2);
		 $$ = $1;
	       }
             ;

%%

static list_t
ali_list_dup (list_t src)
{
  list_t dst;
  iterator_t itr;

  if (list_create (&dst))
    return NULL;

  if (list_get_iterator (src, &itr))
    {
      list_destroy (&dst);
      return NULL;
    }
  
  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      void *data;
      iterator_current (itr, (void **)&data);
      list_append (dst, data);
    }
  iterator_destroy (&itr);
  return dst;
}

static int
ali_member (list_t list, char *name)
{
  iterator_t itr;
  int found = 0;

  if (list_get_iterator (list, &itr))
    return 0;
  for (iterator_first (itr); !found && !iterator_is_done (itr);
       iterator_next (itr))
    {
      char *item;
      address_t tmp;
      
      iterator_current (itr, (void **)&item);
      if (strcmp (item, name) == 0)
	found = 1;
      else if (address_create (&tmp, item) == 0)
	{
	  found = address_contains_email (tmp, name);
	  address_destroy (&tmp);
	}
    }
  iterator_destroy (&itr);
  return found;
}

int
aliascmp (char *pattern, char *name)
{
  int len = strlen (pattern);

  if (len > 1 && pattern[len - 1] == '*')
    return strncmp (pattern, name, len - 2);
  return strcmp (pattern, name);
}


int
_insert_list (list_t list, void *prev, list_t new_list)
{
  iterator_t itr;

  if (list_get_iterator (new_list, &itr))
    return 1;
  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      void *item;
      
      iterator_current (itr, &item);
      list_insert (list, prev, item, 0);
      prev = item;
    }
  iterator_destroy (&itr);
  return 0;
}

static int mh_alias_get_internal __P((char *name, iterator_t start,
				      list_t *return_list, int *inclusive));

int
alias_expand_list (list_t name_list, iterator_t orig_itr, int *inclusive)
{
  iterator_t itr;

  if (list_get_iterator (name_list, &itr))
    return 1;
  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      char *name;
      list_t exlist;
      
      iterator_current (itr, (void **)&name);
      if (mh_alias_get_internal (name, orig_itr, &exlist, inclusive) == 0)
	{
	  _insert_list (name_list, name, exlist);
	  list_remove (name_list, name);
	  list_destroy (&exlist);
	}
    }
  iterator_destroy (&itr);
  return 0;
}  

/* Look up the named alias. If found, return the list of recipient
   names associated with it */
static int
mh_alias_get_internal (char *name, iterator_t start, list_t *return_list,
		       int *inclusive) 
{
  iterator_t itr;
  int rc = 1;

  if (!start)
    {
      if (list_get_iterator (alias_list, &itr))
	return 1;
      iterator_first (itr);
    }
  else
    {
      iterator_dup (&itr, start);
      iterator_next (itr);
    }
	
  for (; !iterator_is_done (itr); iterator_next (itr))
    {
      struct mh_alias *alias;
      iterator_current (itr, (void **)&alias);
      if (inclusive)
	*inclusive |= alias->inclusive;
      if (aliascmp (alias->name, name) == 0)
	{
	  *return_list = ali_list_dup (alias->rcpt_list);
	  alias_expand_list (*return_list, itr, inclusive);
	  rc = 0;
	  break;
	}
    }
  
  iterator_destroy (&itr);
  return rc;
}

int
mh_alias_get (char *name, list_t *return_list)
{
  return mh_alias_get_internal (name, NULL, return_list, NULL);
}

int
mh_alias_get_address (char *name, address_t *paddr, int *incl)
{
  iterator_t itr;
  list_t list;
  const char *domain = NULL;
  
  if (mh_alias_get_internal (name, NULL, &list, incl))
    return 1;
  if (list_is_empty (list))
    {
      list_destroy (&list);
      return 1;
    }
  
  if (list_get_iterator (list, &itr) == 0)
    {
      for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
	{
	  char *item;
	  address_t a;
	  char *ptr = NULL; 

	  iterator_current (itr, (void **)&item);
	  if (incl && *incl)
	    {
	      if (strchr (item, '@') == 0)
		{
		  if (!domain)
		    mu_get_user_email_domain (&domain);
		  asprintf (&ptr, "\"%s\" <%s@%s>", name, item, domain);
		}
	      else
		asprintf (&ptr, "\"%s\" <%s>", name, item);
	      item = ptr;
	    }
	  if (address_create (&a, item))
	    {
	      mh_error (_("Error expanding aliases -- invalid address `%s'"),
			item);
	    }
	  else
	    {
	      address_union (paddr, a);
	      address_destroy (&a);
	    }
	  if (ptr)
	    free (ptr);
	}
      iterator_destroy (&itr);
    }
  list_destroy (&list);
  return 0;
}

/* Look up the given user name in the aliases. Return the list of
   alias names this user is member of */
int
mh_alias_get_alias (char *uname, list_t *return_list)
{
  iterator_t itr;
  int rc = 1;
  
  if (list_get_iterator (alias_list, &itr))
    return 1;
  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      struct mh_alias *alias;
      iterator_current (itr, (void **)&alias);
      if (ali_member (alias->rcpt_list, uname))
	{
	  if (*return_list == NULL && list_create (return_list))
	    break;
	  list_append (*return_list, alias->name);
	  rc = 0;
	}
    }
  
  iterator_destroy (&itr);
  return rc;
}

void
mh_alias_enumerate (mh_alias_enumerator_t fun, void *data)
{
  iterator_t itr;
  int rc = 0;
  
  if (list_get_iterator (alias_list, &itr))
    return ;
  for (iterator_first (itr);
       rc == 0 && !iterator_is_done (itr);
       iterator_next (itr))
    {
      struct mh_alias *alias;
      list_t tmp;
      
      iterator_current (itr, (void **)&alias);

      tmp = ali_list_dup (alias->rcpt_list);
      alias_expand_list (tmp, itr, NULL);

      rc = fun (alias->name, tmp, data);
      list_destroy (&tmp);
    }
  iterator_destroy (&itr);
}

static list_t
unix_group_to_list (char *name)
{
  struct group *grp = getgrnam (name);
  list_t lst = list_create_or_die ();
  
  if (grp)
    {
      char **p;

      for (p = grp->gr_mem; *p; p++)
	list_append (lst, strdup (*p));
    }      
  
  return lst;
}

static list_t
unix_gid_to_list (char *name)
{
  struct group *grp = getgrnam (name);
  list_t lst = list_create_or_die ();

  if (grp)
    {
      struct passwd *pw;
      setpwent();
      while ((pw = getpwent ()))
	{
	  if (pw->pw_gid == grp->gr_gid)
	    list_append (lst, strdup (pw->pw_name));
	}
      endpwent();
    }
  return lst;
}

static list_t
unix_passwd_to_list ()
{
  list_t lst = list_create_or_die ();
  struct passwd *pw;

  setpwent();
  while ((pw = getpwent ()))
    {
      if (pw->pw_uid > 200)
	list_append (lst, strdup (pw->pw_name));
    }
  endpwent();
  return lst;
}

int
mh_read_aliases ()
{
  char *p, *sp;
  
  p = mh_global_profile_get ("Aliasfile", NULL);
  if (p)
    for (p = strtok_r (p, " \t", &sp); p; p = strtok_r (NULL, " \t", &sp))
      mh_alias_read (p, 1);
  mh_alias_read (DEFAULT_ALIAS_FILE, 0);
  return 0;
}

/* Implementation of file-system folder for GNU Mailutils
   Copyright (C) 1999-2001, 2003-2008, 2010-2012 Free Software
   Foundation, Inc.

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

#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <glob.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>

#include <mailutils/sys/folder.h>
#include <mailutils/sys/registrar.h>

#include <mailutils/auth.h>
#include <mailutils/url.h>
#include <mailutils/stream.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/debug.h>
#include <mailutils/property.h>
#include <mailutils/iterator.h>

/* File-system folder is shared between UNIX mbox, maildir and MH
   mailboxes.  It implements all usual folder methods, excepting
   for _delete, which is implemented on the mailbox level.  See
   comment to mu_folder_delete in folder.c */

struct _mu_fsfolder
{
  char *dirname;
  mu_property_t subscription;
};

static int
open_subscription (struct _mu_fsfolder *folder)
{
  int rc;
  mu_property_t prop;
  mu_stream_t str;
  char *filename = mu_make_file_name (folder->dirname, ".mu-subscr");
  
  rc = mu_file_stream_create (&str, filename, MU_STREAM_RDWR|MU_STREAM_CREAT);
  if (rc)
    return rc;
  rc = mu_property_create_init (&prop, mu_assoc_property_init, str);
  free (filename);
  if (rc == 0)
    folder->subscription = prop;
  return rc;
}


static char *
get_pathname (const char *dirname, const char *basename)
{
  char *pathname = NULL, *p;

  /* Skip eventual protocol designator. */
  p = strchr (dirname, ':');
  if (p && p[1] == '/' && p[2] == '/')
    dirname = p + 3;
  
  /* null basename gives dirname.  */
  if (basename == NULL)
    pathname = strdup (dirname ? dirname : ".");
  /* Absolute.  */
  else if (basename[0] == '/')
    pathname = strdup (basename);
  /* Relative.  */
  else
    {
      size_t baselen = strlen (basename);
      size_t dirlen = strlen (dirname);
      while (dirlen > 0 && dirname[dirlen-1] == '/')
	dirlen--;
      pathname = calloc (dirlen + baselen + 2, sizeof (char));
      if (pathname)
	{
	  memcpy (pathname, dirname, dirlen);
	  pathname[dirlen] = '/';
	  strcpy (pathname + dirlen + 1, basename);
	}
    }
  return pathname;
}

static void
_fsfolder_destroy (mu_folder_t folder)
{
  if (folder->data)
    {
      struct _mu_fsfolder *fsfolder = folder->data;
      free (fsfolder->dirname);
      mu_property_destroy (&fsfolder->subscription);
      free (folder->data);
      folder->data = NULL;
    }
}

/* Noop. */
static int
_fsfolder_open (mu_folder_t folder, int flags MU_ARG_UNUSED)
{
  struct _mu_fsfolder *fsfolder = folder->data;
  if (flags & MU_STREAM_CREAT)
    {
      return (mkdir (fsfolder->dirname, S_IRWXU) == 0) ? 0 : errno;
    }
  return 0;
}

/*  Noop.  */
static int
_fsfolder_close (mu_folder_t folder MU_ARG_UNUSED)
{
  int rc = 0;
  struct _mu_fsfolder *fsfolder = folder->data;
  
  if (fsfolder->subscription)
    rc = mu_property_save (fsfolder->subscription);
  return rc;
}

static int
_fsfolder_rename (mu_folder_t folder, const char *oldpath,
		    const char *newpath)
{
  struct _mu_fsfolder *fsfolder = folder->data;
  if (oldpath && newpath)
    {
      int status = 0;
      char *pathold = get_pathname (fsfolder->dirname, oldpath);
      if (pathold)
	{
	  char *pathnew = get_pathname (fsfolder->dirname, newpath);
	  if (pathnew)
	    {
	      if (access (pathnew, F_OK) == 0)
		status = EEXIST;
	      else if (rename (pathold, pathnew) != 0)
		status = errno;
	      free (pathnew);
	    }
	  else
	    status = ENOMEM;
	  free (pathold);
	}
      else
	status = ENOMEM;
      return status;
    }
  return EINVAL;
}

struct inode_list           /* Inode/dev number list used to cut off
			       recursion */
{
  struct inode_list *next;
  ino_t inode;
  dev_t dev;
};

struct search_data
{
  mu_list_t result;
  mu_folder_enumerate_fp enumfun;
  void *enumdata;
  char *dirname;
  size_t dirlen;
  void *pattern;
  int flags;
  size_t max_level;
  size_t errcnt;
  mu_folder_t folder;
};

static int
inode_list_lookup (struct inode_list *list, struct stat *st)
{
  for (; list; list = list->next)
    if (list->inode == st->st_ino && list->dev == st->st_dev)
      return 1;
  return 0;
}

static int
list_helper (struct search_data *data, mu_record_t record,
	     const char *dirname, size_t level,
	     struct inode_list *ilist)
{
  DIR *dirp;
  struct dirent *dp;
  int stop = 0;
    
  if (data->max_level && level > data->max_level)
    return 0;

  dirp = opendir (dirname);
  if (dirp == NULL)
    {
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("list_helper cannot open directory %s: %s",
		 dirname, mu_strerror (errno)));
      data->errcnt++;
      return 1;
    }

  if (!record)
    {
      int type;
      mu_registrar_lookup (dirname, MU_FOLDER_ATTRIBUTE_ALL,
			   &record, &type);
    }
  
  while ((dp = readdir (dirp)))
    {
      char const *ename = dp->d_name;
      char *fname;
      struct stat st;
      
      if (ename[ename[0] != '.' ? 0 : ename[1] != '.' ? 1 : 2] == 0)
	continue;
      if (strncmp (ename, ".mu-", 4) == 0)
	continue;
      fname = get_pathname (dirname, ename);
      if (lstat (fname, &st) == 0)
	{
	  int f;
	  if (S_ISDIR (st.st_mode))
	    f = MU_FOLDER_ATTRIBUTE_DIRECTORY;
	  else if (S_ISREG (st.st_mode))
	    f = MU_FOLDER_ATTRIBUTE_FILE;
	  else if (S_ISLNK (st.st_mode))
	    f = MU_FOLDER_ATTRIBUTE_LINK;
	  else
	    f = 0;
	  if (mu_record_list_p (record, ename, f))
	    {
	      if (data->folder->_match == NULL
		  || data->folder->_match (fname + data->dirlen +
					   ((data->dirlen > 1
					     && data->dirname[data->dirlen-1] != '/') ?
					    1 : 0),
					   data->pattern,
					   data->flags) == 0)
		{
		  char *refname = fname;
		  int type = 0;
		  struct mu_list_response *resp;
		  mu_record_t rec = NULL;
		  
		  resp = malloc (sizeof (*resp));
		  if (resp == NULL)
		    {
		      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
				("list_helper: %s", mu_strerror (ENOMEM)));
		      data->errcnt++;
		      free (fname);
		      continue;
		    }
		  
		  mu_registrar_lookup (refname, MU_FOLDER_ATTRIBUTE_ALL,
				       &rec, &type);
		  
		  resp->name = fname;
		  resp->level = level;
		  resp->separator = '/';
		  resp->type = type;
		  resp->format = rec;
		  
		  if (resp->type == 0)
		    {
		      free (resp->name);
		      free (resp);
		      continue;
		    }
		  
		  if (data->enumfun)
		    {
		      if (data->enumfun (data->folder, resp, data->enumdata))
			{
			  free (resp->name);
			  free (resp);
			  stop = 1;
			  break;
			}
		    }
		  
		  if (data->result)
		    {
		      fname = NULL;
		      mu_list_append (data->result, resp);
		    }
		  else
		    free (resp);
		  
		  if ((type & MU_FOLDER_ATTRIBUTE_DIRECTORY)
		      && !inode_list_lookup (ilist, &st))
		    {
		      struct inode_list idata;
		      
		      idata.inode = st.st_ino;
		      idata.dev   = st.st_dev;
		      idata.next  = ilist;
		      stop = list_helper (data, rec, refname, level + 1,
					  &idata);
		    }
		}
	      else if (S_ISDIR (st.st_mode))
		{
		  struct inode_list idata;
		  
		  idata.inode = st.st_ino;
		  idata.dev   = st.st_dev;
		  idata.next  = ilist;
		  stop = list_helper (data, NULL, fname, level + 1, &idata);
		}
	    }
	}
      else
	{
	  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		    ("list_helper cannot stat %s: %s",
		     fname, mu_strerror (errno)));
	}
      free (fname);
    }
  closedir (dirp);
  return stop;
}

static int
_fsfolder_list (mu_folder_t folder, const char *ref,
		void *pattern,
		int flags,
		size_t max_level,
		mu_list_t flist,
		mu_folder_enumerate_fp enumfun, void *enumdata)
{
  struct _mu_fsfolder *fsfolder = folder->data;
  struct inode_list iroot;
  struct search_data sdata;
  
  memset (&iroot, 0, sizeof iroot);
  sdata.dirname = get_pathname (fsfolder->dirname, ref);
  sdata.dirlen = strlen (sdata.dirname);
  sdata.result = flist;
  sdata.enumfun = enumfun;
  sdata.enumdata = enumdata;
  sdata.pattern = pattern;
  sdata.flags = flags;
  sdata.max_level = max_level;
  sdata.folder = folder;
  sdata.errcnt = 0;
  list_helper (&sdata, NULL, sdata.dirname, 0, &iroot);
  free (sdata.dirname);
  /* FIXME: error code */
  return 0;
}

static int
_fsfolder_lsub (mu_folder_t folder, const char *ref, const char *name,
		mu_list_t flist)
{
  struct _mu_fsfolder *fsfolder = folder->data;
  int rc;
  char *pattern;
  mu_iterator_t itr;
  
  if (name == NULL || *name == '\0')
    name = "*";

  if (!fsfolder->subscription && (rc = open_subscription (fsfolder)))
    return rc;
    
  pattern = mu_make_file_name (ref, name);
  
  rc = mu_property_get_iterator (fsfolder->subscription, &itr);
  if (rc == 0)
    {
      for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  const char *key, *val;
	  
	  mu_iterator_current_kv (itr, (const void **)&key, (void**)&val);

	  if (fnmatch (pattern, key, 0) == 0)
	    {
	      struct mu_list_response *resp;
	      resp = malloc (sizeof (*resp));
	      if (resp == NULL)
		{
		  rc = ENOMEM;
		  break;
		}
	      else if ((resp->name = strdup (key)) == NULL)
		{
		  free (resp);
		  rc = ENOMEM;
		  break;
		}
	      resp->type = MU_FOLDER_ATTRIBUTE_FILE;
	      resp->level = 0;
	      resp->separator = '/';
	      rc = mu_list_append (flist, resp);
	      if (rc)
		{
		  free (resp);
		  break;
		}
	    }
	}
      mu_iterator_destroy (&itr);
    }
  free (pattern);
  return rc;
}

static int
_fsfolder_subscribe (mu_folder_t folder, const char *name)
{
  struct _mu_fsfolder *fsfolder = folder->data;
  int rc;
  
  if (!fsfolder->subscription && (rc = open_subscription (fsfolder)))
    return rc;

  return mu_property_set_value (fsfolder->subscription, name, "", 1);
}  

static int
_fsfolder_unsubscribe (mu_folder_t folder, const char *name)
{
  struct _mu_fsfolder *fsfolder = folder->data;
  int rc;

  if (!fsfolder->subscription && (rc = open_subscription (fsfolder)))
    return rc;

  return mu_property_unset (fsfolder->subscription, name);
}

static int
_fsfolder_get_authority (mu_folder_t folder, mu_authority_t *pauth)
{
  int status = 0;
  if (folder->authority == NULL)
    status = mu_authority_create_null (&folder->authority, folder);
  if (!status && pauth)
    *pauth = folder->authority;
  return status;
}

int
_mu_fsfolder_init (mu_folder_t folder)
{
  struct _mu_fsfolder *dfolder;
  int status = 0;

  /* We create an authority so the API is uniform across the mailbox
     types. */
  status = _fsfolder_get_authority (folder, NULL);
  if (status != 0)
    return status;

  dfolder = folder->data = calloc (1, sizeof (*dfolder));
  if (dfolder == NULL)
    return ENOMEM;

  status = mu_url_aget_path (folder->url, &dfolder->dirname);
  if (status == MU_ERR_NOENT)
    {
      dfolder->dirname = malloc (2);
      if (dfolder->dirname == NULL)
	status = ENOMEM;
      else
	{
	  strcpy (dfolder->dirname, ".");
	  status = 0;
	}
    }
  
  if (status)  
    {
      free (dfolder);
      folder->data = NULL;
      return status;
    }

  folder->_destroy = _fsfolder_destroy;

  folder->_open = _fsfolder_open;
  folder->_close = _fsfolder_close;

  folder->_list = _fsfolder_list;
  folder->_lsub = _fsfolder_lsub;
  folder->_subscribe = _fsfolder_subscribe;
  folder->_unsubscribe = _fsfolder_unsubscribe;
  folder->_delete = NULL;
  folder->_rename = _fsfolder_rename;
  return 0;
}


/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Initialize MH applications. */

#include <mh.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

char mh_list_format[] = 
"%4(msg)%<(cur)+%| %>%<{replied}-%?{encrypted}E%| %>"
"%02(mon{date})/%02(mday{date})"
"%<{date} %|*%>"
"%<(mymbox{from})%<{to}To:%14(friendly{to})%>%>"
"%<(zero)%17(friendly{from})%>"
"  %{subject}%<{body}<<%{body}>>%>";


void
mh_init ()
{
  list_t bookie;

  /* Register mailbox formats */
  registrar_get_list (&bookie);
  list_append (bookie, mh_record);
  list_append (bookie, mbox_record);
  list_append (bookie, path_record);
  list_append (bookie, pop_record);
  list_append (bookie, imap_record);
  /* Possible supported mailers.  */
  list_append (bookie, sendmail_record);
  list_append (bookie, smtp_record);

  /* Read user's profile */
  mh_read_profile ();
}

void
mh_init2 ()
{
  mh_current_folder ();
  mh_global_sequences_get ("cur", NULL);
}

int
mh_read_formfile (char *name, char **pformat)
{
  FILE *fp;
  struct stat st;
  char *ptr;
  size_t off = 0;
  char *format_str;
  
  if (stat (name, &st))
    {
      mh_error ("can't stat format file %s: %s", name, strerror (errno));
      return -1;
    }
  
  fp = fopen (name, "r");
  if (!fp)
    {
      mh_error ("can't open format file %s: %s", name, strerror (errno));
      return -1;
    }

  format_str = xmalloc (st.st_size+1);
  while ((ptr = fgets (format_str + off, st.st_size - off, fp)) != NULL)
    {
      int len = strlen (ptr);
      if (len == 0)
	break;

      if (*ptr == '%' && ptr[1] == ';')
	continue;
      
      if (len > 0 && ptr[len-1] == '\n')
	{
	  if (ptr[len-2] == '\\')
	    {
	      len -= 2;
	      ptr[len] = 0;
	    }
	}
      off += len;
    }
  if (off > 0 && format_str[off-1] == '\n')
    off--;
  format_str[off] = 0;
  fclose (fp);
  *pformat = format_str;
  return 0;
}

static char *my_name;
static char *my_email;

void
mh_get_my_name (char *name)
{
  if (!name)
    {
      struct passwd *pw = getpwuid (getuid ());
      if (!pw)
	{
	  mh_error ("can't determine my username");
	  return;
	}
      name = pw->pw_name;
    }

  my_name = strdup (name);
  my_email = mu_get_user_email (name);
}

int
mh_is_my_name (char *name)
{
  if (!my_email)
    mh_get_my_name (NULL);
  return strcasecmp (name, my_email) == 0;
}

char *
mh_my_email ()
{
  if (!my_email)
    mh_get_my_name (NULL);
  return my_email;
}

int
mh_check_folder (char *pathname, int confirm)
{
  char *p;
  struct stat st;
  
  if ((p = strchr (pathname, ':')) != NULL)
    p++;
  else
    p = pathname;
  
  if (stat (p, &st))
    {
      if (errno == ENOENT)
	{
	  if (!confirm || mh_getyn ("Create folder \"%s\"", p))
	    {
	      int perm = 0711;
	      char *pb = mh_global_profile_get ("Folder-Protect", NULL);
	      if (pb)
		perm = strtoul (pb, NULL, 8);
	      if (mkdir (p, perm)) 
		{
		  mh_error ("Can't create directory %s: %s",
			    p, strerror (errno));
		  return 1;
		}
	      return 0;
	    }
	  else
	    return 1;
	}
      else
	{
	  mh_error ("can't stat %s: %s", p, strerror (errno));
	  return 1;
	}
    }
  return 0;
}

int
mh_getyn (const char *fmt, ...)
{
  va_list ap;
  char repl[64];
  
  va_start (ap, fmt);
  while (1)
    {
      char *p;
      int len;
      
      vfprintf (stdout, fmt, ap);
      fprintf (stdout, "?");
      p = fgets (repl, sizeof repl, stdin);
      if (!p)
	return 0;
      len = strlen (p);
      if (len > 0 && p[len-1] == '\n')
	p[len--] = 0;

      while (*p && isspace (*p))
	p++;
      
      switch (p[0])
	{
	case 'y':
	case 'Y':
	  return 1;
	case 'n':
	case 'N':
	  return 0;
	}

      fprintf (stdout, "Please answer yes or no: ");
    }
  return 0; /* to pacify gcc */
}

FILE *
mh_audit_open (char *name, mailbox_t mbox)
{
  FILE *fp;
  char date[64];
  time_t t;
  struct tm *tm;
  url_t url;
  char *namep;
  
  namep = mu_tilde_expansion (name, "/", NULL);
  if (strchr (namep, '/') == NULL)
    {
      char *p = NULL;
      char *home;

      asprintf (&p, "%s/Mail/%s", home = mu_get_homedir (), namep);
      free (home);
      if (!p)
	{
	  mh_error ("low memory");
	  exit (1);
	}
      free (namep);
      namep = p;
    }

  fp = fopen (namep, "a");
  if (!fp)
    {
      mh_error ("Can't open audit file %s: %s", namep, strerror (errno));
      free (namep);
      return NULL;
    }
  free (namep);
  
  time (&t);
  tm = localtime (&t);
  strftime (date, sizeof date, "%a, %d %b %Y %H:%M:%S %Z", tm);
  mailbox_get_url (mbox, &url);
  
  fprintf (fp, "<<%s>> %s %s\n",
	   program_invocation_short_name,
	   date,
	   url_to_string (url));
  return fp;
}

void
mh_audit_close (FILE *fp)
{
  fclose (fp);
}

int
mh_message_number (message_t msg, size_t *pnum)
{
  return message_get_uid (msg, pnum);	
}

mailbox_t
mh_open_folder (const char *folder, int create)
{
  mailbox_t mbox = NULL;
  char *name;
  int flags = MU_STREAM_READ;
  
  name = mh_expand_name (folder, 1);
  if (create && mh_check_folder (name, 1))
    exit (0);
    
  if (mailbox_create_default (&mbox, name))
    {
      mh_error ("Can't create mailbox %s: %s",
		name, strerror (errno));
      exit (1);
    }

  if (create)
    flags |= MU_STREAM_CREAT;
  
  if (mailbox_open (mbox, flags))
    {
      mh_error ("Can't open mailbox %s: %s", name, strerror (errno));
      exit (1);
    }

  free (name);

  return mbox;
}

char *
mh_get_dir ()
{
  char *mhdir = mh_global_profile_get ("Path", "Mail");
  if (mhdir[0] != '/')
    {
      char *p = mu_get_homedir ();
      asprintf (&mhdir, "%s/%s", p, mhdir);
      free (p);
    }
  else 
    mhdir = strdup (mhdir);
  return mhdir;
}

char *
mh_expand_name (const char *name, int is_folder)
{
  char *tmp = NULL;
  char *p = NULL;
  
  tmp = mu_tilde_expansion (name, "/", NULL);
  if (tmp[0] == '+')
    name = tmp + 1;
  else
    name = tmp;

  if (is_folder)
    asprintf (&p, "mh:%s/%s", mu_path_folder_dir, name);
  else
    asprintf (&p, "%s/%s", mu_path_folder_dir, name);
  free (tmp);
  return p;
}

int
mh_iterate (mailbox_t mbox, mh_msgset_t *msgset,
	    mh_iterator_fp itr, void *data)
{
  int rc;
  size_t i;

  for (i = 0; i < msgset->count; i++)
    {
      message_t msg;
      size_t num;

      num = msgset->list[i];
      if ((rc = mailbox_get_message (mbox, num, &msg)) != 0)
	{
	  mh_error ("can't get message %d: %s", num, mu_errstring (rc));
	  return 1;
	}

      itr (mbox, msg, num, data);
    }

  return 0;
}

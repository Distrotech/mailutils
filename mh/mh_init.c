/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
      mh_error (_("can't stat format file %s: %s"), name, strerror (errno));
      return -1;
    }
  
  fp = fopen (name, "r");
  if (!fp)
    {
      mh_error (_("can't open format file %s: %s"), name, strerror (errno));
      return -1;
    }

  format_str = xmalloc (st.st_size+1);
  while ((ptr = fgets (format_str + off, st.st_size - off + 1, fp)) != NULL)
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

void
mh_err_memory (int fatal)
{
  mh_error (_("not enough memory"));
  if (fatal)
    abort ();
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
	  mh_error (_("can't determine my username"));
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
	  if (!confirm || mh_getyn (_("Create folder \"%s\""), p))
	    {
	      int perm = 0711;
	      char *pb = mh_global_profile_get ("Folder-Protect", NULL);
	      if (pb)
		perm = strtoul (pb, NULL, 8);
	      if (mkdir (p, perm)) 
		{
		  mh_error (_("Can't create directory %s: %s"),
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
	  mh_error (_("can't stat %s: %s"), p, strerror (errno));
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

      fprintf (stdout, _("Please answer yes or no: "));
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
	  mh_error (_("low memory"));
	  exit (1);
	}
      free (namep);
      namep = p;
    }

  fp = fopen (namep, "a");
  if (!fp)
    {
      mh_error (_("Can't open audit file %s: %s"), namep, strerror (errno));
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
  int flags = MU_STREAM_RDWR;
  
  name = mh_expand_name (NULL, folder, 1);
  if (create && mh_check_folder (name, 1))
    exit (0);
    
  if (mailbox_create_default (&mbox, name))
    {
      mh_error (_("Can't create mailbox %s: %s"),
		name, strerror (errno));
      exit (1);
    }

  if (create)
    flags |= MU_STREAM_CREAT;
  
  if (mailbox_open (mbox, flags))
    {
      mh_error (_("Can't open mailbox %s: %s"), name, strerror (errno));
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
mh_expand_name (const char *base, const char *name, int is_folder)
{
  char *tmp = NULL;
  char *p = NULL;
  
  tmp = mu_tilde_expansion (name, "/", NULL);
  if (tmp[0] == '+')
    name = tmp + 1;
  else if (strncmp (tmp, "../", 3) == 0 || strncmp (tmp, "./", 2) == 0)
    {
      char *cwd = mu_getcwd ();
      asprintf (&name, "%s/%s", cwd, tmp);
      free (cwd);
      free (tmp);
      tmp = NULL;
    }
  else
    name = tmp;
  
  if (!base)
    base = mu_path_folder_dir;
  if (is_folder)
    {
      if (name[0] == '/')
	asprintf (&p, "mh:%s", name);
      else
	asprintf (&p, "mh:%s/%s", base, name);
    }
  else if (name[0] != '/')
    asprintf (&p, "%s/%s", base, name);
  else
    return name;
  
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
	  mh_error (_("can't get message %d: %s"), num, mu_strerror (rc));
	  return 1;
	}

      itr (mbox, msg, num, data);
    }

  return 0;
}

int
mh_spawnp (const char *prog, const char *file)
{
  int argc, i, rc, status;
  char **argv, **xargv;

  if (argcv_get (prog, "", "#", &argc, &argv))
    {
      mh_error (_("cannot split line %s"), prog);
      argcv_free (argc, argv);
      return 1;
    }

  xargv = calloc (argc + 2, sizeof (*xargv));
  if (!xargv)
    {
      mh_err_memory (0);
      argcv_free (argc, argv);
      return 1;
    }

  for (i = 0; i < argc; i++)
    xargv[i] = argv[i];
  xargv[i++] = (char*) file;
  xargv[i++] = NULL;

  rc = mu_spawnvp (xargv[0], (const char**) xargv, &status);

  free (xargv);
  argcv_free (argc, argv);

  return rc;
}

int
mh_file_copy (const char *from, const char *to)
{
  char *buffer;
  size_t bufsize, rdsize;
  struct stat st;
  stream_t in;
  stream_t out;
  int rc;
  
  if (stat (from, &st))
    {
      mh_error ("mh_copy: %s", mu_strerror (errno));
      return -1;
    }

  for (bufsize = st.st_size; bufsize > 0 && (buffer = malloc (bufsize)) == 0;
       bufsize /= 2)
    ;

  if (!bufsize)
    mh_err_memory (1);

  if ((rc = file_stream_create (&in, from, MU_STREAM_READ)) != 0
      || (rc = stream_open (in)))
    {
      mh_error (_("cannot open input file \"%s\": %s"),
		from, mu_strerror (rc));
      free (buffer);
      return 1;
    }

  if ((rc = file_stream_create (&out, to, MU_STREAM_RDWR|MU_STREAM_CREAT)) != 0
      || (rc = stream_open (out)))
    {
      mh_error (_("cannot open output file \"%s\": %s"),
		to, mu_strerror (rc));
      free (buffer);
      stream_close (in);
      stream_destroy (&in, stream_get_owner (in));
      return 1;
    }

  while (st.st_size > 0
	 && (rc = stream_sequential_read (in, buffer, bufsize, &rdsize)) == 0
	 && rdsize > 0)
    {
      if ((rc = stream_sequential_write (out, buffer, rdsize)) != 0)
	{
	  mh_error (_("write error on \"%s\": %s"),
		    to, mu_strerror (rc));
	  break;
	}
      st.st_size -= rdsize;
    }

  free (buffer);

  stream_close (in);
  stream_close (out);
  stream_destroy (&in, stream_get_owner (in));
  stream_destroy (&out, stream_get_owner (out));
  
  return rc;
}

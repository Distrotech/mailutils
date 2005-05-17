/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

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

/* Initialize MH applications. */

#include <mh.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>

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
  /* Register all mailbox and mailer formats */
  mu_register_all_formats ();

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
      mh_error (_("Cannot stat format file %s: %s"), name, strerror (errno));
      return -1;
    }
  
  fp = fopen (name, "r");
  if (!fp)
    {
      mh_error (_("Cannot open format file %s: %s"), name, strerror (errno));
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
  mh_error (_("Not enough memory"));
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
	  mh_error (_("Cannot determine my username"));
	  return;
	}
      name = pw->pw_name;
    }

  my_name = strdup (name);
  my_email = mu_get_user_email (name);
}

int
emailcmp (char *pattern, char *name)
{
  char *p;

  p = strchr (pattern, '@');
  if (p)
    for (p++; *p; p++)
      *p = toupper (*p);

  return fnmatch (pattern, name, 0);
}

int
mh_is_my_name (char *name)
{
  char *pname, *p;
  int rc = 0;
  
  pname = strdup (name);
  p = strchr (pname, '@');
  if (p)
    for (p++; *p; p++)
      *p = toupper (*p);
  
  if (!my_email)
    mh_get_my_name (NULL);
  if (emailcmp (my_email, pname) == 0)
    rc = 1;
  else
    {
      char *nlist = mh_global_profile_get ("Alternate-Mailboxes", NULL);
      if (nlist)
	{
	  char *p, *sp;
      
	  p = strtok_r (nlist, ",", &sp);
	  do
	    rc = emailcmp (p, pname) == 0;
	  while (rc == 0 && (p = strtok_r (NULL, ",", &sp)));
	}
    }
  free (pname);
  return rc;
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
		  mh_error (_("Cannot create directory %s: %s"),
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
	  mh_error (_("Cannot stat %s: %s"), p, strerror (errno));
	  return 1;
	}
    }
  return 0;
}

int
mh_interactive_mode_p ()
{
  static int interactive = -1;

  if (interactive < 0)
    interactive = isatty (fileno (stdin)) ? 1 : 0;
  return interactive;
}

int
mh_vgetyn (const char *fmt, va_list ap)
{
  char repl[64];

  while (1)
    {
      char *p;
      int len, rc;
      
      vfprintf (stdout, fmt, ap);
      fprintf (stdout, "? ");
      p = fgets (repl, sizeof repl, stdin);
      if (!p)
	return 0;
      len = strlen (p);
      if (len > 0 && p[len-1] == '\n')
	p[len--] = 0;

      rc = mu_true_answer_p (p);

      if (rc >= 0)
	return rc;

      /* TRANSLATORS: See msgids "nN" and "yY". */
      fprintf (stdout, _("Please answer yes or no: "));
    }
  return 0; /* to pacify gcc */
}

int
mh_getyn (const char *fmt, ...)
{
  va_list ap;
  int rc;
  
  if (!mh_interactive_mode_p ())
      return 1;
  va_start (ap, fmt);
  rc = mh_vgetyn (fmt, ap);
  va_end (ap);
  return rc;
}

int
mh_getyn_interactive (const char *fmt, ...)
{
  va_list ap;
  int rc;
  
  va_start (ap, fmt);
  rc = mh_vgetyn (fmt, ap);
  va_end (ap);
  return rc;
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

      asprintf (&p, "%s/%s", mu_path_folder_dir, namep);
      if (!p)
	{
	  mh_error (_("Not enough memory"));
	  exit (1);
	}
      free (namep);
      namep = p;
    }

  fp = fopen (namep, "a");
  if (!fp)
    {
      mh_error (_("Cannot open audit file %s: %s"), namep, strerror (errno));
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
      mh_error (_("Cannot create mailbox %s: %s"),
		name, strerror (errno));
      exit (1);
    }

  if (create)
    flags |= MU_STREAM_CREAT;
  
  if (mailbox_open (mbox, flags))
    {
      mh_error (_("Cannot open mailbox %s: %s"), name, strerror (errno));
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
  char *namep;
  
  tmp = mu_tilde_expansion (name, "/", NULL);
  if (tmp[0] == '+')
    namep = tmp + 1;
  else if (strncmp (tmp, "../", 3) == 0 || strncmp (tmp, "./", 2) == 0)
    {
      char *cwd = mu_getcwd ();
      asprintf (&namep, "%s/%s", cwd, tmp);
      free (cwd);
      free (tmp);
      tmp = NULL;
    }
  else
    namep = tmp;
  
  if (!base)
    base = mu_path_folder_dir;
  if (is_folder)
    {
      if (namep[0] == '/')
	asprintf (&p, "mh:%s", namep);
      else
	asprintf (&p, "mh:%s/%s", base, namep);
    }
  else if (namep[0] != '/')
    asprintf (&p, "%s/%s", base, namep);
  else
    return namep;
  
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
	  mh_error (_("Cannot get message %d: %s"), num, mu_strerror (rc));
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
      mh_error (_("Cannot split line %s"), prog);
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
      mh_error (_("Cannot open input file `%s': %s"),
		from, mu_strerror (rc));
      free (buffer);
      return 1;
    }

  if ((rc = file_stream_create (&out, to, MU_STREAM_RDWR|MU_STREAM_CREAT)) != 0
      || (rc = stream_open (out)))
    {
      mh_error (_("Cannot open output file `%s': %s"),
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
	  mh_error (_("Write error on `%s': %s"),
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

message_t
mh_file_to_message (char *folder, char *file_name)
{
  struct stat st;
  int rc;
  stream_t instream;
  
  if (folder)
    file_name = mh_expand_name (folder, file_name, 0);
  
  if (stat (file_name, &st) < 0)
    {
      mh_error (_("Cannot stat file %s: %s"), file_name, strerror (errno));
      return NULL;
    }
  
  if ((rc = file_stream_create (&instream, file_name, MU_STREAM_READ)))
    {
      mh_error (_("Cannot create input stream (file %s): %s"),
		file_name, mu_strerror (rc));
      return NULL;
    }
  
  if ((rc = stream_open (instream)))
    {
      mh_error (_("Cannot open input stream (file %s): %s"),
		file_name, mu_strerror (rc));
      stream_destroy (&instream, stream_get_owner (instream));
      return NULL;
    }

  return mh_stream_to_message (instream);
}

void
mh_install_help (char *mhdir)
{
  static char *text = N_(
"Prior to using MH, it is necessary to have a file in your login\n"
"directory (%s) named .mh_profile which contains information\n"
"to direct certain MH operations.  The only item which is required\n"
"is the path to use for all MH folder operations.  The suggested MH\n"
"path for you is %s...\n");

  printf (_(text), mu_get_homedir (), mhdir);
}

void
mh_real_install (char *name, int automode)
{
  char *home = mu_get_homedir ();
  char *mhdir;
  char *ctx;
  FILE *fp;
  
  asprintf (&mhdir, "%s/%s", home, "Mail");
  
  if (!automode)
    {
      size_t n = 0;
      
      if (mh_getyn_interactive (_("Do you need help")))
	mh_install_help (mhdir);

      if (!mh_getyn_interactive (_("Do you want the standard MH path \"%s\""), mhdir))
	{
	  int local;
	  char *p;
	  
	  local = mh_getyn_interactive (_("Do you want a path below your login directory"));
	  if (local)
	    printf (_("What is the path? "));
	  else
	    printf (_("What is the full path? "));
	  if (getline (&p, &n, stdin) <= 0)
	    exit (1);

	  n = strlen (p);
	  if (n == 0)
	    exit (1);

	  if (p[n-1] == '\n')
	    p[n-1] = 0;

	  free (mhdir);
	  if (local)
	    {
	      asprintf (&mhdir, "%s/%s", home, p);
	      free (p);
	    }
	  else
	    mhdir = p;
	}
    }

  if (mh_check_folder (mhdir, !automode))
    exit (1);

  fp = fopen (name, "w");
  if (!fp)
    {
      mu_error (_("Cannot open file %s: %s"), name, mu_strerror (errno));
      exit (1);
    }
  fprintf (fp, "Path: %s\n", mhdir);
  fclose (fp);

  asprintf (&ctx, "%s/%s", mhdir, MH_CONTEXT_FILE);
  fp = fopen (ctx, "w");
  if (fp)
    {
      fprintf (fp, "Current-Folder: inbox\n");
      fclose (fp);
    }
  free (ctx);
  asprintf (&ctx, "%s/inbox", mhdir);
  if (mh_check_folder (ctx, !automode))
    exit (1);
  free (ctx);
  free (mhdir);
}  

void
mh_install (char *name, int automode)
{
  struct stat st;
  
  if (stat(name, &st))
    {
      if (errno == ENOENT)
	{
	  if (automode)
	    printf(_("I'm going to create the standard MH path for you.\n"));
	  mh_real_install (name, automode);
	}
      else
	{
	  mh_error(_("Cannot stat %s: %s"), name, mu_strerror (errno));
	  exit (1);
	}
    }
  else if ((st.st_mode & S_IFREG) || (st.st_mode & S_IFLNK)) 
    {
      mu_error(_("You already have an MH profile, use an editor to modify it"));
      exit (0);
    }
  else
    {
      mu_error(_("You already have file %s which is not a regular file or a symbolic link.\n"
		 "Please remove it and try again"));
      exit (1);
    }
}
        
void
mh_annotate (message_t msg, char *field, char *text, int date)
{
  header_t hdr;
  attribute_t attr;
  
  if (message_get_header (msg, &hdr))
    return;

  if (date)
    {
      time_t t;
      struct tm *tm;
      char datebuf[80];
      t = time (NULL);
      tm = localtime (&t);
      strftime (datebuf, sizeof datebuf, "%a, %d %b %Y %H:%M:%S %Z", tm);

      header_set_value (hdr, field, datebuf, 0);
    }

  if (text)
    header_set_value (hdr, field, text, 0);
  message_get_attribute (msg, &attr);
  attribute_set_modified (attr);
}

char *
mh_draft_name ()
{
  char *draftfolder = mh_global_profile_get ("Draft-Folder",
					     mu_path_folder_dir);
  return mh_expand_name (draftfolder, "draft", 0);
}

char *
mh_create_message_id (int subpart)
{
  char date[4+2+2+2+2+2+1];
  time_t t = time (NULL);
  struct tm *tm = localtime (&t);
  char *host;
  char *p;
	  
  strftime (date, sizeof date, "%Y%m%d%H%M%S", tm);
  mu_get_host_name (&host);

  if (subpart)
    {
      struct timeval tv;
      gettimeofday (&tv, NULL);
      asprintf (&p, "<%s.%lu.%d@%s>",
		date,
		(unsigned long) getpid (),
		subpart,
		host);
    }
  else
    asprintf (&p, "<%s.%lu@%s>", date, (unsigned long) getpid (), host);
  free (host);
  return p;
}

void
mh_set_reply_regex (const char *str)
{
  char *err;
  int rc = munre_set_regex (str, 0, &err);
  if (rc)
    mh_error ("reply_regex: %s%s%s", mu_strerror (rc),
	      err ? ": " : "",
	      err ? err : "");
}

int
mh_decode_2047 (char *text, char **decoded_text)
{
  char *charset = mh_global_profile_get ("Charset", NULL);

  if (!charset)
    return 1;
  if (strcasecmp (charset, "auto") == 0)
    {
      /* Try to deduce the charset from LC_ALL variable */

      char *tmp = getenv ("LC_ALL");
      if (tmp)
	{
	  char *sp;
	  char *lang;
	  char *terr;

	  lang = strtok_r (tmp, "_", &sp);
	  terr = strtok_r (NULL, ".", &sp);
	  charset = strtok_r (NULL, "@", &sp);

	  if (!charset)
	    charset = mu_charset_lookup (lang, terr);
	}
    }

  if (!charset)
    return 1;
  
  return rfc2047_decode (charset, text, decoded_text);
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

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

/* Initialize MH applications. */

#include <mh.h>
#include <mailutils/url.h>
#include <mailutils/tls.h>
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
  "%4(msg)"
  "%<(cur)+%| %>"
  "%<{replied}-%?{encrypted}E%| %>"
  "%02(mon{date})/%02(mday{date})"
  "%<{date} %|*%>"
  "%<(mymbox{from})%<{to}To:%14(decode(friendly{to}))%>%>"
  "%<(zero)%17(decode(friendly{from}))%>"
  "  %(decode{subject})%<{body}<<%{body}>>%>";

void
mh_init ()
{
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);
  
  /* Register all mailbox and mailer formats */
  mu_register_all_formats ();

  /* Read user's profile */
  mh_read_profile ();
}

void
mh_init2 ()
{
  mh_current_folder ();
}

int
mh_read_formfile (char *name, char **pformat)
{
  FILE *fp;
  struct stat st;
  char *ptr;
  size_t off = 0;
  char *format_str;
  char *file_name;
  int rc;
  
  rc = mh_find_file (name, &file_name);
  if (rc)
    {
      mu_error (_("cannot access format file %s: %s"), name, strerror (rc));
      return -1;
    }
  
  if (stat (file_name, &st))
    {
      mu_error (_("cannot stat format file %s: %s"), file_name,
		strerror (errno));
      free (file_name);
      return -1;
    }
  
  fp = fopen (file_name, "r");
  if (!fp)
    {
      mu_error (_("cannot open format file %s: %s"), file_name,
		strerror (errno));
      free (file_name);
      return -1;
    }
  free (file_name);
  
  format_str = mu_alloc (st.st_size+1);
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
  mu_error (_("not enough memory"));
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
	  mu_error (_("cannot determine my username"));
	  return;
	}
      name = pw->pw_name;
    }

  my_name = mu_strdup (name);
  my_email = mu_get_user_email (name);
}

int
emailcmp (char *pattern, char *name)
{
  char *p;

  p = strchr (pattern, '@');
  if (p)
    for (p++; *p; p++)
      *p = mu_toupper (*p);

  return fnmatch (pattern, name, 0);
}

int
mh_is_my_name (const char *name)
{
  char *pname, *p;
  int rc = 0;
  
  pname = mu_strdup (name);
  p = strchr (pname, '@');
  if (p)
    for (p++; *p; p++)
      *p = mu_toupper (*p);
  
  if (!my_email)
    mh_get_my_name (NULL);
  if (emailcmp (my_email, pname) == 0)
    rc = 1;
  else
    {
      const char *nlist = mh_global_profile_get ("Alternate-Mailboxes", NULL);
      if (nlist)
	{
	  const char *end, *p;
	  char *pat;
	  int len;
	  
	  for (p = nlist; rc == 0 && *p; p = end)
	    {
	      
	      while (*p && mu_isspace (*p))
		p++;

	      end = strchr (p, ',');
	      if (end)
		{
		  len = end - p;
		  end++;
		}
	      else
		{
		  len = strlen (p);
		  end = p + len;
		}

	      while (len > 0 && mu_isspace (p[len-1]))
		len--;

	      pat = mu_alloc (len + 1);
	      memcpy (pat, p, len);
	      pat[len] = 0;
	      rc = emailcmp (pat, pname) == 0;
	      free (pat);
	    }
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

static int
make_dir_hier (const char *p, mode_t perm)
{
  int rc = 0;
  char *dir = mu_strdup (p);
  char *q = dir;

  while (!rc && (q = strchr (q + 1, '/')))
    {
      *q = 0;
      if (access (dir, X_OK))
	{
	  if (errno != ENOENT)
	    {
	      mu_error (_("cannot create directory %s: error accessing name component %s: %s"),

			p, dir, strerror (errno));
	      rc = 1;
	    }
	  else if ((rc = mkdir (dir, perm)))
	    mu_error (_("cannot create directory %s: error creating name component %s: %s"),
		      p, dir, mu_strerror (rc));
	}
      *q = '/';
    }
  free (dir);
  return rc;
}

int
mh_makedir (const char *p)
{
  int rc;
  mode_t save_umask;
  mode_t perm = 0711;
  const char *pb = mh_global_profile_get ("Folder-Protect", NULL);
  if (pb)
    perm = strtoul (pb, NULL, 8);

  save_umask = umask (0);

  if ((rc = make_dir_hier (p, perm)) == 0)
    {
      rc = mkdir (p, perm);
      if (rc)
	mu_error (_("cannot create directory %s: %s"),
		  p, strerror (errno));
    }

  umask (save_umask);
  return rc;
}

int
mh_check_folder (const char *pathname, int confirm)
{
  const char *p;
  struct stat st;
  
  if ((p = strchr (pathname, ':')) != NULL)
    p++;
  else
    p = pathname;
  
  if (stat (p, &st))
    {
      if (errno == ENOENT)
	{
	  /* TRANSLATORS: This is a question and will be followed
	     by question mark on output. */
	  if (!confirm || mh_getyn (_("Create folder \"%s\""), p))
	    return mh_makedir (p);
	  else
	    return 1;
	}
      else
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "stat", p, errno);
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
mh_audit_open (char *name, mu_mailbox_t mbox)
{
  FILE *fp;
  char date[64];
  time_t t;
  struct tm *tm;
  mu_url_t url;
  char *namep;
  
  namep = mu_tilde_expansion (name, MU_HIERARCHY_DELIMITER, NULL);
  if (strchr (namep, MU_HIERARCHY_DELIMITER) == NULL)
    {
      char *p = mh_safe_make_file_name (mu_folder_directory (), namep);
      free (namep);
      namep = p;
    }

  fp = fopen (namep, "a");
  if (!fp)
    {
      mu_error (_("cannot open audit file %s: %s"), namep, strerror (errno));
      free (namep);
      return NULL;
    }
  free (namep);
  
  time (&t);
  tm = localtime (&t);
  mu_strftime (date, sizeof date, "%a, %d %b %Y %H:%M:%S %Z", tm);
  mu_mailbox_get_url (mbox, &url);
  
  fprintf (fp, "<<%s>> %s %s\n",
	   mu_program_name,
	   date,
	   mu_url_to_string (url));
  return fp;
}

void
mh_audit_close (FILE *fp)
{
  fclose (fp);
}

int
mh_message_number (mu_message_t msg, size_t *pnum)
{
  return mu_message_get_uid (msg, pnum);	
}

mu_mailbox_t
mh_open_folder (const char *folder, int flags)
{
  mu_mailbox_t mbox = NULL;
  char *name;
  
  name = mh_expand_name (NULL, folder, 1);
  if ((flags & MU_STREAM_CREAT) && mh_check_folder (name, 1))
    exit (0);
    
  if (mu_mailbox_create_default (&mbox, name))
    {
      mu_error (_("cannot create mailbox %s: %s"),
		name, strerror (errno));
      exit (1);
    }

  if (mu_mailbox_open (mbox, flags))
    {
      mu_error (_("cannot open mailbox %s: %s"), name, strerror (errno));
      exit (1);
    }

  free (name);

  return mbox;
}

char *
mh_get_dir ()
{
  const char *mhdir = mh_global_profile_get ("Path", "Mail");
  char *mhcopy;
  
  if (mhdir[0] != '/')
    {
      char *p = mu_get_homedir ();
      mhcopy = mh_safe_make_file_name (p, mhdir);
      free (p);
    }
  else 
    mhcopy = strdup (mhdir);
  if (!mhcopy)
    {
      mu_error (_("not enough memory"));
      abort ();
    }
  return mhcopy;
}

char *
mh_expand_name (const char *base, const char *name, int is_folder)
{
  char *p = NULL;
  char *namep = NULL;
  
  namep = mu_tilde_expansion (name, MU_HIERARCHY_DELIMITER, NULL);
  if (namep[0] == '+')
    memmove (namep, namep + 1, strlen (namep)); /* copy null byte as well */
  else if (strncmp (namep, "../", 3) == 0 || strncmp (namep, "./", 2) == 0)
    {
      char *cwd = mu_getcwd ();
      char *tmp = mh_safe_make_file_name (cwd, namep);
      free (cwd);
      free (namep);
      namep = tmp;
    }
  
  if (is_folder)
    {
      if (memcmp (namep, "mh:/", 4) == 0)
	return namep;
      else if (namep[0] == '/')
	mu_asprintf (&p, "mh:%s", namep);
      else
	mu_asprintf (&p, "mh:%s/%s", base ? base : mu_folder_directory (), 
                     namep);
    }
  else if (namep[0] != '/')
    mu_asprintf (&p, "%s/%s", base ? base : mu_folder_directory (), namep);
  else
    return namep;
  
  free (namep);
  return p;
}

int
mh_find_file (const char *name, char **resolved_name)
{
  char *s;
  int rc;
  
  if (name[0] == '/' ||
      (name[0] == '.' && name[1] == '/') ||
      (name[0] == '.' && name[1] == '.' && name[2] == '/'))
    {
      *resolved_name = mu_strdup (name);
      if (access (name, R_OK) == 0)
	return 0;
      return errno;
    }

  if (name[0] == '~')
    {
      s = mu_tilde_expansion (name, MU_HIERARCHY_DELIMITER, NULL);
      *resolved_name = s;
      if (access (s, R_OK) == 0)
	return 0;
      return errno;
    }
  
  s = mh_expand_name (NULL, name, 0);
  if (access (s, R_OK) == 0)
    {
      *resolved_name = s;
      return 0;
    }
  if (errno != ENOENT)
    mu_diag_output (MU_DIAG_WARNING,
		    _("cannot access %s: %s"), s, mu_strerror (errno));
  free (s);

  s = mh_expand_name (mh_global_profile_get ("mhetcdir", MHLIBDIR), name, 0);
  if (access (s, R_OK) == 0)
    {
      *resolved_name = s;
      return 0;
    }
  if (errno != ENOENT)
    mu_diag_output (MU_DIAG_WARNING,
		    _("cannot access %s: %s"), s, mu_strerror (errno));
  free (s);

  *resolved_name = mu_strdup (name);
  if (access (name, R_OK) == 0)
    return 0;
  rc = errno;
  if (rc != ENOENT)
    mu_diag_output (MU_DIAG_WARNING,
		    _("cannot access %s: %s"), s, mu_strerror (rc));

  return rc;
}

int
mh_spawnp (const char *prog, const char *file)
{
  struct mu_wordsplit ws;
  size_t i;
  int rc, status;
  char **xargv;

  ws.ws_comment = "#";
  if (mu_wordsplit (prog, &ws, MU_WRDSF_DEFFLAGS | MU_WRDSF_COMMENT))
    {
      mu_error (_("cannot split line `%s': %s"), prog,
		mu_wordsplit_strerror (&ws));
      return 1;
    }

  xargv = calloc (ws.ws_wordc + 2, sizeof (*xargv));
  if (!xargv)
    {
      mh_err_memory (0);
      mu_wordsplit_free (&ws);
      return 1;
    }

  for (i = 0; i < ws.ws_wordc; i++)
    xargv[i] = ws.ws_wordv[i];
  xargv[i++] = (char*) file;
  xargv[i++] = NULL;
  
  rc = mu_spawnvp (xargv[0], xargv, &status);

  free (xargv);
  mu_wordsplit_free (&ws);

  return rc;
}

/* Copy data from FROM to TO, creating the latter if necessary.
   FIXME: How about formats?
*/
int
mh_file_copy (const char *from, const char *to)
{
  mu_stream_t in, out, flt;
  int rc;
  
  if ((rc = mu_file_stream_create (&in, from, MU_STREAM_READ)))
    {
      mu_error (_("cannot open input file `%s': %s"),
		from, mu_strerror (rc));
      return 1;
    }

  if ((rc = mu_file_stream_create (&out, to, MU_STREAM_RDWR|MU_STREAM_CREAT)))
    {
      mu_error (_("cannot open output file `%s': %s"),
		to, mu_strerror (rc));
      mu_stream_destroy (&in);
      return 1;
    }

  rc = mu_filter_create (&flt, in, "INLINE-COMMENT", MU_FILTER_DECODE,
			 MU_STREAM_READ);
  mu_stream_unref (in);
  if (rc)
    {
      mu_error (_("cannot open filter stream: %s"), mu_strerror (rc));
      mu_stream_destroy (&out);
      return 1;
    }
  
  rc = mu_stream_copy (out, flt, 0, NULL);
      
  mu_stream_destroy (&flt);
  mu_stream_destroy (&out);

  if (rc)
    mu_error (_("error copying file `%s' to `%s': %s"),
	      from, to, mu_strerror (rc));
  
  return rc;
}

static mu_message_t
_file_to_message (const char *file_name)
{
  struct stat st;
  int rc;
  mu_stream_t instream;

  if (stat (file_name, &st) < 0)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "stat", file_name, errno);
      return NULL;
    }
  
  if ((rc = mu_file_stream_create (&instream, file_name, MU_STREAM_READ)))
    {
      mu_error (_("cannot create input stream (file %s): %s"),
		file_name, mu_strerror (rc));
      return NULL;
    }
  
  return mh_stream_to_message (instream);
}

mu_message_t
mh_file_to_message (const char *folder, const char *file_name)
{
  mu_message_t msg;
  char *tmp_name = NULL;
  
  if (folder)
    {
      tmp_name = mh_expand_name (folder, file_name, 0);
      msg = _file_to_message (tmp_name);
      free (tmp_name);
    }
  else
    msg = _file_to_message (file_name);
  
  return msg;
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
  int rc;
  mu_stream_t profile;

  mhdir = mh_safe_make_file_name (home, "Mail");
  
  if (!automode)
    {
      /* TRANSLATORS: This is a question and will be followed
	 by question mark on output. */
      if (mh_getyn_interactive (_("Do you need help")))
	mh_install_help (mhdir);

      /* TRANSLATORS: This is a question and will be followed
	 by question mark on output. */
      if (!mh_getyn_interactive (_("Do you want the standard MH path \"%s\""), mhdir))
	{
	  int local;
	  char *p, *buf = NULL;
	  size_t size = 0;
	  
	  /* TRANSLATORS: This is a question and will be followed
	     by question mark on output. */
	  local = mh_getyn_interactive (_("Do you want a path below your login directory"));
	  if (local)
	    mu_printf (_("What is the path? "));
	  else
	    mu_printf (_("What is the full path? "));
	  mu_stream_flush (mu_strin);
	  if (mu_stream_getline (mu_strin, &buf, &size, NULL))
	    exit (1);
	  p = mu_str_stripws (buf);
	  if (p > buf)
	    memmove (buf, p, strlen (p) + 1);
	  
	  free (mhdir);
	  if (local)
	    {
              mhdir = mh_safe_make_file_name (home, p);
	      free (p);
	    }
	  else
	    mhdir = p;
	}
    }

  if (mh_check_folder (mhdir, !automode))
    exit (1);

  rc = mu_file_stream_create (&profile, name,
			      MU_STREAM_WRITE | MU_STREAM_CREAT);
  if (rc)
    {
      mu_error (_("cannot open file %s: %s"), name, mu_strerror (rc));
      exit (1);
    }
  mu_stream_printf (profile, "Path: %s\n", mhdir);
  mu_stream_destroy (&profile);

  ctx = mh_safe_make_file_name (mhdir, MH_CONTEXT_FILE);
  rc = mu_file_stream_create (&profile, ctx,
			      MU_STREAM_WRITE | MU_STREAM_CREAT);
  if (rc)
    {
      mu_stream_printf (profile, "Current-Folder: inbox\n");
      mu_stream_destroy (&profile);
    }
  free (ctx);
  ctx = mh_safe_make_file_name (mhdir, "inbox");
  if (mh_check_folder (ctx, !automode))
    exit (1);
  free (ctx);
  free (mhdir);
}  

void
mh_install (char *name, int automode)
{
  struct stat st;
  
  if (stat (name, &st))
    {
      if (errno == ENOENT)
	{
	  if (automode)
	    printf(_("I'm going to create the standard MH path for you.\n"));
	  mh_real_install (name, automode);
	}
      else
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "stat", name, errno);
	  exit (1);
	}
    }
  else if ((st.st_mode & S_IFREG) || (st.st_mode & S_IFLNK)) 
    {
      mu_error(_("You already have an MH profile, use an editor to modify it"));
      exit (1);
    }
  else
    {
      mu_error (_("You already have file %s which is not a regular file or a symbolic link."), name);
      mu_error (_("Please remove it and try again"));
      exit (1);
    }
}
        
void
mh_annotate (mu_message_t msg, const char *field, const char *text, int date)
{
  mu_header_t hdr;
  mu_attribute_t attr;
  
  if (mu_message_get_header (msg, &hdr))
    return;

  if (date)
    {
      time_t t;
      struct tm *tm;
      char datebuf[80];
      t = time (NULL);
      tm = localtime (&t);
      mu_strftime (datebuf, sizeof datebuf, "%a, %d %b %Y %H:%M:%S %Z", tm);

      mu_header_set_value (hdr, field, datebuf, 0);
    }

  if (text)
    mu_header_set_value (hdr, field, text, 0);
  mu_message_get_attribute (msg, &attr);
  mu_attribute_set_modified (attr);
}

char *
mh_draft_name ()
{
  return mh_expand_name (mh_global_profile_get ("Draft-Folder",
						mu_folder_directory ()),
			 "draft", 0);
}

char *
mh_create_message_id (int subpart)
{
  char *p;
  mu_rfc2822_msg_id (subpart, &p);
  return p;
}

void
mh_set_reply_regex (const char *str)
{
  char *err;
  int rc = mu_unre_set_regex (str, 0, &err);
  if (rc)
    mu_error ("reply_regex: %s%s%s", mu_strerror (rc),
	      err ? ": " : "",
	      err ? err : "");
}

const char *
mh_charset (const char *dfl)
{
  const char *charset = mh_global_profile_get ("Charset", dfl);

  if (!charset)
    return NULL;
  if (mu_c_strcasecmp (charset, "auto") == 0)
    {
      static char *saved_charset;

      if (!saved_charset)
	{
	  /* Try to deduce the charset from LC_ALL variable */
	  struct mu_lc_all lc_all;
	  if (mu_parse_lc_all (getenv ("LC_ALL"), &lc_all, MU_LC_CSET) == 0)
	    saved_charset = lc_all.charset; /* FIXME: Mild memory leak */
	}
      charset = saved_charset;
    }
  return charset;
}

int
mh_decode_2047 (char *text, char **decoded_text)
{
  const char *charset = mh_charset (NULL);
  if (!charset)
    return 1;
  
  return mu_rfc2047_decode (charset, text, decoded_text);
}

void
mh_quote (const char *in, char **out)
{
  size_t len = strlen (in);
  if (len && in[0] == '"' && in[len - 1] == '"')
    {
      const char *p;
      char *q;
      
      for (p = in + 1; p < in + len - 1; p++)
        if (*p == '\\' || *p == '"')
	  len++;

      *out = mu_alloc (len + 1);
      q = *out;
      p = in;
      *q++ = *p++;
      while (p[1])
	{
	  if (*p == '\\' || *p == '"')
	    *q++ = '\\';
	  *q++ = *p++;
	}
      *q++ = *p++;
      *q = 0;
    }
  else
    *out = mu_strdup (in);
}

void
mh_expand_aliases (mu_message_t msg,
		   mu_address_t *addr_to,
		   mu_address_t *addr_cc,
		   mu_address_t *addr_bcc)
{
  mu_header_t hdr;
  size_t i, num;
  const char *buf;
  
  mu_message_get_header (msg, &hdr);
  mu_header_get_field_count (hdr, &num);
  for (i = 1; i <= num; i++)
    {
      if (mu_header_sget_field_name (hdr, i, &buf) == 0)
	{
	  if (mu_c_strcasecmp (buf, MU_HEADER_TO) == 0
	      || mu_c_strcasecmp (buf, MU_HEADER_CC) == 0
	      || mu_c_strcasecmp (buf, MU_HEADER_BCC) == 0)
	    {
	      char *value;
	      mu_address_t addr = NULL;
	      int incl;
	      
	      mu_header_aget_field_value_unfold (hdr, i, &value);
	      
	      mh_alias_expand (value, &addr, &incl);
	      free (value);
	      if (mu_c_strcasecmp (buf, MU_HEADER_TO) == 0)
		mu_address_union (addr_to, addr);
	      else if (mu_c_strcasecmp (buf, MU_HEADER_CC) == 0)
		mu_address_union (addr_cc ? addr_cc : addr_to, addr);
	      else if (mu_c_strcasecmp (buf, MU_HEADER_BCC) == 0)
		mu_address_union (addr_bcc ? addr_bcc : addr_to, addr);
	    }
	}
    }
}

int
mh_draft_message (const char *name, const char *msgspec, char **pname)
{
  mu_url_t url;
  size_t uid;
  int rc;
  mu_mailbox_t mbox;
  const char *path;
  
  mbox = mh_open_folder (name, MU_STREAM_RDWR);
  if (!mbox)
    return 1;
  
  mu_mailbox_get_url (mbox, &url);

  if (strcmp (msgspec, "new") == 0)
    {
      mu_property_t prop;
      
      rc = mu_mailbox_uidnext (mbox, &uid);
      if (rc)
	{
	  mu_error (_("cannot obtain sequence number for the new message: %s"),
		    mu_strerror (rc));
	  exit (1);
	}
      mu_mailbox_get_property (mbox, &prop);
      mu_property_set_value (prop, "cur", mu_umaxtostr (0, uid), 1);
    }
  else
    {
      char *argv[2];
      mu_msgset_t msgset;
      
      argv[0] = (char*) msgspec;
      argv[1] = NULL;
      mh_msgset_parse (&msgset, mbox, 1, argv, "cur");
      if (!mh_msgset_single_message (msgset))
	mu_error (_("only one message at a time!"));
      else
	uid = mh_msgset_first_uid (msgset);
      mu_msgset_free (msgset);
    }

  mu_url_sget_path (url, &path);
  rc = mu_asprintf (pname, "%s/%lu", path, (unsigned long) uid);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_asprintf", NULL, rc);
      exit (1);
    }
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return rc;
}

char *
mh_safe_make_file_name (const char *dir, const char *file)
{
  char *name = mu_make_file_name (dir, file);
  if (!name)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_make_file_name", NULL, ENOMEM);
      abort ();
    }
  return name;
}

			  

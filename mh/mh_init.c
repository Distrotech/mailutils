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

char *current_folder = NULL;
size_t current_message;
char *ctx_name;
header_t ctx_header;
header_t profile_header;

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
  char *mh_sequences_name;

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
  
  /* Set MH context */
  if (current_folder)
    current_folder = mu_tilde_expansion (current_folder, "/", NULL);
  else
    current_folder = mh_profile_value ("Current-Folder",
				       mh_profile_value ("Inbox", "inbox"));
  if (strchr (current_folder, '/') == NULL)
    {
      char *mhdir = mh_profile_value ("Path", "Mail");
      char *p = mu_get_homedir ();

      if (mhdir[0] == '/')
	asprintf (&current_folder, "mh:%s/%s", mhdir, current_folder);
      else
	asprintf (&current_folder, "mh:%s/%s/%s", p, mhdir, current_folder);
      if (!current_folder)
	{
	  mh_error ("low memory");
	  exit (1);
	}
      free (p);
    }
  else if (strchr (current_folder, ':') == NULL)
    {
      char *p;
      p = xmalloc (strlen (current_folder) + 4);
      strcat (strcpy (p, "mh:"), current_folder);
      current_folder = p;
    }

  mh_sequences_name = mh_profile_value ("mh-sequences", MH_SEQUENCES_FILE);
  asprintf (&ctx_name, "%s/%s", current_folder+3, mh_sequences_name);
  if (mh_read_context_file (ctx_name, &ctx_header) == 0)
    {
      char buf[64];
      size_t n;
       
      if (!header_get_value (ctx_header, "cur", buf, sizeof buf, &n))
	current_message = strtoul (buf, NULL, 10);
    }
}

char *
mh_profile_value (char *name, char *defval)
{
  char *p;
  if (header_aget_value (profile_header, name, &p))
    p = defval;
  return p;
}

void
mh_read_profile ()
{
  char *p;
  
  p = getenv ("MH");
  if (p)
    p = mu_tilde_expansion (p, "/", NULL);
  else
    {
      char *home = mu_get_homedir ();
      if (!home)
	abort (); /* shouldn't happen */
      asprintf (&p, "%s/%s", home, MH_USER_PROFILE);
      free (home);
    }
  mh_read_context_file (p, &profile_header);
}
  
void
mh_save_context ()
{
  char buf[64];
  snprintf (buf, sizeof buf, "%d", current_message);
  if (!ctx_header)
    {
      if (header_create (&ctx_header, NULL, 0, NULL))
	{
	  mh_error ("Can't create context: %s", strerror (errno));
	  return;
	}
    }
  header_set_value (ctx_header, "cur", buf, 1);
  mh_write_context_file (ctx_name, ctx_header);
}

int 
mh_read_context_file (char *path, header_t *header)
{
  int status;
  char *blurb;
  struct stat st;
  FILE *fp;

  if (stat (path, &st))
    return errno;

  blurb = malloc (st.st_size);
  if (!blurb)
    return ENOMEM;
  
  fp = fopen (path, "r");
  if (!fp)
    {
      free (blurb);
      return errno;
    }

  fread (blurb, st.st_size, 1, fp);
  fclose (fp);
  
  if (status = header_create (header, blurb, st.st_size, NULL))
    free (blurb);

  return status;
}

int 
mh_write_context_file (char *path, header_t header)
{
  stream_t stream;
  char buffer[512];
  size_t off = 0, n;
  FILE *fp;
  
  fp = fopen (path, "w");
  if (!fp)
    {
      mh_error ("can't write context file %s: %s", path, strerror (errno));
      return 1;
    }
  
  header_get_stream (header, &stream);

  while (stream_read (stream, buffer, sizeof buffer - 1, off, &n) == 0
	 && n != 0)
    {
      buffer[n] = '\0';
      fprintf (fp, "%s", buffer);
      off += n;
    }

  fclose (fp);
  return 0;
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

int
mh_check_folder (char *pathname)
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
	  if (mh_getyn ("Create folder \"%s\"", p))
	    {
	      int perm = 0711;
	      char *pb = mh_profile_value ("Folder-Protect", NULL);
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

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

char *current_folder = "inbox";
size_t current_message;
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
  char *ctx_name;
  header_t header = NULL;

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

  /* Set MH context */
  current_folder = mu_tilde_expansion (current_folder, "/", NULL);
  if (strchr (current_folder, '/') == NULL)
    {
      char *p = mu_get_homedir ();
      asprintf (&current_folder, "mh:%s/Mail/%s", p, current_folder);
      if (!current_folder)
	{
	  mh_error ("low memory");
	  exit (1);
	}
    }
  else if (strchr (current_folder, ':') == NULL)
    {
      char *p;
      p = xmalloc (strlen (current_folder) + 4);
      strcat (strcpy (p, "mh:"), current_folder);
      current_folder = p;
    }

  ctx_name = xmalloc (strlen (current_folder)+sizeof (MH_SEQUENCES_FILE)+2);
  sprintf (ctx_name, "%s/%s", current_folder+3, MH_SEQUENCES_FILE);
  if (mh_read_context_file (ctx_name, &header) == 0)
    {
      char buf[64];
      size_t n;
       
      if (!header_get_value (header, "cur", buf, sizeof buf, &n))
 	current_message = strtoul (buf, NULL, 10);
      header_destroy (&header, NULL);
    }
  free (ctx_name);
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
      if (len > 0 && ptr[len-1] == '\n')
	ptr[--len] = 0;
      off += len;
    }
  format_str[off] = 0;
  fclose (fp);
  *pformat = format_str;
  return 0;
}

static char *my_name;
static char *my_email;

/* FIXME: this lacks domain name part! */
void
mh_get_my_name (char *name)
{
  char hostname[256];

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
  gethostname(hostname, sizeof(hostname));
  hostname[sizeof(hostname)-1] = 0;
  my_email = xmalloc (strlen (name) + strlen (hostname) + 2);
  sprintf (my_email, "%s@%s", name, hostname);
}


int
mh_is_my_name (char *name)
{
  if (!my_email)
    mh_get_my_name (NULL);
  return strcasecmp (name, my_email) == 0;
}


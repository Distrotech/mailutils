/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>
#include <sysexits.h>
#include <fnmatch.h>
#include <regex.h>
#include <mailutils/mailutils.h>
#include <mailutils/dbm.h>
#include "argp.h"
#include "mu.h"

static char dbm_doc[] = N_("mu dbm - DBM management tool\n"
"Valid COMMANDs are:\n"
"\n"
"  create or load - create the database\n"
"  dump           - dump the database\n"
"  list           - list contents of the database\n"
"  delete         - delete specified keys from the database\n"
"  add            - add records to the database\n"
"  replace        - add records replacing ones with matching keys\n");
char dbm_docstring[] = N_("DBM management tool");
static char dbm_args_doc[] = N_("COMMAND FILE [KEY...]");

enum mode
  {
    mode_list,
    mode_dump,
    mode_create,
    mode_delete,
    mode_add,
    mode_replace,
  };

enum key_type
  {
    key_literal,
    key_glob,
    key_regex
  };

static enum mode mode;
static char *db_name;
static char *input_file;
static int file_mode = 0600;
static struct mu_auth_data *auth;
static uid_t owner_uid;
static gid_t owner_gid;
static char *owner_user;
static char *owner_group;
static int copy_permissions;

#define META_FILE_MODE 0x0001
#define META_UID       0x0002
#define META_GID       0x0004
#define META_USER      0x0008
#define META_GROUP     0x0010
#define META_AUTH      0x0020

static int known_meta_data;

static enum key_type key_type = key_literal;
static int case_sensitive = 1;
static int include_zero = -1;

static int suppress_header = -1;


static void
init_datum (struct mu_dbm_datum *key, char *str)
{
  memset (key, 0, sizeof *key);
  key->mu_dptr = str;
  key->mu_dsize = strlen (str) + !!include_zero;
}

static mu_dbm_file_t
open_db_file (int mode)
{
  int rc;
  mu_dbm_file_t db;

  if (!db_name)
    {
      mu_error (_("database name not given"));
      exit (EX_USAGE);
    }
  
  rc = mu_dbm_create (db_name, &db, 0);
  if (rc)
    {
      mu_diag_output (MU_DIAG_ERROR, _("unable to create database %s: %s"),
		      db_name, mu_strerror (rc));
      exit (EX_SOFTWARE);
    }

  rc = mu_dbm_open (db, mode, file_mode);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_dbm_open", db_name, rc);
      exit (EX_SOFTWARE);
    }

  return db;
}

static void
set_db_ownership (mu_dbm_file_t db)
{
  int rc, dirfd, pagfd;
  struct stat st;
  
  rc = mu_dbm_get_fd (db, &pagfd, &dirfd);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_dbm_get_fd", db_name, rc);
      exit (EX_SOFTWARE);
    }
  
  if (known_meta_data & META_USER)
    {
      struct mu_auth_data *ap;
      
      ap = mu_get_auth_by_name (owner_user);
      if (!ap)
	{
	  if (!(known_meta_data & META_UID))
	    {
	      mu_error (_("no such user: %s"), owner_user);
	      exit (EX_NOUSER);
	    }
	}
      else
	{
	  owner_uid = ap->uid;
	  known_meta_data |= META_UID;
	  mu_auth_data_free (ap);
	}
    }
  
  if (known_meta_data & META_GROUP)
    {
      struct group *gr = getgrnam (owner_group);
      if (!gr)
	{
	  if (!(known_meta_data & META_GID))
	    {
	      mu_error (_("no such group: %s"), owner_group);
	      exit (EX_DATAERR);
	    }
	}
      else
	{
	  owner_gid = gr->gr_gid;
	  known_meta_data |= META_GID;
	}
    }
  
  if (fstat (dirfd, &st))
    {
      mu_diag_funcall (MU_DIAG_ERROR, "fstat", db_name, errno);
      exit (EX_SOFTWARE);
    }
  
  if (known_meta_data & META_FILE_MODE)
    {
      if (fchmod (pagfd, file_mode) ||
	  (pagfd != dirfd && fchmod (dirfd, file_mode)))
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "fchmod", db_name, errno);
	  exit (EX_SOFTWARE);
	}
    }	      
  
  if (known_meta_data & (META_UID|META_GID))
    {
      if (!(known_meta_data & META_UID))
	owner_uid = st.st_uid;
      else if (!(known_meta_data & META_GID))
	owner_gid = st.st_gid;
      
      if (fchown (pagfd, owner_uid, owner_gid) ||
	  (pagfd != dirfd && fchown (dirfd, owner_uid, owner_gid)))
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "fchown", db_name, errno);
	  exit (EX_SOFTWARE);
	}
    }
}


#define IOBUF_REREAD 1
#define IOBUF_EOF    2

struct iobuf
{
  mu_stream_t stream;
  char *buffer;
  size_t bufsize;
  size_t length;
  int flag;            /* 0 or one of the IOBUF_ constants above. */
};

static int is_dbm_directive (struct iobuf *input);
static int is_len_directive (struct iobuf *input);
static int is_ignored_directive (const char *name);
static int set_directive (char *val);
static void print_header (const char *name, mu_stream_t str);

#define input_eof(iob) ((iob)->flag == IOBUF_EOF)

static int
input_getline (struct iobuf *inp)
{
  int rc;
  size_t len;

  switch (inp->flag)
    {
    case 0:
      break;

    case IOBUF_REREAD:
      inp->flag = 0;
      return 0;

    case IOBUF_EOF:
      inp->length = 0;
      return 0;
    }
  
  while (1)
    {
      rc = mu_stream_getline (inp->stream, &inp->buffer, &inp->bufsize, &len);
      if (rc)
	return rc;
      if (len == 0)
	{
	  inp->flag = IOBUF_EOF;
	  inp->length = 0;
	  break;
	}
      mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
		       MU_IOCTL_LOGSTREAM_ADVANCE_LOCUS_LINE, NULL);
      inp->length = mu_rtrim_class (inp->buffer, MU_CTYPE_ENDLN);
      if (inp->buffer[0] == '#' && !is_dbm_directive (inp))
	{
	  struct mu_locus loc;
	  mu_stream_ioctl (mu_strerr,
			   MU_IOCTL_LOGSTREAM, MU_IOCTL_LOGSTREAM_GET_LOCUS,
			   &loc);
	  loc.mu_line = strtoul (inp->buffer + 1, NULL, 10);
	  mu_stream_ioctl (mu_strerr,
			   MU_IOCTL_LOGSTREAM, MU_IOCTL_LOGSTREAM_SET_LOCUS,
			   &loc);
	  free (loc.mu_file);
	  continue;
	}
      break;
    }
  return 0;
}

static void
input_ungetline (struct iobuf *inp)
{
  if (inp->flag == 0)
    inp->flag = IOBUF_REREAD;
}

static size_t
input_length (struct iobuf *inp)
{
  return inp->length;
}

struct xfer_format
{
  const char *name;
  void (*init) (struct xfer_format *fmt, const char *version,
		struct iobuf *iop, int wr);
  int (*reader) (struct iobuf *, void *data, struct mu_dbm_datum *key,
		 struct mu_dbm_datum *content);
  int (*writer) (struct iobuf *, void *data, struct mu_dbm_datum const *key,
		 struct mu_dbm_datum const *content);
  void *data;
};

static int ascii_reader (struct iobuf *, void *data,
			 struct mu_dbm_datum *key,
			 struct mu_dbm_datum *content);
static int ascii_writer (struct iobuf *, void *data,
			 struct mu_dbm_datum const *key,
			 struct mu_dbm_datum const *content);

static int C_reader (struct iobuf *, void *data,
		     struct mu_dbm_datum *key,
		     struct mu_dbm_datum *content);
static int C_writer (struct iobuf *, void *data,
		     struct mu_dbm_datum const *key,
		     struct mu_dbm_datum const *content);

static void newfmt_init (struct xfer_format *fmt,
			 const char *version, struct iobuf *iop, int wr);

static int newfmt_reader (struct iobuf *, void *data,
			  struct mu_dbm_datum *key,
			  struct mu_dbm_datum *content);
static int newfmt_writer (struct iobuf *, void *data,
			  struct mu_dbm_datum const *key,
			  struct mu_dbm_datum const *content);

static struct xfer_format format_tab[] = {
  { "C",   NULL, C_reader,     C_writer, NULL },
  { "0.0", NULL, ascii_reader, ascii_writer, NULL },
  { "1.0", newfmt_init, newfmt_reader, newfmt_writer, NULL },
  { NULL }
};
						     
#define DEFAULT_LIST_FORMAT (&format_tab[1])
#define DEFAULT_DUMP_FORMAT (&format_tab[2])
#define DEFAULT_LOAD_FORMAT (&format_tab[1])

static struct xfer_format *format;
static const char *dump_format_version;

						     
static int
read_data (struct iobuf *inp, struct mu_dbm_datum *key,
	   struct mu_dbm_datum *content)
{
  return format->reader (inp, format->data, key, content);
}

static int
write_data (struct iobuf *iop, struct mu_dbm_datum const *key,
	    struct mu_dbm_datum const *content)
{
  return format->writer (iop, format->data, key, content);
}     
						     
static int
select_format (const char *version)
{
  struct xfer_format *fmt;

  dump_format_version = version;
  for (fmt = format_tab; fmt->name; fmt++)
    if (strcmp (fmt->name, version) == 0)
      {
	format = fmt;
	return 0;
      }
  mu_error (_("unsupported format version: %s"), version);
  return 1;
}

static void
init_format (int wr, struct iobuf *iop)
{
  if (format->init)
    format->init (format,
		  dump_format_version ? dump_format_version : format->name,
		  iop, wr);
}


static int
ascii_writer (struct iobuf *iop, void *data,
	      struct mu_dbm_datum const *key,
	      struct mu_dbm_datum const *content)
{
  size_t len;

  if (!data)
    {
      if (!suppress_header && include_zero == 1 &&
	  !is_ignored_directive ("null"))
	mu_stream_printf (iop->stream, "#:null\n");
      data = ascii_writer;
    }
  
  len = key->mu_dsize;
  if (key->mu_dptr[len - 1] == 0)
    len--;
  mu_stream_write (iop->stream, key->mu_dptr, len, NULL);
  mu_stream_write (iop->stream, "\t", 1, NULL);

  len = content->mu_dsize;
  if (content->mu_dptr[len - 1] == 0)
    len--;
  mu_stream_write (iop->stream, content->mu_dptr, len, NULL);
  mu_stream_write (iop->stream, "\n", 1, NULL);
  return 0;
}

static int
ascii_reader (struct iobuf *inp, void *data MU_ARG_UNUSED,
	      struct mu_dbm_datum *key,
	      struct mu_dbm_datum *content)
{
  char *kstr, *val;
  int rc;
  
  rc = input_getline (inp);
  if (rc)
    {
      mu_error ("%s", mu_strerror (rc));
      return -1;
    }
  if (input_eof (inp))
    return 1;
  if (input_length (inp) == 0)
    {
      key->mu_dsize = 0;
      return 0;
    }
  kstr = mu_str_stripws (inp->buffer);
  val = mu_str_skip_class_comp (kstr, MU_CTYPE_SPACE);
  *val++ = 0;
  val = mu_str_skip_class (val, MU_CTYPE_SPACE);
  if (*val == 0)
    {
      mu_error (_("malformed line"));
      key->mu_dsize = 0;
    }
  else
    {
      init_datum (key, kstr);
      init_datum (content, val);
    }
  return 0;
}  

void
write_quoted_string (mu_stream_t stream, char *str, size_t len)
{
  for (; len; str++, len--)
    {
      if (*str == '"')
	{
	  mu_stream_write (stream, "\\", 1, NULL);
	  mu_stream_write (stream, str, 1, NULL);
	}
      else if (*str != '\t' && *str != '\\' && mu_isprint (*str))
	mu_stream_write (stream, str, 1, NULL);
      else
	{
	  int c = mu_wordsplit_c_quote_char (*str);

	  mu_stream_write (stream, "\\", 1, NULL);
	  if (c != -1)
	    mu_stream_write (stream, str, 1, NULL);
	  else
	    mu_stream_printf (stream, "%03o", *(unsigned char *) str);
	}
    }
}

static int
C_writer (struct iobuf *iop, void *data MU_ARG_UNUSED,
	  struct mu_dbm_datum const *key,
	  struct mu_dbm_datum const *content)
{
  write_quoted_string (iop->stream, key->mu_dptr, key->mu_dsize);
  mu_stream_write (iop->stream, "\n", 1, NULL);
  write_quoted_string (iop->stream, content->mu_dptr, content->mu_dsize);
  mu_stream_write (iop->stream, "\n\n", 2, NULL);
  return 0;
}

#define ISODIGIT(c) ((c) >= '0' && c < '8')
#define OCTVAL(c) ((c) - '0')

static int
C_read_datum (struct iobuf *inp, struct mu_dbm_datum *datum)
{
  size_t i = 0;
  char *base, *ptr;
  size_t length;
  int rc;
  
  free (datum->mu_dptr);
  memset (datum, 0, sizeof (*datum));

  rc = input_getline (inp);
  if (rc)
    {
      mu_error ("%s", mu_strerror (rc));
      return -1;
    }
  if (input_eof (inp))
    return 1;
  if ((length = input_length (inp)) == 0)
    {
      mu_error (_("empty line"));
      return -1;
    }

  memset (datum, 0, sizeof (*datum));
  datum->mu_dptr = ptr = mu_alloc (length);
  base = inp->buffer;
  for (i = 0; i < length; ptr++)
    {
      if (base[i] == '\\')
	{
	  if (++i >= length)
	    {
	      mu_error (_("unfinished string"));
	      return -1;
	    }
	  else if (ISODIGIT (base[i]))
	    {
	      unsigned c;

	      if (i + 3 > length)
		{
		  mu_error (_("unfinished string"));
		  return -1;
		}
	      c = OCTVAL (base[i]);
	      c <<= 3;
	      c |= OCTVAL (base[i + 1]);
	      c <<= 3;
	      c |= OCTVAL (base[i + 2]);
	      *ptr = c;
	      i += 3;
	    }
	  else
	    {
	      int c = mu_wordsplit_c_unquote_char (base[i]);
	      if (c == -1)
		{
		  mu_error (_("invalid escape sequence (\\%c)"), base[i]);
		  return -1;
		}
	      *ptr = c;
	    }
	}
      else
	*ptr = base[i++];
    }
  datum->mu_dsize = ptr - datum->mu_dptr;
  return 0;
}

static int
C_reader (struct iobuf *inp, void *data MU_ARG_UNUSED,
	  struct mu_dbm_datum *key,
	  struct mu_dbm_datum *content)
{
  int rc;

  rc = C_read_datum (inp, key);
  if (rc < 0)
    exit (EX_DATAERR);
  else if (rc == 1)
    return 1;
  
  if (C_read_datum (inp, content))
    exit (EX_DATAERR);

  rc = input_getline (inp);
  if (rc)
    {
      mu_error ("%s", mu_strerror (rc));
      return -1;
    }

  if (input_length (inp) != 0)
    {
      mu_error (_("unrecognized line"));
      return -1;
    }

  return 0;
}  

static void
newfmt_init (struct xfer_format *fmt, const char *version,
	     struct iobuf *iop, int wr)
{
  int rc;

  if (!wr)
    {
      mu_stream_t flt;
	
      rc = mu_linelen_filter_create (&flt, iop->stream, 76, MU_STREAM_WRITE);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_linelen_filter_create",
			   NULL, rc);
	  exit (EX_SOFTWARE);
	}
      iop->stream = flt;
    }
  else
    {
      mu_opool_t pool;
      rc = mu_opool_create (&pool, 0);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_opool_create",
			   NULL, rc);
	  exit (EX_SOFTWARE);
	}
      fmt->data = pool;
    }      
}

static int
newfmt_read_datum (struct iobuf *iop, mu_opool_t pool,
		   struct mu_dbm_datum *datum)
{
  int rc;
  unsigned char *ptr, *out;
  size_t len;

  free (datum->mu_dptr);
  memset (datum, 0, sizeof (*datum));
	  
  rc = input_getline (iop);
  if (rc)
    {
      mu_error ("%s", mu_strerror (rc));
      exit (EX_IOERR);
    }
  if (input_eof (iop))
    return 1;
  
  if (!is_len_directive (iop))
    {
      mu_error (_("unrecognized input line"));
      exit (EX_DATAERR);
    }
  else
    {
      char *p;
      unsigned long n;
  
      errno = 0;
      n = strtoul (iop->buffer + 6, &p, 10);
      if (*p || errno)
	{
	  mu_error (_("invalid length"));
	  exit (EX_DATAERR);
	}
      datum->mu_dsize = n;
    }

  mu_opool_clear (pool);
  while (1)
    {
      rc = input_getline (iop);
      if (rc)
	{
	  mu_error ("%s", mu_strerror (rc));
	  exit (EX_IOERR);
	}
      if (input_eof (iop))
	break;
      if (is_len_directive (iop))
	{
	  input_ungetline (iop);
	  break;
	}
      mu_opool_append (pool, iop->buffer, iop->length);
    }

  ptr = mu_opool_finish (pool, &len);
  
  rc = mu_base64_decode (ptr, len, &out, &len);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_base64_decode", NULL, rc);
      exit (EX_SOFTWARE);
    }

  datum->mu_dptr = (void*) out;
  return 0;
}

static int
newfmt_reader (struct iobuf *input, void *data,
	       struct mu_dbm_datum *key,
	       struct mu_dbm_datum *content)
{
  mu_opool_t pool = data;
  if (newfmt_read_datum (input, pool, key))
    return 1;
  if (newfmt_read_datum (input, pool, content))
    return 1;
  return 0;
}

static void
newfmt_write_datum (struct iobuf *iop, struct mu_dbm_datum const *datum)
{
  int rc;
  
  mu_stream_printf (iop->stream, "#:len=%lu\n",
		    (unsigned long) datum->mu_dsize);

  rc = mu_base64_encode ((unsigned char*)datum->mu_dptr, datum->mu_dsize,
			 (unsigned char **)&iop->buffer, &iop->bufsize);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_base64_encode", NULL, rc);
      exit (EX_SOFTWARE);
    }
  mu_stream_write (iop->stream, iop->buffer, iop->bufsize, NULL);
  free (iop->buffer); /* FIXME */
  mu_stream_write (iop->stream, "\n", 1, NULL);
}

static int
newfmt_writer (struct iobuf *iop, void *data,
	       struct mu_dbm_datum const *key,
	       struct mu_dbm_datum const *content)
{
  newfmt_write_datum (iop, key);
  newfmt_write_datum (iop, content);
  return 0;
}


struct print_data
{
  mu_dbm_file_t db;
  struct iobuf iob;
};
  
static int
print_action (struct mu_dbm_datum const *key, void *data)
{
  int rc;
  struct mu_dbm_datum contents;
  struct print_data *pd = data;
  
  if (!suppress_header)
    {
      int fd;
      struct stat st;
      const char *name, *p;
      rc = mu_dbm_get_fd (pd->db, &fd, NULL);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_dbm_get_fd", db_name, rc);
	  exit (EX_SOFTWARE);
	}
      if (fstat (fd, &st))
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "fstat", db_name, errno);
	  exit (EX_UNAVAILABLE);
	}
  
      if (!(known_meta_data & META_UID))
	{
	  known_meta_data |= META_UID;
	  owner_uid = st.st_uid;
	}
      if (!(known_meta_data & META_GID))
	{
	  known_meta_data |= META_GID;
	  owner_gid = st.st_gid;
	}
      if (!(known_meta_data & META_FILE_MODE))
	{
	  known_meta_data |= META_FILE_MODE;
	  file_mode = st.st_mode & 0777;
	}
      
      if (!(known_meta_data & META_USER))
	{
	  struct mu_auth_data *ap = mu_get_auth_by_uid (owner_uid);
	  if (ap)
	    {
	      owner_user = mu_strdup (ap->name);
	      known_meta_data |= META_USER;
	      mu_auth_data_free (ap);
	    }
	}

      if (!(known_meta_data & META_GROUP))
	{
	  struct group *gr = getgrgid (owner_gid);
	  if (gr)
	    {
	      owner_group = mu_strdup (gr->gr_name);
	      known_meta_data |= META_GROUP;
	    }
	}
      
      mu_dbm_get_name (pd->db, &name);
      p = strrchr (name, '/');
      if (p)
	p++;
      else
	p = name;
      print_header (p, mu_strout);
    }
  
  memset (&contents, 0, sizeof contents);
  rc = mu_dbm_fetch (pd->db, key, &contents);
  if (rc)
    {
      mu_error (_("database fetch error: %s"), mu_dbm_strerror (pd->db));
      exit (EX_UNAVAILABLE);
    }
  rc = write_data (&pd->iob, key, &contents);
  mu_dbm_datum_free (&contents);
  suppress_header = 1;
  return rc;
}

static void
iterate_database (mu_dbm_file_t db,
		  int (*matcher) (const char *, void *), void *matcher_data,
		  int (*action) (struct mu_dbm_datum const *, void *),
		  void *action_data)
{
  int rc;
  struct mu_dbm_datum key;
  char *buf = NULL;
  size_t bufsize = 0;

  memset (&key, 0, sizeof key);
  for (rc = mu_dbm_firstkey (db, &key); rc == 0;
       rc = mu_dbm_nextkey (db, &key))
    {
      if (include_zero == -1)
	include_zero = key.mu_dptr[key.mu_dsize-1] == 0;

      if (matcher)
	{
	  if (key.mu_dsize + 1 > bufsize)
	    {
	      bufsize = key.mu_dsize + 1;
	      buf = mu_realloc (buf, bufsize);
	    }
	  memcpy (buf, key.mu_dptr, key.mu_dsize);
	  buf[key.mu_dsize] = 0;
	  if (!matcher (buf, matcher_data))
	    continue;
	}
      if (action (&key, action_data))
	break;
    }
  free (buf);
}

static int
match_literal (const char *str, void *data)
{
  char **argv = data;

  for (; *argv; argv++)
    {
      if ((case_sensitive ? strcmp : strcasecmp) (str, *argv) == 0)
	return 1;
    }
  return 0;
}

#ifndef FNM_CASEFOLD
# define FNM_CASEFOLD 0
#endif

static int
match_glob (const char *str, void *data)
{
  char **argv = data;
  
  for (; *argv; argv++)
    {
      if (fnmatch (*argv, str, case_sensitive ? FNM_CASEFOLD : 0) == 0)
	return 1;
    }
  return 0;
}

struct regmatch
{
  int regc;
  regex_t *regs;
};
  
static int
match_regex (const char *str, void *data)
{
  struct regmatch *match = data;
  int i;
  
  for (i = 0; i < match->regc; i++)
    {
      if (regexec (&match->regs[i], str, 0, NULL, 0) == 0)
	return 1;
    }
  return 0;
}

static void
compile_regexes (int argc, char **argv, struct regmatch *match)
{
  regex_t *regs = mu_calloc (argc, sizeof (regs[0]));
  int i;
  int cflags = (case_sensitive ? 0: REG_ICASE) | REG_EXTENDED | REG_NOSUB;
  int errors = 0;
  
  for (i = 0; i < argc; i++)
    {
      int rc = regcomp (&regs[i], argv[i], cflags);
      if (rc)
	{
	  char errbuf[512];
	  regerror (rc, &regs[i], errbuf, sizeof (errbuf));
	  mu_error (_("error compiling `%s': %s"), argv[i], errbuf);
	  errors++;
	}
    }
  if (errors)
    exit (EX_USAGE);
  match->regc = argc;
  match->regs = regs;
}

static void
free_regexes (struct regmatch *match)
{
  int i;
  for (i = 0; i < match->regc; i++)
    regfree (&match->regs[i]);
  free (match->regs);
}

static void
list_database (int argc, char **argv)
{
  mu_dbm_file_t db = open_db_file (MU_STREAM_READ);
  struct print_data pdata;
  
  memset (&pdata, 0, sizeof pdata);
  pdata.db = db;
  pdata.iob.stream = mu_strout;

  init_format (0, &pdata.iob);
  
  if (argc == 0)
    iterate_database (db, NULL, NULL, print_action, &pdata);
  else
    {
      switch (key_type)
	{
	case key_literal:
	  iterate_database (db, match_literal, argv, print_action, &pdata);
	  break;

	case key_glob:
	  iterate_database (db, match_glob, argv, print_action, &pdata);
	  break;

	case key_regex:
	  {
	    struct regmatch m;
	    compile_regexes (argc, argv, &m);
	    iterate_database (db, match_regex, &m, print_action, &pdata);
	    free_regexes (&m);
	  }
	}
    }
  mu_dbm_destroy (&db);
  mu_stream_close (pdata.iob.stream);
}

static int
is_dbm_directive (struct iobuf *input)
{
  return input->length > 2 &&
         input->buffer[0] == '#' &&
         input->buffer[1] == ':';
}

static int
is_len_directive (struct iobuf *input)
{
  return input->length > 6 && memcmp (input->buffer, "#:len=", 6) == 0;
}

/*
  #:version=1.0                       # Version number
  #:filename=S                        # Original database name (basename)
  #:uid=N,user=S,gid=N,group=S,mode=N # File meta info
  #:null[=0|1]                        # Null termination (v.0.0 only)
  #:len=N                             # record length (v.1.0)
*/

static int
_set_version (const char *val)
{
  if (select_format (val))
    exit (EX_DATAERR);
  return 0;
}

static int
_set_file (const char *val)
{
  if (!db_name)
    db_name = mu_strdup (val);
  return 0;
}

static int
_set_null (const char *val)
{
  if (!val)
    include_zero = 1;
  else if (val[0] == '0' && val[1] == 0)
    include_zero = 0;
  else if (val[0] == '1' && val[1] == 0)
    include_zero = 1;
  else
    {
      mu_error (_("bad value for `null' directive: %s"), val);
      exit (EX_DATAERR);
    }
  return 0;
}

static int
_set_uid (const char *val)
{
  char *p;
  unsigned long n;

  if (known_meta_data & META_UID)
    return 0;
  errno = 0;
  n = strtoul (val, &p, 8);
  if (*p || errno)
    {
      mu_error (_("invalid UID"));
      exit (EX_NOUSER);
    }
  owner_uid = n;
  known_meta_data |= META_UID;
  return 0;
}

static int
_set_gid (const char *val)
{
  char *p;
  unsigned long n;
  
  if (known_meta_data & META_GID)
    return 0;
  errno = 0;
  n = strtoul (val, &p, 8);
  if (*p || errno)
    {
      mu_error (_("invalid GID"));
      exit (EX_DATAERR);
    }
  owner_gid = n;
  known_meta_data |= META_GID;
  return 0;
}

static int
_set_user (const char *val)
{
  if (known_meta_data & META_USER)
    return 0;
  free (owner_user);
  owner_user = mu_strdup (val);
  known_meta_data |= META_USER;
  return 0;
}

static int
_set_group (const char *val)
{
  if (known_meta_data & META_GROUP)
    return 0;
  free (owner_group);
  owner_group = mu_strdup (val);
  known_meta_data |= META_GROUP;
  return 0;
}

static int
_set_mode (const char *val)
{
  char *p;
  unsigned long n;
  
  if (known_meta_data & META_FILE_MODE)
    return 0;
  errno = 0;
  n = strtoul (val, &p, 8);
  if (*p || errno || n > 0777)
    {
      mu_error (_("invalid file mode"));
      exit (EX_DATAERR);
    }
  file_mode = n;
  known_meta_data |= META_FILE_MODE;
  return 0;
}

struct dbm_directive
{
  const char *name;
  size_t len;
  int (*set) (const char *val);
  int flags;
};

#define DF_VALUE   0x01  /* directive requires a value */
#define DF_HEADER  0x02  /* can appear only in file header */
#define DF_PROTECT 0x04  /* directive cannot be ignored */
#define DF_META    0x08  /* directive keeps file meta-information */
#define DF_SEEN    0x10  /* directive was already seen */
#define DF_IGNORE  0x20  /* ignore this directive */

static struct dbm_directive dbm_directive_table[] = {
#define S(s) #s, sizeof #s - 1
  { S(version), _set_version, DF_HEADER|DF_VALUE|DF_PROTECT },
  { S(file),    _set_file,    DF_HEADER|DF_VALUE },
  { S(uid),     _set_uid,     DF_HEADER|DF_META|DF_VALUE },
  { S(user),    _set_user,    DF_HEADER|DF_META|DF_VALUE },
  { S(gid),     _set_gid,     DF_HEADER|DF_META|DF_VALUE },
  { S(group),   _set_group,   DF_HEADER|DF_META|DF_VALUE },
  { S(mode),    _set_mode,    DF_HEADER|DF_META|DF_VALUE },
  { S(null),    _set_null,    DF_HEADER|DF_VALUE },
  { S(null),    _set_null,    DF_HEADER },
#undef S
  { NULL }
};

static void
ignore_directives (const char *arg)
{
  struct mu_wordsplit ws;
  size_t i;
  
  ws.ws_delim = ",";
  if (mu_wordsplit (arg, &ws,
		    MU_WRDSF_NOVAR | MU_WRDSF_NOCMD | MU_WRDSF_DELIM))
    {
      mu_error (_("cannot split input line: %s"), mu_wordsplit_strerror (&ws));
      exit (EX_SOFTWARE);
    }

  for (i = 0; i < ws.ws_wordc; i++)
    {
      char *arg = ws.ws_wordv[i];
      size_t len = strlen (arg);
      struct dbm_directive *p;
      int found = 0;
      
      for (p = dbm_directive_table; p->name; p++)
	{
	  if (p->len == len && memcmp (p->name, arg, len) == 0)
	    {
	      if (p->flags & DF_PROTECT)
		mu_error (_("directive %s cannot be ignored"), p->name);
	      else
		p->flags |= DF_IGNORE;
	      found = 1;
	    }
	}
      if (!found)
	mu_error (_("unknown directive: %s"), arg);
    }
  mu_wordsplit_free (&ws);
}

static int
is_ignored_directive (const char *arg)
{
  size_t len = strlen (arg);
  struct dbm_directive *p;
      
  for (p = dbm_directive_table; p->name; p++)
    {
      if (p->len == len && memcmp (p->name, arg, len) == 0)
	return p->flags & DF_IGNORE;
    }
  return 0;
}

static void
ignore_flagged_directives (int flag)
{
  struct dbm_directive *p;
  
  for (p = dbm_directive_table; p->name; p++)
    if (!(p->flags & DF_PROTECT) && (p->flags & flag))
      p->flags |= DF_IGNORE;
}

static int
set_directive (char *val)
{
  size_t len;
  struct dbm_directive *p, *match = NULL;

  mu_ltrim_class (val, MU_CTYPE_BLANK);
  mu_rtrim_class (val, MU_CTYPE_BLANK);
  len = strcspn (val, "=");
  for (p = dbm_directive_table; p->name; p++)
    {
      if (p->len == len && memcmp (p->name, val, len) == 0)
	{
	  int rc;
	  
	  match = p;
	  if (val[len] == '=')
	    {
	      if (!(p->flags & DF_VALUE))
		continue;
	    }
	  else if (p->flags & DF_VALUE)
	    continue;
		  
	  if (p->flags & DF_IGNORE)
	    return 0;
	  
	  if (p->flags & DF_SEEN)
	    {
	      mu_error (_("directive %s appeared twice"), val);
	      return 1;
	    }
	  rc = p->set ((p->flags & DF_VALUE) ? val + len + 1 : NULL);
	  if (p->flags & DF_HEADER)
	    p->flags |= DF_SEEN;
	  return rc;
	}
    }
  if (match)
    {
      if (match->flags & DF_VALUE)
	mu_error (_("directive requires argument: %s"), val);
      else
	mu_error (_("directive does not take argument: %s"), val);
    }
  else
    mu_error (_("unknown or unsupported directive %s"), val);
  return 1;
}

static void
print_header (const char *name, mu_stream_t str)
{
  char *delim;
  time_t t;

  time (&t);
  mu_stream_printf (str, "# ");
  mu_stream_printf (str, _("Database dump file created by %s on %s"),
		    PACKAGE_STRING,
		    ctime (&t));
  mu_stream_printf (str, "#:version=%s\n", format->name);
  if (!is_ignored_directive ("file"))
    mu_stream_printf (str, "#:file=%s\n", name);

  delim = "#:";
  if ((known_meta_data & META_UID) && !is_ignored_directive ("uid"))
    {
      mu_stream_printf (str, "%suid=%lu", delim, (unsigned long) owner_uid);
      delim = ",";
    }
  if ((known_meta_data & META_USER) && !is_ignored_directive ("user"))
    {
      mu_stream_printf (str, "%suser=%s", delim, owner_user);
      delim = ",";
    }
  
  if ((known_meta_data & META_GID) && !is_ignored_directive ("gid"))
    {
      mu_stream_printf (str, "%sgid=%lu", delim, (unsigned long) owner_gid);
      delim = ",";
    }

  if ((known_meta_data & META_GROUP) && !is_ignored_directive ("group"))
    {
      mu_stream_printf (str, "%sgroup=%s", delim, owner_group);
      delim = ",";
    }
  
  if ((known_meta_data & META_FILE_MODE) && !is_ignored_directive ("mode"))
    mu_stream_printf (str, "%smode=%03o", delim, file_mode);

  if (delim[0] == ',')
    mu_stream_printf (str, "\n");
}


static void
add_records (int mode, int replace)
{
  mu_dbm_file_t db;
  mu_stream_t instream, flt;
  const char *flt_argv[] = { "inline-comment", "#", "-S", "-i", "#", NULL };
  int rc;
  int save_log_mode = 0, log_mode;
  struct mu_locus save_locus = { NULL, }, locus;
  struct mu_dbm_datum key, contents;
  struct iobuf input;
  struct mu_wordsplit ws;
  int wsflags = MU_WRDSF_NOVAR | MU_WRDSF_NOCMD | MU_WRDSF_DELIM;
  
  if (mode != MU_STREAM_CREAT)
    {
      ignore_flagged_directives (DF_META);
      known_meta_data = 0;
    }
  
  /* Prepare input data */
  if (input_file)
    {
      rc = mu_file_stream_create (&instream, input_file, MU_STREAM_READ);
      if (rc)
	{
	  mu_error (_("cannot open input file %s: %s"), input_file,
		    mu_strerror (rc));
	  exit (EX_NOINPUT);
	}
    }
  else
    {
      instream = mu_strin;
      mu_stream_ref (instream);
    }
  
  rc = mu_filter_create_args (&flt, instream, "inline-comment",
			      MU_ARRAY_SIZE (flt_argv) - 1, flt_argv,
			      MU_FILTER_DECODE, MU_STREAM_READ);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_filter_create_args", input_file, rc);
      exit (EX_UNAVAILABLE);
    }
  mu_stream_unref (instream);
  instream = flt;

  /* Configure error stream to output input file location before each error
     message */
  locus.mu_file = input_file ? input_file : "<stdin>";
  locus.mu_line = 0;
  locus.mu_col = 0;
  
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM, MU_IOCTL_LOGSTREAM_GET_MODE,
		   &save_log_mode);
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM, MU_IOCTL_LOGSTREAM_GET_LOCUS,
		   &save_locus);
  log_mode = save_log_mode | MU_LOGMODE_LOCUS;
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM, MU_IOCTL_LOGSTREAM_SET_MODE,
		   &log_mode);
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM, MU_IOCTL_LOGSTREAM_SET_LOCUS,
		   &locus);

  /* Initialize I/O data */
  memset (&key, 0, sizeof key);
  memset (&contents, 0, sizeof contents);

  /* Initialize input buffer */
  input.buffer = NULL;
  input.bufsize = 0;
  input.length = 0;
  input.flag = 0;
  input.stream = instream;

  /* Read directive header */
  ws.ws_delim = ",";
  while ((rc = input_getline (&input)) == 0 &&
	 is_dbm_directive (&input) &&
	 !is_len_directive (&input))
    {
      size_t i;

      if (mu_wordsplit (input.buffer + 2, &ws, wsflags))
	{
	  mu_error (_("cannot split input line: %s"),
		    mu_wordsplit_strerror (&ws));
	  exit (EX_SOFTWARE);
	}
      for (i = 0; i < ws.ws_wordc; i++)
	{
	  set_directive (ws.ws_wordv[i]);
	}
      wsflags |= MU_WRDSF_REUSE;
    }
  if (wsflags & MU_WRDSF_REUSE)
    mu_wordsplit_free (&ws);

  if (!format)
    format = DEFAULT_LOAD_FORMAT;
  init_format (1, &input);
  
  /* Open the database */
  db = open_db_file (mode);
  
  /* Read and store the actual data */
  if (rc == 0 && input_length (&input) > 0)
    {
      ignore_flagged_directives (DF_HEADER);
      input_ungetline (&input);

      memset (&key, 0, sizeof key);
      memset (&contents, 0, sizeof contents);
      
      while ((rc = read_data (&input, &key, &contents)) == 0)
	{
	  if (key.mu_dsize)
	    {
	      rc = mu_dbm_store (db, &key, &contents, replace);
	      if (rc)
		mu_error (_("cannot store datum: %s"),
			  rc == MU_ERR_FAILURE ?
			  mu_dbm_strerror (db) : mu_strerror (rc));
	    }
	}
    }
  
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM, MU_IOCTL_LOGSTREAM_SET_MODE,
		   &save_log_mode);
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM, MU_IOCTL_LOGSTREAM_SET_LOCUS,
		   &save_locus);
  
  if (known_meta_data)
    set_db_ownership (db);
  mu_dbm_destroy (&db);
  mu_stream_unref (instream);
}

static void
create_database ()
{
  if (getuid() != 0)
    ignore_flagged_directives (DF_META);
  
  if (copy_permissions)
    {
      struct stat st;
      
      if (!input_file)
	{
	  mu_error (_("--copy-permissions used without --file"));
	  exit (EX_USAGE);
	}
      if (stat (input_file, &st))
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "stat", input_file, errno);
	  exit (EX_UNAVAILABLE);
	}
      owner_uid = st.st_uid;
      owner_gid = st.st_gid;
      file_mode = st.st_mode & 0777;
      known_meta_data |= META_UID | META_GID | META_FILE_MODE;
    }
  else if (auth)
    {
      if (!(known_meta_data & META_UID))
	{
	  owner_uid = auth->uid;
	  known_meta_data |= META_UID;
	}
      if (!(known_meta_data & META_GID))
	{
	  owner_gid = auth->gid;
	  known_meta_data |= META_GID;
	}
    }
  add_records (MU_STREAM_CREAT, 0);
}

static int
store_to_list (struct mu_dbm_datum const *key, void *data)
{
  int rc;
  mu_list_t list = data;
  char *p = mu_alloc (key->mu_dsize + 1);
  memcpy (p, key->mu_dptr, key->mu_dsize);
  p[key->mu_dsize] = 0;
  rc = mu_list_append (list, p);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_append", p, rc);
      exit (EX_SOFTWARE);
    }
  return 0;
}

static int
do_delete (void *item, void *data)
{
  char *str = item;
  mu_dbm_file_t db = data;
  struct mu_dbm_datum key;
  int rc;

  init_datum (&key, str);
  rc = mu_dbm_delete (db, &key);
  if (rc == MU_ERR_NOENT)
    {
      mu_error (_("cannot remove record for %s: %s"),
		str, mu_strerror (rc));
    }
  else if (rc)
    {
      mu_error (_("cannot remove record for %s: %s"),
		str, mu_dbm_strerror (db));
      if (rc != MU_ERR_NOENT)
	exit (EX_UNAVAILABLE);
    }
  return 0;
}

static void
delete_database (int argc, char **argv)
{
  mu_dbm_file_t db = open_db_file (MU_STREAM_RDWR);
  mu_list_t templist = NULL;
  int rc, i;
  
  if (argc == 0)
    {
      mu_error (_("not enough arguments for delete"));
      exit (EX_USAGE);
    }

  /* Collect matching keys */
  rc = mu_list_create (&templist);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_create", NULL, rc);
      exit (EX_SOFTWARE);
    }
  mu_list_set_destroy_item (templist, mu_list_free_item);
  switch (key_type)
    {
    case key_literal:
      for (i = 0; i < argc; i++)
	{
	  char *p = mu_strdup (argv[i]);
	  rc = mu_list_append (templist, p);
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_append", p, rc);
	      exit (EX_SOFTWARE);
	    }
	}
      break;

    case key_glob:
      iterate_database (db, match_glob, argv,
			store_to_list, templist);
      break;

    case key_regex:
      {
	struct regmatch m;
	
	compile_regexes (argc, argv, &m);
	iterate_database (db, match_regex, &m, store_to_list, templist);
	free_regexes (&m);
      }
    }
  mu_list_foreach (templist, do_delete, db);
  mu_list_destroy (&templist);
  mu_dbm_destroy (&db);
}

/*
  mu dbm --create a.db < input
  mu dbm --list a.db
  mu dbm --delete a.db key [key...]
  mu dbm --add a.db < input
  mu dbm --replace a.db < input
 */

static struct argp_option dbm_options[] = {
  { NULL, 0, NULL, 0, N_("Create Options"), 0},
  { "file",   'f', N_("FILE"), 0,
    N_("read input from FILE (with create, delete, add and replace)") },
  { "permissions", 'p', N_("NUM"), 0,
    N_("set permissions on the created database") },
  { "user",        'u', N_("USER"), 0,
    N_("set database owner name") },
  { "group",       'g', N_("GROUP"), 0,
    N_("set database owner group") },
  { "copy-permissions", 'P', NULL, 0,
    N_("copy database permissions and ownership from the input file") },
  { "ignore-meta",      'm', NULL, 0,
    N_("ignore meta-information from input file headers") },
  { "ignore-directives", 'I', N_("NAMES"), 0,
    N_("ignore the listed directives") },
  { NULL, 0, NULL, 0, N_("List and Dump Options"), 0},
  { "format",           'H', N_("TYPE"), 0,
    N_("select output format") },
  { "no-header",        'q', NULL, 0,
    N_("suppress format header") },
  { NULL, 0, NULL, 0, N_("List, Dump and Delete Options"), 0},
  { "glob",        'G', NULL, 0,
    N_("treat keys as globbing patterns") },
  { "regex",       'R', NULL, 0,
    N_("treat keys as regular expressions") },
  { "ignore-case", 'i', NULL, 0,
    N_("case-insensitive matches") },
  { NULL, 0, NULL, 0, N_("Options for Use with Format 0.0"), 0 },
  { "count-null",  'N', NULL, 0,
    N_("data length accounts for terminating zero") },
  { "no-count-null", 'n', NULL, 0,
    N_("data length does not account for terminating zero") },
  { NULL }
};

static error_t
dbm_parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'f':
      input_file = arg;
      break;

    case 'H':
      select_format (arg);
      break;
      
    case 'p':
      {
	char *p;
	unsigned long d = strtoul (arg, &p, 8);
	if (*p || (d & ~0777))
	  argp_error (state, _("invalid file mode: %s"), arg);
	file_mode = d;
	known_meta_data |= META_FILE_MODE;
      }
      break;

    case 'P':
      copy_permissions = 1;
      break;

    case 'q':
      suppress_header = 1;
      break;
      
    case 'u':
      auth = mu_get_auth_by_name (arg);
      if (auth)
	known_meta_data |= META_AUTH;
      else
	{
	  char *p;
	  unsigned long n = strtoul (arg, &p, 0);
	  if (*p == 0)
	    {
	      owner_uid = n;
	      known_meta_data |= META_UID;
	    }
	  else
	    argp_error (state, _("no such user: %s"), arg);
	}
      ignore_directives ("user,uid");
      break;

    case 'g':
      {
	struct group *gr = getgrnam (arg);
	if (!gr)
	  {
	    char *p;
	    unsigned long n = strtoul (arg, &p, 0);
	    if (*p == 0)
	      owner_gid = n;
	    else
	      argp_error (state, _("no such group: %s"), arg);
	  }
	else
	  owner_gid = gr->gr_gid;
	known_meta_data |= META_GID;
	ignore_directives ("group,gid");
      }
      break;

    case 'G':
      key_type = key_glob;
      break;

    case 'R':
      key_type = key_regex;
      break;

    case 'm':/* FIXME: Why m? */
      ignore_flagged_directives (DF_META);
      break;

    case 'I':
      ignore_directives (arg);
      break;
      
    case 'i':
      case_sensitive = 0;
      break;

    case 'N':
      include_zero = 1;
      break;

    case 'n':
      include_zero = 0;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp dbm_argp = {
  dbm_options,
  dbm_parse_opt,
  dbm_args_doc,
  dbm_doc,
  NULL,
  NULL,
  NULL
};

struct mu_kwd mode_tab[] =
{
  { "add",     mode_add },
  { "create",  mode_create },
  { "load",    mode_create },
  { "list",    mode_list },
  { "replace", mode_replace },
  { "delete",  mode_delete },
  { "dump",    mode_dump },
  { NULL }
};

int
mutool_dbm (int argc, char **argv)
{
  int index;

  if (argp_parse (&dbm_argp, argc, argv, 0, &index, NULL))
    return 1;

  argc -= index;
  argv += index;

  if (argc == 0)
    {
      mu_error (_("subcommand not given"));
      exit (EX_USAGE);
    }
  
  if (mu_kwd_xlat_name (mode_tab, argv[0], &index))
    {
      mu_error (_("unknown subcommand: %s"), argv[0]);
      exit (EX_USAGE);
    }
  mode = index;
  argc--;
  argv++;
  
  if (argc == 0)
    {
      if (mode != mode_create)
	{
	  mu_error (_("database name not given"));
	  exit (EX_USAGE);
	}
    }
  else
    {
      db_name = *argv++;
      --argc;
    }
  
  switch (mode)
    {
    case mode_list:
      if (!format)
	format = DEFAULT_LIST_FORMAT;
      if (suppress_header == -1)
	suppress_header = 1;
      list_database (argc, argv);
      break;

    case mode_dump:
      if (!format)
	format = DEFAULT_DUMP_FORMAT;
      if (suppress_header == -1)
	suppress_header = 0;
      list_database (argc, argv);
      break;
      
    case mode_create:
      if (argc)
	{
	  mu_error (_("too many arguments for create"));
	  exit (EX_USAGE);
	}
      create_database ();
      break;
      
    case mode_delete:
      delete_database (argc, argv);
      break;
      
    case mode_add:
      if (argc)
	{
	  mu_error (_("too many arguments for add"));
	  exit (EX_USAGE);
	}
      add_records (MU_STREAM_RDWR, 0);
      break;
      
    case mode_replace:
      if (argc)
	{
	  mu_error (_("too many arguments for replace"));
	  exit (EX_USAGE);
	}
      add_records (MU_STREAM_RDWR, 1);
      break;
    }
  return 0;
}

/*
  MU Setup: dbm
  mu-handler: mutool_dbm
  mu-docstring: dbm_docstring
  mu-cond: ENABLE_DBM
  End MU Setup:
*/
  


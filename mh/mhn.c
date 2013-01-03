/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005-2012 Free Software Foundation, Inc.

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

/* MH mhn command */

#include <mh.h>
#include <signal.h>
#include <mailutils/mime.h>
#include <setjmp.h>

static char doc[] = N_("GNU MH mhn")"\v"
N_("Options marked with `*' are not yet implemented.\n\
Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[MSGLIST]");

static struct argp_option options[] = {
  {"folder",       ARG_FOLDER,       N_("FOLDER"), 0,
   N_("specify folder to operate upon"), 0},
  {"file",         ARG_FILE,         N_("FILE"),   0,
   N_("specify file to operate upon"), 0},

#define GRID 10
  {N_("MIME editing options"),   0,  NULL, OPTION_DOC,  NULL, GRID},
  {"compose",      ARG_COMPOSE,      N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("compose the MIME message (default)"), GRID+1},
  {"build",        0,                NULL,       OPTION_ALIAS,
   NULL, GRID+1 },
  {"nocompose",    ARG_NOCOMPOSE,    NULL, OPTION_HIDDEN, NULL, GRID+1},
  {"nobuild",      ARG_NOCOMPOSE,    NULL, OPTION_HIDDEN|OPTION_ALIAS,
   NULL, GRID+1},
#undef GRID
#define GRID 20  
  {N_("Listing options"),   0,  NULL, OPTION_DOC,  NULL, GRID},
  {"list",         ARG_LIST,         N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("list the table of contents"), GRID+1 },
  {"nolist",       ARG_NOLIST,       NULL, OPTION_HIDDEN, "", GRID+1 },
  {"headers",      ARG_HEADER,       N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("print the banner above the listing"), GRID+1 },
  {"noheaders",    ARG_NOHEADERS,    NULL, OPTION_HIDDEN, "", GRID+1 },
  {"realsize",     ARG_REALSIZE,     N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("list the decoded sizes"), GRID+1 },
  {"norealsize",   ARG_NOREALSIZE,   NULL, OPTION_HIDDEN, "", GRID+1 },
#undef GRID
#define GRID 40
  {N_("Display options"),   0,  NULL, OPTION_DOC,  NULL, GRID},
  {"show",         ARG_SHOW,         N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("display the contents of the messages"), GRID+1},
  {"noshow",       ARG_NOSHOW,       NULL, OPTION_HIDDEN, "", GRID+1 },
  {"serialonly",   ARG_SERIALONLY,   N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* display messages serially"), GRID+1},
  {"noserialonly", ARG_NOSERIALONLY, NULL, OPTION_HIDDEN, "", GRID+1 },
  {"form",         ARG_FORM,         N_("FILE"), 0,
   N_("read mhl format from FILE"), GRID+1},
  {"pause",        ARG_PAUSE,        N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("pause prior to displaying content"), GRID+1},
  {"nopause",      ARG_NOPAUSE,      NULL, OPTION_HIDDEN, "", GRID+1 },
#undef GRID
#define GRID 50
  {N_("Saving options"),   0,  NULL, OPTION_DOC,  NULL, GRID},
  {"store",        ARG_STORE,        N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("write extracted message parts to disk"), GRID+1},
  {"nostore",      ARG_NOSTORE,      NULL, OPTION_HIDDEN, "", GRID+1 },
  {"auto",         ARG_AUTO,         N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("use filenames from the content headers"), GRID+1},
  {"noauto",       ARG_NOAUTO,       NULL, OPTION_HIDDEN, "", GRID+1 },
  {"charset",      ARG_CHARSET,      N_("NAME"), 0,
   N_("use this charset to represent attachment file names"), GRID+1},
#undef GRID
#define GRID 60
  {N_("Other options"),    0,  NULL, OPTION_DOC,  NULL, GRID},
  {"part",         ARG_PART,         N_("PART"), 0,
   N_("limit the scope of the operation to the given part"), GRID+1},
  {"type",         ARG_TYPE,         N_("CONTENT"), 0,
   N_("operate on message part with given multipart content"), GRID+1 },
  {"verbose",      ARG_VERBOSE,     N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("print additional information"), GRID+1 },
  {"noverbose",    ARG_NOVERBOSE,   NULL, OPTION_HIDDEN, "", GRID+1 },
  {"quiet",        ARG_QUIET, 0, 0,
   N_("be quiet"), GRID+1},
#undef GRID
  {NULL}
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "file",          MH_OPT_ARG, "filename" },
  { "compose" },
  { "build" },
  { "list",          MH_OPT_BOOL },
  { "headers",       MH_OPT_BOOL },
  { "realsize",      MH_OPT_BOOL },
  { "show",          MH_OPT_BOOL },
  { "serialonly",    MH_OPT_BOOL },
  { "form",          MH_OPT_ARG, "formfile" },
  { "pause",         MH_OPT_BOOL },
  { "store",         MH_OPT_BOOL },
  { "auto",          MH_OPT_BOOL },
  { "part",          MH_OPT_ARG, "part" },
  { "type",          MH_OPT_ARG, "type" },
  { "verbose",       MH_OPT_BOOL },
  { NULL }
};

typedef struct _msg_part *msg_part_t;

static msg_part_t msg_part_create (size_t num);
static void msg_part_destroy (msg_part_t p);
static int msg_part_eq (msg_part_t a, msg_part_t b);
static void msg_part_incr (msg_part_t p);
static void msg_part_decr (msg_part_t p);
static void msg_part_set_subpart (msg_part_t p, size_t subpart);
static void msg_part_print (msg_part_t p, int width);
static msg_part_t msg_part_parse (char *str);
static int msg_part_level (msg_part_t p);
static size_t msg_part_subpart (msg_part_t p, int level);

enum mhn_mode
  {
    mode_compose,
    mode_list,
    mode_show,
    mode_store,
  };

static enum mhn_mode mode = mode_compose;

#define OPT_HEADERS    001
#define OPT_REALSIZE   002
#define OPT_AUTO       004
#define OPT_SERIALONLY 010
#define OPT_VERBOSE    020
#define OPT_QUIET      040

static int mode_options = OPT_HEADERS;
static int pause_option = -1; /* -pause option. -1 means "not given" */
static char *formfile;
static char *content_type;
static char *content_subtype;
static char *input_file;
static int width = 80;
static char *charset;  /* Charset for output file names.  NULL means
			  no recoding is necessary. */

static mu_msgset_t msgset;
static mu_mailbox_t mbox;
static mu_message_t message;
static msg_part_t req_part;

/* Show flags */
#define MHN_EXCLUSIVE_EXEC  001
#define MHN_STDIN           002
#define MHN_LISTING         004
#define MHN_PAUSE           010

void
sfree (char **ptr)
{
  if (!*ptr)
    return;
  free (*ptr);
  *ptr = NULL;
}

void
split_content (const char *content, char **type, char **subtype)
{
  char *p = strchr (content, '/');
  if (p)
    {
      int len = p - content;
      *type = mu_alloc (len + 1);
      memcpy (*type, content, len);
      (*type)[len] = 0;

      p++;
      *subtype = mu_alloc (strlen (p) + 1);
      strcpy (*subtype, p);
    }
  else
    {
      *type = mu_alloc (strlen (content) + 1);
      strcpy (*type, content);
      *subtype = NULL;
    }
}

static void
split_args (const char *argstr, size_t len, int *pargc, char ***pargv)
{
  struct mu_wordsplit ws;

  ws.ws_delim = ";";
  if (mu_wordsplit_len (argstr, len, &ws,
			(MU_WRDSF_DEFFLAGS & ~MU_WRDSF_QUOTE) |
			MU_WRDSF_DELIM | MU_WRDSF_WS))
    {
      mu_error (_("cannot split line `%s': %s"), argstr,
		mu_wordsplit_strerror (&ws));
      *pargc = 0;
      *pargv = NULL;
    }
  else
    {
      *pargc = ws.ws_wordc;
      *pargv = ws.ws_wordv;
      ws.ws_wordc = 0;
      ws.ws_wordv = NULL;
      mu_wordsplit_free (&ws);
    }
}

int
_get_content_type (mu_header_t hdr, char **value, char **rest)
{
  char *type = NULL;
  mu_header_aget_value_unfold (hdr, MU_HEADER_CONTENT_TYPE, &type);
  if (type == NULL || *type == '\0')
    {
      if (type)
	free (type);
      type = mu_strdup ("text/plain"); /* Default.  */
      if (rest)
	*rest = NULL;
    }
  else
    {
      char *p = strchr (type, ';');
      if (p)
	*p++ = 0;
      if (rest)
	*rest = p;
    }
  *value = type;
  return 0;
}

static int
_get_content_encoding (mu_header_t hdr, char **value)
{
  char *encoding = NULL;
  mu_header_aget_value_unfold (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING,
			       &encoding);
  if (encoding == NULL || *encoding == '\0')
    {
      if (encoding)
	free (encoding);
      encoding = mu_strdup ("7bit"); /* Default.  */
    }
  *value = encoding;
  return 0;
}

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      mh_set_current_folder (arg);
      break;

    case ARG_FILE:
      input_file = arg;
      break;
      
      /* Operation mode */
    case ARG_COMPOSE:
      if (is_true (arg))
	{
	  mode = mode_compose;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOCOMPOSE:
      if (mode == mode_compose)
	mode = mode_compose;
      break;

    case ARG_LIST:
      if (is_true (arg))
	{
	  mode = mode_list;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOLIST:
      if (mode == mode_list)
	mode = mode_compose;
      break;

    case ARG_SHOW:
      if (is_true (arg))
	{
	  mode = mode_show;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOSHOW:
      if (mode == mode_show)
	mode = mode_compose;
      break;

    case ARG_STORE:
      if (is_true (arg))
	{
	  mode = mode_store;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOSTORE:
      if (mode == mode_store)
	mode = mode_compose;
      break;

      /* List options */
    case ARG_HEADER:
      if (is_true (arg))
	{
	  mode_options |= OPT_HEADERS;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOHEADERS:
      mode_options &= ~OPT_HEADERS;
      break;
      
    case ARG_REALSIZE:
      if (is_true (arg))
	{
	  mode_options |= OPT_REALSIZE;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOREALSIZE:
      mode_options &= ~OPT_REALSIZE;
      break;

      /* Display options */

    case ARG_SERIALONLY:
      mh_opt_notimpl_warning ("-[no]serialonly");
      if (is_true (arg))
	{
	  mode_options |= OPT_SERIALONLY;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOSERIALONLY:
      mode_options &= ~OPT_SERIALONLY;
      break;

    case ARG_PAUSE:
      pause_option = is_true (arg);
      break;
      
    case ARG_NOPAUSE:
      pause_option = 0;
      break;

      /* Store options */
    case ARG_AUTO:
      if (is_true (arg))
	{
	  mode_options |= OPT_AUTO;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOAUTO:
      mode_options &= ~OPT_AUTO;
      break;

    case ARG_FORM:
      mh_find_file (arg, &formfile);
      break;

      /* Common options */
    case ARG_VERBOSE:
      if (is_true (arg))
	{
	  mode_options |= OPT_VERBOSE;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOVERBOSE:
      mode_options &= ~OPT_VERBOSE;
      break;

    case ARG_TYPE:
      sfree (&content_type);
      sfree (&content_subtype);
      split_content (arg, &content_type, &content_subtype);
      break;
      
    case ARG_PART:
      req_part = msg_part_parse (arg);
      break;

    case ARG_QUIET:
      mode_options |= OPT_QUIET;
      break;
	
    case ARG_CHARSET:
      charset = arg;
      break;
      
    case ARGP_KEY_FINI:
      if (!formfile)
	mh_find_file ("mhl.headers", &formfile);
      if (!isatty (0))
	pause_option = 0;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


/* *********************** Message part functions ************************* */

struct _msg_part
{
  int level;
  int maxlevel;
  size_t *part;
};

msg_part_t 
msg_part_create (size_t num)
{
  msg_part_t p = mu_alloc (sizeof (*p));
  p->maxlevel = 16;
  p->part = mu_alloc (sizeof (*p->part) * p->maxlevel);
  p->part[0] = num;
  p->level = 0;
  return p;
}

void
msg_part_destroy (msg_part_t p)
{
  free (p->part);
  free (p);
}

int
msg_part_eq (msg_part_t a, msg_part_t b)
{
  int i, level = a->level < b->level ? a->level : b->level;

  for (i = 1; i <= level; i++)
    if (a->part[i] != b->part[i])
      return 1;
  return 0;
}

void
msg_part_incr (msg_part_t p)
{
  if (p->level == p->maxlevel)
    {
      p->maxlevel += 16;
      p->part = mu_realloc (p->part, sizeof (*p->part) * p->maxlevel);
    }
  p->level++;
}

void
msg_part_decr (msg_part_t p)
{
  if (p->level <= 0)
    abort ();
  p->level--;
}

void
msg_part_set_subpart (msg_part_t p, size_t subpart)
{
  p->part[p->level] = subpart;
}

void
msg_part_print (msg_part_t p, int max_width)
{
  int i;
  int width = 0;
  
  for (i = 1; i <= p->level; i++)
    {
      if (i > 1)
	{
	  printf (".");
	  width++;
	}
      width += printf ("%s", mu_umaxtostr (0, p->part[i]));
    }
  for (; width < max_width; width++)
    putchar (' ');
}

char *
msg_part_format (msg_part_t p)
{
  int i;
  int width = 0;
  char *str, *s;
  
  for (i = 1; i <= p->level; i++)
    {
      if (i > 1)
	width++;
      width += strlen (mu_umaxtostr (0, p->part[i]));
    }

  str = s = mu_alloc (width + 1);
  for (i = 1; i <= p->level; i++)
    {
      if (i > 1)
	*s++ = '.';
      s += sprintf (s, "%s", mu_umaxtostr (0, p->part[i]));
    }
  *s = 0;
  return str;
}

void
msg_part_format_pool (mu_opool_t pool, msg_part_t p)
{
  int i;
  
  for (i = 1; i <= p->level; i++)
    {
      int len;
      const char *buf;
  
      if (i > 1)
	mu_opool_append_char (pool, '.');

      buf = mu_umaxtostr (0, p->part[i]);
      len = strlen (buf);
      mu_opool_append (pool, buf, len);
    }
}

msg_part_t
msg_part_parse (char *str)
{
  msg_part_t p = msg_part_create (0);

  do
    {
      char *endp;
      size_t num = strtoul (str, &endp, 10);
      
      switch (*endp)
	{
	case '.':
	  str = endp + 1;
	  break;
	case 0:
	  str = endp;
	  break;
	default:
	  mu_error (_("malformed part specification (near %s)"), endp);
	  exit (1);
	}
      msg_part_incr (p);
      msg_part_set_subpart (p, num);
    }
  while (*str);
  return p;
}

int
msg_part_level (msg_part_t p)
{
  return p->level;
}

size_t
msg_part_subpart (msg_part_t p, int level)
{
  if (level <= p->level)
    return p->part[level];
  return 0;
}


/* *********************** Context file accessors ************************* */

const char *
_mhn_profile_get (const char *prefix, const char *type, const char *subtype,
		  const char *defval)
{
  char *name;
  const char *str = NULL;
  
  if (subtype)
    {
      mu_asprintf (&name, "mhn-%s-%s/%s", prefix, type, subtype);
      str = mh_global_profile_get (name, NULL);
      free (name);
    }

  if (!str)
    {
      mu_asprintf (&name, "mhn-%s-%s", prefix, type);
      str = mh_global_profile_get (name, defval);
      free (name);
    }
  return str;
}

char *
mhn_compose_command (char *typestr, char *typeargs, int *flags, char *file)
{
  const char *p, *str;
  char *type, *subtype, **typeargv = NULL;
  int typeargc = 0;
  mu_opool_t pool;

  split_content (typestr, &type, &subtype);
  str = _mhn_profile_get ("compose", type, subtype, NULL);
  if (!str) 
    return NULL;
  
  /* Expand macro-notations:
    %a  additional arguments
    %f  filename containing content
    %F  %f, and stdout is not redirected
    %s  subtype */

  mu_opool_create (&pool, 1);

  p = mu_str_skip_class (str, MU_CTYPE_SPACE);
  
  if (*p == '|')
    p++;

  for ( ; *p; p++)
    {
      if (*p == '%')
	{
	  switch (*++p)
	    {
	    case 'a':
	      /* additional arguments */
	      if (typeargs)
		{
		  int i;
		  
		  if (!typeargv)
		    split_args (typeargs, strlen (typeargs),
				&typeargc, &typeargv);
		  for (i = 0; i < typeargc; i++)
		    {
		      if (i > 0)
			mu_opool_append_char (pool, ' ');
		      mu_opool_appendz (pool, typeargv[i]);
		    }
		}
	      break;
	      
	    case 'F':
	      /* %f, and stdout is not redirected */
	      *flags |= MHN_STDIN;
	      /*FALLTHRU*/
	    case 'f':
	      mu_opool_appendz (pool, file);
	      break;
	      
	    case 's':
	      /* subtype */
	      mu_opool_appendz (pool, subtype);
	      break;
	      
	    default:
	      mu_opool_append_char (pool, *p);
	      p++;
	    }
	}
      else
	mu_opool_append_char (pool, *p);
    }
  mu_opool_append_char (pool, 0);

  free (type);
  free (subtype);
  mu_argcv_free (typeargc, typeargv);
    
  str = mu_opool_finish (pool, NULL);
  p = mu_str_skip_class (str, MU_CTYPE_SPACE);
  if (!*p)
    str = NULL;
  else
    str = mu_strdup (p);

  mu_opool_destroy (&pool);
  return (char*) str;
}

static void
mhn_tempfile_name (char **tempfile, const char *type, const char *subtype)
{
  struct mu_tempfile_hints hints;
  int flags = 0, rc;
  const char *s = _mhn_profile_get ("suffix", type, subtype, NULL);;
  
  if (s)
    {
      hints.suffix = (char*) s;
      flags |= MU_TEMPFILE_SUFFIX;
    }
  rc = mu_tempfile (&hints, flags, NULL, tempfile);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_tempfile", NULL, rc);
      exit (1);
    }
}

static int
check_type (const char *typeargs, const char *typeval)
{
  int rc = 1;
  
  if (typeargs)
    {
      int i, argc;
      char **argv;
		      
      split_args (typeargs, strlen (typeargs), &argc, &argv);
      for (i = 0; i < argc; i++)
	{
	  if (strlen (argv[i]) > 5
	      && memcmp (argv[i], "type=", 5) == 0
	      && mu_c_strcasecmp (argv[i] + 5, typeval) == 0)
	    {
	      rc = 0;
	      break;
	    }
	}
      mu_argcv_free (argc, argv);
    }
  return rc;
}

char *
mhn_show_command (mu_message_t msg, msg_part_t part, int *flags,
		  char **tempfile)
{
  const char *p, *str, *tmp;
  char *typestr, *type, *subtype, *typeargs;
  mu_opool_t pool;
  mu_header_t hdr;
  char *temp_cmd = NULL;
  int typeargc = 0;
  char **typeargv = NULL;
  
  mu_message_get_header (msg, &hdr);
  _get_content_type (hdr, &typestr, &typeargs);
  split_content (typestr, &type, &subtype);
  str = _mhn_profile_get ("show", type, subtype, NULL);
  if (!str)
    {
      if (mu_c_strcasecmp (typestr, "text/plain") == 0
	  /* FIXME: The following one should produce
	     %pshow -file '%F' */
	  || mu_c_strcasecmp (typestr, "message/rfc822") == 0)
	{
	  int rc;
	  const char *moreproc = mh_global_profile_get ("moreproc",
							getenv ("PAGER"));
	  if (!moreproc)
	    return NULL;
	  rc = mu_asprintf (&temp_cmd, "%%p%s '%%F'", moreproc);
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_asprintf", NULL, rc);
	      return NULL;
	    }
	  str = temp_cmd;
	}
      else if (mu_c_strcasecmp (typestr, "application/octet-stream") == 0 &&
	       check_type (typeargs, "tar") == 0)
	/* Use temporary file to give tar a chance to select appropriate
	   decompressor, if the archive is compressed. */
	str = "tar tvf '%F'";
      else
	{
	  char *parts = msg_part_format (part);
	  
	  /* FIXME: This message should in fact occupy two lines (exactly
	     as split below), but mu_error malfunctions if a \n appears
	     within the format string */
	  mu_error (_("don't know how to display content"
		      " (content %s in message %lu, part %s)"),
		    typestr, (unsigned long)part->part[0], parts);
	  free (parts);
	  return NULL;
	}
    }
  
  /* Expand macro-notations:
    %a  additional arguments
    %e  exclusive execution
    %f  filename containing content
    %F  %e, %f, and stdin is terminal not content
    %l  display listing prior to displaying content
    %p  %l, and ask for confirmation
    %s  subtype
    %d  content description */

  mu_opool_create (&pool, 1);

  p = mu_str_skip_class (str, MU_CTYPE_SPACE);
  
  if (*p == '|')
    p++;

  for ( ; *p; p++)
    {
      if (*p == '%')
	{
	  switch (*++p)
	    {
	    case 'a':
	      /* additional arguments */
	      if (typeargs)
		{
		  int i;
		  
		  if (!typeargv)
		    split_args (typeargs, strlen (typeargs),
				&typeargc, &typeargv);
		  for (i = 0; i < typeargc; i++)
		    {
		      if (i > 0)
			mu_opool_append_char (pool, ' ');
		      mu_opool_appendz (pool, typeargv[i]);
		    }
		}
	      break;
	      
	    case 'e':
	      /* exclusive execution */
	      *flags |= MHN_EXCLUSIVE_EXEC;
	      break;
	      
	    case 'f':
	      /* filename containing content */
	      if (!*tempfile)
		mhn_tempfile_name (tempfile, type, subtype);
	      mu_opool_appendz (pool, *tempfile);
	      break;
				     
	    case 'F':
	      /* %e, %f, and stdin is terminal not content */
	      *flags |= MHN_STDIN|MHN_EXCLUSIVE_EXEC;
	      if (!*tempfile)
		mhn_tempfile_name (tempfile, type, subtype);
	      mu_opool_appendz (pool, *tempfile);
	      break;
	      
	    case 'l':
	      /* display listing prior to displaying content */
	      *flags |= MHN_LISTING;
	      break;
	      
	    case 'p':
	      /* %l, and ask for confirmation */
	      *flags |= MHN_LISTING|MHN_PAUSE;
	      break;
	      
	    case 's':
	      /* subtype */
	      mu_opool_appendz (pool, subtype);
	      break;
	      
	    case 'd':
	      /* content description */
	      if (mu_header_sget_value (hdr, MU_HEADER_CONTENT_DESCRIPTION,
					&tmp) == 0)
		mu_opool_appendz (pool, tmp);
	      break;
	      
	    default:
	      mu_opool_append_char (pool, *p);
	      p++;
	    }
	}
      else
	mu_opool_append_char (pool, *p);
    }
  mu_opool_append_char (pool, 0);

  free (typestr);
  free (type);
  free (subtype);
  free (temp_cmd);
  mu_argcv_free (typeargc, typeargv);
  
  str = mu_opool_finish (pool, NULL);
  p = mu_str_skip_class (str, MU_CTYPE_SPACE);
  if (!*p)
    str = NULL;
  else
    str = mu_strdup (p);

  mu_opool_destroy (&pool);
  return (char*) str;
}

enum store_destination
  {
    store_to_folder,
    store_to_folder_msg,
    store_to_file,
    store_to_command,
    store_to_stdout
  };

enum store_destination
mhn_store_command (mu_message_t msg, msg_part_t part, const char *name,
		   char **return_string)
{
  const char *p, *str, *tmp;
  char *typestr, *type, *subtype, *typeargs;
  mu_opool_t pool;
  mu_header_t hdr;
  enum store_destination dest;
  
  mu_message_get_header (msg, &hdr);
  _get_content_type (hdr, &typestr, &typeargs);
  split_content (typestr, &type, &subtype);
  str = _mhn_profile_get ("store", type, subtype, NULL);

  if (!str)
    {
      if (mu_c_strcasecmp (type, "message") == 0)
	{
	  /* If the content is not application/octet-stream, then mhn
	     will check to see if the content is a message.  If so, mhn
	     will use the value "+".  */
	  *return_string = mu_strdup (mh_current_folder ());
	  return store_to_folder_msg;
	}
      else if (mu_c_strcasecmp (typestr, "application/octet-stream") == 0 &&
	       check_type (typeargs, "tar") == 0)
	/* [If the mhn-store component] isn't found, mhn will check to see if
	   the content is application/octet-stream with parameter "type=tar".
	   If so, mhn will choose an appropriate filename. */
	str = "%m%P.tar";
      else
	str = "%m%P.%s";
    }
  
  switch (str[0])
    {
    case '+':
      if (str[1])
	*return_string = mu_strdup (str);
      else
	*return_string = mu_strdup (mh_current_folder ());
      return store_to_folder;

    case '-':
      *return_string = NULL;
      return store_to_stdout;

    case '|':
      dest = store_to_command;
      str = mu_str_skip_class (str + 1, MU_CTYPE_SPACE);
      break;

    default:
      dest = store_to_file;
    }
  
  /* Expand macro-notations:
     %m  message number
     %P  .part
     %p  part
     %s  subtype */

  mu_opool_create (&pool, 1);
  
  for (p = str; *p; p++)
    {
      if (*p == '%')
	{
	  switch (*++p)
	    {
	    case 'a':
	      /* additional arguments */
	      mu_opool_appendz (pool, typeargs);
	      break;

	    case 'm':
	      if (name)
		mu_opool_appendz (pool, name);
	      else
		{
                  const char *buf = mu_umaxtostr (0, msg_part_subpart (part, 0));
		  mu_opool_appendz (pool, buf);
		}
	      break;

	    case 'P':
	      if (msg_part_level (part) >= 1)
		mu_opool_append_char (pool, '.');
	      /*FALLTHRU*/
	    case 'p':
	      if (msg_part_level (part) >= 1)
		msg_part_format_pool (pool, part);
	      break;
	      
	    case 's':
	      /* subtype */
	      mu_opool_appendz (pool, subtype);
	      break;
	      
	    case 'd':
	      /* content description */
	      if (mu_header_sget_value (hdr, MU_HEADER_CONTENT_DESCRIPTION,
				        &tmp) == 0)
		mu_opool_appendz (pool, tmp);
	      break;
	      
	    default:
	      mu_opool_append_char (pool, *p);
	      p++;
	    }
	}
      else
	mu_opool_append_char (pool, *p);
    }
  mu_opool_append_char (pool, *p);

  free (typestr);
  free (type);
  free (subtype);
  
  str = mu_opool_finish (pool, NULL);
  p = mu_str_skip_class (str, MU_CTYPE_SPACE);
  if (!*p)
    *return_string = NULL;
  else
    *return_string = mu_strdup (p);
  
  mu_opool_destroy (&pool);
  return dest;
}


/* ************************* Auxiliary functions ************************** */

int
_message_is_external_body (mu_message_t msg, char ***env)
{
  int rc;
  mu_header_t hdr;
  char *typestr, *argstr, *type, *subtype;

  if (mu_message_get_header (msg, &hdr))
    return 0;
  _get_content_type (hdr, &typestr, &argstr);
  split_content (typestr, &type, &subtype);
  rc = subtype && strcmp (subtype, "external-body") == 0;
  if (rc && env)
    {
      int c;
      split_args (argstr, strlen (argstr), &c, env);
    }

  free (typestr);
  free (type);
  free (subtype);
  return rc;
}

char *
_get_env (char **env, char *name)
{
  int nlen = strlen (name);
  for (; *env; env++)
    {
      int len = strlen (*env);
      if (nlen < len
	  && (*env)[len+1] == '='
	  && mu_c_strncasecmp (*env, name, nlen) == 0)
	return *env + len + 1;
    }
  return NULL;
}

void
_free_env (char **env)
{
  char **p;

  for (p = env; *p; p++)
    free (*p);
  free (env);
}

/* FIXME: Use mimehdr.c functions instead */
int
get_extbody_params (mu_message_t msg, char **content, char **descr)
{
  int rc = 0;
  mu_body_t body = NULL;
  mu_stream_t stream = NULL;
  char buf[128];
  size_t n;
	
  mu_message_get_body (msg, &body);
  mu_body_get_streamref (body, &stream);

  while (rc == 0
	 && mu_stream_readline (stream, buf, sizeof buf, &n) == 0
	 && n > 0)
    {
      char *p;
      int len = strlen (buf);

      if (len > 0 && buf[len-1] == '\n')
	buf[len-1] = 0;

      if (descr
	  && mu_c_strncasecmp (buf, MU_HEADER_CONTENT_DESCRIPTION ":",
			       sizeof (MU_HEADER_CONTENT_DESCRIPTION)) == 0)
	{
	  p = mu_str_skip_class (buf + sizeof (MU_HEADER_CONTENT_DESCRIPTION),
				 MU_CTYPE_SPACE);
	  *descr = mu_strdup (p);
	}
      else if (content
	       && mu_c_strncasecmp (buf, MU_HEADER_CONTENT_TYPE ":",
			            sizeof (MU_HEADER_CONTENT_TYPE)) == 0)
	{
	  char *q;
	  p = mu_str_skip_class (buf + sizeof (MU_HEADER_CONTENT_TYPE),
				 MU_CTYPE_SPACE);
	  q = strchr (p, ';');
	  if (q)
	    *q = 0;
	  *content = mu_strdup (p);
	}
    }
  mu_stream_destroy (&stream);
  return 0;
}


/* ************************** Message iterators *************************** */


typedef int (*msg_handler_t) (mu_message_t msg, msg_part_t part,
			      char *type, char *encoding,
			      void *data);

int
match_content (char *content)
{
  int rc;
  char *type, *subtype;

  if (!content_type && !content_subtype)
    return 0;
  if (!content)
    return 0;

  split_content (content, &type, &subtype);

  if ((rc = mu_c_strcasecmp (content_type, type)) == 0)
    {
      if (content_subtype)
	rc = mu_c_strcasecmp (content_subtype, subtype);
    }
  else 
    rc = mu_c_strcasecmp (content_type, subtype);

  free (type);
  free (subtype);
  return rc;
}

int
call_handler (mu_message_t msg, msg_part_t part, char *type, char *encoding,
	      msg_handler_t fun, void *data)
{
  if (req_part && msg_part_eq (req_part, part))
    return 0;
  if (match_content (type))
    return 0;
  return fun (msg, part, type, encoding, data);
}

int
handle_message (mu_message_t msg, msg_part_t part, msg_handler_t fun, void *data)
{
  mu_header_t hdr;
  char *type = NULL;
  char *encoding;
  int ismime = 0;
  
  mu_message_get_header (msg, &hdr);
  _get_content_type (hdr, &type, NULL);
  _get_content_encoding (hdr, &encoding);

  fun (msg, part, type, encoding, data);
  sfree (&type);
  sfree (&encoding);
  
  mu_message_is_multipart (msg, &ismime);
  if (ismime)
    {
      size_t i, nparts;

      mu_message_get_num_parts (msg, &nparts);

      msg_part_incr (part);
      for (i = 1; i <= nparts; i++)
	{
	  mu_message_t message = NULL;
	  
	  if (mu_message_get_part (msg, i, &message) == 0)
	    {
	      mu_message_get_header (message, &hdr);
	      _get_content_type (hdr, &type, NULL);
	      _get_content_encoding (hdr, &encoding);

	      msg_part_set_subpart (part, i);

	      if (mu_message_is_multipart (message, &ismime) == 0 && ismime)
		handle_message (message, part, fun, data);
	      else
		call_handler (message, part, type, encoding, fun, data);
	      sfree (&type);
	      sfree (&encoding);
	    }
	}
      msg_part_decr (part);
    }
  return 0;
}

int
mhn_message_size (mu_message_t msg, mu_off_t *psize)
{
  int rc;
  size_t size;
  mu_body_t body;
  
  *psize = 0;
  mu_message_get_body (msg, &body);
  if (mode_options & OPT_REALSIZE)
    {
      mu_stream_t dstr = NULL, bstr = NULL;

      if (mu_body_get_streamref (body, &bstr) == 0)
	{
	  mu_header_t hdr;
	  char *encoding;
	  
	  mu_message_get_header (msg, &hdr);
	  _get_content_encoding (hdr, &encoding);

	  rc = mu_filter_create (&dstr, bstr, encoding,
			 	 MU_FILTER_DECODE, 
				 MU_STREAM_READ);
	  free (encoding);
	  if (rc == 0)
	    {
	      mu_stream_stat_buffer stat;
	      mu_stream_t null;

	      mu_nullstream_create (&null, MU_STREAM_WRITE);
	      mu_stream_set_stat (null,
				  MU_STREAM_STAT_MASK (MU_STREAM_STAT_OUT),
				  stat);
	      mu_stream_copy (null, dstr, 0, NULL);
	      mu_stream_destroy (&null);
	      mu_stream_destroy (&dstr);
	      *psize = stat[MU_STREAM_STAT_OUT];
	      return 0;
	    }
	  mu_stream_destroy (&bstr);
	}
    }

  rc = mu_body_size (body, &size);
  if (rc == 0)
    *psize = size;
  return rc;
}


/* ***************************** List Mode ******************************* */

int
list_handler (mu_message_t msg, msg_part_t part, char *type, char *encoding,
	      void *data)
{
  mu_off_t size;
  mu_header_t hdr;
  
  if (msg_part_level (part) == 0)
    printf ("%4lu      ", (unsigned long) msg_part_subpart (part, 0));
  else
    {
      printf ("     ");
      msg_part_print (part, 4);
      putchar (' ');
    }
  printf ("%-25s", type);
  
  mhn_message_size (msg, &size);
  if (size < 1024)
    printf (" %4lu", (unsigned long) size);
  else if (size < 1024*1024)
    printf ("%4luK", (unsigned long) (size + 1024 - 1) / 1024);
  else
    printf ("%4luM", (unsigned long) (size + 1024*1024 - 1) / 1024 / 1024);
  
  if (mu_message_get_header (msg, &hdr) == 0)
    {
      const char *descr;
      if (mu_header_sget_value (hdr, "Content-Description", &descr) == 0)
	printf (" %s", descr);
    }
  
  printf ("\n");

  if (_message_is_external_body (msg, NULL))
    {
      char *content_type = NULL;
      char *content_descr = NULL;

      get_extbody_params (msg, &content_type, &content_descr);
	
      printf ("          ");
      printf ("%-25s", content_type ? content_type : "");
      if (content_descr)
	printf ("       %s", content_descr);
      printf ("\n");
      
      free (content_type);
      free (content_descr);
    }
  
  return 0;
}

int
list_message (mu_message_t msg)
{
  size_t uid;
  msg_part_t part;

  mh_message_number (msg, &uid);
  part = msg_part_create (uid);
  handle_message (msg, part, list_handler, NULL);
  msg_part_destroy (part);
  return 0;
}

int
list_iterator (size_t num, mu_message_t msg, void *data)
{
  return list_message (msg);
}

int
mhn_list ()
{
  int rc; 

  if (mode_options & OPT_HEADERS)
    printf (_(" msg part type/subtype              size  description\n"));

  if (message)
    rc = list_message (message);
  else
    rc = mu_msgset_foreach_message (msgset, list_iterator, NULL);
  return rc;
}


/* ***************************** Show Mode ******************************* */

static mu_list_t mhl_format;

int
show_internal (mu_message_t msg, msg_part_t part, char *encoding,
	       mu_stream_t out)
{
  int rc;
  mu_body_t body = NULL;
  mu_stream_t dstr, bstr;

  if ((rc = mu_message_get_body (msg, &body)))
    {
      mu_error (_("%s: cannot get message body: %s"),
		mu_umaxtostr (0, msg_part_subpart (part, 0)),
		mu_strerror (rc));
      return 0;
    }
  mu_body_get_streamref (body, &bstr);
  rc = mu_filter_create (&dstr, bstr, encoding,
		         MU_FILTER_DECODE, MU_STREAM_READ);
  if (rc == 0)
    bstr = dstr;
  rc = mu_stream_copy (out, bstr, 0, NULL);
  mu_stream_destroy (&bstr);
  return rc;
}
  
int
mhn_exec (mu_stream_t *str, const char *cmd, int flags)
{
  int rc = mu_command_stream_create (str, cmd, MU_STREAM_WRITE);
  if (rc)
    {
      mu_error (_("cannot create proc stream (command %s): %s"),
		cmd, mu_strerror (rc));
    }
  return rc;
}

int
exec_internal (mu_message_t msg, msg_part_t part, char *encoding,
	       const char *cmd, int flags)
{
  int rc;
  mu_stream_t tmp;

  if ((rc = mhn_exec (&tmp, cmd, flags)))
    return rc;
  show_internal (msg, part, encoding, tmp);      
  mu_stream_destroy (&tmp);
  return rc;
}

/* FIXME: stdin is always opened when invoking subprocess, no matter
   what the MHN_STDIN bit is */
   
int 
mhn_run_command (mu_message_t msg, msg_part_t part,
		 char *encoding,
		 char *cmd, int flags,
		 char *tempfile)
{
  int rc = 0;
  
  if (tempfile)
    {
      /* pass content via a tempfile */
      int status;
      mu_stream_t tmp;
      struct mu_wordsplit ws;

      ws.ws_comment = "#";
      if (mu_wordsplit (cmd, &ws, MU_WRDSF_DEFFLAGS | MU_WRDSF_COMMENT))
	{
	  mu_error (_("cannot parse command line `%s': %s"), cmd,
		    mu_wordsplit_strerror (&ws));
	  return ENOSYS;
	}

      rc = mu_file_stream_create (&tmp, tempfile,
				  MU_STREAM_RDWR|MU_STREAM_CREAT);
      if (rc)
	{
	  mu_error (_("cannot create temporary stream (file %s): %s"),
		    tempfile, mu_strerror (rc));
	  mu_wordsplit_free (&ws);
	  return rc;
	}
      show_internal (msg, part, encoding, tmp);      
      mu_stream_destroy (&tmp);
      rc = mu_spawnvp (ws.ws_wordv[0], ws.ws_wordv, &status);
      if (status)
	rc = status;
      mu_wordsplit_free (&ws);
    }
  else
    rc = exec_internal (msg, part, encoding, cmd, flags);

  return rc;
}

static RETSIGTYPE
sigint (int sig)
{
  /* nothing */
}

static int
mhn_pause ()
{
  char c;
  int rc = 0;
  RETSIGTYPE (*saved_sig) (int) = signal (SIGINT, sigint);

  printf (_("Press <return> to show content..."));
  fflush (stdout);
  do
    {
      fd_set set;
      int res;
      
      FD_ZERO (&set);
      FD_SET (0, &set);
      res = select (1, &set, NULL, NULL, NULL);
      if (res != 1
	  || read (0, &c, 1) != 1)
	{
	  putchar ('\n');
	  rc = 1;
	  break;
	}
    }
  while (c != '\n');
  signal (SIGINT, saved_sig);
  return rc;
}  

int
show_handler (mu_message_t msg, msg_part_t part, char *type, char *encoding,
	      void *data)
{
  mu_stream_t out = data;
  char *cmd;
  int flags = 0;
  char *tempfile = NULL;
  int ismime;
  
  if (mu_message_is_multipart (msg, &ismime) == 0 && ismime)
    return 0;

  cmd = mhn_show_command (msg, part, &flags, &tempfile);
  if (!cmd)
    return 0;
  if (pause_option == 0)
    flags &= ~MHN_PAUSE;
  else if (pause_option > 0)
    flags |= MHN_PAUSE;
  
  if (flags & MHN_LISTING)
    {
      mu_header_t hdr = NULL;
      char *parts;
      const char *descr;
      mu_off_t size = 0;

      mhn_message_size (msg, &size);
      parts = msg_part_format (part);
      mu_stream_printf (out, _("part %5s %-24.24s %lu"), parts, type, 
			(unsigned long) size);
      free (parts);
      if (mu_message_get_header (msg, &hdr) == 0 &&
	  mu_header_sget_value (hdr, MU_HEADER_CONTENT_DESCRIPTION,
				&descr) == 0)
	mu_stream_printf (out, " %s", descr);
      mu_stream_write (out, "\n", 1, NULL);
      mu_stream_flush (out);
    }

  if (!((flags & MHN_PAUSE) && mhn_pause ()))
    mhn_run_command (msg, part, encoding, cmd, flags, tempfile);
  free (cmd);
  if (tempfile)
    {
      unlink (tempfile);
      free (tempfile);
    }
  
  return 0;
}

int
show_message (mu_message_t msg, size_t num, void *data)
{
  msg_part_t part = msg_part_create (num);

  if (mhl_format)
    mhl_format_run (mhl_format, width, 0, MHL_DISABLE_BODY, msg,
		    (mu_stream_t) data);

  handle_message (msg, part, show_handler, data);
  msg_part_destroy (part);
  return 0;
}

int
show_iterator (size_t num, mu_message_t msg, void *data)
{
  msg_part_t part;
  
  mh_message_number (msg, &num);
  part = msg_part_create (num);
  show_message (msg, num, data);
  msg_part_destroy (part);
  return 0;
}

int
mhn_show ()
{
  
  int rc;
  mhl_format = mhl_format_compile (formfile);

  if (message)
    rc = show_message (message, 0, mu_strout);
  else
    rc = mu_msgset_foreach_message (msgset, show_iterator, mu_strout);
  mu_stream_flush (mu_strout);
  return rc;
}

/* ***************************** Store Mode ****************************** */

char *
normalize_path (const char *cwd, char *path)
{
  size_t len;
  char *pcwd = NULL;
  
  if (!path)
    return path;

  if (!cwd)
    cwd = pcwd = mu_getcwd ();

  path = mu_normalize_path (mh_safe_make_file_name (cwd, path));

  len = strlen (cwd);
  if (strlen (path) < len || memcmp (path, cwd, len))
    sfree (&path);
  free (pcwd);
  return path;
}

int
store_handler (mu_message_t msg, msg_part_t part, char *type, char *encoding,
	       void *data)
{
  const char *prefix = data;
  char *name = NULL;
  char *partstr;
  int ismime;
  int rc;
  mu_stream_t out;
  const char *dir = mh_global_profile_get ("mhn-storage", NULL);
  enum store_destination dest = store_to_file;
  
  if (mu_message_is_multipart (msg, &ismime) == 0 && ismime)
    return 0;
  
  if (mode_options & OPT_AUTO)
    {
      char *val;
      int rc = mu_message_aget_decoded_attachment_name (msg, charset,
							&val, NULL);
      if (rc == 0)
	{
	  name = normalize_path (dir, val);
	  free (val);
	  dest = store_to_file;
	}
      else if (rc != MU_ERR_NOENT)
	{
	  char *pstr = msg_part_format (part);
	  mu_diag_output (MU_DIAG_WARNING,
			  _("%s: cannot decode attachment name: %s"),
			  pstr, mu_strerror (rc));
	  free (pstr);
	}
    }
  
  if (!name)
    dest = mhn_store_command (msg, part, prefix, &name);

  partstr = msg_part_format (part);
  /* Set prefix for diagnostic purposes */
  if (!prefix)
    prefix = mu_umaxtostr (0, msg_part_subpart (part, 0));
  out = NULL;
  rc = 0;
  switch (dest)
    {
    case store_to_folder_msg:
    case store_to_folder:
      {
	mu_mailbox_t mbox = mh_open_folder (name, 
                                            MU_STREAM_RDWR|MU_STREAM_CREAT);
	size_t uid;

	mu_mailbox_uidnext (mbox, &uid);
	printf (_("storing message %s part %s to folder %s as message %lu\n"),
		prefix, partstr, name, (unsigned long) uid);
	if (dest == store_to_folder_msg)
	  {
	    mu_body_t body;
	    mu_stream_t str;
	    mu_message_t tmpmsg;
	    
	    rc = mu_message_get_body (msg, &body);
	    if (rc)
	      {
		mu_diag_funcall (MU_DIAG_ERROR, "mu_message_get_body",
				 NULL, rc);
		break;
	      }
	    rc = mu_body_get_streamref (body, &str);
	    if (rc)
	      {
		mu_diag_funcall (MU_DIAG_ERROR, "mu_body_get_stream",
				 NULL, rc);
		break;
	      }
	    rc = mu_stream_to_message (str, &tmpmsg);
	    mu_stream_unref (str);
	    if (rc)
	      {
		mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_to_message",
				 NULL, rc);
		break;
	      }
	    
	    rc = mu_mailbox_append_message (mbox, tmpmsg);
	    mu_message_destroy (&tmpmsg, mu_message_get_owner (tmpmsg));
	  }
	else
	  rc = mu_mailbox_append_message (mbox, msg);
	
	if (rc)
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_append_message", NULL,
			   rc);
	mu_mailbox_close (mbox);
	mu_mailbox_destroy (&mbox);
      }
      break;

    case store_to_file:
      if (dir && name[0] != '/')
	{
	  char *s = mu_make_file_name (dir, name);
	  if (!s)
	    {
	      rc = ENOMEM;
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_make_file_name", NULL, rc);
	      break;
	    }
	  free (name);
	  name = s;
	}
      printf (_("storing message %s part %s as file %s\n"),
	      prefix, partstr, name);

      if (!(mode_options & OPT_QUIET) && access (name, R_OK) == 0)
	{
	  int ok;
	  
	  ok = mh_getyn (_("File %s already exists. Rewrite"), name);
	  if (!ok)
	    {
	      free (name);
	      return 0;
	    }
	  unlink (name);
	}
  
      rc = mu_file_stream_create (&out, name, MU_STREAM_WRITE|MU_STREAM_CREAT);
      if (rc)
	mu_error (_("cannot create output stream (file %s): %s"),
		  name, mu_strerror (rc));
      break;

    case store_to_command:
      {
	struct mu_prog_hints hints;
	struct mu_wordsplit ws;
      
	hints.mu_prog_workdir = mu_get_homedir ();
	ws.ws_comment = "#";
	if (mu_wordsplit (name, &ws, MU_WRDSF_DEFFLAGS|MU_WRDSF_COMMENT))
	  {
	    mu_error (_("cannot split line `%s': %s"), name,
		      mu_wordsplit_strerror (&ws));
	    break;
	  }
	
	printf (_("storing msg %s part %s using command (cd %s; %s)\n"),
		prefix, partstr, hints.mu_prog_workdir, name);
	rc = mu_prog_stream_create (&out,
				    ws.ws_wordv[0],
				    ws.ws_wordc, ws.ws_wordv,
				    MU_PROG_HINT_WORKDIR,
				    &hints, MU_STREAM_WRITE);
	mu_wordsplit_free (&ws);
	if (rc)
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_prog_stream_create", NULL, rc);
      }
      break;
      
    case store_to_stdout:
      printf (_("storing msg %s part %s to stdout\n"),
	      prefix, partstr);
      rc = 0;
      out = mu_strout;
      mu_stream_ref (out);
      break;
    }
			   
  if (out)
    {
      show_internal (msg, part, encoding, out);
      mu_stream_flush (out);
      mu_stream_destroy (&out);
    }
      
  free (name);
  free (partstr);
  
  return rc;
}

void
store_message (mu_message_t msg, void *data)
{
  size_t uid;
  msg_part_t part;

  mh_message_number (msg, &uid);
  part = msg_part_create (uid);
  handle_message (msg, part, store_handler, data);
  msg_part_destroy (part);
}

int
store_iterator (size_t num, mu_message_t msg, void *data)
{
  store_message (msg, data);
  return 0;
}

int
mhn_store ()
{
  int rc = 0;
  
  if (message)
    {
      char *p = strrchr (input_file, '/');
      if (p)
	p++;
      else
	p = input_file;
      store_message (message, p);
    }
  else
    rc = mu_msgset_foreach_message (msgset, store_iterator, NULL);
  return rc;
}


/* ***************************** Compose Mode **************************** */

struct compose_env
{
  mu_stream_t input;
  mu_mime_t mime;
  size_t line;
  int subpart;
};

size_t
mhn_error_loc (struct compose_env *env)
{
  mu_header_t hdr = NULL;
  size_t n = 0;
  mu_message_get_header (message, &hdr);
  mu_header_lines (hdr, &n);
  return n + 1 + env->line;
}

static int
parse_brace (char **pval, char **cmd, int c, struct compose_env *env)
{
  char *val;
  int len;
  char *rest = *cmd;
  char *sp = strchr (rest, c);
  
  if (!sp)
    {
      mu_error (_("%s:%lu: missing %c"),
		input_file,
		(unsigned long) mhn_error_loc (env),
		c);
      return 1;
    }
  len = sp - rest;
  val = mu_alloc (len + 1);
  memcpy (val, rest, len);
  val[len] = 0;
  *cmd = sp + 1;
  *pval = val;
  return 0;
}

#define isdelim(c) (mu_isspace (c) || strchr (";<[(", c))
#define skipws(ptr) (ptr) = mu_str_skip_class (ptr, MU_CTYPE_SPACE)

int
parse_content_type (struct compose_env *env,
		    mu_opool_t pool, char **prest, char **id, char **descr)
{
  int status = 0, stop = 0;
  char *rest = *prest;
  char *sp;
  char *comment = NULL;

  while (stop == 0 && status == 0 && *rest)
    {
      skipws (rest);
      switch (*rest++)
	{
	case '(':
	  if (comment)
	    {
	      mu_error (_("%s:%lu: comment redefined"),
			input_file,
			(unsigned long) mhn_error_loc (env));
	      status = 1;
	      break;
	    }
	  status = parse_brace (&comment, &rest, ')', env);
	  break;

	case '[':
	  if (!descr)
	    {
	      mu_error (_("%s:%lu: syntax error"),
			input_file,
			(unsigned long) mhn_error_loc (env));
	      status = 1;
	      break;
	    }
	    
	  if (*descr)
	    {
	      mu_error (_("%s:%lu: description redefined"),
			input_file,
			(unsigned long) mhn_error_loc (env));
	      status = 1;
	      break;
	    }
	  status = parse_brace (descr, &rest, ']', env);
	  break;
	  
	case '<':
	  if (*id)
	    {
	      mu_error (_("%s:%lu: content id redefined"),
			input_file,
			(unsigned long) mhn_error_loc (env));
	      status = 1;
	      break;
	    }
	  status = parse_brace (id, &rest, '>', env);
	  break;

	case ';':
	  mu_opool_append_char (pool, ';');
	  mu_opool_append_char (pool, ' ');
	  skipws (rest);
	  sp = rest;
	  for (; *rest && !mu_isspace (*rest) && *rest != '='; rest++)
	    mu_opool_append_char (pool, *rest);
	  skipws (rest);
	  if (*rest != '=')
	    {
	      mu_error (_("%s:%lu: syntax error"),
			input_file,
			(unsigned long) mhn_error_loc (env));
	      status = 1;
	      break;
	    }
	  rest++;
	  mu_opool_append_char (pool, '=');
	  skipws (rest);
	  for (; *rest; rest++)
	    {
	      if (isdelim (*rest))
		break;
	      mu_opool_append_char (pool, *rest);
	    }
	  break;
	    
	default:
	  rest--;
	  stop = 1;
	  break;
	}
    }

  if (comment)
    {
      mu_opool_append (pool, " (", 2);
      mu_opool_appendz (pool, comment);
      mu_opool_append_char (pool, ')');
      free (comment);
    }
  *prest = rest;
  return status;
}

/* cmd is:  type "/" subtype
            0*(";" attribute "=" value)
            [ "(" comment ")" ]
            [ "<" id ">" ]
            [ "[" description "]" ]
*/
int
parse_type_command (char **pcmd, struct compose_env *env, mu_header_t hdr)
{
  int status = 0;
  char *sp, c;
  char *type = NULL;
  char *subtype = NULL;
  char *descr = NULL, *id = NULL;
  mu_opool_t pool;
  char *rest = *pcmd;
  
  skipws (rest);
  for (sp = rest; *sp && !isdelim (*sp); sp++)
    ;
  c = *sp;
  *sp = 0;
  split_content (rest, &type, &subtype);
  *sp = c;
  rest = sp;
  
  if (!subtype)
    {
      mu_error (_("%s:%lu: missing subtype"),
		input_file,
		(unsigned long) mhn_error_loc (env));
      return 1;
    }

  mu_opool_create (&pool, 1);

  mu_opool_appendz (pool, type);
  mu_opool_append_char (pool, '/');
  mu_opool_appendz (pool, subtype);
  status = parse_content_type (env, pool, &rest, &id, &descr);
  mu_opool_append_char (pool, 0);
  
  mu_header_set_value (hdr, MU_HEADER_CONTENT_TYPE,
		       mu_opool_finish (pool, NULL), 1);
  mu_opool_destroy (&pool);

  if (!id)
    id = mh_create_message_id (env->subpart);

  mu_header_set_value (hdr, MU_HEADER_CONTENT_ID, id, 1);
  free (id);

  if (descr)
    {
      mu_header_set_value (hdr, MU_HEADER_CONTENT_DESCRIPTION, descr, 1);
      free (descr);
    }
      
  *pcmd = rest;
  return status;
}

void
copy_header (mu_message_t msg, mu_header_t out)
{
  size_t i, count;
  mu_header_t hdr;

  mu_message_get_header (msg, &hdr);
  mu_header_get_field_count (hdr, &count);
  for (i = 1; i <= count; i++)
    {
      const char *name, *value;

      if (mu_header_sget_field_name (hdr, i, &name) ||
	  mu_header_sget_field_value (hdr, i, &value))
	continue;

      mu_header_set_value (out, name, value, 0);
    }
}

void
copy_header_to_stream (mu_message_t msg, mu_stream_t stream)
{
  int rc;
  mu_header_t hdr;
  mu_stream_t flt, str;

  mu_message_get_header (msg, &hdr);
  mu_header_get_streamref (hdr, &str);
  rc = mu_filter_create (&flt, str, "HEADER",
			 MU_FILTER_DECODE, MU_STREAM_READ);
  mu_stream_unref (str);
  if (rc)
    {
      mu_error (_("cannot open filter stream: %s"), mu_strerror (rc));
      exit (1);
    }
  rc = mu_stream_copy (stream, flt, 0, NULL);
  if (rc)
    {
      mu_error (_("error reading headers: %s"), mu_strerror (rc));
      exit (1);
    }
  mu_stream_destroy (&flt);
}

void
finish_msg (struct compose_env *env, mu_message_t *msg)
{
  mu_header_t hdr;

  if (!msg || !*msg)
    return;
  mu_message_get_header (*msg, &hdr);
  if (mu_header_get_value (hdr, MU_HEADER_CONTENT_TYPE, NULL, 0, NULL))
    mu_header_set_value (hdr, MU_HEADER_CONTENT_TYPE, "text/plain", 1);
  if (mu_header_get_value (hdr, MU_HEADER_CONTENT_ID, NULL, 0, NULL))
    {
      char *p = mh_create_message_id (env->subpart);
      mu_header_set_value (hdr, MU_HEADER_CONTENT_ID, p, 1);
      free (p);
    }
  mu_mime_add_part (env->mime, *msg);
  *msg = NULL;
}

void
finish_text_msg (struct compose_env *env, mu_message_t *msg, int ascii)
{
  if (!ascii)
    {
      int rc;
      mu_message_t newmsg;
      mu_header_t hdr;
      mu_body_t body;
      mu_stream_t input, output, fstr;
      
      mu_message_create (&newmsg, NULL);
      mu_message_get_header (newmsg, &hdr);
      copy_header (*msg, hdr);
      mu_header_set_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING,
			   "quoted-printable", 0);
      
      mu_message_get_body (newmsg, &body);
      mu_body_get_streamref (body, &output);

      mu_message_get_body (*msg, &body);
      mu_body_get_streamref (body, &input);
      rc = mu_filter_create (&fstr, input, "quoted-printable",
			     MU_FILTER_ENCODE, 
			     MU_STREAM_READ);
      if (rc == 0)
	{
	  rc = mu_stream_copy (output, fstr, 0, NULL);
	  mu_stream_destroy (&fstr);
	  mu_message_unref (*msg);
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_copy", NULL, rc);
	      exit (1);
	    }
	  *msg = newmsg;
	}
      else
	mu_message_destroy (&newmsg, NULL);
      mu_stream_destroy (&input);
      mu_stream_destroy (&output);
    }
  finish_msg (env, msg);
}

#define EXTCONTENT "message/external-body"

int
edit_extern (char *cmd, struct compose_env *env, mu_message_t *msg, int level)
{
  int rc;
  char *rest;
  char *id = NULL;
  mu_header_t hdr, hdr2;
  mu_body_t body;
  mu_stream_t in, out = NULL;
  mu_opool_t pool;

  if (!*msg)
    mu_message_create (msg, NULL);
  
  if ((rc = mu_header_create (&hdr2, NULL, 0)) != 0)
    {
      mu_error (_("cannot create header: %s"),
		mu_strerror (rc));
      return 1;
    }

  rest = cmd;
  rc = parse_type_command (&rest, env, hdr2);

  mu_message_get_header (*msg, &hdr);

  mu_opool_create (&pool, 1);
  mu_opool_append (pool, EXTCONTENT, sizeof (EXTCONTENT) - 1);
  *--rest = ';'; /* FIXME */
  rc = parse_content_type (env, pool, &rest, &id, NULL);
  mu_opool_append_char (pool, 0);
  mu_header_set_value (hdr, MU_HEADER_CONTENT_TYPE,
		       mu_opool_finish (pool, NULL), 1);
  mu_opool_destroy (&pool);
  if (rc)
    return 1;

  mu_message_get_body (*msg, &body);
  mu_body_get_streamref (body, &out);

  if (!id)
    id = mh_create_message_id (env->subpart);
  mu_header_set_value (hdr2, MU_HEADER_CONTENT_ID, id, 1);
  free (id);

  mu_header_get_streamref (hdr2, &in);
  mu_stream_copy (out, in, 0, NULL);
  mu_stream_destroy (&in);
  mu_stream_close (out);
  mu_stream_destroy (&out);
  
  mu_header_destroy (&hdr2);

  finish_msg (env, msg);
  return 0;
}

int
edit_forw (char *cmd, struct compose_env *env, mu_message_t *pmsg, int level)
{
  char *id = NULL, *descr = NULL;
  int stop = 0, status = 0;
  size_t i, npart;
  struct mu_wordsplit ws;
  mu_header_t hdr;
  mu_mime_t mime;
  mu_message_t msg;
  const char *val;
  
  skipws (cmd);
  while (stop == 0 && status == 0 && *cmd)
    {
      switch (*cmd++)
	{
	case '[':
	  if (descr)
	    {
	      mu_error (_("%s:%lu: description redefined"),
			input_file,
			(unsigned long) mhn_error_loc (env));
	      status = 1;
	      break;
	    }
	  status = parse_brace (&descr, &cmd, ']', env);
	  break;
	  
	case '<':
	  if (id)
	    {
	      mu_error (_("%s:%lu: content id redefined"),
			input_file,
			(unsigned long) mhn_error_loc (env));
	      status = 1;
	      break;
	    }
	  status = parse_brace (&id, &cmd, '>', env);
	  break;

	default:
	  cmd--;
	  stop = 1;
	  break;
	}
      skipws (cmd);
    }
  if (status)
    return status;

  if (mu_wordsplit (cmd, &ws, MU_WRDSF_DEFFLAGS))
    {
      mu_error (_("%s:%lu: cannot split line: %s"),
		input_file,
		(unsigned long) mhn_error_loc (env),
		mu_wordsplit_strerror (&ws));
      return 1;
    }

  mu_mime_create (&mime, NULL, 0);

  if (ws.ws_wordv[0][0] == '+')
    {
      mbox = mh_open_folder (ws.ws_wordv[0], MU_STREAM_READ);
      i = 1;
    }
  else
    {
      mbox = mh_open_folder (mh_current_folder (), MU_STREAM_READ);
      i = 0;
    }
  
  for (npart = 1; i < ws.ws_wordc; i++, npart++)
    {
      mu_message_t input_msg, newmsg;
      mu_body_t body;
      mu_stream_t input_str, bstr;
      char *endp;
      size_t n = strtoul (ws.ws_wordv[i], &endp, 10);

      if (*endp)
	{
	  mu_error (_("%s:%lu: malformed directive near %s"),
		    input_file,
		    (unsigned long) mhn_error_loc (env),
		    endp);
	  return 1;
	}
		    
      if (mh_get_message (mbox, n, &input_msg) == 0)
	{
	  mu_error (_("%s:%lu: no such message: %lu"),
		    input_file,
		    (unsigned long) mhn_error_loc (env),
		    (unsigned long)i);
	  return 1;
	}

      status = mu_message_get_streamref (input_msg, &input_str);
      if (status)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_message_get_streamref", NULL,
			   status);
	  exit (1);
	}
	
      status = mu_message_create (&newmsg, NULL);
      if (status)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_message_create", NULL, status);
	  exit (1);
	}
      status = mu_message_get_body (newmsg, &body);
      if (status)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_message_get_body", NULL,
			   status);
	  exit (1);
	}
      status = mu_body_get_streamref (body, &bstr);
      if (status)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_body_get_streamref", NULL,
			   status);
	  exit (1);
	}
      status = mu_stream_copy (bstr, input_str, 0, NULL);
      if (status)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_copy", NULL,
			   status);
	  exit (1);
	}
      mu_stream_unref (bstr);
      mu_stream_unref (input_str);

      mu_message_get_header (newmsg, &hdr);
      mu_header_set_value (hdr, MU_HEADER_CONTENT_TYPE, "message/rfc822", 1);
      mu_mime_add_part (mime, newmsg);
    }
  mu_wordsplit_free (&ws);

  if (*pmsg)
    {
      mu_message_unref (*pmsg);
      *pmsg = NULL;
    }

  mu_mime_get_message (mime, &msg);
  mu_message_unref (msg);
  mu_message_get_header (msg, &hdr);
  
  if (npart > 2)
    {
      char *newval, *sp;
      
      mu_header_sget_value (hdr, MU_HEADER_CONTENT_TYPE, &val);
      sp = strchr (val, ';');
      if (sp)
	{
	  mu_asprintf (&newval, "multipart/digest%s", sp);
	  mu_header_set_value (hdr, MU_HEADER_CONTENT_TYPE, newval, 1);
	  free (newval);
	}
    }

  if (!id)
    id = mh_create_message_id (env->subpart);

  mu_header_set_value (hdr, MU_HEADER_CONTENT_ID, id, 1);
  free (id);

  if (descr)
    {
      mu_header_set_value (hdr, MU_HEADER_CONTENT_DESCRIPTION, descr, 1);
      free (descr);
    }
  
  finish_msg (env, &msg);
  return status;
}

int
edit_modify (char *cmd, struct compose_env *env, mu_message_t *msg)
{
  mu_header_t hdr;
  if (!*msg)
    mu_message_create (msg, NULL);
  mu_message_get_header (*msg, &hdr);
  return parse_type_command (&cmd, env, hdr);
}

int
edit_mime (char *cmd, struct compose_env *env, mu_message_t *msg, int level)
{
  int rc;
  mu_header_t hdr;
  mu_body_t body;
  mu_stream_t in, out = NULL, fstr;
  char *encoding;
  char *typestr, *typeargs;
  char *shell_cmd;
  int flags;
  
  if (!*msg)
    mu_message_create (msg, NULL);
  mu_message_get_header (*msg, &hdr);
  rc = parse_type_command (&cmd, env, hdr);
  if (rc)
    return 1;

  mu_rtrim_class (cmd, MU_CTYPE_SPACE);

  _get_content_type (hdr, &typestr, &typeargs);
  shell_cmd = mhn_compose_command (typestr, typeargs, &flags, cmd);
  free (typestr);

  /* Open the input stream, whatever it is */
  if (shell_cmd)
    {
      if (mhn_exec (&in, cmd, flags))
	return 1;
    }
  else if (cmd[0] == 0)
    {
      mu_error (_("%s:%lu: missing filename"),
		input_file,
		(unsigned long) mhn_error_loc (env));
      finish_msg (env, msg);
      return 1;
    }
  else
    {
      rc = mu_file_stream_create (&in, cmd, MU_STREAM_READ);
      if (rc)
	{
	  mu_error (_("cannot create input stream (file %s): %s"),
		    cmd, mu_strerror (rc));
	  return rc;
	}
    }
  
  /* Create filter */

  if (mu_header_aget_value_unfold (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING,
				   &encoding))
    {
      char *typestr, *type, *subtype;
      
      _get_content_type (hdr, &typestr, NULL);
      split_content (typestr, &type, &subtype);
      if (mu_c_strcasecmp (type, "message") == 0)
	encoding = mu_strdup ("7bit");
      else if (mu_c_strcasecmp (type, "text") == 0)
	encoding = mu_strdup ("quoted-printable");
      else
	encoding = mu_strdup ("base64");
      mu_header_set_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING, encoding, 1);
      free (typestr);
      free (type);
      free (subtype);
    }

  rc = mu_filter_create (&fstr, in, encoding, MU_FILTER_ENCODE, MU_STREAM_READ);
  if (rc)
    {
      fstr = in;
      mu_stream_ref (in);
    }
  mu_stream_unref (in);
  free (encoding);

  mu_message_get_body (*msg, &body);
  mu_body_get_streamref (body, &out);
  rc = mu_stream_copy (out, fstr, 0, NULL);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_copy", NULL, rc);
      exit (1);
    }
  
  mu_stream_close (out);
  mu_stream_destroy (&out);
  mu_stream_destroy (&fstr);
  finish_msg (env, msg);
  return rc;
}

static int
has_nonascii (const char *buf, size_t n)
{
  size_t i;
  for (i = 0; i < n; i++)
    if (!mu_isascii (buf[i]))
      return 1;
  return 0;
}

int
mhn_edit (struct compose_env *env, int level)
{
  int status = 0;
  char *buf = NULL;
  size_t bufsize = 0, n;
  mu_stream_t output = NULL;
  mu_message_t msg = NULL;
  size_t line_count = 0;
  int ascii_buf;
  
  while (status == 0
	 && mu_stream_getline (env->input, &buf, &bufsize, &n) == 0 && n > 0)
    {
      env->line++;
      if (!msg)
	{
	  mu_header_t hdr;

	  /* Destroy old stream */
	  mu_stream_destroy (&output);
	  
	  /* Create new message */
	  mu_message_create (&msg, NULL);
	  mu_message_get_header (msg, &hdr);
	}
      if (!output)
	{
	  mu_body_t body;

	  mu_message_get_body (msg, &body);
	  mu_body_get_streamref (body, &output);
	  mu_message_ref (msg);
	  line_count = 0;
	  ascii_buf = 1; /* Suppose it is ascii */
	  env->subpart++;
	}
      
      if (buf[0] == '#')
	{
	  if (buf[1] == '#')
	    {
	      mu_stream_write (output, buf+1, n-1, NULL);
	      line_count++;
	    }
	  else
	    {
	      char *b2 = NULL;
	      size_t bs = 0, n2;
	      char *tok, *sp, c;

	      /* Collect the whole line */
	      while (n > 2 && buf[n-2] == '\\')
		{
		  int rc = mu_stream_getline (env->input, &b2, &bs, &n2);
		  env->line++;
		  if (rc == 0 && n2 > 0)
		    {
		      if (n + n2 > bufsize)
			{
			  bufsize += 128;
			  buf = mu_realloc (buf, bufsize);
			}
		      memcpy (buf + n - 2, b2, n2);
		      n += n2 - 2;
		    }
		}
	      free (b2);

	      
	      if (line_count)
		{
		  /* Close and append the previous part */
		  mu_stream_close (output);
		  mu_stream_destroy (&output);
		  mu_message_unref (msg);
		  finish_text_msg (env, &msg, ascii_buf);
		}
	      
	      mu_rtrim_cset (buf, "\n");
	      
	      /* Execute the directive */
	      tok = buf;
	      sp = mu_str_skip_class_comp (buf, MU_CTYPE_SPACE);
	      c = *sp;
	      *sp = 0;

	      if (tok[1] == 0)
		/* starts a new message */;
	      else if (tok[1] == '<')
		{
		  *sp = c;
		  status = edit_modify (tok+2, env, &msg);
		}
	      else if (tok[1] == '@')
		{
		  *sp = c;
		  status = edit_extern (tok + 2, env, &msg, level);
		}
	      else if (strcmp (tok, "#forw") == 0)
		{
		  *sp = c;
		  status = edit_forw (sp, env, &msg, level);
		}
	      else if (strcmp (tok, "#begin") == 0)
		{
		  struct compose_env new_env;
		  mu_message_t new_msg;
		  
		  new_env.input = env->input;
		  new_env.line = env->line;
		  new_env.subpart = env->subpart;
		  mu_mime_create (&new_env.mime, NULL, 0);
		  status = mhn_edit (&new_env, level + 1);
		  env->line = new_env.line;
		  env->subpart = new_env.subpart;
		  if (status == 0)
		    {
		      mu_mime_get_message (new_env.mime, &new_msg);
		      mu_mime_add_part (env->mime, new_msg);
		    }
		}
	      else if (strcmp (tok, "#end") == 0)
		{
		  if (level == 0)
		    {
		      mu_error (_("%s:%lu: unmatched #end"),
				input_file,
				(unsigned long) mhn_error_loc (env));
		      status = 1;
		    }
		  break;
		}
	      else
		{
		  *sp = c;
		  status = edit_mime (tok + 1, env, &msg, level);
		}
	    }
	}
      else if (line_count > 0 || buf[0] != '\n')
	{
	  if (ascii_buf && has_nonascii (buf, n))
	    ascii_buf = 0;
	  mu_stream_write (output, buf, n, NULL);
	  line_count++;
	}
    }
  free (buf);

  mu_stream_destroy (&output);
  if (msg)
    {
      if (line_count)
	finish_text_msg (env, &msg, ascii_buf);
      else
	mu_message_unref (msg);
    }
  
  return status;
}

int
parse_header_directive (const char *val, char **encoding, char **charset,
			char **subject)
{
  const char *p;

  /* Provide default values */
  *encoding = NULL;
  *charset = NULL;
  *subject = NULL;

  if (*val != '#')
    {
      *subject = mu_strdup (val);
      return 1;
    }
  val++;
  
  switch (*val)
    {
    case '#':
      break;

    case '<':
      p = strchr (val, '>');
      if (p)
	{
	  int i, argc;
	  char **argv;
	    
	  *subject = mu_strdup (p + 1);
	  split_args (val + 1, p - val - 1, &argc, &argv);
	  for (i = 0; i < argc; i++)
	    {
	      if (strlen (argv[i]) > 8
		  && memcmp (argv[i], "charset=", 8) == 0)
		{
		  free (*charset);
		  *charset = mu_strdup (argv[i] + 8);
		}
	      else if (strlen (argv[i]) > 9
		       && memcmp (argv[i], "encoding=", 9) == 0)
		{
		  free (*encoding);
		  *encoding = mu_strdup (argv[i] + 9);
		}
	    }
	  mu_argcv_free (argc, argv);
	  return 0;
	}
      break;

    default:
      val--;
    }
  *subject = mu_strdup (val);
  return 1;
}


void
mhn_header (mu_message_t msg, mu_message_t omsg)
{
  mu_header_t hdr = NULL;
  const char *val;
  
  mu_message_get_header (msg, &hdr);
  if (mu_header_sget_value (hdr, MU_HEADER_SUBJECT, &val) == 0)
    {
      char *subject, *encoding, *charset;
      
      if (parse_header_directive (val, &encoding, &charset, &subject))
	{
	  if (has_nonascii (val, strlen (val)))
	    {
	      int ismime = 0;
	      mu_message_t part = NULL;
	      
	      mu_message_is_multipart (omsg, &ismime);
	      if (ismime)
		mu_message_get_part (omsg, 1, &part);
	      else
		part = omsg;
	      if (part);
		{
		  mu_header_t parthdr = NULL;
		  char *typestr, *typeargs;
		  
		  mu_message_get_header (part, &parthdr);
		  _get_content_type (parthdr, &typestr, &typeargs);
		  _get_content_encoding (parthdr, &encoding);
		  if (typeargs)
		    {
		      int i, argc;
		      char **argv;
		      
		      split_args (typeargs, strlen (typeargs), &argc, &argv);
		      for (i = 0; i < argc; i++)
			if (strlen (argv[i]) > 8
			    && memcmp (argv[i], "charset=", 8) == 0)
			  {
			    charset = mu_strdup (argv[i]+8);
			    break;
			  }
		      mu_argcv_free (argc, argv);
		    }
		  free (typestr);
		}
	    }
	}

      if (charset)
	{
	  char *p;
	  int rc;
	  
	  if (!encoding || strcmp (encoding, "7bit") == 0)
	    {
	      free (encoding);
	      encoding = mu_strdup ("base64");
	    }
 	  rc = mu_rfc2047_encode (charset, encoding, subject, &p);
	  if (rc)
	    mu_error (_("cannot encode subject using %s, %s: %s"),
		      charset, encoding, mu_strerror (rc));
	  else
	    {
	      mu_header_set_value (hdr, MU_HEADER_SUBJECT, p, 1);
	      free (p);
	    }
	}
      free (charset);
      free (encoding);
      free (subject);
    }
}

int
mhn_compose ()
{
  int rc;
  mu_mime_t mime = NULL;
  mu_body_t body;
  mu_stream_t stream, in;
  struct compose_env env;
  mu_message_t msg;
  char *name, *backup, *p;
  
  mu_mime_create (&mime, NULL, 0);

  mu_message_get_body (message, &body);
  mu_body_get_streamref (body, &stream);

  env.mime = mime;
  env.input = stream;
  env.subpart = 0;
  env.line = 0;
  rc = mhn_edit (&env, 0);
  mu_stream_destroy (&stream);
  if (rc)
    return rc;

  mu_mime_get_message (mime, &msg);
  mu_message_unref (msg);

  p = strrchr (input_file, '/');
  /* Prepare file names */
  if (p)
    {
      *p = 0;
      name = mu_tempname (input_file);
      mu_asprintf (&backup, "%s/,%s.orig", input_file, p + 1);
      *p = '/';
    }
  else
    {
      name = mu_tempname (NULL);
      mu_asprintf (&backup, ",%s.orig", input_file);
    }
  
  /* Create working draft file */
  unlink (name);
  rc = mu_file_stream_create (&stream, name, MU_STREAM_RDWR|MU_STREAM_CREAT);
  if (rc)
    {
      mu_error (_("cannot create output stream (file %s): %s"),
		name, mu_strerror (rc));
      free (name);
      mu_mime_unref (mime); 
      return rc;
    }

  mhn_header (message, msg);
  copy_header_to_stream (message, stream);
  mu_message_get_streamref (msg, &in);
  mu_stream_copy (stream, in, 0, NULL);
  mu_stream_destroy (&in);
  mu_stream_destroy (&stream);
  
  /* Preserve the backup copy and replace the draft */
  unlink (backup);
  rename (input_file, backup);
  rename (name, input_file);

  free (name);
  mu_mime_unref (mime); 
  return 0;
}


/* *************************** Main Entry Point ************************** */
     
int
main (int argc, char **argv)
{
  int rc;
  int index;
  
  MU_APP_INIT_NLS ();
  
  mh_argp_init ();
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  argc -= index;
  argv += index;

  signal (SIGPIPE, SIG_IGN);

  if (input_file)
    {
      if (argc)
	{
	  mu_error (_("extra arguments"));
	  return 1;
	}
      message = mh_file_to_message (NULL,
				    mu_tilde_expansion (input_file,
							MU_HIERARCHY_DELIMITER,
							NULL));
      if (!message)
	return 1;
    }
  else if (mode == mode_compose)
    {
      if (argc > 1)
	{
	  mu_error (_("extra arguments"));
	  return 1;
	}
      input_file = mh_expand_name (mu_folder_directory (),
				   argc == 1 ? argv[0] : "draft", 0);
      message = mh_file_to_message (NULL, input_file);
      if (!message)
	return 1;
    }
  else
    {
      mbox = mh_open_folder (mh_current_folder (), MU_STREAM_READ);
      mh_msgset_parse (&msgset, mbox, argc, argv, "cur");
    }
  
  switch (mode)
    {
    case mode_compose:
      rc = mhn_compose ();
      break;
      
    case mode_list:
      rc = mhn_list ();
      break;
      
    case mode_show:
      rc = mhn_show ();
      break;
      
    case mode_store:
      rc = mhn_store ();
      break;
      
    default:
      abort ();
    }
  return rc ? 1 : 0;
}
  

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

/* MH mhn command */

#include <mh.h>
#include <mailutils/mime.h>
#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free
#include <obstack.h>

const char *argp_program_version = "mhn (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH mhn\v"
"Options marked with `*' are not yet implemented.\n"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = "[msgs]";

static struct argp_option options[] = {
  {"folder",       ARG_FOLDER,       N_("FOLDER"), 0,
   N_("Specify folder to operate upon"), 0},
  {"file",         ARG_FILE,         N_("FILE"),   0,
   N_("Specify file to operate upon"), 0},
  
  {N_("MIME editing options"),   0,  NULL, OPTION_DOC,  NULL, 5},
  {"compose",      ARG_COMPOSE,      N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Compose the MIME message (default)"), 6},
  {"nocompose",    ARG_NOCOMPOSE,    NULL, OPTION_HIDDEN, "", 6},
  
  {N_("Listing options"),   0,  NULL, OPTION_DOC,  NULL, 0},
  {"list",         ARG_LIST,         N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("List the table of contents"), 11 },
  {"nolist",       ARG_NOLIST,       NULL, OPTION_HIDDEN, "", 11 },
  {"headers",      ARG_HEADER,       N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Print the banner above the listing"), 12},
  {"noheaders",    ARG_NOHEADERS,    NULL, OPTION_HIDDEN, "", 12 },
  {"realsize",     ARG_REALSIZE,     N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("List the decoded sizes"), 12},
  {"norealsize",   ARG_NOREALSIZE,   NULL, OPTION_HIDDEN, "", 12 },

  {N_("Display options"),   0,  NULL, OPTION_DOC,  NULL, 20},
  {"show",         ARG_SHOW,         N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Display the contents of the messages"), 21},
  {"noshow",       ARG_NOSHOW,       NULL, OPTION_HIDDEN, "", 21 },
  {"serialonly",   ARG_SERIALONLY,   N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Display messages serially"), 22},
  {"noserialonly", ARG_NOSERIALONLY, NULL, OPTION_HIDDEN, "", 22 },
  {"form",         ARG_FORM,         N_("FILE"), 0,
   N_("Read mhl format from FILE"), 22},
  {"pause",        ARG_PAUSE,        N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Pause prior to displaying content"), 22},
  {"nopause",      ARG_NOPAUSE,      NULL, OPTION_HIDDEN, "", 22 },
   
  {N_("Saving options"),   0,  NULL, OPTION_DOC,  NULL, 30},
  {"store",        ARG_STORE,        N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Store the contents of the messages on disk"), 31},
  {"nostore",      ARG_NOSTORE,      NULL, OPTION_HIDDEN, "", 31 },
  {"auto",         ARG_AUTO,         N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Use filenames from the content headers"), 31},
  {"noauto",       ARG_NOAUTO,       NULL, OPTION_HIDDEN, "", 31 },

  {N_("Other options"),    0,  NULL, OPTION_DOC,  NULL, 40},
  {"part",         ARG_PART,         N_("PART"), 0,
   N_("Limit the scope of the operation to the given part"), 41},
  {"type",         ARG_TYPE,         N_("CONTENT"), 0,
   N_("Operate on message part with given multipart content"), 41 },
  {"verbose",       ARG_VERBOSE,     N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Print additional information"), 41 },
  {"noverbose",     ARG_NOVERBOSE,   NULL, OPTION_HIDDEN, "", 41 },
  {"quiet",         ARG_QUIET, 0, 0,
   N_("Be quiet")},
  {NULL}
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"file",          2, 0, "filename"},
  {"list",          1, MH_OPT_BOOL, NULL},
  {"headers",       1, MH_OPT_BOOL, NULL},
  {"realsize",      1, MH_OPT_BOOL, NULL},
  {"show",          2, MH_OPT_BOOL, NULL},
  {"serialonly",    2, MH_OPT_BOOL, NULL},
  {"form",          2, 0, "formfile"},
  {"pause",         3, MH_OPT_BOOL, NULL},
  {"store",         1, MH_OPT_BOOL, NULL},
  {"auto",          1, MH_OPT_BOOL, NULL},
  {"part",          3, 0, "part"},
  {"type",          1, 0, "type"},
  {"verbose",       1, MH_OPT_BOOL, NULL},
  {NULL}
};

typedef struct _msg_part *msg_part_t;

static msg_part_t msg_part_create __P((size_t num));
static void msg_part_destroy __P((msg_part_t p));
static int msg_part_eq __P((msg_part_t a, msg_part_t b));
static void msg_part_incr __P((msg_part_t p));
static void msg_part_decr __P((msg_part_t p));
static void msg_part_set_subpart __P((msg_part_t p, size_t subpart));
static void msg_part_print __P((msg_part_t p, int width));
static msg_part_t msg_part_parse __P((char *str));
static int msg_part_level __P((msg_part_t p));
static size_t msg_part_subpart __P((msg_part_t p, int level));

enum mhn_mode {
  mode_compose,
  mode_list,
  mode_show,
  mode_store,
};

static enum mhn_mode mode = mode_compose;

#define OPT_HEADERS    001
#define OPT_REALSIZE   002
#define OPT_PAUSE      004
#define OPT_AUTO       010
#define OPT_SERIALONLY 020
#define OPT_VERBOSE    040
#define OPT_QUIET      100

static int mode_options = OPT_HEADERS;
static char *formfile;
static char *content_type;
static char *content_subtype;
static char *input_file;
static int width = 80;

static mh_msgset_t msgset;
static mailbox_t mbox;
static message_t message;
static msg_part_t req_part;

/* Show flags */
#define MHN_EXCLUSIVE_EXEC  001
#define MHN_STDIN           002
#define MHN_LISTING         004
#define MHN_CONFIRM         010

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
      *type = xmalloc (len + 1);
      memcpy (*type, content, len);
      (*type)[len] = 0;

      p++;
      *subtype = xmalloc (strlen (p) + 1);
      strcpy (*subtype, p);
    }
  else
    {
      *type = xmalloc (strlen (content) + 1);
      strcpy (*type, content);
      *subtype = NULL;
    }
}

int
_get_hdr_value (header_t hdr, const char *name, char **value)
{
  int status = header_aget_value (hdr, name, value);
  if (status == 0)
    {
      /* Remove the newlines.  */
      char *nl;
      while ((nl = strchr (*value, '\n')) != NULL)
	*nl = ' ';
    }
  return status;
}

int
_get_content_type (header_t hdr, char **value, char **rest)
{
  char *type = NULL;
  _get_hdr_value (hdr, MU_HEADER_CONTENT_TYPE, &type);
  if (type == NULL || *type == '\0')
    {
      if (type)
	free (type);
      type = strdup ("text/plain"); /* Default.  */
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
_get_content_encoding (header_t hdr, char **value)
{
  char *encoding = NULL;
  _get_hdr_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING, &encoding);
  if (encoding == NULL || *encoding == '\0')
    {
      if (encoding)
	free (encoding);
      encoding = strdup ("7bit"); /* Default.  */
    }
  *value = encoding;
  return 0;
}

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      current_folder = arg;
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
      if (is_true (arg))
	{
	  mode_options |= OPT_PAUSE;
	  break;
	}
      /*FALLTHRU*/
    case ARG_NOPAUSE:
      mode_options &= ~OPT_PAUSE;
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
      formfile = arg;
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
	
    default:
      return 1;
    }
  return 0;
}


/* *********************** Message part functions ************************* */

struct _msg_part {
  int level;
  int maxlevel;
  size_t *part;
};

msg_part_t 
msg_part_create (size_t num)
{
  msg_part_t p = xmalloc (sizeof (*p));
  p->maxlevel = 16;
  p->part = xmalloc (sizeof (*p->part) * p->maxlevel);
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
      p->part = xrealloc (p->part, sizeof (*p->part) * p->maxlevel);
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
      width += printf ("%lu", (unsigned long) p->part[i]);
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
  char buf[64];
  
  for (i = 1; i <= p->level; i++)
    {
      if (i > 1)
	width++;
      width += snprintf (buf, sizeof buf, "%lu", (unsigned long) p->part[i]);
    }

  str = s = xmalloc (width + 1);
  for (i = 1; i <= p->level; i++)
    {
      if (i > 1)
	*s++ = '.';
      s += sprintf (s, "%lu", (unsigned long) p->part[i]);
    }
  *s = 0;
  return str;
}

void
msg_part_format_stk (struct obstack *stk, msg_part_t p)
{
  int i;
  char buf[64];
  
  for (i = 1; i <= p->level; i++)
    {
      int len;
  
      if (i > 1)
	obstack_1grow (stk, '.');

      len = snprintf (buf, sizeof buf, "%lu", (unsigned long) p->part[i]);
      obstack_grow (stk, buf, len);
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
	  mh_error (_("malformed part specification (near %s)"), endp);
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

char *
_mhn_profile_get (char *prefix, char *type, char *subtype, char *defval)
{
  char *str, *name;
  
  if (subtype)
    {
      asprintf (&name, "mhn-%s-%s/%s", prefix, type, subtype);
      str = mh_global_profile_get (name, NULL);
      free (name);
      if (!str)
	return _mhn_profile_get (prefix, type, NULL, defval);
    }
  else
    {
      asprintf (&name, "mhn-%s-%s", prefix, type);
      str = mh_global_profile_get (name, defval);
      free (name);
    }
  return str;
}

char *
mhn_show_command (message_t msg, msg_part_t part, int *flags, char **tempfile)
{
  char *p, *str, *tmp;
  char *typestr, *type, *subtype, *typeargs;
  struct obstack stk;
  header_t hdr;

  message_get_header (msg, &hdr);
  _get_content_type (hdr, &typestr, &typeargs);
  split_content (typestr, &type, &subtype);
  str = _mhn_profile_get ("show", type, subtype, NULL);
  if (!str) /* FIXME */
    return NULL;
  
  /* Expand macro-notations:
    %a  additional arguments
    %e  exclusive execution
    %f  filename containing content
    %F  %e, %f, and stdin is terminal not content
    %l  display listing prior to displaying content
    %p  %l, and ask for confirmation
    %s  subtype
    %d  content description */

  obstack_init (&stk);
  for (p = str; *p && isspace (*p); p++)
    ;
  
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
	      obstack_grow (&stk, typeargs, strlen (typeargs));
	      break;
	      
	    case 'e':
	      /* exclusive execution */
	      *flags |= MHN_EXCLUSIVE_EXEC;
	      break;
	      
	    case 'f':
	      /* filename containing content */
	      if (!*tempfile)
		*tempfile = mu_tempname (NULL);
	      obstack_grow (&stk, *tempfile, strlen (*tempfile));
	      break;
				     
	    case 'F':
	      /* %e, %f, and stdin is terminal not content */
	      *flags |= MHN_STDIN|MHN_EXCLUSIVE_EXEC;
	      if (!*tempfile)
		*tempfile = mu_tempname (NULL);
	      obstack_grow (&stk, *tempfile, strlen (*tempfile));
	      break;
	      
	    case 'l':
	      /* display listing prior to displaying content */
	      *flags |= MHN_LISTING;
	      break;
	      
	    case 'p':
	      /* %l, and ask for confirmation */
	      *flags |= MHN_LISTING|MHN_CONFIRM;
	      break;
	      
	    case 's':
	      /* subtype */
	      obstack_grow (&stk, subtype, strlen (subtype));
	      break;
	      
	    case 'd':
	      /* content description */
	      if (header_aget_value (hdr, MU_HEADER_CONTENT_DESCRIPTION,
				     &tmp) == 0)
		{
		  obstack_grow (&stk, tmp, strlen (tmp));
		  free (tmp);
		}
	      break;
	      
	    default:
	      obstack_1grow (&stk, *p);
	      p++;
	    }
	}
      else
	obstack_1grow (&stk, *p);
    }
  obstack_1grow (&stk, 0);

  free (typestr);
  free (type);
  free (subtype);

  str = obstack_finish (&stk);
  for (p = str; *p && isspace (*p); p++)
    ;
  if (!*p)
    str = NULL;
  else
    str = strdup (str);

  obstack_free (&stk, NULL);
  return str;
}

char *
mhn_store_command (message_t msg, msg_part_t part, char *name)
{
  char *p, *str, *tmp;
  char *typestr, *type, *subtype, *typeargs;
  struct obstack stk;
  header_t hdr;
  char buf[64];
  
  message_get_header (msg, &hdr);
  _get_content_type (hdr, &typestr, &typeargs);
  split_content (typestr, &type, &subtype);
  str = _mhn_profile_get ("show", type, subtype, "%m%P.%s");
  
  /* Expand macro-notations:
     %m  message number
     %P  .part
     %p  part
     %s  subtype */

  obstack_init (&stk);
  for (p = str; *p; p++)
    {
      if (*p == '%')
	{
	  switch (*++p)
	    {
	    case 'a':
	      /* additional arguments */
	      obstack_grow (&stk, typeargs, strlen (typeargs));
	      break;

	    case 'm':
	      if (name)
		obstack_grow (&stk, name, strlen (name));
	      else
		{
		  snprintf (buf, sizeof buf, "%lu",
			    (unsigned long) msg_part_subpart (part, 0));
		  obstack_grow (&stk, buf, strlen (buf));
		}
	      break;

	    case 'P':
	      obstack_1grow (&stk, '.');
	      /*FALLTHRU*/
	    case 'p':
	      msg_part_format_stk (&stk, part);
	      break;
	      
	    case 's':
	      /* subtype */
	      obstack_grow (&stk, subtype, strlen (subtype));
	      break;
	      
	    case 'd':
	      /* content description */
	      if (header_aget_value (hdr, MU_HEADER_CONTENT_DESCRIPTION,
				     &tmp) == 0)
		{
		  obstack_grow (&stk, tmp, strlen (tmp));
		  free (tmp);
		}
	      break;
	      
	    default:
	      obstack_1grow (&stk, *p);
	      p++;
	    }
	}
      else
	obstack_1grow (&stk, *p);
    }
  obstack_1grow (&stk, *p);

  free (typestr);
  free (type);
  free (subtype);
  
  str = obstack_finish (&stk);
  for (p = str; *p && isspace (*p); p++)
    ;
  if (!*p)
    str = NULL;
  else
    str = strdup (str);
  
  obstack_free (&stk, NULL);
  return str;
}


/* ************************** Message iterators *************************** */


typedef int (*msg_handler_t) __PMT((message_t msg, msg_part_t part,
				    char *type, char *encoding,
				    void *data));

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

  if ((rc = strcasecmp (content_type, type)) == 0)
    {
      if (content_subtype)
	rc = strcasecmp (content_subtype, subtype);
    }
  else 
    rc = strcasecmp (content_type, subtype);

  free (type);
  free (subtype);
  return rc;
}

int
call_handler (message_t msg, msg_part_t part, char *type, char *encoding,
	      msg_handler_t fun, void *data)
{
  if (req_part && msg_part_eq (req_part, part))
    return 0;
  if (match_content (type))
    return 0;
  return fun (msg, part, type, encoding, data);
}

int
handle_message (message_t msg, msg_part_t part, msg_handler_t fun, void *data)
{
  header_t hdr;
  char *type = NULL;
  char *encoding;
  int ismime = 0;
  
  message_get_header (msg, &hdr);
  _get_content_type (hdr, &type, NULL);
  _get_content_encoding (hdr, &encoding);

  fun (msg, part, type, encoding, data);
  sfree (&type);
  sfree (&encoding);
  
  message_is_multipart (msg, &ismime);
  if (ismime)
    {
      unsigned int i, nparts;

      message_get_num_parts (msg, &nparts);

      msg_part_incr (part);
      for (i = 1; i <= nparts; i++)
	{
	  message_t message = NULL;
	  
	  if (message_get_part (msg, i, &message) == 0)
	    {
	      message_get_header (message, &hdr);
	      _get_content_type (hdr, &type, NULL);
	      _get_content_encoding (hdr, &encoding);

	      msg_part_set_subpart (part, i);

	      if (message_is_multipart (message, &ismime) == 0 && ismime)
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
mhn_message_size (message_t msg, size_t *psize)
{
  body_t body;
  *psize = 0;
  message_get_body (msg, &body);
  if (mode_options & OPT_REALSIZE)
    {
      stream_t dstr = NULL, bstr = NULL;

      if (body_get_stream (body, &bstr) == 0)
	{
	  header_t hdr;
	  char *encoding;
	  size_t size = 0;
	  int rc;
	  
	  message_get_header (msg, &hdr);
	  _get_content_encoding (hdr, &encoding);

	  rc = filter_create(&dstr, bstr, encoding,
			     MU_FILTER_DECODE, MU_STREAM_READ);
	  free (encoding);
	  if (rc == 0)
	    {
	      char buf[512];
	      size_t n;
	      
	      while (stream_sequential_read (dstr, buf, sizeof buf, &n) == 0
		     && n > 0)
		size += n;

	      stream_destroy (&dstr, NULL);
	      *psize = size;
	      return 0;
	    }
	}
    }

  return body_size (body, psize);
}


/* ***************************** List Mode ******************************* */

int
list_handler (message_t msg, msg_part_t part, char *type, char *encoding,
	      void *data)
{
  size_t size;
  header_t hdr;
  
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
    printf ("%4luK", (unsigned long) size / 1024);
  else
    printf ("%4luM", (unsigned long) size / 1024 / 1024);
  
  if (message_get_header (msg, &hdr) == 0)
    {
      char *descr;
      if (header_aget_value (hdr, "Content-Description", &descr) == 0)
	{
	  printf (" %s", descr);
	  free (descr);
	}
    }
  
  printf ("\n");
  return 0;
}

int
list_message (message_t msg, size_t num)
{
  size_t uid;
  msg_part_t part;

  mh_message_number (msg, &uid);
  part = msg_part_create (uid);
  handle_message (msg, part, list_handler, NULL);
  msg_part_destroy (part);
  return 0;
}

void
list_iterator (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  list_message (msg, num);
}

int
mhn_list ()
{
  int rc; 

  if (mode_options & OPT_HEADERS)
    printf (_(" msg part type/subtype              size  description\n"));

  if (message)
    rc = list_message (message, 0);
  else
    rc = mh_iterate (mbox, &msgset, list_iterator, NULL);
  return rc;
}


/* ***************************** Show Mode ******************************* */

static list_t mhl_format;

void
cat_message (stream_t out, stream_t in)
{
  int rc = 0;
  char buf[512];
  size_t n;

  stream_seek (in, 0, SEEK_SET);
  while (rc == 0
	 && stream_sequential_read (in, buf, sizeof buf, &n) == 0
	 && n > 0)
    rc = stream_sequential_write (out, buf, n);
}

int
show_internal (message_t msg, msg_part_t part, char *encoding, stream_t out)
{
  int rc;
  body_t body = NULL;
  stream_t dstr, bstr;
  
  if ((rc = message_get_body (msg, &body)))
    {
      mh_error (_("%lu: can't get message body: %s"),
		(unsigned long) msg_part_subpart (part, 0),
		mu_strerror (rc));
      return 0;
    }
  body_get_stream (body, &bstr);
  rc = filter_create(&dstr, bstr, encoding,
		     MU_FILTER_DECODE, MU_STREAM_READ);
  if (rc == 0)
    bstr = dstr;
  cat_message (out, bstr);
  if (dstr)
    stream_destroy (&dstr, stream_get_owner (dstr));
  return 0;
}

int
exec_internal (message_t msg, msg_part_t part, char *encoding, char *cmd)
{
  int rc;
  stream_t tmp;

  rc = prog_stream_create (&tmp, cmd, MU_STREAM_WRITE);
  if (rc)
    {
      mh_error (_("can't create proc stream (command %s): %s"),
		cmd, mu_strerror (rc));
      return rc;
    }
  rc = stream_open (tmp);
  if (rc)
    {
      mh_error (_("can't open proc stream (command %s): %s"),
		cmd, mu_strerror (rc));
      return rc;
    }
  show_internal (msg, part, encoding, tmp);      
  stream_destroy (&tmp, stream_get_owner (tmp));
  return rc;
}

/* FIXME: stdin is always opened when invoking subprocess, no matter
   what the MHN_STDIN bit is */
   
int 
mhn_run_command (message_t msg, msg_part_t part,
		 char *encoding,
		 char *cmd, int flags,
		 char *tempfile)
{
  int rc = 0;
  
  if (tempfile)
    {
      /* pass content via a tempfile */
      int status;
      int argc;
      char **argv;
      stream_t tmp;
      
      if (argcv_get (cmd, "", "#", &argc, &argv))
	{
	  mh_error (_("can't parse command line `%s'"), cmd);
	  return ENOSYS;
	}

      rc = file_stream_create (&tmp, tempfile, MU_STREAM_RDWR);
      if (rc)
	{
	  mh_error (_("can't create temporary stream (file %s): %s"),
		    tempfile, mu_strerror (rc));
	  argcv_free (argc, argv);
	  return rc;
	}
      rc = stream_open (tmp);
      if (rc)
	{
	  mh_error (_("can't open temporary stream (file %s): %s"),
		    tempfile, mu_strerror (rc));
	  stream_destroy (&tmp, stream_get_owner (tmp));
	  argcv_free (argc, argv);
	  return rc;
	}
      show_internal (msg, part, encoding, tmp);      
      stream_destroy (&tmp, stream_get_owner (tmp));
      rc = mu_spawnvp (argv[0], argv, &status);
      if (status)
	rc = status;
      argcv_free (argc, argv);
    }
  else
    rc = exec_internal (msg, part, encoding, cmd);

  return rc;
}

int
show_handler (message_t msg, msg_part_t part, char *type, char *encoding,
	      void *data)
{
  stream_t out = data;
  char *cmd;
  int flags = 0;
  char buf[64];
  int fd = 1;
  char *tempfile = NULL;
  int ismime;
  
  if (message_is_multipart (msg, &ismime) == 0 && ismime)
    return 0;

  stream_get_fd (out, &fd);

  if (mode_options & OPT_PAUSE)
    flags |= MHN_CONFIRM;
  cmd = mhn_show_command (msg, part, &flags, &tempfile);
  if (!cmd)
    flags |= MHN_LISTING;
  if (flags & MHN_LISTING)
    {
      char *str;
      size_t size = 0;

      str = _("part ");
      stream_sequential_write (out, str, strlen (str));
      str = msg_part_format (part);
      stream_sequential_write (out, str, strlen (str));
      free (str);
      stream_sequential_write (out, " ", 1);
      stream_sequential_write (out, type, strlen (type));
      mhn_message_size (msg, &size);
      snprintf (buf, sizeof buf, " %lu", (unsigned long) size);
      stream_sequential_write (out, buf, strlen (buf));
      stream_sequential_write (out, "\n", 1);
      stream_flush (out);
    }

  if (flags & MHN_CONFIRM)
    {
      if (isatty (fd) && isatty (0))
	{
	  printf (_("Press <return> to show content..."));
	  if (!fgets (buf, sizeof buf, stdin) || buf[0] != '\n')
	    return 0;
	}
    } 
      
  if (!cmd)
    {
      char *pager = mh_global_profile_get ("moreproc", getenv ("PAGER"));
      if (pager)
	exec_internal (msg, part, encoding, pager);
      else
	show_internal (msg, part, encoding, out);
    }
  else
    {
      mhn_run_command (msg, part, encoding, cmd, flags, tempfile);
      free (cmd);
      unlink (tempfile);
      free (tempfile);
    }

  return 0;
}

int
show_message (message_t msg, size_t num, void *data)
{
  msg_part_t part = msg_part_create (num);

  if (mhl_format)
    mhl_format_run (mhl_format, width, 0, MHL_DISABLE_BODY, msg,
		    (stream_t) data);

  handle_message (msg, part, show_handler, data);
  msg_part_destroy (part);
  return 0;
}

void
show_iterator (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  msg_part_t part;
  
  mh_message_number (msg, &num);
  part = msg_part_create (num);
  show_message (msg, num, data);
  msg_part_destroy (part);
}

int
mhn_show ()
{
  int rc;
  stream_t ostr;
  
  rc = stdio_stream_create (&ostr, stdout, 0);
  if (rc)
    {
      mh_error (_("can't create output stream: %s"), mu_strerror (rc));
      exit (1);
    }
  rc = stream_open (ostr);
  if (rc)
    {
      mh_error (_("can't open output stream: %s"), mu_strerror (rc));
      exit (1);
    }
  
  if (formfile)
    {
      char *s = mh_expand_name (MHLIBDIR, "mhl.headers", 0);
      if (access (s, R_OK) == 0)
	formfile = "mhl.headers";
      free (s);
    }

  if (formfile)
    {
      char *s = mh_expand_name (MHLIBDIR, formfile, 0);
      mhl_format = mhl_format_compile (s);
      if (!mhl_format)
	exit (1);
      free (s);
    }
    
  if (message)
    rc = show_message (message, 0, ostr);
  else
    rc = mh_iterate (mbox, &msgset, show_iterator, ostr);
  return rc;
}

/* ***************************** Store Mode ****************************** */

char *
normalize_path (char *cwd, char *path)
{
  int len;
  char *p;
  char *pcwd = NULL;
  
  if (!path)
    return path;

  if (path[0] == '/')
    return NULL;

  if (!cwd)
    cwd = pcwd = mu_getcwd ();

  len = strlen (cwd) + strlen (path) + 2;
  p = xmalloc (len);
  sprintf (p, "%s/%s", cwd, path);
  path = p;

  /* delete trailing delimiter if any */
  if (len && path[len-1] == '/')
    path[len-1] = 0;

  /* Eliminate any /../ */
  for (p = strchr (path, '.'); p; p = strchr (p, '.'))
    {
      if (p > path && p[-1] == '/')
	{
	  if (p[1] == '.' && (p[2] == 0 || p[2] == '/'))
	    /* found */
	    {
	      char *q, *s;

	      /* Find previous delimiter */
	      for (q = p-2; *q != '/' && q >= path; q--)
		;

	      if (q < path)
		break;
	      /* Copy stuff */
	      s = p + 2;
	      p = q;
	      while (*q++ = *s++)
		;
	      continue;
	    }
	}

      p++;
    }

  if (path[0] == 0)
    {
      path[0] = '/';
      path[1] = 0;
    }

  len = strlen (cwd);
  if (strlen (path) < len || memcmp (path, cwd, len))
    sfree (&path);
  else
    memmove (path, path + len + 1, strlen (path) - len);
  free (pcwd);
  return path;
}

int
store_handler (message_t msg, msg_part_t part, char *type, char *encoding,
	       void *data)
{
  char *prefix = data;
  char *name = NULL;
  char *tmp;
  int ismime;
  int rc;
  stream_t out;
  char *dir = mh_global_profile_get ("mhn-storage", NULL);
  
  if (message_is_multipart (msg, &ismime) == 0 && ismime)
    return 0;
  
  if (mode_options & OPT_AUTO)
    {
      header_t hdr;
      char *val;

      if (message_get_header (msg, &hdr) == 0
	  && header_aget_value (hdr, MU_HEADER_CONTENT_DISPOSITION, &val) == 0)
	{
	  int argc;
	  char **argv;

	  if (argcv_get (val, "=", NULL, &argc, &argv) == 0)
	    {
	      int i;

	      for (i = 0; i < argc; i++)
		{
		  if (strcmp (argv[i], "filename") == 0
		      && ++i < argc
		      && argv[i][0] == '='
		      && ++i < argc)
		    {
		      name = normalize_path (dir, argv[i]);
		      break;
		    }
		}
	      argcv_free (argc, argv);
	    }
	  free (val);
	}
    }

  if (!name)
    {
      char *fname = mhn_store_command (msg, part, prefix);
      if (dir)
	asprintf (&name, "%s/%s", dir, fname);
      else
	name = fname;
    }
  
  tmp = msg_part_format (part);
  if (prefix)
    printf (_("storing message %s part %s as file %s\n"),
	    prefix,
	    tmp,
	    name);
  else
    printf (_("storing message %lu part %s as file %s\n"),
	    (unsigned long) msg_part_subpart (part, 0),
	    tmp,
	    name);
  free (tmp);

  if (!(mode_options & OPT_QUIET) && access (name, R_OK) == 0)
    {
      char *p;
      int rc;
	
      asprintf (&p, _("File %s already exists. Rewrite"), name);
      rc = mh_getyn (p);
      free (p);
      if (!rc)
	{
	  free (name);
	  return 0;
	}
    }
  
  rc = file_stream_create (&out, name, MU_STREAM_WRITE|MU_STREAM_CREAT);
  if (rc)
    {
      mh_error (_("can't create output stream (file %s): %s"),
		name, mu_strerror (rc));
      free (name);
      return rc;
    }
  rc = stream_open (out);
  if (rc)
    {
      mh_error (_("can't open output stream (file %s): %s"),
		name, mu_strerror (rc));
      free (name);
      stream_destroy (&out, stream_get_owner (out));
      return rc;
    }
  show_internal (msg, part, encoding, out);      

  stream_destroy (&out, stream_get_owner (out));
  free (name);
  
  return 0;
}

void
store_message (message_t msg, void *data)
{
  size_t uid;
  msg_part_t part;

  mh_message_number (msg, &uid);
  part = msg_part_create (uid);
  handle_message (msg, part, store_handler, data);
  msg_part_destroy (part);
}

void
store_iterator (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  store_message (msg, data);
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
    rc = mh_iterate (mbox, &msgset, store_iterator, NULL);
  return rc;
}


/* ***************************** Compose Mode **************************** */

int
stream_getline (stream_t str, char **buf, size_t *bufsize, size_t *pnum)
{
  int rc;
  size_t numread, n;

  if (!*buf)
    {
      *bufsize = 128;
      *buf = xmalloc (*bufsize);
    }
  numread = 0;
  while ((rc = stream_sequential_readline (str, *buf + numread,
					   *bufsize, &n)) == 0
	 && n > 0)
    {
      numread += n;
      if ((*buf)[numread - 1] != '\n')
	{
	  if (numread == *bufsize)
	    {
	      *bufsize += 128;
	      *buf = xrealloc (*buf, *bufsize);
	    }
	  continue;
	}
      break;
    }
  if (pnum)
    *pnum = numread;
  return rc;
}

struct compose_env {
  stream_t input;
  mime_t mime;
  size_t line;
};

size_t
mhn_error_loc (struct compose_env *env)
{
  header_t hdr = NULL;
  size_t n = 0;
  message_get_header (message, &hdr);
  header_lines (hdr, &n);
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
      mh_error ("%s:%lu: missing %c",
		input_file,
		(unsigned long) mhn_error_loc (env),
		c);
      return 1;
    }
  len = sp - rest;
  val = xmalloc (len + 1);
  memcpy (val, rest, len);
  val[len] = 0;
  *cmd = sp + 1;
  *pval = val;
  return 0;
}

/* cmd is:  type "/" subtype
            0*(";" attribute "=" value)
            [ "(" comment ")" ]
            [ "<" id ">" ]
            [ "[" description "]" ]
*/
int
parse_type_command (char *cmd, char **prest, struct compose_env *env,
		    header_t hdr)
{
  int status = 0, stop = 0;
  char *sp;
  char *type = NULL;
  char *subtype = NULL;
  char *comment = NULL, *descr = NULL, *id = NULL;
  struct obstack stk;
  char *rest = *prest;
  
  while (*rest && isspace (*rest))
    rest++;
  split_content (cmd, &type, &subtype);
  if (!subtype)
    {
      if (*rest != '/')
	{
	  mh_error ("%s:%lu: expected / but found %s",
		    input_file,
		    (unsigned long) mhn_error_loc (env),
		    rest);
	  return 1;
	}
      for (rest++; *rest && isspace (*rest); rest++)
	;
      if (!*rest)
	{
	  mh_error ("%s:%lu: missing subtype",
		    input_file,
		    (unsigned long) mhn_error_loc (env));
	  return 1;
	}
      subtype = strtok_r (rest, ";\n", &sp);
      subtype = strdup (subtype);
      rest = sp;
    }

  obstack_init (&stk);
  obstack_grow (&stk, type, strlen (type));
  obstack_1grow (&stk, '/');
  obstack_grow (&stk, subtype, strlen (subtype));
  
  while (stop == 0 && status == 0 && *rest)
    {
      switch (*rest++)
	{
	case '(':
	  if (comment)
	    {
	      mh_error ("%s:%lu: comment redefined",
			input_file,
			(unsigned long) mhn_error_loc (env));
	      status = 1;
	      break;
	    }
	  status = parse_brace (&comment, &rest, ')', env);
	  break;

	case '[':
	  if (descr)
	    {
	      mh_error ("%s:%lu: description redefined",
			input_file,
			(unsigned long) mhn_error_loc (env));
	      status = 1;
	      break;
	    }
	  status = parse_brace (&descr, &rest, ']', env);
	  break;
	  
	case '<':
	  if (id)
	    {
	      mh_error ("%s:%lu: content id redefined",
			input_file,
			(unsigned long) mhn_error_loc (env));
	      status = 1;
	      break;
	    }
	  status = parse_brace (&id, &rest, '>', env);
	  break;

	case ';':
	  obstack_1grow (&stk, ';');
	  while (*rest && isspace (*rest))
	    rest++;
	  sp = rest;
	  for (; *rest && !isspace (*rest); rest++)
	    obstack_1grow (&stk, *rest);
	  while (*rest && isspace (*rest))
	    rest++;
	  if (*rest != '=')
	    {
	      mh_error ("%s:%lu: syntax error",
			input_file,
			(unsigned long) mhn_error_loc (env));
	      status = 1;
	      break;
	    }
	  obstack_1grow (&stk, '=');
	  while (*rest && isspace (*rest))
	    rest++;
	  for (; *rest; rest++)
	    {
	      if (strchr (";<[(", *rest))
		break;
	      obstack_1grow (&stk, *rest);
	    }
	  break;
	    
	default:
	  stop = 1;
	  break;
	}
    }

  obstack_1grow (&stk, 0);
  header_set_value (hdr, MU_HEADER_CONTENT_TYPE, obstack_finish (&stk), 1);
  obstack_free (&stk, NULL);

  if (!id)
    id = mh_create_message_id (1);

  header_set_value (hdr, MU_HEADER_CONTENT_ID, id, 1);

  free (comment); /* FIXME: comment was not used */
  free (descr);
  free (id);
  *prest = rest;
  return status;
}

int
edit_extern (char *cmd, char *rest, struct compose_env *env, int level)
{
  return 0;
}

int
edit_forw (char *cmd, struct compose_env *env, int level)
{
  return 0;
}

int
edit_mime (char *cmd, char *rest, struct compose_env *env, int level)
{
  int rc;
  message_t msg;
  header_t hdr;
  
  message_create (&msg, NULL);
  message_get_header (msg, &hdr);
  rc = parse_type_command (cmd, &rest, env, hdr);
  if (rc == 0)
    mime_add_part (env->mime, msg);
  message_unref (msg);
  return rc;
}

int
mhn_edit (struct compose_env *env, int level)
{
  int status = 0;
  char *buf = NULL;
  size_t bufsize = 0, n;
  body_t body;
  stream_t output;
  message_t msg = NULL;
  
  while (status == 0
	 && stream_getline (env->input, &buf, &bufsize, &n) == 0 && n > 0)
    {
      env->line++;
      if (!msg)
	{
	  /* Create new message */
	  message_create (&msg, NULL);
	  /*FIXME: Headers*/
	  message_get_body (msg, &body);
	  body_get_stream (body, &output);
	  stream_seek (output, 0, SEEK_SET);
	}
      
      if (buf[0] == '#')
	{
	  if (buf[1] == '#')
	    stream_sequential_write (output, buf+1, n-1);
	  else
	    {
	      char *b2 = NULL;
	      size_t bs = 0, n2;
	      char *tok, *sp;

	      /* Collect the whole line */
	      while (n > 2 && buf[n-2] == '\\')
		{
		  int rc = stream_getline (env->input, &b2, &bs, &n2);
		  env->line++;
		  if (rc == 0 && n2 > 0)
		    {
		      if (n + n2 > bufsize)
			{
			  bufsize += 128;
			  buf = xrealloc (buf, bufsize);
			}
		      memcpy (buf + n - 2, b2, n2);
		      n += n2 - 2;
		    }
		}
	      free (b2);

	      /* Close and append the previous part */
	      stream_close (output);
	      mime_add_part (env->mime, msg);
	      message_unref (msg);
	      msg = NULL;

	      /* Execute the directive */
	      tok = strtok_r (buf, " \n", &sp);
	      if (tok[1] == '@')
		status = edit_extern (tok + 2, sp, env, level);
	      else if (strcmp (tok, "#forw") == 0)
		status = edit_forw (sp, env, level);
	      else if (strcmp (tok, "#begin") == 0)
		{
		  struct compose_env new_env;
		  message_t new_msg;
		  
		  new_env.input = env->input;
		  new_env.line = env->line;
		  mime_create (&new_env.mime, NULL, 0);
		  status = mhn_edit (&new_env, level + 1);
		  env->line = new_env.line;
		  if (status == 0)
		    {
		      mime_get_message (new_env.mime, &new_msg);
		      mime_add_part (env->mime, new_msg);
		      message_unref (new_msg);
		    }
		}
	      else if (strcmp (tok, "#end") == 0)
		{
		  if (level == 0)
		    {
		      mh_error ("%s:%lu: unmatched #end",
				input_file,
				(unsigned long) mhn_error_loc (env));
		      status = 1;
		    }
		  break;
		}
	      else 
		status = edit_mime (tok + 1, sp, env, level);
	    }
	}
      else
	stream_sequential_write (output, buf, n);
    }
  free (buf);

  if (msg)
    {
      stream_close (output);
      mime_add_part (env->mime, msg);
      message_unref (msg);
    }
  
  return status;
}

void
copy_header (message_t msg, stream_t stream)
{
  header_t hdr;
  stream_t in;
  char *buf = NULL;
  size_t bufsize, n;
  
  message_get_header (msg, &hdr);
  header_get_stream (hdr, &in);
  stream_seek (in, 0, SEEK_SET);
  while (stream_getline (in, &buf, &bufsize, &n) == 0 && n > 0)
    {
      if (n == 1 && buf[0] == '\n')
	break;
      stream_sequential_write (stream, buf, n);
    }
  free (buf);
}

int
mhn_compose ()
{
  int rc;
  mime_t mime = NULL;
  body_t body;
  stream_t stream, in;
  struct compose_env env;
  message_t msg;
  char *name;
  
  mime_create (&mime, NULL, 0);

  message_get_body (message, &body);
  body_get_stream (body, &stream);
  stream_seek (stream, 0, SEEK_SET);

  env.mime = mime;
  env.input = stream;
  rc = mhn_edit (&env, 0);
  if (rc)
    return rc;

  mime_get_message (mime, &msg);
  asprintf (&name, "%s/draft.mhn", mu_path_folder_dir);
  unlink (name);
  rc = file_stream_create (&stream, name, MU_STREAM_RDWR|MU_STREAM_CREAT);
  if (rc)
    {
      mh_error (_("can't create output stream (file %s): %s"),
		name, mu_strerror (rc));
      free (name);
      return rc;
    }
  rc = stream_open (stream);
  if (rc)
    {
      mh_error (_("can't open output stream (file %s): %s"),
		name, mu_strerror (rc));
      free (name);
      stream_destroy (&stream, stream_get_owner (stream));
      return rc;
    }
  free (name);
  
  copy_header (message, stream);
  message_get_stream (msg, &in);
  cat_message (stream, in);
  stream_destroy (&stream, stream_get_owner (stream));
  return 0;
}


/* *************************** Main Entry Point ************************** */
     
int
main (int argc, char **argv)
{
  int rc;
  int index;
  
  mu_init_nls ();
  
  mh_argp_parse (argc, argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  argc -= index;
  argv += index;

  if (input_file)
    {
      if (argc)
	{
	  mh_error (_("extra arguments"));
	  return 1;
	}
      message = mh_file_to_message (mu_path_folder_dir, input_file);
      if (!message)
	return 1;
    }
  else if (mode == mode_compose)
    {
      if (argc > 1)
	{
	  mh_error (_("extra arguments"));
	  return 1;
	}
      input_file = argc == 1 ? argv[0] : "draft";
      message = mh_file_to_message (mu_path_folder_dir, input_file);
      if (!message)
	return 1;
    }
  else
    {
      mbox = mh_open_folder (current_folder, 0);
      mh_msgset_parse (mbox, &msgset, argc, argv, "cur");
    }
  
  switch (mode)
    {
    case mode_compose:
      /* Prepare filename for diagnostic purposes */
      if (input_file[0] != '/')
	asprintf (&input_file, "%s/%s",  mu_path_folder_dir, input_file);
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
  

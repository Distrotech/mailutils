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
   N_("* Pause prior to displaying content"), 22},
  {"nopause",      ARG_NOPAUSE,      NULL, OPTION_HIDDEN, "", 22 },
   
  {N_("Saving options"),   0,  NULL, OPTION_DOC,  NULL, 30},
  {"store",        ARG_STORE,        N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Store the contents of the messages on disk"), 31},
  {"nostore",      ARG_NOSTORE,      NULL, OPTION_HIDDEN, "", 31 },
  {"auto",         ARG_AUTO,         N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Use filenames from the content headers"), 31},
  {"noauto",       ARG_NOAUTO,       NULL, OPTION_HIDDEN, "", 31 },

  {N_("Other options"),    0,  NULL, OPTION_DOC,  NULL, 40},
  {"part",         ARG_PART,         N_("PART"), 0,
   N_("Limit the scope of the operation to the given part"), 41},
  {"type",         ARG_TYPE,         N_("CONTENT"), 0,
   N_("Operate on message part with given multipart content"), 41 },
  {"verbose",       ARG_VERBOSE,     N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Print additional information"), 41 },
  {"noverbose",     ARG_NOVERBOSE,   NULL, OPTION_HIDDEN, "", 41 },
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


/* ************************** Message iterators *************************** */

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
_get_content_type (header_t hdr, char **value)
{
  char *type = NULL;
  _get_hdr_value (hdr, MU_HEADER_CONTENT_TYPE, &type);
  if (type == NULL || *type == '\0')
    {
      if (type)
	free (type);
      type = strdup ("text/plain"); /* Default.  */
    }
  else
    {
      char *p = strchr (type, ';');
      if (p)
	*p = 0;
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

  if (strcasecmp (content_type, type) == 0)
    rc = strcasecmp (content_subtype, subtype);
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
  _get_content_type (hdr, &type);
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
	      _get_content_type (hdr, &type);
	      _get_content_encoding (hdr, &encoding);

	      msg_part_set_subpart (part, i);

	      if (strcasecmp (type, "multipart/mixed") == 0)
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
    printf ("%3lu       ", (unsigned long) msg_part_subpart (part, 0));
  else
    {
      printf ("    ");
      msg_part_print (part, 5);
      putchar (' ');
    }
  printf ("%-26s", type);
  
  mhn_message_size (msg, &size);
  printf ("%4lu", (unsigned long) size);

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
  msg_part_t part = msg_part_create (num);
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
    printf (_("msg part  type/subtype              size description\n"));

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
show_handler (message_t msg, msg_part_t part, char *type, char *encoding,
	      void *data)
{
  stream_t out = data;

  if (strncasecmp (type, "multipart", 9) == 0)
    return 0;
  if (mhl_format)
    mhl_format_run (mhl_format, width, 0, MHL_DECODE, msg, out);
  else
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
    }
}

int
show_message (message_t msg, size_t num, void *data)
{
  msg_part_t part = msg_part_create (num);
  handle_message (msg, part, show_handler, data);
  msg_part_destroy (part);
  return 0;
}

void
show_iterator (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  show_message (msg, num, data);
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
  else
    {
      mbox = mh_open_folder (current_folder, 0);
      mh_msgset_parse (mbox, &msgset, argc, argv, "cur");
    }
  
  switch (mode)
    {
    case mode_compose:
      mh_error ("mode is not yet implemented");
      rc = 1;
      break;
      
    case mode_list:
      rc = mhn_list ();
      break;
      
    case mode_show:
      rc = mhn_show ();
      break;
      
    case mode_store:
      mh_error ("mode is not yet implemented");
      rc = 1;
      break;
      
    default:
      abort ();
    }
  return rc ? 1 : 0;
}
  

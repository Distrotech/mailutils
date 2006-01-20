/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 
   2005 Free Software Foundation, Inc.

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

/* This module implements execution of MH format strings. */

#include <mh.h>
#include <mailutils/mime.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free
#include <obstack.h>

typedef struct       /* A string object type */
{
  int size;          /* Allocated size or 0 for static storage */
  char *ptr;         /* Actual data */
} strobj_t;

struct mh_machine
{
  strobj_t reg_str;         /* String register */
  int reg_num;              /* Numeric register */

  strobj_t arg_str;         /* String argument */
  long arg_num;             /* Numeric argument */
  
  size_t pc;                /* Program counter */
  size_t progsize;          /* Size of allocated program*/
  mh_instr_t *prog;         /* Program itself */
  int stop;                 /* Stop execution immediately */

  struct obstack stk;       /* Output buffer */
  size_t width;             /* Output buffer width */
  size_t ind;               /* Output buffer index */

  mu_list_t addrlist;          /* The list of email addresses output this far */
  int fmtflags;             /* Current formatting flags */
  
  mu_message_t message;        /* Current message */
  size_t msgno;             /* Its number */
};

static char *_get_builtin_name (mh_builtin_fp ptr);

/* Functions for handling string objects. */

void
strobj_free (strobj_t *obj)
{
  if (obj->size)
    free (obj->ptr);
  obj->size = 0;
  obj->ptr = NULL;
}

#define strobj_ptr(p) ((p)->ptr ? (p)->ptr : "")
#define strobj_len(p) (strobj_is_null(p) ? 0 : strlen((p)->ptr))
#define strobj_is_null(p) ((p)->ptr == NULL)
#define strobj_is_static(p) ((p)->size == 0)

static void
strobj_create (strobj_t *lvalue, char *str)
{
  if (!str)
    {
      lvalue->size = 0;
      lvalue->ptr = NULL;
    }
  else
    {
      lvalue->size = strlen (str) + 1;
      lvalue->ptr = xmalloc (lvalue->size);
      memcpy (lvalue->ptr, str, lvalue->size);
    }
}

static void
strobj_set (strobj_t *lvalue, char *str)
{
  lvalue->size = 0;
  lvalue->ptr = str;
}

static void
strobj_assign (strobj_t *lvalue, strobj_t *rvalue)
{
  strobj_free (lvalue);
  *lvalue = *rvalue;
  rvalue->size = 0;
  rvalue->ptr = NULL;
}

static void
strobj_copy (strobj_t *lvalue, strobj_t *rvalue)
{
  if (strobj_is_null (rvalue))
    strobj_free (lvalue);
  else if (lvalue->size >= strobj_len (rvalue) + 1)
    memcpy (lvalue->ptr, strobj_ptr (rvalue), strobj_len (rvalue) + 1);
  else
    {
      if (lvalue->size)
	strobj_free (lvalue);
      strobj_create (lvalue, strobj_ptr (rvalue));
    }
}

static void
strobj_realloc (strobj_t *obj, size_t length)
{
  if (strobj_is_static (obj))
    {
      char *value = strobj_ptr (obj);
      obj->ptr = xmalloc (length);
      strncpy (obj->ptr, value, length-1);
      obj->ptr[length-1] = 0;
      obj->size = length;
    }
  else
    {
      obj->ptr = xrealloc (obj->ptr, length);
      obj->ptr[length-1] = 0;
      obj->size = length;
    }
}

/* Compress whitespace in a string */
static void
compress_ws (char *str, size_t *size)
{
  unsigned char *p, *q;
  size_t len = *size;
  int space = 0;

  for (p = q = (unsigned char*) str; len; len--, q++)
    {
      if (isspace (*q))
	{
	  if (!space)
	    *p++ = ' ';
	  space++;
	  continue;
	}
      else if (space)
	space = 0;

      if (isprint (*q))
	*p++ = *q;
    }
  *p = 0;
  *size = p - (unsigned char*) str;
}

static void
put_string (struct mh_machine *mach, char *str, int len)
{
  if (len == 0)
    len = strlen (str);
  obstack_grow (&mach->stk, str, len);
  mach->ind += len;
}

static void
print_hdr_segment (struct mh_machine *mach, char *str, size_t len)
{
  if (!len)
    len = strlen (str);

  if (len < mach->width)
    put_string (mach, str, len);
  else
    {
      char *endp = str + len;
      
      while (str < endp)
	{
	  size_t rest;
	  char *p;
	  size_t size;
	  
	  size = endp - str;
	  rest = mach->width - mach->ind;
	  if (size < rest)
	    {
	      put_string (mach, str, size);
	      break;
	    }
	  
	  for (p = str + rest - 1; p > str && !isspace (*p); p--)
	    ;

	  if (p > str)
	    {
	      put_string (mach, str, p - str);
	      put_string (mach, "\n\t", 0);
	      mach->ind = 8;
	      str = p;
	    }
	  else
	    {
	      put_string (mach, str, size);
	      str += len;
	    }
	}
    }
}

/* Print len bytes from str into mach->outbuf */
static void
print_hdr_string (struct mh_machine *mach, char *str)
{
  char *p;
  
  if (!str)
    str = "";

  p = strchr (str, '\n');
  while (p)
    {
      print_hdr_segment (mach, str, p - str + 1);
      mach->ind = 0;
      str = p + 1;
      p = strchr (str, '\n');
    }
    
  if (str[0])
    print_hdr_segment (mach, str, 0);
}

static void
print_simple_segment (struct mh_machine *mach, size_t width,
		      char *str, size_t len)
{
  size_t rest;

  if (!str)
    str = "";

  if (!len)
    len = strlen (str);

  if (!width)
    width = mach->width;

  rest = width - mach->ind;
  if (rest == 0)
    {
      if (len == 1 && str[0] == '\n')
	put_string (mach, str, len);
      return;
    }
  
  if (len > rest)
    len = rest;

  put_string (mach, str, len);
}

static void
print_string (struct mh_machine *mach, size_t width, char *str)
{
  char *p;
  
  if (!str)
    str = "";

  if (!width)
    width = mach->width;

  p = strchr (str, '\n');
  while (p)
    {
      print_simple_segment (mach, width, str, p - str + 1);
      mach->ind = 0;
      str = p + 1;
      p = strchr (str, '\n');
    }
    
  if (str[0])
    print_simple_segment (mach, width, str, 0);
}
  
static void
print_fmt_string (struct mh_machine *mach, size_t fmtwidth, char *str)
{
  size_t len = strlen (str);
  if (len > fmtwidth)
    len = fmtwidth;
  put_string (mach, str, len);

  if (fmtwidth > len)
    {
      fmtwidth -= len;
      mach->ind += fmtwidth;
      while (fmtwidth--)
	obstack_1grow (&mach->stk, ' ');
    }
}

/* FIXME: width? */
static void
print_obj (struct mh_machine *mach, size_t width, strobj_t *obj)
{
  if (!strobj_is_null (obj))
    print_string (mach, 0, strobj_ptr (obj));
}

static void
reset_fmt_defaults (struct mh_machine *mach)
{
  mach->fmtflags = 0;
}

static void
format_num (struct mh_machine *mach, long num)
{
  int n;
  char buf[64];
  char *ptr;
  int fmtwidth = mach->fmtflags & MH_WIDTH_MASK;
  int padchar = mach->fmtflags & MH_FMT_ZEROPAD ? '0' : ' ';
	    
  n = snprintf (buf, sizeof buf, "%ld", num);

  if (fmtwidth)
    {
      if (n > fmtwidth)
	{
	  ptr = buf + n - fmtwidth;
	  *ptr = '?';
	}
      else
	{
	  int i;
	  ptr = buf;
	  for (i = n; i < fmtwidth && mach->ind < mach->width;
	       i++, mach->ind++)
	    obstack_1grow (&mach->stk, padchar);
	}
    }
  else
    ptr = buf;

  print_string (mach, 0, ptr);
  reset_fmt_defaults (mach);
}

static void
format_str (struct mh_machine *mach, char *str)
{
  if (!str)
    str = "";
  if (mach->fmtflags)
    {
      int len = strlen (str);
      int fmtwidth = mach->fmtflags & MH_WIDTH_MASK;
      int padchar = ' ';
				
      if (mach->fmtflags & MH_FMT_RALIGN)
	{
	  int i, n;
	  
	  n = fmtwidth - len;
	  for (i = 0; i < n && mach->ind < mach->width;
	       i++, mach->ind++, fmtwidth--)
	    obstack_1grow (&mach->stk, padchar);
	}
	      
      print_fmt_string (mach, fmtwidth, str);
      reset_fmt_defaults (mach);
    }
  else
    print_string (mach, 0, str);
}

static int
addr_cmp (void *item, void *data)
{
  mu_address_t a = item;
  mu_address_t b = data;
  size_t i, count;
  int rc = 0;
  
  mu_address_get_count (a, &count);
  for (i = 1; rc == 0 && i <= count; i++)
    {
      char *str;
      if (mu_address_aget_email (a, i, &str))
	continue;
      rc = mu_address_contains_email (b, str);
      free (str);
    }
  return rc;
}

static int
addrlist_lookup (mu_list_t list, mu_address_t addr)
{
  return mu_list_do (list, addr_cmp, addr);
}

static int
addr_free (void *item, void *data)
{
  mu_address_t addr = item;
  mu_address_destroy (&addr);
  return 0;
}

static void
addrlist_destroy (mu_list_t *list)
{
  mu_list_do (*list, addr_free, NULL);
  mu_list_destroy (list);
}

/* Execute pre-compiled format on message msg with number msgno.
   buffer and bufsize specify output storage */
int
mh_format (mh_format_t *fmt, mu_message_t msg, size_t msgno,
	   size_t width, char **pret)
{
  struct mh_machine mach;
  char buf[64];
  
  memset (&mach, 0, sizeof (mach));
  mach.progsize = fmt->progsize;
  mach.prog = fmt->prog;

  mach.message = msg;
  mach.msgno = msgno;
  
  mach.width = width - 1; /* Count the newline */
  mach.pc = 1;
  obstack_init (&mach.stk);
  mu_list_create (&mach.addrlist);

  reset_fmt_defaults (&mach);
  
  while (!mach.stop)
    {
      mh_opcode_t opcode;
      switch (opcode = MHI_OPCODE(mach.prog[mach.pc++]))
	{
	case mhop_nop:
	  break;
	  
	case mhop_stop:
	  mach.stop = 1;
	  break;

	case mhop_branch:
	  mach.pc += MHI_NUM(mach.prog[mach.pc]);
	  break;

	case mhop_num_asgn:
	  mach.reg_num = mach.arg_num;
	  break;
	  
	case mhop_str_asgn:
	  strobj_assign (&mach.reg_str, &mach.arg_str);
	  break;
	  
	case mhop_num_arg:
	  mach.arg_num = MHI_NUM(mach.prog[mach.pc++]);
	  break;
	  
	case mhop_str_arg:
	  {
	    size_t skip = MHI_NUM(mach.prog[mach.pc++]);
	    strobj_set (&mach.arg_str, MHI_STR(mach.prog[mach.pc]));
	    mach.pc += skip;
	  }
	  break;

	case mhop_num_branch:
	  if (!mach.arg_num)
	    mach.pc += MHI_NUM(mach.prog[mach.pc]);
	  else
	    mach.pc++;
	  break;

	case mhop_str_branch:
	  if (!*strobj_ptr (&mach.arg_str))
	    mach.pc += MHI_NUM(mach.prog[mach.pc]);
	  else
	    mach.pc++;
	  break;

	case mhop_call:
	  MHI_BUILTIN (mach.prog[mach.pc++]) (&mach);
	  break;

	case mhop_header:
	  {
	    mu_header_t hdr = NULL;
	    char *value = NULL;
	    mu_message_get_header (mach.message, &hdr);
	    mu_header_aget_value_unfold (hdr, strobj_ptr (&mach.arg_str), &value);
	    strobj_free (&mach.arg_str);
	    if (value)
	      {
		size_t len = strlen (value);
		mach.arg_str.size = len + 1;
      		compress_ws (value, &len);
		mach.arg_str.ptr = value;
		mach.arg_num = 1;
	      }
	    else
	      mach.arg_num = 0;
	  }
	  break;

	case mhop_body:
	  {
	    mu_body_t body = NULL;
	    mu_stream_t stream = NULL;
	    size_t size = 0, off, str_off, nread;
	    size_t rest = mach.width - mach.ind;

	    strobj_free (&mach.arg_str);
	    mu_message_get_body (mach.message, &body);
	    mu_body_size (body, &size);
	    mu_body_get_stream (body, &stream);
	    if (size == 0 || !stream)
	      break;
	    if (size > rest)
	      size = rest;

	    mach.arg_str.ptr = xmalloc (size+1);
	    mach.arg_str.size = size;
	    
	    off = 0;
	    str_off = 0;
	    while (!mu_stream_read (stream, mach.arg_str.ptr + str_off,
				 mach.arg_str.size - str_off, off, &nread)
		   && nread != 0
		   && str_off < size)
	      {
		off += nread;
       		compress_ws (mach.arg_str.ptr + str_off, &nread);
		if (nread)
		  str_off += nread;
	      }
	    mach.arg_str.ptr[str_off] = 0;
	  }
	  break;
	  
	  /* assign reg_num to arg_num */
	case mhop_num_to_arg:
	  mach.arg_num = mach.reg_num;
	  break;
	  
	  /* assign reg_str to arg_str */
	case mhop_str_to_arg:
	  strobj_copy (&mach.arg_str, &mach.reg_str);
	  break;

	  /* Convert arg_str to arg_num */
	case mhop_str_to_num:
	  mach.arg_num = strtoul (strobj_ptr (&mach.arg_str), NULL, 0);
	  break;
  
	  /* Convert arg_num to arg_str */
	case mhop_num_to_str:
	  snprintf (buf, sizeof buf, "%lu", mach.arg_num);
	  strobj_free (&mach.arg_str);
	  strobj_create (&mach.arg_str, buf);
	  break;

	case mhop_num_print:
	  format_num (&mach, mach.reg_num);
	  break;
	  
	case mhop_str_print:
	  format_str (&mach, strobj_ptr (&mach.reg_str));
	  break;

	case mhop_fmtspec:
	  mach.fmtflags = MHI_NUM(mach.prog[mach.pc++]);
	  break;

	default:
	  mh_error (_("INTERNAL ERROR: Unknown opcode: %x"), opcode);
	  abort ();
	}
    }
  strobj_free (&mach.reg_str);
  strobj_free (&mach.arg_str);
  addrlist_destroy (&mach.addrlist);
  
  if (pret)
    {
      obstack_1grow (&mach.stk, 0);
      *pret = strdup (obstack_finish (&mach.stk));
    }
  obstack_free (&mach.stk, NULL);
  return mach.ind;
}

int
mh_format_str (mh_format_t *fmt, char *str, size_t width, char **pret)
{
  mu_message_t msg = NULL;
  mu_header_t hdr = NULL;
  int rc;
  
  if (mu_message_create (&msg, NULL))
    return -1;
  mu_message_get_header (msg, &hdr);
  mu_header_set_value (hdr, "text", str, 1);
  rc = mh_format (fmt, msg, 1, width, pret);
  mu_message_destroy (&msg, NULL);
  return rc;
}
  

void
mh_format_dump (mh_format_t *fmt)
{
  mh_instr_t *prog = fmt->prog;
  size_t pc = 1;
  int stop = 0;
  
  while (!stop)
    {
      mh_opcode_t opcode;
      int num;
      
      printf ("% 4.4ld: ", (long) pc);
      switch (opcode = MHI_OPCODE(prog[pc++]))
	{
	case mhop_nop:
	  printf ("nop");
	  break;
	  
	case mhop_stop:
	  printf ("stop");
	  stop = 1;
	  break;

	case mhop_branch:
	  num = MHI_NUM(prog[pc++]);
	  printf ("branch %d, %lu",
		  num, (unsigned long) pc + num - 1);
	  break;

	case mhop_num_asgn:
	  printf ("num_asgn");
	  break;
	  
	case mhop_str_asgn:
	  printf ("str_asgn");
	  break;
	  
	case mhop_num_arg:
	  num = MHI_NUM(prog[pc++]);
	  printf ("num_arg %d", num);
	  break;
	  
	case mhop_str_arg:
	  {
	    size_t skip = MHI_NUM(prog[pc++]);
	    char *s = MHI_STR(prog[pc]);
	    printf ("str_arg \"");
	    for (; *s; s++)
	      {
		switch (*s)
		  {
		  case '\a':
		    printf ("\\a");
		    break;
		    
		  case '\b':
		    printf ("\\b");
		    break;
		    
		  case '\f':
		    printf ("\\f");
		    break;
		    
		  case '\n':
		    printf ("\\n");
		    break;
		    
		  case '\r':
		    printf ("\\r");
		    break;
		    
		  case '\t':
		    printf ("\\t");
		    break;

		  case '"':
		    printf ("\\\"");
		    break;
		    
		  default:
		    if (isprint (*s))
		      putchar (*s);
		    else
		      printf ("\\%03o", *s);
		    break;
		  }
	      }
	    printf ("\"");
	    pc += skip;
	  }
	  break;

	case mhop_num_branch:
	  num = MHI_NUM(prog[pc++]);
	  printf ("num_branch %d, %lu",
		  num, (unsigned long) (pc + num - 1));
	  break;

	case mhop_str_branch:
	  num = MHI_NUM(prog[pc++]);
	  printf ("str_branch %d, %lu",
		  num, (unsigned long) (pc + num - 1));
	  break;

	case mhop_call:
	  {
	    char *name = _get_builtin_name (MHI_BUILTIN (prog[pc++]));
	    printf ("call %s", name ? name : "UNKNOWN");
	  }
	  break;

	case mhop_header:
	  printf ("header");
	  break;

	case mhop_body:
	  printf ("body");
	  break;
	  
	case mhop_num_to_arg:
	  printf ("num_to_arg");
	  break;
	  
	  /* assign reg_str to arg_str */
	case mhop_str_to_arg:
	  printf ("str_to_arg");
	  break;

	  /* Convert arg_str to arg_num */
	case mhop_str_to_num:
	  printf ("str_to_num");
	  break;
  
	  /* Convert arg_num to arg_str */
	case mhop_num_to_str:
	  printf ("num_to_str");
	  break;

	case mhop_num_print:
	  printf ("print");
	  break;
	  
	case mhop_str_print:
	  printf ("str_print");
	  break;

	case mhop_fmtspec:
	  {
	    int space = 0;
	    
	    num = MHI_NUM(prog[pc++]);
	    printf ("fmtspec: %#x, ", num);
	    if (num & MH_FMT_RALIGN)
	      {
		printf ("MH_FMT_RALIGN");
		space++;
	      }
	    if (num & MH_FMT_ZEROPAD)
	      {
		if (space)
		  printf ("|");
		printf ("MH_FMT_ZEROPAD");
		space++;
	      }
	    if (space)
	      printf ("; ");
	    printf ("%d", num & MH_WIDTH_MASK);
	  }
	  break;

	default:
	  mh_error ("Unknown opcode: %x", opcode);
	  abort ();
	}
      printf ("\n");
    }
}

/* Free any memory associated with a format structure. The structure
   itself is assumed to be in static storage. */
void
mh_format_free (mh_format_t *fmt)
{
  if (fmt->prog)
    free (fmt->prog);
  fmt->progsize = 0;
  fmt->prog = NULL;
}

/* Built-in functions */

/* Handler for unimplemented functions */
static void
builtin_not_implemented (char *name)
{
  mh_error ("%s is not yet implemented.", name);
}

static void
builtin_msg (struct mh_machine *mach)
{
  size_t msgno = mach->msgno;
  mh_message_number (mach->message, &msgno);
  mach->arg_num = msgno;
}

static void
builtin_cur (struct mh_machine *mach)
{
  size_t msgno = mach->msgno;
  mh_message_number (mach->message, &msgno);
  mach->arg_num = msgno == current_message;
}

static void
builtin_size (struct mh_machine *mach)
{
  size_t size;
  if (mu_message_size (mach->message, &size) == 0)
    mach->arg_num = size;
}

static void
builtin_strlen (struct mh_machine *mach)
{
  mach->arg_num = strlen (strobj_ptr (&mach->arg_str));
}

static void
builtin_width (struct mh_machine *mach)
{
  mach->arg_num = mach->width;
}

static void
builtin_charleft (struct mh_machine *mach)
{
  mach->arg_num = mach->width - mach->ind;
}

static void
builtin_timenow (struct mh_machine *mach)
{
  time_t t;
  
  time (&t);
  mach->arg_num = t;
}

static void
builtin_me (struct mh_machine *mach)
{
  char *s = mh_my_email ();
  strobj_realloc (&mach->arg_str, strlen (s) + 3);
  sprintf (strobj_ptr (&mach->arg_str), "<%s>", s);
}

static void
builtin_eq (struct mh_machine *mach)
{
  mach->arg_num = mach->reg_num == mach->arg_num;
}

static void
builtin_ne (struct mh_machine *mach)
{
  mach->arg_num = mach->reg_num != mach->arg_num;
}

static void
builtin_gt (struct mh_machine *mach)
{
  mach->arg_num = mach->reg_num > mach->arg_num;
}

static void
builtin_match (struct mh_machine *mach)
{
  mach->arg_num = strstr (strobj_ptr (&mach->reg_str),
			  strobj_ptr (&mach->arg_str)) != NULL;
}

static void
builtin_amatch (struct mh_machine *mach)
{
  int len = strobj_len (&mach->arg_str);
  mach->arg_num = strncmp (strobj_ptr (&mach->reg_str),
			   strobj_ptr (&mach->arg_str), len);
}

static void
builtin_plus (struct mh_machine *mach)
{
  mach->arg_num += mach->reg_num;
}

static void
builtin_minus (struct mh_machine *mach)
{
  mach->arg_num -= mach->reg_num;
}

static void
builtin_divide (struct mh_machine *mach)
{
  if (!mach->arg_num)
    {
      /* TRANSLATORS: Do not translate the word 'format'! */
      mh_error (_("format: divide by zero"));
      mach->stop = 1;
    }
  else
    mach->arg_num = mach->reg_num / mach->arg_num;
}

static void
builtin_modulo (struct mh_machine *mach)
{
  if (!mach->arg_num)
    {
      mh_error (_("format: divide by zero"));
      mach->stop = 1;
    }
  else
    mach->arg_num = mach->reg_num % mach->arg_num;
}

static void
builtin_num (struct mh_machine *mach)
{
  mach->reg_num = mach->arg_num;
}

static void
builtin_lit (struct mh_machine *mach)
{
  /* do nothing */
}

static void
builtin_getenv (struct mh_machine *mach)
{
  char *val = getenv (strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  strobj_create (&mach->arg_str, val);
}

static void
builtin_profile (struct mh_machine *mach)
{
  char *val = strobj_ptr (&mach->arg_str);
  strobj_free (&mach->arg_str);
  strobj_create (&mach->arg_str, mh_global_profile_get (val, ""));
}

static void
builtin_nonzero (struct mh_machine *mach)
{
    mach->arg_num = mach->reg_num;
}

static void
builtin_zero (struct mh_machine *mach)
{
  mach->arg_num = !mach->reg_num;
}

static void
builtin_null (struct mh_machine *mach)
{
  char *s = strobj_ptr (&mach->reg_str);
  mach->arg_num = !s && !s[0];
}

static void
builtin_nonnull (struct mh_machine *mach)
{
  char *s = strobj_ptr (&mach->reg_str);
  mach->arg_num = s && s[0];
}

/*     comp       comp     string   Set str to component text*/
static void
builtin_comp (struct mh_machine *mach)
{
  strobj_assign (&mach->reg_str, &mach->arg_str);
}

/*     compval    comp     integer  num set to "atoi(comp)"*/
static void
builtin_compval (struct mh_machine *mach)
{
  mach->reg_num = strtoul (strobj_ptr (&mach->arg_str), NULL, 0);
}

/*     trim       expr              trim trailing white-space from str*/
static void
builtin_trim (struct mh_machine *mach)
{
  char *p, *start;
  int len;
  
  if (strobj_is_static (&mach->arg_str))
    strobj_copy (&mach->arg_str, &mach->arg_str);

  start = strobj_ptr (&mach->arg_str);
  len = strlen (start);
  if (len == 0)
    return;
  for (p = start + len - 1; p >= start && isspace (*p); p--)
    ;
  p[1] = 0;
}

/*     putstr     expr              print str*/
static void
builtin_putstr (struct mh_machine *mach)
{
  print_string (mach, 0, strobj_ptr (&mach->arg_str));
}

/*     putstrf    expr              print str in a fixed width*/
static void
builtin_putstrf (struct mh_machine *mach)
{
  format_str (mach, strobj_ptr (&mach->arg_str));
}

/*     putnum     expr              print num*/
static void
builtin_putnum (struct mh_machine *mach)
{
  char *p;
  asprintf (&p, "%ld", mach->arg_num);
  print_string (mach, 0, p);
  free (p);
}

/*     putnumf    expr              print num in a fixed width*/
static void
builtin_putnumf (struct mh_machine *mach)
{
  format_num (mach, mach->arg_num);
}

static int
_parse_date (struct mh_machine *mach, struct tm *tm, mu_timezone *tz)
{
  char *date = strobj_ptr (&mach->arg_str);
  const char *p = date;
  
  if (mu_parse822_date_time (&p, date+strlen(date), tm, tz))
    {
      time_t t;
      
      /*mh_error ("can't parse date: [%s]", date);*/
      time (&t);
      *tm = *localtime (&t);
      tz->utc_offset = mu_utc_offset ();
    }
  
  return 0;
}

/*     sec        date     integer  seconds of the minute*/
static void
builtin_sec (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_sec;
}

/*     min        date     integer  minutes of the hour*/
static void
builtin_min (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_min;
}

/*     hour       date     integer  hours of the day (0-23)*/
static void
builtin_hour (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_hour;
}

/*     wday       date     integer  day of the week (Sun=0)*/
static void
builtin_wday (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_wday;
}

/*     day        date     string   day of the week (abbrev.)*/
static void
builtin_day (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  char buf[80];
  
  if (_parse_date (mach, &tm, &tz))
    return;

  strftime (buf, sizeof buf, "%a", &tm);
  strobj_free (&mach->arg_str);
  strobj_create (&mach->arg_str, buf);
}

/*     weekday    date     string   day of the week */
static void
builtin_weekday (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  char buf[80];
  
  if (_parse_date (mach, &tm, &tz))
    return;

  strftime (buf, sizeof buf, "%A", &tm);
  strobj_free (&mach->arg_str);
  strobj_create (&mach->arg_str, buf);
}

/*      sday       date     integer  day of the week known?
	(0=implicit,-1=unknown) */
static void
builtin_sday (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;

  /*FIXME: more elaborate check needed */
  if (_parse_date (mach, &tm, &tz))
    mach->arg_num = -1;
  else
    mach->arg_num = 1;
}

/*     mday       date     integer  day of the month*/
static void
builtin_mday (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_mday;
}

/*      yday       date     integer  day of the year */
static void
builtin_yday (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_yday;
}

/*     mon        date     integer  month of the year*/
static void
builtin_mon (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_mon+1;
}

/*     month      date     string   month of the year (abbrev.) */
static void
builtin_month (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  char buf[80];
  
  if (_parse_date (mach, &tm, &tz))
    return;

  strftime (buf, sizeof buf, "%b", &tm);
  strobj_free (&mach->arg_str);
  strobj_create (&mach->arg_str, buf);
}

/*      lmonth     date     string   month of the year*/
static void
builtin_lmonth (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  char buf[80];
  
  if (_parse_date (mach, &tm, &tz))
    return;

  strftime (buf, sizeof buf, "%B", &tm);
  strobj_free (&mach->arg_str);
  strobj_create (&mach->arg_str, buf);
}

/*     year       date     integer  year (may be > 100)*/
static void
builtin_year (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_year + 1900;
}

/*     zone       date     integer  timezone in hours*/
static void
builtin_zone (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tz.utc_offset;
}

/*     tzone      date     string   timezone string */
static void
builtin_tzone (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;
  
  strobj_free (&mach->arg_str);
  if (tz.tz_name)
    strobj_create (&mach->arg_str, (char*) tz.tz_name);
  else
    {
      char buf[6];
      int s;
      if (tz.utc_offset < 0)
	{
	  s = '-';
	  tz.utc_offset = - tz.utc_offset;
	}
      else
	s = '+';
      snprintf (buf, sizeof buf, "%c%02d%02d", s,
		tz.utc_offset/3600, tz.utc_offset/60);
      strobj_create (&mach->arg_str, buf);
    }
}

/*      szone      date     integer  timezone explicit?
	(0=implicit,-1=unknown) */
static void
builtin_szone (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;

  /*FIXME: more elaborate check needed */
  if (_parse_date (mach, &tm, &tz))
    mach->arg_num = -1;
  else
    mach->arg_num = 1;
}

/*     date2local date              coerce date to local timezone*/
static void
builtin_date2local (struct mh_machine *mach)
{
  /*FIXME: Noop*/
}

/*     date2gmt   date              coerce date to GMT*/
static void
builtin_date2gmt (struct mh_machine *mach)
{
  /*FIXME: Noop*/
}

/*     dst        date     integer  daylight savings in effect?*/
static void
builtin_dst (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;

  if (_parse_date (mach, &tm, &tz))
    return;
#ifdef HAVE_STRUCT_TM_TM_ISDST  
  mach->arg_num = tm.tm_isdst;
#else
  mach->arg_num = 0;
#endif
}

/*     clock      date     integer  seconds since the UNIX epoch*/
static void
builtin_clock (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;

  if (_parse_date (mach, &tm, &tz))
    return;
  mach->arg_num = mu_tm2time (&tm, &tz);
}

/*     rclock     date     integer  seconds prior to current time*/
void
builtin_rclock (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  time_t now = time (NULL);
  
  if (_parse_date (mach, &tm, &tz))
    return;
  mach->arg_num = now - mu_tm2time (&tm, &tz);
}

struct
{
  const char *std;
  const char *dst;
  int utc_offset;     /* offset from GMT (hours) */
} tzs[] = {
  { "GMT", "BST", 0 },
  { "EST", "EDT", -5 },
  { "CST", "CDT", -6 },
  { "MST", "MDT", -7 },
  { "PST", "PDT", -8 },
  { "EET", "EEST", -2 },
  { NULL, 0}
};

static void
date_cvt (struct mh_machine *mach, int pretty)
{
  struct tm tm;
  mu_timezone tz;
  char buf[80];
  int i, len;
  const char *tzname = NULL;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  if (pretty)
    {
      for (i = 0; tzs[i].std; i++)
	{
	  int offset = tzs[i].utc_offset;
	  int dst = 0;
	  
#ifdef HAVE_STRUCT_TM_TM_ISDST
	  if (tm.tm_isdst)
	    dst = -1;
#endif

	  if (tz.utc_offset == (offset + dst) * 3600)
	    {
	      if (dst)
		tzname = tzs[i].dst;
	      else
		tzname = tzs[i].std;
	      break;
	    }
	}
    }
  
  len = strftime (buf, sizeof buf,
		  "%a, %d %b %Y %H:%M:%S ", &tm);
  if (tzname)
    snprintf (buf + len, sizeof(buf) - len, "%s", tzname);
  else
    {
      int min, hrs, sign;
      int offset = tz.utc_offset;
      
      if (offset < 0)
	{
	  sign = '-';
	  offset = - offset;
	}
      else
	sign = '+';
      min = offset / 60;
      hrs = min / 60;
      min %= 60;
      snprintf (buf + len, sizeof(buf) - len, "%c%02d%02d", sign, hrs, min);
    }
  strobj_create (&mach->arg_str, buf);
}

/*      tws        date     string   official 822 rendering */
static void
builtin_tws (struct mh_machine *mach)
{
  date_cvt (mach, 0);
}

/*     pretty     date     string   user-friendly rendering*/
static void
builtin_pretty (struct mh_machine *mach)
{
  date_cvt (mach, 1);
}

/*     nodate     date     integer  str not a date string */
static void
builtin_nodate (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  mach->arg_num = _parse_date (mach, &tm, &tz);
}

/*     proper     addr     string   official 822 rendering */
static void
builtin_proper (struct mh_machine *mach)
{
  /*FIXME: noop*/
}

/*     friendly   addr     string   user-friendly rendering*/
static void
builtin_friendly (struct mh_machine *mach)
{
  /*FIXME: noop*/
}

/*     addr       addr     string   mbox@host or host!mbox rendering*/
static void
builtin_addr (struct mh_machine *mach)
{
  mu_address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = mu_address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (mu_address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    strobj_create (&mach->arg_str, buf);
  mu_address_destroy (&addr);
}

/*     pers       addr     string   the personal name**/
static void
builtin_pers (struct mh_machine *mach)
{
  mu_address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = mu_address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (mu_address_get_personal (addr, 1, buf, sizeof buf, &n) == 0
      && n > 1)
    {
      char *p;
      asprintf (&p, "\"%s\"", buf);
      strobj_create (&mach->arg_str, p);
      free (p);
    }
  mu_address_destroy (&addr);
}

/* FIXME: mu_address_get_comments never returns any comments. */
/*     note       addr     string   commentary text*/
static void
builtin_note (struct mh_machine *mach)
{
  mu_address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = mu_address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (mu_address_get_comments (addr, 1, buf, sizeof buf, &n) == 0)
    strobj_create (&mach->arg_str, buf);
  mu_address_destroy (&addr);
}

/*     mbox       addr     string   the local mailbox**/
static void
builtin_mbox (struct mh_machine *mach)
{
  mu_address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = mu_address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (mu_address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    {
      char *p = strchr (buf, '@');
      if (p)
	*p = 0;
      strobj_create (&mach->arg_str, p);
    }
  mu_address_destroy (&addr);
}

/*     mymbox     addr     integer  the user's addresses? (0=no,1=yes)*/
static void
builtin_mymbox (struct mh_machine *mach)
{
  mu_address_t addr;
  size_t n;
  char buf[80];
  
  mach->arg_num = 0;
  if (mu_address_create (&addr, strobj_ptr (&mach->arg_str)))
    return;

  if (mu_address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    mach->arg_num = mh_is_my_name (buf);
  mu_address_destroy (&addr);
}

/*     host       addr     string   the host domain**/
static void
builtin_host (struct mh_machine *mach)
{
  mu_address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = mu_address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (mu_address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    {
      char *p = strchr (buf, '@');
      if (p)
	strobj_create (&mach->arg_str, p+1);
    }
  mu_address_destroy (&addr);
}

/*     nohost     addr     integer  no host was present**/
static void
builtin_nohost (struct mh_machine *mach)
{
  mu_address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = mu_address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (mu_address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    mach->arg_num = strchr (buf, '@') != NULL;
  else
    mach->arg_num = 0;
  mu_address_destroy (&addr);
}

/*     type       addr     integer  host type* (0=local,1=network,
       -1=uucp,2=unknown)*/
static void
builtin_type (struct mh_machine *mach)
{
  mu_address_t addr;
  size_t n;
  char buf[80];
  int rc;

  rc = mu_address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (mu_address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    {
      if (strchr (buf, '@'))
	mach->arg_num = 1;
      else if (strchr (buf, '@'))
	mach->arg_num = -1;
      else
	mach->arg_num = 0; /* assume local */
    }
  else
    mach->arg_num = 2;
  mu_address_destroy (&addr);
}

/*     path       addr     string   any leading host route**/
static void
builtin_path (struct mh_machine *mach)
{
  mu_address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = mu_address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;
  if (mu_address_get_route (addr, 1, buf, sizeof buf, &n))
    strobj_create (&mach->arg_str, buf);
  mu_address_destroy (&addr);
}

/*     ingrp      addr     integer  address was inside a group**/
static void
builtin_ingrp (struct mh_machine *mach)
{
  /*FIXME:*/
  builtin_not_implemented ("ingrp");
}

/*     gname      addr     string   name of group**/
static void
builtin_gname (struct mh_machine *mach)
{
  /*FIXME:*/
  builtin_not_implemented ("gname");
}

/*     formataddr expr              append arg to str as a
       (comma separated) address list */
static void
builtin_formataddr (struct mh_machine *mach)
{
  mu_address_t addr, dest;
  size_t size;
  int i;
  size_t num;
  char *buf;
  
  if (strobj_len (&mach->reg_str) == 0)
    dest = NULL;
  else if (mu_address_create (&dest, strobj_ptr (&mach->reg_str)))
    return;
    
  if (mu_address_create (&addr, strobj_ptr (&mach->arg_str)))
    {
      mu_address_destroy (&dest);
      return;
    }

  mu_address_get_count (addr, &num);
  for (i = 1; i <= num; i++)
    {
      if (mu_address_aget_email (addr, i, &buf) == 0)
	{
	  if ((rcpt_mask & RCPT_ME) || !mh_is_my_name (buf))
	    {
	      mu_address_t subaddr;
	      mu_address_get_nth (addr, i, &subaddr);
	      if (!addrlist_lookup (mach->addrlist, subaddr))
		{
		  mu_list_append (mach->addrlist, subaddr);
		  mu_address_union (&dest, subaddr);
		}
	      else
		mu_address_destroy (&subaddr);
	    }
	  free (buf);
	}
    }

  if (mu_address_to_string (dest, NULL, 0, &size) == 0)
    {
      strobj_realloc (&mach->reg_str, size + 1);
      mu_address_to_string (dest, strobj_ptr (&mach->reg_str), size + 1, NULL);
      mu_address_destroy (&dest);
    }
}

/*      putaddr    literal        print str address list with
                                  arg as optional label;
                                  get line width from num
   FIXME: Currently it's the same as puthdr. Possibly it should do
   some address-checking as well.
*/
static void
builtin_putaddr (struct mh_machine *mach)
{
  if (!strobj_is_null (&mach->arg_str))
    print_hdr_string (mach, strobj_ptr (&mach->arg_str));
  if (!strobj_is_null (&mach->reg_str))
    print_hdr_string (mach, strobj_ptr (&mach->reg_str));
}

/* GNU extension: Strip leading whitespace and eventual Re: (or Re\[[0-9]+\]:)
   prefix from the argument */
static void
builtin_unre (struct mh_machine *mach)
{
  const char *p;
  int rc = mu_unre_subject (strobj_ptr (&mach->arg_str), &p);
  if (rc == 0 && p != strobj_ptr (&mach->arg_str))
    {
      char *q = strdup (p); /* Create a copy, since strobj_create will
			       destroy p */
      strobj_free (&mach->arg_str);
      strobj_create (&mach->arg_str, q);
      free (q);
    }
}  

static void
builtin_isreply (struct mh_machine *mach)
{
  int rc;
  
  if (strobj_is_null (&mach->arg_str))
    {
      mu_header_t hdr = NULL;
      char *value = NULL;
      mu_message_get_header (mach->message, &hdr);
      
      mu_header_aget_value (hdr, MU_HEADER_SUBJECT, &value);
      rc = mu_unre_subject (value, NULL);
      free (value);
    }
  else
    rc = mu_unre_subject (strobj_ptr (&mach->arg_str), NULL);

  mach->arg_num = !rc;
}

static void
decode_string (strobj_t *obj)
{
  char *tmp;
  
  if (strobj_is_null (obj))
    return;

  if (mh_decode_2047 (strobj_ptr (obj), &tmp) == 0)
    {
      strobj_free (obj);
      strobj_create (obj, tmp);
      free (tmp);
    }
}

static void
builtin_decode (struct mh_machine *mach)
{
  decode_string (&mach->arg_str);
}

static void
builtin_reply_regex (struct mh_machine *mach)
{
  mh_set_reply_regex (strobj_ptr (&mach->arg_str));
}

int
mh_decode_rcpt_flag (const char *arg)
{
  if (strcmp (arg, "to") == 0)
    return RCPT_TO;
  else if (strcmp (arg, "cc") == 0)
    return RCPT_CC;
  else if (strcmp (arg, "me") == 0)
    return RCPT_ME;
  else if (strcmp (arg, "all") == 0)
    return RCPT_ALL;

  return RCPT_NONE;
}

static void
builtin_rcpt (struct mh_machine *mach)
{
  int rc = mh_decode_rcpt_flag (strobj_ptr (&mach->arg_str));
  if (rc == RCPT_NONE)
    {
      mh_error (_("Invalid recipient mask"));
      /* try to continue anyway */
    }
  mach->arg_num = rc & rcpt_mask;
}

static void
builtin_concat (struct mh_machine *mach)
{
  size_t size = strobj_len (&mach->arg_str);

  if (size == 0)
    return;
  compress_ws (strobj_ptr (&mach->arg_str), &size);
  if (strobj_len (&mach->reg_str) == 0)
    strobj_copy (&mach->reg_str, &mach->arg_str);
  else
    {
      int length = 1;
    
      length += 1 + strobj_len (&mach->reg_str); /* reserve en extra space */
      length += strobj_len (&mach->arg_str);
      strobj_realloc (&mach->reg_str, length);
      strcat (strcat (strobj_ptr (&mach->reg_str), " "),
	      strobj_ptr (&mach->arg_str));
    }
}

static void
builtin_printhdr (struct mh_machine *mach)
{
  char *tmp = NULL;
  size_t s = 0;
  
  if (!strobj_is_null (&mach->arg_str))
    {
      s = strobj_len (&mach->arg_str);
      tmp = strdup (strobj_ptr (&mach->arg_str));
    }
  
  if (!strobj_is_null (&mach->reg_str))
    {
      s += strobj_len (&mach->reg_str) + 1;
      tmp = realloc (tmp, s);
      strcat (tmp, strobj_ptr (&mach->reg_str));
    }

  if (tmp)
    {
      print_hdr_string (mach, tmp);
      free (tmp);
    }
}

static void
builtin_in_reply_to (struct mh_machine *mach)
{
  char *value;

  strobj_free (&mach->arg_str);
  if (mu_rfc2822_in_reply_to (mach->message, &value) == 0)
    {
      strobj_create (&mach->arg_str, value);
      free (value);
    }
}

static void
builtin_references (struct mh_machine *mach)
{
  char *value;

  strobj_free (&mach->arg_str);
  if (mu_rfc2822_references (mach->message, &value) == 0)
    {
      strobj_create (&mach->arg_str, value);
      free (value);
    }
}

static void
builtin_package (struct mh_machine *mach)
{
  strobj_free (&mach->arg_str);
  strobj_set (&mach->arg_str, PACKAGE);
}

static void
builtin_package_string (struct mh_machine *mach)
{
  strobj_free (&mach->arg_str);
  strobj_set (&mach->arg_str, PACKAGE_STRING);
}

static void
builtin_version (struct mh_machine *mach)
{
  strobj_free (&mach->arg_str);
  strobj_set (&mach->arg_str, VERSION);
}

/* Builtin function table */

mh_builtin_t builtin_tab[] = {
  /* Name       Handling function Return type  Arg type      Opt. arg */ 
  { "msg",      builtin_msg,      mhtype_num,  mhtype_none },
  { "cur",      builtin_cur,      mhtype_num,  mhtype_none },
  { "size",     builtin_size,     mhtype_num,  mhtype_none },
  { "strlen",   builtin_strlen,   mhtype_num,  mhtype_none },
  { "width",    builtin_width,    mhtype_num,  mhtype_none },
  { "charleft", builtin_charleft, mhtype_num,  mhtype_none },
  { "timenow",  builtin_timenow,  mhtype_num,  mhtype_none },
  { "me",       builtin_me,       mhtype_str,  mhtype_none },
  { "eq",       builtin_eq,       mhtype_num,  mhtype_num  },
  { "ne",       builtin_ne,       mhtype_num,  mhtype_num  },
  { "gt",       builtin_gt,       mhtype_num,  mhtype_num  },
  { "match",    builtin_match,    mhtype_num,  mhtype_str },
  { "amatch",   builtin_amatch,   mhtype_num,  mhtype_str },
  { "plus",     builtin_plus,     mhtype_num,  mhtype_num },
  { "minus",    builtin_minus,    mhtype_num,  mhtype_num },
  { "divide",   builtin_divide,   mhtype_num,  mhtype_num },
  { "modulo",   builtin_modulo,   mhtype_num,  mhtype_num },
  { "num",      builtin_num,      mhtype_num,  mhtype_num },
  { "lit",      builtin_lit,      mhtype_str,  mhtype_str,  MHA_OPT_CLEAR },
  { "getenv",   builtin_getenv,   mhtype_str,  mhtype_str },
  { "profile",  builtin_profile,  mhtype_str,  mhtype_str },
  { "nonzero",  builtin_nonzero,  mhtype_num,  mhtype_num,  MHA_OPTARG },
  { "zero",     builtin_zero,     mhtype_num,  mhtype_num,  MHA_OPTARG },
  { "null",     builtin_null,     mhtype_num,  mhtype_str,  MHA_OPTARG },
  { "nonnull",  builtin_nonnull,  mhtype_num,  mhtype_str,  MHA_OPTARG },
  { "comp",     builtin_comp,     mhtype_num,  mhtype_str,  MHA_OPTARG },
  { "compval",  builtin_compval,  mhtype_num,  mhtype_str },	   
  { "trim",     builtin_trim,     mhtype_str,  mhtype_str,  MHA_OPTARG },
  { "putstr",   builtin_putstr,   mhtype_none,  mhtype_str, MHA_OPTARG },
  { "putstrf",  builtin_putstrf,  mhtype_none,  mhtype_str, MHA_OPTARG },
  { "putnum",   builtin_putnum,   mhtype_none,  mhtype_num, MHA_OPTARG },
  { "putnumf",  builtin_putnumf,  mhtype_none,  mhtype_num, MHA_OPTARG },
  { "sec",      builtin_sec,      mhtype_num,  mhtype_str },
  { "min",      builtin_min,      mhtype_num,  mhtype_str },
  { "hour",     builtin_hour,     mhtype_num,  mhtype_str },
  { "wday",     builtin_wday,     mhtype_num,  mhtype_str },
  { "day",      builtin_day,      mhtype_str,  mhtype_str },
  { "weekday",  builtin_weekday,  mhtype_str,  mhtype_str },
  { "sday",     builtin_sday,     mhtype_num,  mhtype_str },
  { "mday",     builtin_mday,     mhtype_num,  mhtype_str },
  { "yday",     builtin_yday,     mhtype_num,  mhtype_str },
  { "mon",      builtin_mon,      mhtype_num,  mhtype_str },
  { "month",    builtin_month,    mhtype_str,  mhtype_str },
  { "lmonth",   builtin_lmonth,   mhtype_str,  mhtype_str },
  { "year",     builtin_year,     mhtype_num,  mhtype_str },
  { "zone",     builtin_zone,     mhtype_num,  mhtype_str },
  { "tzone",    builtin_tzone,    mhtype_str,  mhtype_str },
  { "szone",    builtin_szone,    mhtype_num,  mhtype_str },
  { "date2local", builtin_date2local, mhtype_str, mhtype_str },
  { "date2gmt", builtin_date2gmt, mhtype_str,  mhtype_str },
  { "dst",      builtin_dst,      mhtype_num,  mhtype_str },
  { "clock",    builtin_clock,    mhtype_num,  mhtype_str },
  { "rclock",   builtin_rclock,   mhtype_num,  mhtype_str },
  { "tws",      builtin_tws,      mhtype_str,  mhtype_str },
  { "pretty",   builtin_pretty,   mhtype_str,  mhtype_str },
  { "nodate",   builtin_nodate,   mhtype_num,  mhtype_str },
  { "proper",   builtin_proper,   mhtype_str,  mhtype_str },
  { "friendly", builtin_friendly, mhtype_str,  mhtype_str },
  { "addr",     builtin_addr,     mhtype_str,  mhtype_str },
  { "pers",     builtin_pers,     mhtype_str,  mhtype_str },
  { "note",     builtin_note,     mhtype_str,  mhtype_str },
  { "mbox",     builtin_mbox,     mhtype_str,  mhtype_str },
  { "mymbox",   builtin_mymbox,   mhtype_num,  mhtype_str },
  { "host",     builtin_host,     mhtype_str,  mhtype_str },
  { "nohost",   builtin_nohost,   mhtype_num,  mhtype_str },
  { "type",     builtin_type,     mhtype_num,  mhtype_str },
  { "path",     builtin_path,     mhtype_str,  mhtype_str },
  { "ingrp",    builtin_ingrp,    mhtype_num,  mhtype_str },
  { "gname",    builtin_gname,    mhtype_str,  mhtype_str},
  { "formataddr", builtin_formataddr, mhtype_none, mhtype_str, MHA_OPTARG },
  { "putaddr",  builtin_putaddr,  mhtype_none, mhtype_str },
  { "unre",     builtin_unre,     mhtype_str,  mhtype_str },
  { "rcpt",     builtin_rcpt,     mhtype_num,  mhtype_str },
  { "concat",   builtin_concat,   mhtype_none, mhtype_str,     MHA_OPTARG },
  { "printhdr", builtin_printhdr, mhtype_none, mhtype_str },
  { "in_reply_to", builtin_in_reply_to, mhtype_str,  mhtype_none },
  { "references", builtin_references, mhtype_str,  mhtype_none },
  { "package",  builtin_package,  mhtype_str, mhtype_none },
  { "package_string",  builtin_package_string,  mhtype_str, mhtype_none },
  { "version",  builtin_version,  mhtype_str, mhtype_none },
  { "reply_regex", builtin_reply_regex, mhtype_none, mhtype_str },
  { "isreply", builtin_isreply, mhtype_num, mhtype_str, MHA_OPTARG },
  { "decode", builtin_decode, mhtype_str, mhtype_str },
  { 0 }
};

mh_builtin_t *
mh_lookup_builtin (char *name, int *rest)
{
  mh_builtin_t *bp;
  int namelen = strlen (name);
  
  for (bp = builtin_tab; bp->name; bp++)
    {
      int len = strlen (bp->name);
      if (len >= namelen
	  && memcmp (name, bp->name, len) == 0)
	{
	  *rest = namelen - len;
	  return bp;
	}
    }
  return NULL;
}

char *
_get_builtin_name (mh_builtin_fp ptr)
{
  mh_builtin_t *bp;

  for (bp = builtin_tab; bp->name; bp++)
    if (bp->fun == ptr)
      return bp->name;
  return NULL;
}

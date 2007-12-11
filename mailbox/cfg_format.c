/* cfg_print.c -- convert configuration parse tree to human-readable format.
   Copyright (C) 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <mailutils/stream.h>
#include <mailutils/error.h>
#include <mailutils/cfg.h>
#include <mailutils/argcv.h>
#include <mailutils/nls.h>
#include <ctype.h>

struct tree_print
{
  unsigned level;
  mu_stream_t stream;
  char *buf;
  size_t bufsize;
};

static void
format_level (mu_stream_t stream, int level)
{
  while (level--)
    mu_stream_sequential_write (stream, "  ", 2);
}

static void
format_label (struct tree_print *tp, const char *label)
{
  size_t size;
  int quote;
  char *p;
	
  size = mu_argcv_quoted_length (label, &quote);
  if (quote)
    size += 2;
  size++;
  if (size > tp->bufsize)
    {
      p = realloc (tp->buf, size);
      if (!p)
	{
	  mu_stream_sequential_printf (tp->stream, "%s\n",
				       _("ERROR: not enough memory"));
	  return;
	}
      tp->bufsize = size;
      tp->buf = p;
    }
  
  p = tp->buf;
  if (quote)
    {
      tp->buf[0] = '"';
      tp->buf[size-2] = '"';
      p++;
    }
  tp->buf[size-1] = 0;
  mu_argcv_quote_copy (p, label);
  mu_stream_sequential_write (tp->stream, tp->buf, size - 1);
}

static int
format_node (const mu_cfg_node_t *node, void *data)
{
  struct tree_print *tp = data;

  if (node->locus.file)
    mu_stream_sequential_printf (tp->stream, "# %d \"%s\"\n",
				 node->locus.line, node->locus.file);
  format_level (tp->stream, tp->level);
  switch (node->type)
    {
    case mu_cfg_node_undefined:
      mu_stream_sequential_printf (tp->stream, "%s",
				   _("ERROR: undefined statement"));
      break;

    case mu_cfg_node_tag:
      {
	mu_stream_sequential_write (tp->stream, node->tag_name,
				    strlen (node->tag_name));
	if (node->tag_label)
	  {
	    mu_stream_sequential_write (tp->stream, " ", 1);
	    format_label (tp, node->tag_label);
	  }
	mu_stream_sequential_write (tp->stream, " {", 2);
	tp->level++;
      }
      break;

    case mu_cfg_node_param:
      mu_stream_sequential_write (tp->stream, node->tag_name,
				  strlen (node->tag_name));
      if (node->tag_label)
	{
	  mu_stream_sequential_write (tp->stream, " ", 1);
	  format_label (tp, node->tag_label);
	  mu_stream_sequential_write (tp->stream, ";", 1);
	}
      break;
    }
  mu_stream_sequential_write (tp->stream, "\n", 1);
  return MU_CFG_ITER_OK;
}

static int
format_node_end (const mu_cfg_node_t *node, void *data)
{
  struct tree_print *tp = data;
  tp->level--;
  format_level (tp->stream, tp->level);
  mu_stream_sequential_write (tp->stream, "};\n", 3);
  return MU_CFG_ITER_OK;
}

void
mu_cfg_format_parse_tree (mu_stream_t stream, mu_cfg_tree_t *tree)
{
  struct tree_print t;
  t.level = 0;
  t.stream = stream;
  t.buf = NULL;
  t.bufsize = 0;
  mu_cfg_preorder (tree->node, format_node, format_node_end, &t);
  free (t.buf);
}


const char *
mu_cfg_data_type_string (enum mu_cfg_param_data_type type)
{
  switch (type)
    {
    case mu_cfg_string:
      return N_("string");
    case mu_cfg_short:
    case mu_cfg_ushort:
    case mu_cfg_int:
    case mu_cfg_uint:
    case mu_cfg_long:
    case mu_cfg_ulong:
    case mu_cfg_size:
    case mu_cfg_off:
      return N_("number");
    case mu_cfg_time:
      return N_("time");
    case mu_cfg_bool:
      return N_("boolean");
    case mu_cfg_ipv4:
      return N_("ipv4");
    case mu_cfg_cidr:
      return N_("cidr");
    case mu_cfg_host:
      return N_("host");
    case mu_cfg_callback:
      return N_("string");
    case mu_cfg_section:
      return N_("section");
    }
  return N_("unknown");
}

void
mu_cfg_format_docstring (mu_stream_t stream, const char *docstring, int level)
{
  size_t len = strlen (docstring);
  int width = 78 - level * 2;

  if (width < 0)
    {
      width = 78;
      level = 0;
    }
  
  while (len)
    {
      size_t seglen;
      const char *p;
      
      for (seglen = 0, p = docstring; p < docstring + width && *p; p++)
	{
	  if (*p == '\n')
	    {
	      seglen = p - docstring;
	      break;
	    }
	  if (isspace (*p))
	    seglen = p - docstring;
	}
      if (seglen == 0 || *p == 0)
	seglen = p - docstring;

      format_level (stream, level);
      mu_stream_sequential_write (stream, "# ", 2);
      mu_stream_sequential_write (stream, docstring, seglen);
      mu_stream_sequential_write (stream, "\n", 1);
      len -= seglen;
      docstring += seglen;
      if (*docstring == '\n')
	{
	  docstring++;
	  len--;
	}
      else
	while (*docstring && isspace (*docstring))
	  {
	    docstring++;
	    len--;
	  }
    }
}

static void
format_param (mu_stream_t stream, struct mu_cfg_param *param, int level)
{
  if (param->docstring)
    mu_cfg_format_docstring (stream, gettext (param->docstring), level);
  format_level (stream, level);
  mu_stream_sequential_printf (stream, "%s <%s: %s>;\n",
			       param->ident,
			       gettext (param->argname ?
					param->argname : N_("arg")),
			       gettext (mu_cfg_data_type_string (param->type)));
}

static void format_container (mu_stream_t stream, struct mu_cfg_cont *cont,
			      int level);

static int
_f_helper (void *item, void *data)
{
  struct tree_print *tp = data;
  struct mu_cfg_cont *cont = item;
  format_container (tp->stream, cont, tp->level);
  return 0;
}
  
static void
format_section (mu_stream_t stream, struct mu_cfg_section *sect, int level)
{
  struct tree_print c;
  if (sect->docstring)
    mu_cfg_format_docstring (stream, gettext (sect->docstring), level);
  format_level (stream, level);
  if (sect->ident)
    {
      mu_stream_sequential_write (stream, sect->ident, strlen (sect->ident));
      if (sect->label)
	{
	  mu_stream_sequential_write (stream, " ", 1);
	  mu_stream_sequential_write (stream, sect->label,
				      strlen (sect->label));
	}
      mu_stream_sequential_write (stream, " {\n", 3);
      c.stream = stream;
      c.level = level + 1; 
      mu_list_do (sect->children, _f_helper, &c);
      format_level (stream, level);
      mu_stream_sequential_write (stream, "};\n\n", 4);
    }
  else
    {
      c.stream = stream;
      c.level = level; 
      mu_list_do (sect->children, _f_helper, &c);
    }
}

static void
format_container (mu_stream_t stream, struct mu_cfg_cont *cont, int level)
{
  switch (cont->type)
    {
    case mu_cfg_cont_section:
      format_section (stream, &cont->v.section, level);
      break;

    case mu_cfg_cont_param:
      format_param (stream, &cont->v.param, level);
      break;
    }
}

void
mu_cfg_format_container (mu_stream_t stream, struct mu_cfg_cont *cont)
{
  format_container (stream, cont, 0);
}

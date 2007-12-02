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

struct tree_print
{
  unsigned level;
  mu_stream_t stream;
  char *buf;
  size_t bufsize;
};

static void
format_level (struct tree_print *tp)
{
  int i;
  for (i = 0; i < tp->level; i++)
    mu_stream_sequential_write (tp->stream, "  ", 2);
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
  format_level (tp);
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
  format_level (tp);
  mu_stream_sequential_write (tp->stream, "};\n", 3);
  return MU_CFG_ITER_OK;
}

void
mu_cfg_format_tree (mu_stream_t stream, mu_cfg_tree_t *tree)
{
  struct tree_print t;
  t.level = 0;
  t.stream = stream;
  t.buf = NULL;
  t.bufsize = 0;
  mu_cfg_preorder (tree->node, format_node, format_node_end, &t);
  free (t.buf);
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailutils/mailutils.h>

#define S(str) ((str) ? (str) : "")

static void
print_param (const char *prefix, mu_assoc_t assoc, int indent)
{
  mu_iterator_t itr;
  int i;

  mu_printf ("%*s%s:\n", indent, "", prefix);
  if (!assoc)
    return;
  indent += 4;
  MU_ASSERT (mu_assoc_get_iterator (assoc, &itr));
      
  for (i = 0, mu_iterator_first (itr);
       !mu_iterator_is_done (itr);
       i++, mu_iterator_next (itr))
    {
      const char *name;
      struct mu_mime_param *p;
      
      mu_iterator_current_kv (itr, (const void **)&name, (void**)&p);
      mu_printf ("%*s%d: %s=%s\n", indent, "", i, name, p->value);
    }
  mu_iterator_destroy (&itr);
}

struct print_data
{
  int num;
  int level;
};

static void print_bs (struct mu_bodystructure *bs, int level);

static int
print_item (void *item, void *data)
{
  struct mu_bodystructure *bs = item;
  struct print_data *pd = data;
  mu_printf ("%*sPart #%d\n", (pd->level-1) << 2, "", pd->num);
  print_bs (bs, pd->level);
  ++pd->num;
  return 0;
}

static void
print_address (const char *title, mu_address_t addr, int indent)
{
  mu_printf ("%*s%s: ", indent, "", title);
  mu_stream_format_address (mu_strout, addr);
  mu_printf ("\n");
}

static void
print_imapenvelope (struct mu_imapenvelope *env, int level)
{
  int indent = (level << 2);

  mu_printf ("%*sEnvelope:\n", indent, "");
  indent += 4;
  mu_printf ("%*sTime: ", indent, "");
  mu_c_streamftime (mu_strout, "%c%n", &env->date, &env->tz);
  mu_printf ("%*sSubject: %s\n", indent, "", S(env->subject));
  print_address ("From", env->from, indent);
  print_address ("Sender", env->sender, indent);
  print_address ("Reply-to", env->reply_to, indent);
  print_address ("To", env->to, indent);
  print_address ("Cc", env->cc, indent);
  print_address ("Bcc", env->bcc, indent);
  mu_printf ("%*sIn-Reply-To: %s\n", indent, "", S(env->in_reply_to));
  mu_printf ("%*sMessage-ID: %s\n", indent, "", S(env->message_id));
}

static void
print_bs (struct mu_bodystructure *bs, int level)
{
  int indent = level << 2;
  mu_printf ("%*sbody_type=%s\n", indent, "", S(bs->body_type));
  mu_printf ("%*sbody_subtype=%s\n", indent, "", S(bs->body_subtype));
  print_param ("Parameters", bs->body_param, indent);
  mu_printf ("%*sbody_id=%s\n", indent, "", S(bs->body_id));
  mu_printf ("%*sbody_descr=%s\n", indent, "", S(bs->body_descr));
  mu_printf ("%*sbody_encoding=%s\n", indent, "", S(bs->body_encoding));
  mu_printf ("%*sbody_size=%lu\n", indent, "", (unsigned long) bs->body_size);
  /* Optional */
  mu_printf ("%*sbody_md5=%s\n", indent, "", S(bs->body_md5));
  mu_printf ("%*sbody_disposition=%s\n", indent, "", S(bs->body_disposition));
  print_param ("Disposition Parameters", bs->body_disp_param, indent);
  mu_printf ("%*sbody_language=%s\n", indent, "", S(bs->body_language));
  mu_printf ("%*sbody_location=%s\n", indent, "", S(bs->body_location));

  mu_printf ("%*sType ", indent, "");
  switch (bs->body_message_type)
    {
    case mu_message_other:
      mu_printf ("mu_message_other\n");
      break;
      
    case mu_message_text:
      mu_printf ("mu_message_text:\n%*sbody_lines=%lu\n", indent + 4, "",
		 (unsigned long) bs->v.text.body_lines);
      break;
      
    case mu_message_rfc822:
      mu_printf ("mu_message_rfc822:\n%*sbody_lines=%lu\n", indent + 4, "",
		 (unsigned long) bs->v.rfc822.body_lines);
      print_imapenvelope (bs->v.rfc822.body_env, level + 1);
      print_bs (bs->v.rfc822.body_struct, level + 1);
      break;
      
    case mu_message_multipart:
      {
	struct print_data pd;
	pd.num = 0;
	pd.level = level + 1;
	mu_printf ("mu_message_multipart:\n");
	mu_list_foreach (bs->v.multipart.body_parts, print_item, &pd);
      }
    }
}

int
main (int argc, char **argv)
{
  mu_mailbox_t mbox;
  mu_message_t mesg;
  struct mu_bodystructure *bs;
  
  if (argc != 3)
    {
      fprintf (stderr, "usage: %s URL NUM\n", argv[0]);
      return 1;
    }

  mu_register_all_mbox_formats ();
  MU_ASSERT (mu_mailbox_create (&mbox, argv[1]));
  MU_ASSERT (mu_mailbox_open (mbox, MU_STREAM_READ));
  MU_ASSERT (mu_mailbox_get_message (mbox, atoi (argv[2]), &mesg));
  MU_ASSERT (mu_message_get_bodystructure (mesg, &bs));
  print_bs (bs, 0);
  mu_bodystructure_free (bs);
  
  return 0;
}

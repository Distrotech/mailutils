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

typedef enum
{
  node_and,
  node_or,
  node_not,
  node_regex,
  node_datefield
} node_type;
  
typedef struct node node_t;

struct node
{
  node_type type;
  union
  {
    struct
    {
      node_t *larg;
      node_t *rarg;
    } op;
    struct
    {
      char *comp;
      regex_t *regex;
    } re;
    struct
    {
      char *datefield;
    } df;
    struct
    {
      void *a;
      void *b;
    } gen;
  } v;
};

void pick_add_token __P((list_t *list, int tok, char *val));
int pick_parse __P((list_t toklist));
int pick_eval __P((message_t msg));

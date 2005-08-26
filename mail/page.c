/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005 Free Software Foundation, Inc.

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

#include "mail.h"

size_t top_of_page = 1; /* Number of the topmost message on the page */
size_t cursor;          /* Number of current message */

size_t *page_map;       /* Array of message numbers. page_map[N] holds
			   number of the message occupying Nth line on
			   the screen */
unsigned page_size;     /* Capacity of page_map */
unsigned page_avail;     /* First non-used entry in page map. Can be
			   equal to page_size */

/* Auxiliary function: Store number of message from mspec into page_map */
static int
_fill_map (msgset_t *mspec, message_t msg, void *data)
{
  unsigned *pos = data;
  page_map[*pos] = mspec->msg_part[0];
  ++*pos;
  return 0;
}

/* Fill page_map.
   page_avail must be set to zero before calling this function */
static void
fill_page_map ()
{
  util_range_msg (top_of_page, page_size,
		  MSG_COUNT|MSG_NODELETED|MSG_SILENT, _fill_map,
		  &page_avail);
}

/* Check if the page_map is valid. If not, fill it.
   Page_avail can be set to zero to force re-filling page_map. In particular
   this happens when deleting current message or when SIGWINCH is delivered
   to the program */
static void
check_page_map ()
{
  if (!page_map)
    {
      page_size = util_screen_lines ();
      page_map = xrealloc (page_map, sizeof (page_map[0]) * page_size);
      page_avail = 0;
    }
  if (page_avail == 0)
    fill_page_map ();
}  

/* Invalidate page_map. HARD=1 means 'hard invalidation', implying
   the need to reallocate the array */
void
page_invalidate (int hard)
{
  page_avail = 0;
  if (hard)
    {
      free (page_map);
      page_map = NULL;
    }
}

/* Invalidate page_map if VALUE is number of the current message */
void
cond_page_invalidate (size_t value)
{
  unsigned i;

  if (page_map == NULL || page_avail == 0)
    return;
  for (i = 0; i < page_avail; i++)
    if (page_map[i] >= value && value <= page_map[i+1])
      {
	page_invalidate (0);
	return;
      }
}

/* Return a 1-based index of page_map entry occupied by number VALUE.
   Return 0 if VALUE is not found in page_map */
static int
page_line (size_t value)
{
  unsigned i;

  for (i = 0; i < page_avail; i++)
    if (page_map[i] == value)
      return i+1;
  return 0;
}

/* Return number of the current message.
   Zero is returned if page_map is empty (i.e. mailbox is empy) */
size_t
get_cursor ()
{
  check_page_map ();
  if (page_avail == 0)
    return 0;
  return page_map[cursor];
}

/* Move cursor to message number VALUE */
void
set_cursor (unsigned value)
{
  int n;
  
  if (total == 0)
    {
      cursor = 0;
      return;
    }
  
  check_page_map ();
  n = page_line (value);
  if (n == 0)
    {
      top_of_page = value;
      cursor = 0;
      page_avail = 0;
      page_move (0);
    }
  else
    cursor = n - 1;
}

/* Return T if the cursor points to message number N */
int
is_current_message (size_t n)
{
  check_page_map ();
  return page_map[cursor] == n;
}

/* Apply function FUNC to each message from the page. DATA supplies
   call-specific data to the function. */
void
page_do (msg_handler_t func, void *data)
{
  unsigned i;
  
  check_page_map ();
  for (i = 0; i < page_avail; i++)
    {
      message_t msg;
      msgset_t set;
      
      set.next = NULL;
      set.npart = 1;
      set.msg_part = page_map + i;
      mu_mailbox_get_message (mbox, page_map[i], &msg);
      func (&set, msg, data);
    }
}

/* Move current page OFFSET lines forward, if OFFSET > 0, or backward,
   if OFFSET < 0.
   Return number of items that will be displayed */
size_t
page_move (off_t offset)
{
  size_t start;
  size_t count = 0;
  
  check_page_map ();

  if (offset < 0 && -offset > page_map[0])
    start = 1;
  else
    start = page_map[0] + offset;
  
  util_range_msg (start, page_size,
		  MSG_COUNT|MSG_NODELETED|MSG_SILENT, _fill_map, &count);
  
  if (offset < 0 && top_of_page == page_map[0])
    {
      page_avail = count;
      return 0;
    }
  
  if (count)
    {
      top_of_page = page_map[0];

      if (count < page_size && top_of_page > 1)
	{
	  for (start = top_of_page - 1; count < page_size && start > 1;
	       start--)
	    {
	      if (!util_isdeleted (start))
		{
		  top_of_page = start;
		  cursor++;
		  count++;
		}
	    }
	  page_avail = 0;
	  fill_page_map ();
	}
      else
	page_avail = count;
    }
  return count;
}

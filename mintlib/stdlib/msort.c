/* An alternative to qsort, with an identical interface.
   This file is part of the GNU C Library.
   Copyright (C) 1992, 1995, 1996, 1997 Free Software Foundation, Inc.
   Written by Mike Haertel, September 1988.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <stdlib.h>
#include <string.h>
#include <memcopy.h>
#include <errno.h>

static void msort_with_tmp __P ((void *b, size_t n, size_t s,
				 __compar_fn_t cmp, char *t));

static void
msort_with_tmp (b, n, s, cmp, t)
     void *b;
     size_t n;
     size_t s;
     __compar_fn_t cmp;
     char *t;
{
  char *tmp;
  char *b1, *b2;
  size_t n1, n2;

  if (n <= 1)
    return;

  n1 = n / 2;
  n2 = n - n1;
  b1 = b;
  b2 = (char *) b + (n1 * s);

  msort_with_tmp (b1, n1, s, cmp, t);
  msort_with_tmp (b2, n2, s, cmp, t);

  tmp = t;

  if (s == OPSIZ && (b1 - (char *) 0) % OPSIZ == 0)
    /* We are operating on aligned words.  Use direct word stores.  */
    while (n1 > 0 && n2 > 0)
      {
	if ((*cmp) (b1, b2) <= 0)
	  {
	    --n1;
	    *((op_t *) tmp)++ = *((op_t *) b1)++;
	  }
	else
	  {
	    --n2;
	    *((op_t *) tmp)++ = *((op_t *) b2)++;
	  }
      }
  else
    while (n1 > 0 && n2 > 0)
      {
	if ((*cmp) (b1, b2) <= 0)
	  {
	    tmp = (char *) __mempcpy (tmp, b1, s);
	    b1 += s;
	    --n1;
	  }
	else
	  {
	    tmp = (char *) __mempcpy (tmp, b2, s);
	    b2 += s;
	    --n2;
	  }
      }
  if (n1 > 0)
    memcpy (tmp, b1, n1 * s);
  memcpy (b, t, (n - n2) * s);
}

void
qsort (b, n, s, cmp)
     void *b;
     size_t n;
     size_t s;
     __compar_fn_t cmp;
{
  const size_t size = n * s;

  if (size < 1024)
    /* The temporary array is small, so put it on the stack.  */
    msort_with_tmp (b, n, s, cmp, __alloca (size));
  else
    {
      /* It's somewhat large, so malloc it.  */
      int save = errno;
      char *tmp = malloc (size);
      if (tmp == NULL)
	{
	  /* Couldn't get space, so use the slower algorithm
	     that doesn't need a temporary array.  */
	  extern void _quicksort __P ((void *const __base,
				       size_t __nmemb, size_t __size,
				       __compar_fn_t __compar));
	  _quicksort (b, n, s, cmp);
	}
      else
	{
	  msort_with_tmp (b, n, s, cmp, tmp);
	  free (tmp);
	}
      __set_errno (save);
    }
}

/* Copyright (C) 1991, 1992, 1996, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#include <stdio.h>
#include <shadow.h>

#define _S(x)	x ? x : ""

/* Write an entry to the given stream.
   This must know the format of the password file.  */
int
putspent (const struct spwd *p, FILE *stream)
{
  int errors = 0;

  if (fprintf (stream, "%s:%s:", p->sp_namp, _S(p->sp_pwdp)) < 0)
    ++errors;

  if ((p->sp_lstchg != (long int) -1
       && fprintf (stream, "%ld:", p->sp_lstchg) < 0)
      || (p->sp_lstchg == (long int) -1
	  && putc (':', stream) == EOF))
    ++errors;

  if ((p->sp_min != (long int) -1
       && fprintf (stream, "%ld:", p->sp_min) < 0)
      || (p->sp_min == (long int) -1
	  && putc (':', stream) == EOF))
    ++errors;

  if ((p->sp_max != (long int) -1
       && fprintf (stream, "%ld:", p->sp_max) < 0)
      || (p->sp_max == (long int) -1
	  && putc (':', stream) == EOF))
    ++errors;

  if ((p->sp_warn != (long int) -1
       && fprintf (stream, "%ld:", p->sp_warn) < 0)
      || (p->sp_warn == (long int) -1
	  && putc (':', stream) == EOF))
    ++errors;

  if ((p->sp_inact != (long int) -1
       && fprintf (stream, "%ld:", p->sp_inact) < 0)
      || (p->sp_inact == (long int) -1
	  && putc (':', stream) == EOF))
    ++errors;

  if ((p->sp_expire != (long int) -1
       && fprintf (stream, "%ld:", p->sp_expire) < 0)
      || (p->sp_expire == (long int) -1
	  && putc (':', stream) == EOF))
    ++errors;

  if (p->sp_flag != ~0ul
      && fprintf (stream, "%ld", p->sp_flag) < 0)
    ++errors;

  if (putc ('\n', stream) == EOF)
    ++errors;

  return errors ? -1 : 0;
}

/* Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, August 1995.

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

/* Modified by Guido Flohr <gufl0000@stud.uni-sb.de> for MiNTLib.  */

#include <stdlib.h>

/* Global state for non-reentrant functions.  Defined in drand48-iter.c.  */
extern struct drand48_data __libc_drand48_data;

__EXTERN int __nrand48_r __PROTO ((unsigned short int __xsubi[3],
                                   struct drand48_data*, long int*));

long int
nrand48 (xsubi)
     unsigned short int xsubi[3];
{
  long int result;

  (void) __nrand48_r (xsubi, &__libc_drand48_data, &result);

  return result;
}

/* Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
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

/* Modified for MiNTLib by Guido Flohr <guido@freemint.de.  */

#include <errno.h>
#include <bits/libc-lock.h>
#include <shadow.h>
#include <stdlib.h>

__EXTERN int __fgetspent_r __PROTO ((FILE*, struct spwd*, char*, size_t,
				     struct spwd**));

/* A reasonable size for a buffer to start with.  */
#define BUFLEN_SPWD 1024

/* We need to protect the dynamic buffer handling.  */
__libc_lock_define_initialized (static, lock);

/* Read one shadow entry from the given stream.  */
struct spwd *
fgetspent (FILE *stream)
{
  static char *buffer;
  static size_t buffer_size;
  static struct spwd resbuf;
  struct spwd *result;
  int save, save_errno;

  /* Get lock.  */
  __libc_lock_lock (lock);

  /* Allocate buffer if not yet available.  */
  if (buffer == NULL)
    {
      buffer_size = BUFLEN_SPWD;
      buffer = malloc (buffer_size);
    }

  /* We don't want to pass errno == 0 or errno == ERANGE back */
  save_errno = errno;
  while (buffer != NULL
	 && __fgetspent_r (stream, &resbuf, buffer, buffer_size, &result) != 0
	 && errno == ERANGE)
    {
      char *new_buf;
      buffer_size += BUFLEN_SPWD;
      __set_errno (0);
      new_buf = realloc (buffer, buffer_size);
      if (new_buf == NULL)
	{
	  /* We are out of memory.  Free the current buffer so that the
	     process gets a chance for a normal termination.  */
	  save = errno;
	  free (buffer);
	  __set_errno (save);
	}
      buffer = new_buf;
    }

  if (errno == 0)
    __set_errno (save_errno);

  if (buffer == NULL)
    result = NULL;

  /* Release lock.  Preserve error value.  */
  save = errno;
  __libc_lock_unlock (lock);
  __set_errno (save);

  return result;
}

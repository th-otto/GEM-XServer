/* Copyright (C) 1991, 92, 93, 94, 95, 96, 97 Free Software Foundation, Inc.
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

/* Heavily modified for MiNTLib by Guido Flohr <guido@freemint.de>:
   - Implemented that stupid text mode.  */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

extern __io_read_fn __stdio_read;
extern __io_write_fn __stdio_write;
extern __io_read_fn __stdio_text_read;
extern __io_write_fn __stdio_text_write;
extern __io_seek_fn __stdio_seek;
extern __io_close_fn __stdio_close;
extern __io_fileno_fn __stdio_fileno;

__EXTERN ssize_t __read	__PROTO ((int, void*, size_t));
__EXTERN off_t __lseek __PROTO ((int, long, int));
__EXTERN ssize_t __write __PROTO ((int, const void*, size_t));
__EXTERN int __close __PROTO ((int));
__EXTERN int __open __PROTO ((const char*, int, ...));
__EXTERN int __dup2 __PROTO ((int, int));

/* Read N bytes into BUF from COOKIE.  */
ssize_t
__stdio_read (void *cookie, char *buf, size_t n)
{
#ifndef __MSHORT__
  const int fd = (int) cookie;
#else
  const long int fd = (long int) cookie;
#endif
#if defined EINTR && defined EINTR_REPEAT
  int save = errno;
  ssize_t nread;

 try:;
  __set_errno (0);
  nread = __read (fd, buf, (int) n);
  if (nread < 0)
    {
      if (errno == EINTR)
	goto try;
      return -1;
    }
  __set_errno (save);
  return nread;

#else	/* No EINTR.  */
  return __read (fd, buf, n);
#endif
}

/* Read in so-called textmode.  */
ssize_t
__stdio_text_read (void* cookie, char* buf, size_t n)
{
#ifndef __MSHORT__
  const int fd = (int) cookie;
#else
  const long int fd = (long int) cookie;
#endif

#if 0
/* old implementation */

  int save = errno;
  ssize_t nread = 0;
  char* bufptr = buf;

  while (nread < n) 
    {
      ssize_t read_bytes = __read (fd, bufptr, (int) (n - nread));
      ssize_t i;
      ssize_t skipped = 0;

      if (read_bytes < 0) 
        {
          if (nread == 0)
            return -1;
          else
            break;
        }
      else if (read_bytes == 0)
        break;
      
      /* Now squeeze '\r' characters out of our buffer.  */
      for (i = 0; i < read_bytes; i++)
        {
          if (bufptr[i] == '\r')
            {
              skipped++;
              continue;
            }
          bufptr[i - skipped] = bufptr[i];
        }
      
      read_bytes -= skipped;
      bufptr += read_bytes;
      nread += read_bytes;
    }
    
    __set_errno (save);
    return nread;
#else
/* new implementation from jens */

/* Completly recoded, but I'm not really sure if it is a good idea.
 * But now it is working more compatible to the binary version
 * 'stdio_read' of it, and fgets is now working also on standard
 * TOS systems. fgetc is no longer skipping <CR> keys .... but I'm
 * not sure what may happen in other library functions.
 */
	
  int save = errno;
  ssize_t read_bytes = __read (fd, buf, (int) (n));
  ssize_t i;

  if (read_bytes < 0)
    return -1;

  /* Now squeeze '\r' characters out of our buffer.  */
  for (i = 0; i < read_bytes; i++)
    {
      if (buf[i] == '\r')
	buf[i] = '\n';
    }

  __set_errno (save);
  return read_bytes;
#endif
}

/* Write N bytes from BUF to COOKIE.  */
ssize_t
__stdio_write (void *cookie, const char *buf, size_t n)
{
#ifndef __MSHORT__
  const int fd = (int) cookie;
#else
  const long int fd = (long int) cookie;
#endif
  register size_t written = 0;

  while (n > 0)
    {
      ssize_t count = __write (fd, buf, (ssize_t) n);
      if (count > 0)
	{
	  buf += count;
	  written += count;
	  n -= count;
	}
      else if (count < 0
#if defined EINTR && defined EINTR_REPEAT
	       && errno != EINTR
#endif
	       )
	/* Write error.  */
	return -1;
    }

  return (ssize_t) written;
}

/* Write N bytes from BUF to COOKIE in so-called text mode.  */
ssize_t
__stdio_text_write (void *cookie, const char *buf, size_t n)
{
#ifndef __MSHORT__
  const int fd = (int) cookie;
#else
  const long int fd = (long int) cookie;
#endif
  register size_t written = 0;
  ssize_t blocksize = 8192;
  struct stat statbuf;
  int save = errno;
  char* chunk;

  if (fstat (fd, &statbuf) == 0) 
    {
      blocksize = (ssize_t) statbuf.st_blksize;
      if (blocksize > 8192 || blocksize <= 0)
        blocksize = 8192;
    }
  
  __set_errno (save);

  /* The buffer is one byte bigger than actually needed for the case of
     a linefeed as the last char in the current chunk.  */  
  chunk = alloca (blocksize + 1);
  
  while (n > 0)
    {
      ssize_t count;
      register ssize_t put;
      register ssize_t got;
      char* writeptr = chunk;
      ssize_t n1;
      ssize_t limit = blocksize > n ? n : blocksize;
      
      for (put = 0, got = 0; put < limit && got < n; put++, got++)
        {
          if (buf[got] == '\n')
            chunk[put++] = '\r';
          chunk[put] = buf[got];
        }

      n1 = put;
      while (n1 > 0)
        {
          count = __write (fd, writeptr, n1);
          if (count > 0)
            {
              writeptr += count;
              n1 -= count;
            }
          else if (count < 0
#if defined EINTR && defined EINTR_REPEAT
                   && errno != EINTR
#endif
                   )
            /* Write error.  */
            return -1;
        }
      
      buf += got;
      written += got;
      n -= got;      
    }

  return (ssize_t) written;
}

/* Move COOKIE's file position *POS bytes, according to WHENCE.
   The new file position is stored in *POS.
   Returns zero if successful, nonzero if not.  */
int
__stdio_seek (void *cookie, fpos_t *pos, int whence)
{
  off_t new;
#ifndef __MSHORT__
  new = __lseek ((int) cookie, (off_t) *pos, whence);
#else
  new = __lseek ((long int) cookie, (off_t) * pos, whence);
#endif
  if (new < 0)
    return 1;
  *pos = (fpos_t) new;
  return 0;
}


/* Close COOKIE.  */
int
__stdio_close (void *cookie)
{
  return __close ((int) ((long) cookie));
}

/* Return the POSIX.1 file descriptor associated with COOKIE,
   or -1 for errors.  If COOKIE does not relate to any POSIX.1 file
   descriptor, this should return -1 with errno set to EOPNOTSUPP.  */
int
__stdio_fileno (void *cookie)
{
  return (int) ((long) cookie);
}


/* Open the given file with the mode given in the __io_mode argument.  */
int
__stdio_open (const char *filename, __io_mode m, void **cookieptr)
{
  int fd;
  int mode;

  if (m.__read && m.__write)
    mode = O_RDWR;
  else
    mode = m.__read ? O_RDONLY : O_WRONLY;

  if (m.__append)
    mode |= O_APPEND;
  if (m.__exclusive)
    mode |= O_EXCL;
  if (m.__truncate)
    mode |= O_TRUNC;

  if (m.__create)
    fd = __open (filename, mode | O_CREAT,
		 S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  else
    fd = __open (filename, mode);

  if (fd < 0)
    return -1;

  *cookieptr = (void *) ((long) fd);
  return 0;
}


/* Open FILENAME with the mode in M.  Use the same magic cookie
   already in *COOKIEPTR if possible, closing the old cookie with CLOSEFN.  */
int
__stdio_reopen (const char *filename, __io_mode m, void **cookieptr,
		__io_close_fn closefn)
{
  void *newcookie;

  /* We leave the old descriptor open while we open the file.
     That way ``freopen ("/dev/stdin", "r", stdin)'' works.  */

  if (__stdio_open (filename, m, &newcookie))
    {
      if (errno == ENFILE || errno == EMFILE)
	{
	  /* We are out of file descriptors.  Try closing the old one and
	     retrying the open.  */
	  (void) (*closefn) (*cookieptr);
	  if (__stdio_open (filename, m, &newcookie))
	    return -1;
	}
      else
	return -1;
    }

  if (newcookie != *cookieptr)
    {
      if (closefn != __stdio_close ||
	  /* Try to move the descriptor to the desired one.  */
	  __dup2 ((int) ((long) newcookie), (int) ((long) *cookieptr)) < 0)
	/* Didn't work.  Give the caller the new cookie.  */
	*cookieptr = newcookie;
    }

  return 0;
}

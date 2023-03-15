/*  times.c -- MiNTLib.
    Copyright (C) 1999 Guido Flohr <gufl0000@stud.uni-sb.de>

    This file is part of the MiNTLib project, and may only be used
    modified and distributed under the terms of the MiNTLib project
    license, COPYMINT.  By continuing to use, modify, or distribute
    this file you indicate that you have read the license and
    understand and accept it fully.
*/

#include <time.h>

#ifdef __TURBOC__
# include <sys\times.h>
#else
# include <sys/times.h>
#endif

#include <errno.h>
#include <mintbind.h>

extern int __mint;
extern long _childtime;
extern int errno;

clock_t 
__times (buf)
  struct tms* buf;
{
  clock_t retval = clock ();
  
  if (!buf) {
    __set_errno (EFAULT);
    return ((clock_t) -1);
  }
  
  if (__mint != 0) {
    long int r[8];
    (void) Prusage (r);
    buf->tms_utime = (r[1] * CLOCKS_PER_SEC) / 1000;
    buf->tms_stime = (r[0] * CLOCKS_PER_SEC) / 1000;
    buf->tms_cutime = (r[3] * CLOCKS_PER_SEC) / 1000;
    buf->tms_cstime = (r[2] * CLOCKS_PER_SEC) / 1000;
  } else {
    buf->tms_stime = buf->tms_cstime = 0;
    buf->tms_cutime = retval;
    buf->tms_utime = retval - _childtime;
  }
  return retval;
}

weak_alias (__times, times)

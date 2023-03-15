/*  time.c -- MiNTLib.
    Copyright (C) 1999 Guido Flohr <gufl0000@stud.uni-sb.de>

    This file is part of the MiNTLib project, and may only be used
    modified and distributed under the terms of the MiNTLib project
    license, COPYMINT.  By continuing to use, modify, or distribute
    this file you indicate that you have read the license and
    understand and accept it fully.
*/

#include <time.h>

#ifdef __TURBOC__
# include <sys/time.h>
#else
# include <sys/time.h>
#endif

time_t 
time (time_t* buf)
{
  struct timeval now;
  
  if (gettimeofday (&now, NULL) != 0)
    return ((time_t) -1);
  
  if (buf)
    *buf = now.tv_sec;
  return now.tv_sec;
}

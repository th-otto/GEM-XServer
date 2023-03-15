/*  src/umask.c -- MiNTLib.
    Copyright (C) 1999 Guido Flohr <gufl0000@stud.uni-sb.de>

    This file is part of the MiNTLib project, and may only be used
    modified and distributed under the terms of the MiNTLib project
    license, COPYMINT.  By continuing to use, modify, or distribute
    this file you indicate that you have read the license and
    understand and accept it fully.
*/

/* umask -- change default file creation mask.  */

#ifdef __TURBOC__
# include <sys\stat.h>
#else
# include <sys/stat.h>
#endif

#include <errno.h>
#include <mintbind.h>

extern int __current_umask;  /* Sigh, in open.c.  */
static void _get_umask ();

/*
 * function to set the initial value of __current_umask
 */

static void
_get_umask()
{
  __current_umask = Pumask (0);
  if (__current_umask < 0)
    __current_umask = 0;
}

int __umask (complmode)
     int complmode;
{
  int old_umask;
  int retval;

  if (__current_umask == -1)
    _get_umask ();
  old_umask = __current_umask;
  __current_umask = complmode;

  retval = Pumask (complmode);
  if (retval < 0 && retval != -ENOSYS)
    {
      __current_umask = old_umask;
      __set_errno (-retval);
      return -1;
    }
  return old_umask;
}

weak_alias (__umask, umask)

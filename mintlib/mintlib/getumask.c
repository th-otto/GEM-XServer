/*  src/getumask.c -- MiNTLib.
    Copyright (C) 1999, 2000 Guido Flohr <guido@freemint.de>

    This file is part of the MiNTLib project, and may only be used
    modified and distributed under the terms of the MiNTLib project
    license, COPYMINT.  By continuing to use, modify, or distribute
    this file you indicate that you have read the license and
    understand and accept it fully.
*/

#include <sys/stat.h>
#include <mintbind.h>

extern int __current_umask;

mode_t
__getumask ()
{
  /* The only possible failure for Pumask is ENOSYS.  */
  mode_t old_umask = Pumask (0);
  
  if (old_umask >= 0)
  	(void) Pumask (old_umask);
  else
  	old_umask = __current_umask;

  return old_umask;
}
weak_alias (__getumask, getumask)

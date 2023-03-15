/*  sys/time.h -- MiNTLib.
    Copyright (C) 1999 Guido Flohr <gufl0000@stud.uni-sb.de>

    This file is part of the MiNTLib project, and may only be used
    modified and distributed under the terms of the MiNTLib project
    license, COPYMINT.  By continuing to use, modify, or distribute
    this file you indicate that you have read the license and
    understand and accept it fully.
*/

#ifndef	_SYS_UTSNAME_H
#define	_SYS_UTSNAME_H 1

#ifndef _COMPILER_H
# include <compiler.h>
#endif

#ifdef __TURBOC__
# include <sys\param.h>
#else
# include <sys/param.h>
#endif

__BEGIN_DECLS

/* The sizes of the arrays here must be coordinated with the array
 * sizes in sysinfo.c.
 */
struct utsname {
  char sysname[9];                    /* sysinfo (SI_SYSNAME, ...).  */
  char nodename[MAXHOSTNAMELEN + 1];  /* sysinfo (SI_HOSTNAME, ...).  */
  char release[13];                   /* sysinfo (SI_RELEASE, ...).  */
  char version[22];                   /* sysinfo (SI_VERSION, ...).  */
  char machine[14];                   /* sysinfo (SI_PLATFORM, ...).  */

#if 0  
  /* Non-portable extension.  Since the GNU sh-utils write such
     a field for the uname command it doesn't seem to be such a bad
     idea.  */
  char architecture[8];               /* sysinfo (SI_ARCHITECTURE, ...).  */
#endif
};

/* Fill INFO with the system information obtained via sysinfo.  */
__EXTERN int uname __PROTO ((struct utsname* info));

__END_DECLS

#endif /* SYS_UTSNAME_H */

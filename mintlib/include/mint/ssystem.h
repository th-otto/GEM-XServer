/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _ssystem_h
# define _ssystem_h

#ifdef __KERNEL
# include "include/mint.h"
#endif

# define HAS_SSYSTEM
# define S_OSNAME	0
# define S_OSXNAME	1
# define S_OSVERSION	2
# define S_OSHEADER	3
# define S_OSBUILDDATE	4
# define S_OSBUILDTIME	5
# define S_OSCOMPILE	6
# define S_OSFEATURES	7
# define S_GETCOOKIE	8
# define S_SETCOOKIE	9
# define S_GETLVAL	10
# define S_GETWVAL	11
# define S_GETBVAL	12
# define S_SETLVAL	13
# define S_SETWVAL	14
# define S_SETBVAL	15
# define S_SECLEVEL	16
# define S_RUNLEVEL	17	/* currently disabled, reserved */
# define S_TSLICE	18
# define S_FASTLOAD	19
# define S_SYNCTIME	20
# define S_BLOCKCACHE	21
# define S_FLUSHCACHE	22
# define S_CTRLCACHE	23
# define S_CTRLALTDEL	25
# define S_CLOCKUTC	100
# define S_TIOCMGET	0x54f8	/* 21752 */

/* experimental - need feedback
 * additional informations about the kernel
 * reserved 900 - 999
 */
# define S_KNAME	900	/* kernel name - arg1 pointer to a buffer of arg2 len */
# define S_CNAME	910	/* compiler name - arg1 pointer to a buffer of arg2 len */
# define S_CVERSION	911	/* compiler version - arg1 pointer to a buffer of arg2 len */
# define S_CDEFINES	912	/* compiler definitions - arg1 pointer to a buffer of arg2 len */
# define S_COPTIM	913	/* compiler flags - arg1 pointer to a buffer of arg2 len */

/* debug section
 * reserved 1000 - 1999
 */
# define S_DEBUGLEVEL	1000	/* debug level */
# define S_DEBUGDEVICE	1001	/* BIOS device number */


# ifdef __KERNEL__
long _cdecl s_system (int mode, ulong arg1, ulong arg2);
# endif


# endif /* _ssystem_h */


#ifndef _SYS_DIRENT_H
# error "Never use <bits/dirent.h> directly; include <dirent.h> instead."
#endif

#ifndef _LIB_NAME_MAX
# define _LIB_NAME_MAX NAME_MAX
#endif

struct dirent {
       long            d_ino;          /* garbage under TOS */
       off_t           d_off;          /* position in directory  */
       unsigned short  d_namlen;       /* for us, length of d_name */
       char            d_name[NAME_MAX+1];
};

#define __DIRENTSIZ(x) (sizeof(struct dirent))

#ifndef _OSTRUCT_H
# include <mint/ostruct.h>
#endif

struct __dirstream {
	short	status;		/* status of the search so far: */
#define _INSEARCH	0	/* need to call Fsnext for a new entry */
#define _STARTSEARCH	1	/* Fsfirst called once, successfully */
#define _NMFILE		2	/* no more files in directory */
	_DTA	dta;		/* TOS DTA for this directory */
	char	*dirname;	/* directory of the search (used under
				   TOS for rewinddir) */
	struct dirent buf;	/* dirent struct for this directory */
	long	handle;		/* Dreaddir handle */
};

#define d_fileno	d_ino	/* Backwards compatibility.  */

#undef _DIRENT_HAVE_D_TYPE
#undef _DIRENT_HAVE_D_RECLEN

#define _DIRENT_HAVE_D_NAMLEN
#define _DIRENT_HAVE_D_OFF

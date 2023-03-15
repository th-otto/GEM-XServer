/* from the original GCC TOS library by jrd */
/* this algorithm is due to Allan Pratt @ Atari.  Thanks Allan! */

#include <fcntl.h>

#ifdef __TURBOC__
# include <sys\ioctl.h>
#else
# include <sys/ioctl.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <mintbind.h>
#include <errno.h>
#include "lib.h"

struct __open_file __open_stat[__NHANDLES];

int
__isatty(fd)
  int fd;
{
  int rc, retval;
  long oldloc;
  int handle = __OPEN_INDEX(fd);
  long dummy;

  if (handle < __NHANDLES)
	if (__open_stat[handle].status != FH_UNKNOWN)
		return(__open_stat[handle].status == FH_ISATTY);
  
 /* save 1 or 2 system calls (isatty gets called on every open...) */
	
  retval = Fcntl(fd, &dummy, TIOCGPGRP);
  if (retval == -ENOSYS) {
    oldloc = Fseek(0L, fd, SEEK_CUR);	/* save current location */
    if (Fseek(1L, fd, SEEK_CUR) != 0) {	/* try to seek ahead one byte */
      /* got either a file position or an error (usually EBADARG indicating
	 a range error from trying to seek past EOF), so it is not a tty */
      rc = 0;
      (void) Fseek(oldloc, fd, SEEK_SET);/* seek back to original location */
    }
    else  {
      rc = 1;				/* yes, tty */
    }
  }
  else {
    rc = (retval == 0);
  }
  if (handle < __NHANDLES) {
	if (rc) {
		__open_stat[handle].status = FH_ISATTY;
		__open_stat[handle].flags = CRMOD|ECHO;
	}
	else {
		__open_stat[handle].status = FH_ISAFILE;
	}
  }
  return (rc);			/* return true, false, or error */
}
weak_alias (__isatty, isatty)

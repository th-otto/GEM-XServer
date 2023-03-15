/*
Public domain termios tcflow() for the MiNT library
10 October 1993 entropy@terminator.rs.itd.umich.edu -- first attempt
*/

#include <mintbind.h>
#include <errno.h>

#ifdef __TURBOC__
# include <sys\ioctl.h>
# include <sys\types.h>
# include <sys\termios.h>
#else
# include <sys/ioctl.h>
# include <sys/types.h>
# include <sys/termios.h>
#endif

int
tcflow(fd, action)
  int fd;
  int action;
{
  long r;

  switch (action)
  {
    case TCOOFF:
      r = Fcntl((short) fd, (long) 0, TIOCSTOP);
      break;
    case TCOON:
      r = Fcntl((short) fd, (long) 0, TIOCSTART);
      break;
    case TCIOFF:
      r = -EINVAL;
      break;
    case TCION:
      r = -EINVAL;
      break;
    default:
      r = -EINVAL;
      break;
  }
  if (r < 0) {
    __set_errno ((int) -r);
    return -1;
  }
  return 0;
}

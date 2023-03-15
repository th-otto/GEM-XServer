/* from Dale Schumacher's dLibs library */

/* will have to be adjusted at some time ++jrb */
/* use with caution, TOS 1.4 still has double re-direction bug! */

#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <mintbind.h>
#include <unistd.h>
#include "lib.h"

int
__dup2(handle1, handle2)
	int handle1, handle2;
{
	int rv;
	long flags;

	if (handle1 == handle2)
		return (handle2);

	if ((rv = (int)Fforce(handle2, handle1)) < 0)
		__set_errno (-rv);
	else {
		if (__OPEN_INDEX(handle2) < __NHANDLES)
			__open_stat[__OPEN_INDEX(handle2)] =
				__open_stat[__OPEN_INDEX(handle1)];
		
		if ((flags = (long)Fcntl(handle2, (long)0, F_GETFD)) != -ENOSYS)
			(void)Fcntl(handle2, flags & ~FD_CLOEXEC, F_SETFD);
	}
	return (rv < 0) ? -1 : handle2;
}
weak_alias (__dup2, dup2)

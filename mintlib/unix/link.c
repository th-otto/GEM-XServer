/* make a hard link */

#include <errno.h>
#include <mintbind.h>
#include <unistd.h>

#ifdef __TURBOC__
# include <sys\param.h>
# include <sys\stat.h>
#else
# include <sys/param.h>
# include <sys/stat.h>
#endif

#include "lib.h"

/*
 * if MiNT is not active, we try to fail gracefully
 */

int
__link(_old, _new)
	const char *_old, *_new;
{
	long r;
	char oldbuf[MAXPATHLEN], newbuf[MAXPATHLEN];
	char* old = (char*) _old;
	char* new = (char*) _new;
	
	if (!__libc_unix_names)
	  {
	    old = oldbuf;
	    new = newbuf;
	    _unx2dos(_old, old, sizeof (oldbuf));
	    _unx2dos(_new, new, sizeof (newbuf));
	  }
	  
	r = Flink(old, new);
	if (r < 0 && r != -ENOSYS) {
		struct xattr sb;

		if ((r == -ENOTDIR)) {
			if (_enoent(Fxattr(1, old, &sb) ? old : new))
				r = -ENOENT;
		} else if ((r == -EACCES) && (!Fxattr(1, new, &sb)))
			r = -EEXIST;
		__set_errno ((int) -r);
		return -1;
	}
	else if (r == -ENOSYS) {
		__set_errno (EXDEV);
		return -1;
	}	
	return 0;
}

weak_alias (__link, link)

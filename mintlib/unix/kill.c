/* kill() for MiNT */

#include <stdlib.h>
#include <errno.h>
#include <osbind.h>
#include <signal.h>
#include <unistd.h>
#include <mintbind.h>

#include "lib.h"

/* vector of signal handlers (for TOS) */
extern sighandler_t _sig_handler[];	/* in signal.c */

/* vector giving which signals are currently blocked from delivery (for TOS) */
extern long _sigmask;			/* in signal.c */

/* which signals are pending? */
extern long _sigpending;

static short _have_pkill = 1;
extern int __mint;

int
__kill(pid, sig)
	int pid, sig;
{
	long r = 0;
	sighandler_t hndle;

	if (_have_pkill) {
		r = Pkill(pid, sig);
		
		if (r == -ENOSYS)
			_have_pkill = 0;
		else if (r < 0) {
			if (r == -EACCES)
			  r = -EPERM;
			else if (r == -ENOENT)
			  r = -ESRCH;
			else if (sig < 0 || sig >= __NSIG)
			  r = -EINVAL;
			__set_errno ((int) -r);
			return -1;
		}
		else
			return 0;
	}
	/* fall through to TOS emulation */

		if (sig < 0 || sig >= NSIG || (pid && pid != getpid())) {
			__set_errno (EINVAL);
			return -1;
		}
		hndle = _sig_handler[sig];
		if (hndle == SIG_IGN)
			return 0;
	/* check here for masked signals */
		else if (sig != SIGKILL && (_sigmask & (1L << sig)))
			_sigpending |= (1L << sig);
		else {
			_sigpending &= ~(1L << sig);
			if (hndle == SIG_DFL) {
				switch (sig) {
				case SIGCONT:
				case SIGCHLD:
					return 0;
				default:
					exit(sig << 8);
				}
			} else {
				(*hndle)(sig);
			}
		}

	return 0;
}

weak_alias (__kill, kill)

/* I think ANSI wants this. It's useful, at any rate.
   -- ERS
*/

#include <unistd.h>
#include <signal.h>

int
raise(sig)
int sig;
{
	return kill(getpid(), sig);
}

/*
	execl for MiNT/TOS; written by Eric R. Smith, and
	placed in the public domain
*/

#include <stdarg.h>
#include <process.h>
#include <unistd.h>

int
execl(const char *path, const char *arg, ...)
{
	return execve(path, (char **) &arg, NULL);
}

/* functions for manipulating the environment */
/* written by Eric R. Smith and placed in the public domain */
/* 5/5/92 sb -- separated for efficiency, see also putenv.c */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

extern char** environ;

char *
getenv(tag)
	const char *tag;
{
	char **var;
	char *name;
	size_t len = strlen(tag);

	if (!environ) return 0;

	for (var = environ; (name = *var) != 0; var++) {
		if (!strncmp(name, tag, len) && name[len] == '=')
			return name+len+1;
	}
	return 0;
}

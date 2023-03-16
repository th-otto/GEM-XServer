/* from Henry Spencer's stringlib */
#include <string.h>
#undef strlen

/*
 * strlen - length of string (not including NUL)
 */
size_t
strlen(const char *scan)
{
	const char *start = scan+1;

#if !__GNUC_PREREQ(4, 0)
	/*
	 * Newer compilers will remove that check anyway.
	 * GCC >= 7 even complains about it.
	 */
	if (!scan) return 0;
#endif
	while (*scan++ != '\0')
		continue;
	return scan - start;
}

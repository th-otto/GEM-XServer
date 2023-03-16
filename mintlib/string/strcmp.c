/* from Henry Spencer's stringlib */
/* modified by ERS */
/* Fixed by Guido:  Nobody ever thought of 8 bit characters???  */

#include <string.h>
#undef strcmp

/*
 * strcmp - compare string s1 to s2
 */

int				/* <0 for <, 0 for ==, >0 for > */
strcmp(const char *scan1, const char *scan2)
{
	unsigned char c1, c2;

#if !__GNUC_PREREQ(4, 0)
	/*
	 * Newer compilers will remove that check anyway.
	 * GCC >= 7 even complains about it.
	 */
	if (!scan1)
		return scan2 ? -1 : 0;
	if (!scan2) return 1;
#endif

	do {
		c1 = (unsigned char) *scan1++; c2 = (unsigned char) *scan2++;
	} while (c1 && c1 == c2);

	/*
	 * The following case analysis is necessary so that characters
	 * which look negative collate low against normal characters but
	 * high against the end-of-string NUL.
	 */
	if (c1 == c2)
		return(0);
	else if (c1 == '\0')
		return(-1);
	else if (c2 == '\0')
		return(1);
	else
		return(c1 - c2);
}

	.text
	.globl _ldexp
	.globl __infinitydf
	.even
#ifdef	ERROR_CHECK
#include "errbase.h"
_Overflow:
	.ascii "ldexp: OVERFLOW\12\15\0"
# if defined(__M68881__) || defined(sfp004)
_Domain:
	.ascii "ldexp: NAN\12\15\0"
	.even
double_max:
	.long	0x7fee42d1
	.long	0x30273b76
double_min:
	.long	0xffee42d1
	.long	0x30273b76
NaN:
	.long	0x7fffffff
	.long	0xffffffff
p_Inf:
	.long	0x7ff00000
	.long	0x00000000
m_Inf:
	.long	0xfff00000
	.long	0x00000000
# endif
	.even
#endif	/* ERROR_CHECK */
_ldexp:
#if !defined (__M68881__) && !defined (sfp004)

 | add exponent to floating point number
 | C Interface
 | double ldexp(double value, unsigned int exp);
 | returns value * 2**exp
 | (int is 16 bits if -mshort, 32 bits if !-mshort)
 |-----------------------------------------------------------------------------
 | ported to 68000 by Kai-Uwe Bloem, 12/89
 |  #1  original author: Peter S. Housel 9/21/88,01/17/89,03/19/89,5/24/89
 |  #2	added support for denormalized numbers			-kub-, 01/90
 |  #3  ported to gcc  ++jrb  04/90
 |  #4  handle exponent overflow when ints are 32 bits		-kub-, 04/90
 |  #5  make returned zero signed				  mj,  05/93
 |-----------------------------------------------------------------------------

	lea	%sp@(4),%a1
#ifdef __mcoldfire__
	lea	%sp@(-24),%sp
	moveml	%d2-%d7,%sp@	| save d2-d7
#else
	moveml	%d2-%d7,%sp@-	| save d2-d7
#endif

	movew	%a1@,%d0		| extract value.exp
	movew	%d0,%d2		| extract value.sign
#ifdef __mcoldfire__
	andil	#0x7fff,%d0	| kill sign bit
	lsrl	#4,%d0
#else
	bclr	#15,%d0		| kill sign bit
	lsrw	#4,%d0
#endif

	moveq	#0x0f,%d3	| remove exponent from value.mantissa
#ifdef __mcoldfire__
	andl	%d2,%d3		| four upper bits of value in d3
#else
	andb	%d2,%d3		| four upper bits of value in d3
#endif
	bset	#4,%d3		| implied leading 1
	tstw	%d0		| check for zero exponent
	jne	1f
#ifdef __mcoldfire__
	addql	#1,%d0
#else
	addqw	#1,%d0
#endif
	bclr	#4,%d3		| nah, we do not need stinkin leadin 1
1:	movew	%d3,%a1@		| save results of our efforts
#ifdef __MSHORT__
#ifdef __mcoldfire__
    movew   %a1@(8),%d1
	addl	%d1,%d0	| add in exponent
#else
	addw	%a1@(8),%d0	| add in exponent
#endif
	extl	%d0
#else
	extl	%d0
	addl	%a1@(8),%d0	| add in exponent
#endif
	cmpl	#-53,%d0		| hmm. works only if 1 in implied position...
	jle	retz		| range error - underflow
	cmpl	#0x7ff,%d0
	jge	rangerr		| range error - overflow

	clrw	%d1		| zero rounding bits
	moveml	%a1@,%d4-%d5	| value into d4,d5
	jmp	___norm_df	| norm_df will pop d2-d7 and rts

retz:
	moveq	#0,%d0		| zero return value
	movel	%d0,%d1
#ifdef __mcoldfire__
	btst	#15,%d2
	jeq	L0
	bset 	#31,%d0
#else
	lslw	#1,%d2		| transfer argument sign
	roxrl	#1,%d0
#endif
	jra	L0

rangerr:

#ifdef ERROR_CHECK
	moveq	#Erange,%d0
	Emove	%d0,Errno
	pea	%pc@(_Overflow)	| for printf
	pea	Stderr		|
	jbsr	_fprintf	|
	addql	#8,%sp		|
#endif /* ERROR_CHECK */

#ifdef __mcoldfire__
	movel	__infinitydf,%d0    | return HUGE_VAL (same as in <math.h>)
	movel	__infinitydf+4,%d1
#else
	moveml	__infinitydf,%d0-%d1 | return HUGE_VAL (same as in <math.h>)
#endif
	tstw	%d2
	jge	L0
	bset	#31,%d0
L0:
#ifdef __mcoldfire__
	moveml	%sp@,%d2-%d7	| pop saved reggies
	lea	%sp@(24),%sp
#else
	moveml	%sp@+,%d2-%d7	| pop saved reggies
#endif
	rts

#else	/* __M68881__ || sfp004 */
#ifdef	__M68881__
|#######################################################################
|
|# add exponent to floating point number
|# C Interface
|# double ldexp(double value, unsigned int exp);
|# returns value * 2**exp
|# (int is 16 bits if -mshort, 32 bits if !-mshort)
|# performed entirely on the 68881 to avoid overfow as good as possible
|##############################################################################
|# hacked for the 68881 by Michael Ritzert, 5.10.90
|##############################################################################
|# ported fromt sfp004 to real coprocessor, mjr, August 1991
|##############################################################################

#ifdef __MSHORT__
	movew	%sp@(12),%d0		| get exponent
	extl	%d0
#else
	movel	%sp@(12),%d0		| get exponent
#endif
	fgetexpd %sp@(4),%fp1		| extract exponent of 1st arg
	fmovel	%fp1,%d1			| d1 serves as accumulator
	addl	%d0,%d1			| sum of exp_s of both args

	ftwotoxl %d0,%fp0			| ftwotox to fp0 (as long int!)
	fmuld	%sp@(4),%fp0		| fmul value,fp0
	fmoved	%fp0,%sp@-		| get double from fp0
	moveml	%sp@+,%d0-%d1

#endif  /* __M68881__ */
#ifdef	sfp004

| add exponent to floating point number
| C Interface
| double ldexp(double value, unsigned int exp);
| returns value * 2**exp
| (int is 16 bits if -mshort, 32 bits if !-mshort)
| performed entirely on the 68881 to avoid overfow as good as possible
|
| hacked for the 68881 by Michael Ritzert, 5.10.90
|

| addresses of the 68881 data port. This choice is fastest when much data is
| transferred between the two processors.

comm =	 -6	|	fpu command reg
resp =	-16	|	fpu response reg
zahl =	  0	|	fpu data reg

| waiting loop ...
|
| wait:
| ww:	cmpiw	#0x8900,%a1@(resp)
| 	beq	ww
| is coded directly by
|	.byte	0x0c,0x69,0x89,0x00,0xff,0xf0,0x67,0xf8 (a1)
|	or
|	.long	0x0c6889000, 0x000067f8			(a0)

#ifdef __MSHORT__
	movew	%sp@(12),%d0		| get exponent
	extl	%d0
#else
	movel	%sp@(12),%d0		| get exponent
#endif
	lea	0xfffffa50:w,%a0		| fpu address
	movew	#0x549e,%a0@(comm)	| fgetexpd %sp@(4),%fp1
	cmpiw	#0x8900,%a0@(resp)
	movel	%sp@(4),%a0@
	movel	%sp@(8),%a0@
	movew	#0x6080,%a0@(comm)	| fmovel %fp1,%d1
	.long	0x0c688900, 0xfff067f8
	movel	%a0@,%d1
	addl	%d0,%d1
	movew	#0x4011,%a0@(comm)	| ftwotoxl %d0,%fp0
	.long	0x0c688900, 0xfff067f8
	movel	%d0,%a0@
	movew	#0x5423,%a0@(comm)	| fmuld %sp@(4),%fp0
	.long	0x0c688900, 0xfff067f8
	movel	%sp@(4),%a0@
	movel	%sp@(8),%a0@
	movew	#0x7400,%a0@(comm)	| fmoved %fp0,%d0/%d1
	.long	0x0c688900, 0xfff067f8
	movel	%a0@,%d0
	movel	%a0@,%d1
#endif	/* sfp004 */

#ifdef	ERROR_CHECK
	lea	double_max,%a0	|
	swap	%d0		| exponent into lower word
	cmpw	%a0@(16),%d0	| == NaN ?
	jeq	error_nan	|
	cmpw	%a0@(24),%d0	| == + Infinity ?
	jeq	error_plus	|
	swap	%d0		| result ok,
	rts			| restore d0
error_plus:
	swap	%d0
	moveml	%d0-%d1,%sp@-	| print error message
	moveq	#Erange,%d0	| Overflow: errno = ERANGE
	Emove	%d0,Errno
	pea	%pc@(_Domain)	| for printf
	jra	error_exit	|
error_nan:
	moveq	#Edom,%d0	| NAN => errno = EDOM
	Emove	%d0,Errno
	moveml	%a0@(24),%d0-%d1	| result = +inf
	moveml	%d0-%d1,%sp@-	| print error message
	pea	%pc@(_Overflow)	| for printf
error_exit:
	pea	Stderr		|
	jbsr	_fprintf	|
	addql	#8,%sp		|
	moveml	%sp@+,%d0-%d1
#endif	/* ERROR_CHECK */
	rts

#endif	/* !__M68881__ && !sfp004	*/

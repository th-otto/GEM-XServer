/*
 *==============================================================================
 *
 * x_mint.h
 *
 * Copyright (C) 2000 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-08-28 - Initial Version.
 *==============================================================================
 */
#ifndef __X_MINT_H__
#define __X_MINT_H__

#include <mintbind.h>
#undef Finstat
#undef Foutstat
#undef Fread
#undef Fwrite


/*
 * These (re-)bindings seem to be needed
 * to prevent register d0 being clobbered
 * by longjmp() in clnt.c.
 */
static __inline long Finstat(short fd)
{
	long rtn;

	__asm__ volatile (
		"\tmove.w	%1,-(%%sp)\n"
		"\tmove.w	#0x105,-(%%sp)\n"
		"\ttrap		#1\n"
		"\taddq.l	#4,%%sp\n"
		"\tmoveq.l	#1,%%d1\n"
		"\tadd.l	%%d0,%%d1\n"
		"\tbvc.s	9f\n"
		"\text.l	%%d0\n"
		"\t9:\n"
		"\tmove.l	%%d0,%0\n"
		: "=d" (rtn)					/* output */
		: "g"(fd)			/* input */
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc" AND_MEMORY	/* clobbered */
		);

	return rtn;
}

static __inline long Foutstat(short fd)
{
	long rtn;

	__asm__ volatile (
		"\tmove.w	%1,-(%%sp)\n"
		"\tmove.w	#0x106,-(%%sp)\n"
		"\ttrap		#1\n"
		"\taddq.l	#4,%%sp\n"
		"\tmoveq.l	#1,%%d1\n"
		"\tadd.l	%%d0,%%d1\n"
		"\tbvc.s	9f\n"
		"\text.l	%%d0\n"
		"\t9:\n"
		"\tmove.l	%%d0,%0\n"
		: "=d" (rtn)					/* output */
		: "g"(fd)			/* input */
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc" AND_MEMORY	/* clobbered */
		);

	return rtn;
}


static __inline long Fread(short fd, long count, void *buf)
{
	long rtn;

	__asm__ volatile (
		"\tmove.l	%3,-(%%sp)\n"
		"\tmove.l	%2,-(%%sp)\n"
		"\tmove.w	%1,-(%%sp)\n"
		"\tmove.w	#0x3F,-(%%sp)\n"
		"\ttrap		#1\n"
		"\tlea		12(%%sp),%%sp\n"
		"\tmove.l	%%d0,%0\n"
		: "=d" (rtn)					/* outputs */
		: "g"(fd), "g"(count), "g"(buf)	/* inputs */
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc" AND_MEMORY	/* clobbered regs */
		);

	return rtn;
}

static __inline long Fwrite(short fd, long count, void *buf)
{
	long rtn;

	__asm__ volatile (
		"\tmove.l	%3,-(%%sp)\n"
		"\tmove.l	%2,-(%%sp)\n"
		"\tmove.w	%1,-(%%sp)\n"
		"\tmove.w	#0x40,-(%%sp)\n"
		"\ttrap		#1\n"
		"\tlea		12(%%sp),%%sp\n"
		"\tmove.l	%%d0,%0\n"
		: "=d" (rtn)					/* outputs */
		: "g"(fd), "g"(count), "g"(buf)	/* inputs */
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc" AND_MEMORY	/* clobbered regs */
		);

	return rtn;
}

#endif /* __X_MINT_H__ */

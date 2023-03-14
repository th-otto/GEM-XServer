/*
 *==============================================================================
 *
 * tools.h
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-08-14 - Initial Version.
 *==============================================================================
 */
#ifndef __TOOLS_H__
#define __TOOLS_H__

#include "types.h"


static inline CARD16 Swap16(CARD16 val)
{
	__asm__ volatile (
		"\trol.w #8, %0\n"
		: "=d"(val) /* output */
		: "0"(val)  /* input */
		: "cc"
	);
	return val;
}

static inline CARD32 Swap32(CARD32 val)
{
	__asm__ volatile (
		"\trol.w	#8, %0\n"
		"\tswap	%0\n"
		"\trol.w	#8, %0\n"
		: "=d"(val) /* output */
		: "0"(val)  /* input */
		: "cc"
	);
	return val;
}

static inline void SwapPXY(PXY * dst, const PXY * src)
{
	__asm__ volatile (
		"\tmove.l	(%0), d0\n"
		"\trol.w		#8, d0\n"
		"\tswap		d0\n"
		"\trol.w		#8, d0\n"
		"\tswap		d0\n"
		"\tmove.l	d0, (%1)\n"
		:                   /* output */
		: "a"(src),"a"(dst) /* input */
		: "d0", "cc" AND_MEMORY              /* clobbered */
	);
}

static inline void SwapRCT(GRECT * dst, const GRECT * src)
{
	__asm__ volatile (
		"\tmovem.l	(%0), d0/d1\n"
		"\trol.w		#8, d0\n"
		"\tswap		d0\n"
		"\trol.w		#8, d0\n"
		"\tswap		d0\n"
		"\trol.w		#8, d1\n"
		"\tswap		d1\n"
		"\trol.w		#8, d1\n"
		"\tswap		d1\n"
		"\tmovem.l	d0/d1, (%1)\n"
		:                   /* output */
		: "a"(src),"a"(dst) /* input */
		: "d0","d1", "cc" AND_MEMORY         /* clobbered */
	);
}


static inline BOOL PXYinRect(const PXY * p, const GRECT * r)
{
	return (p->x >= r->x && p->y >= r->y && p->x < r->x + r->w && p->y < r->y + r->h);
}

static inline BOOL RectIntersect(const GRECT *a, const GRECT *b)
{
	return (a->x < b->x + b->w && a->y < b->y + b->h && b->x < a->x + a->w && b->y < a->y + a->h);
}


#define SWAP16(v)   ( (((v) & 0xFF)<<8) | (((v)>>8) & 0xFF) )
#define SWAP32(v)   ( (((v)>>24) & 0xFF)  | (((v)>>8) & 0xFF00) | (((v) & 0xFF00)<<8) | (((v) & 0xFF)<<24) )
#define Units(n)    (((n) +3) /4)
#define Align(n)    (((n) +3) & ~3ul)


#define numberof(array)   (sizeof(array) / sizeof(*array))


#endif /* __TOOLS_H__ */

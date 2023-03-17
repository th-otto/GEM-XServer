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

#ifndef __mcoldfire__
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

#else

static inline CARD16 Swap16(CARD16 val)
{
	return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

static inline CARD32 Swap32(CARD32 val)
{
	return ((val >> 24) & 0xFF) | ((val >> 8) & 0xFF00) | ((val & 0xFF00) << 8) | ((val & 0xFF) << 24);
}

#endif


static inline BOOL PXYinRect(const PXY *p, const GRECT *r)
{
	return p->p_x >= r->g_x && p->p_y >= r->g_y && p->p_x < r->g_x + r->g_w && p->p_y < r->g_y + r->g_h;
}

static inline BOOL RectIntersect(const GRECT *a, const GRECT *b)
{
	return a->g_x < b->g_x + b->g_w && a->g_y < b->g_y + b->g_h && b->g_x < a->g_x + a->g_w && b->g_y < a->g_y + a->g_h;
}


#define Units(n)    (((n) + 3) / 4)
#define Align(n)    (((n) + 3) & ~3ul)


#define numberof(array)   (sizeof(array) / sizeof(*array))


#endif /* __TOOLS_H__ */

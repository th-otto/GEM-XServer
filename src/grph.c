/*
 *==============================================================================
 *
 * grph.c
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-06-05 - Initial Version.
 *==============================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mint/cookie.h>

#include "main.h"
#include "tools.h"
#include "clnt.h"
#include "grph.h"
#include "colormap.h"
#include "font.h"
#include "window.h"
#include "event.h"
#include "gcontext.h"
#include "x_gem.h"
#include "gemx.h"
#include "x_printf.h"

#include <X11/Xprotostr.h>


#define _CONST_DPI 72


#define DFLT_IMSK    (NoEventMask)

short GRPH_Handle = -1;
short GRPH_Vdi = 0;
short GRPH_ScreenW;
short GRPH_ScreenH;
short GRPH_Depth;
short GRPH_muWidth;
short GRPH_muHeight;
int GRPH_DepthNum = 1;
short GRPH_Format = SCRN_Interleaved;
short GRPH_Fonts = 0x8000;

static struct xDepthAndVisual __depth_msb_1 = { {1, 00, 1}, {DFLT_VISUAL + 0, StaticGray, 1, 2, 0, 0, 0} };
static struct xDepthAndVisual __depth_msb_n = { {8, 00, 1}, {DFLT_VISUAL + 1, StaticColor, 8, 256, 0, 0, 0} };

static struct xDepthAndVisual __depth_lsb_1;
static struct xDepthAndVisual __depth_lsb_n;
struct xDepthAndVisual *GRPH_DepthMSB[2] = { &__depth_msb_1, &__depth_msb_n };
struct xDepthAndVisual *GRPH_DepthLSB[2] = { &__depth_lsb_1, &__depth_lsb_n };


static BOOL _r_put_I4(MFDB *mfdb, CARD16 width, CARD16 height);
static BOOL _r_put_I8(MFDB *mfdb, CARD16 width, CARD16 height);
static BOOL _r_put_P8(MFDB *mfdb, CARD16 width, CARD16 height);
static BOOL _r_put_16(MFDB *mfdb, CARD16 width, CARD16 height);
static BOOL _r_put_24(MFDB *mfdb, CARD16 width, CARD16 height);
static BOOL _r_put_32(MFDB *mfdb, CARD16 width, CARD16 height);

static BOOL _r_get_I4(MFDB *mfdb, PRECT *pxy, MFDB *ptr);
static BOOL _r_get_I8(MFDB *mfdb, PRECT *pxy, MFDB *ptr);
static BOOL _r_get_P8(MFDB *mfdb, PRECT *pxy, MFDB *ptr);
static BOOL _r_get_16(MFDB *mfdb, PRECT *pxy, MFDB *ptr);
static BOOL _r_get_24(MFDB *mfdb, PRECT *pxy, MFDB *ptr);
static BOOL _r_get_32(MFDB *mfdb, PRECT *pxy, MFDB *ptr);


/* ============================================================================== */
BOOL GrphInit(void)
{
	if (GRPH_Vdi <= 0)
	{
		short work_in[16] = { 1, SOLID, G_BLACK, MRKR_DOT, G_BLACK, 1, G_BLACK,
			FIS_SOLID, 0, 0, 2, 0, 0, 0, 0, 0
		};
		short w_out[57];
		short dmy;

		GRPH_Vdi = GRPH_Handle = graf_handle(&dmy, &dmy, &dmy, &dmy);
		v_opnvwk(work_in, &GRPH_Vdi, w_out);
		if (GRPH_Vdi <= 0)
		{
			x_printf("\033pFATAL\033q can't initialize VDI for #%i: %i!\n", GRPH_Handle, GRPH_Vdi);
			return xFalse;
		}
		GRPH_ScreenW = w_out[0];
		GRPH_ScreenH = w_out[1];
		GRPH_muWidth = w_out[3];
		GRPH_muHeight = w_out[4];
		GRPH_Fonts |= w_out[10];
		vq_extnd(GRPH_Vdi, 1, w_out);
		GRPH_Depth = w_out[4];

		vst_alignment(GRPH_Vdi, TA_LEFT, TA_BASE, w_out, w_out);
		vsf_perimeter(GRPH_Vdi, PERIMETER_OFF);
	}
	return xTrue;
}

/* ============================================================================== */
void GrphExit(void)
{
	if (GRPH_Vdi > 0)
	{
		if (GRPH_Fonts > 0)
		{
			if (vq_gdos())
				vst_unload_fonts(GRPH_Vdi, 0);
		}
		v_clsvwk(GRPH_Vdi);
	}
}


/* ============================================================================== */
int GrphSetup(void *format_arr)
{
	int n_form = 1;
	xPixmapFormat *pfrm = format_arr;
	CARD8 r_dpth = GRPH_DepthMSB[0]->dpth.depth;
	VisualID r_visl = GRPH_DepthMSB[0]->visl.visualID;
	CARD32 wht_px = G_WHITE;
	CARD32 blk_px = G_BLACK;
	short EdDI = 0;
	xWindowRoot *root;
	int i;

	{									/* search for EdDI ---------------------------------- */
		short (*func)(short) = NULL;

		if (Getcookie(C_EdDI, (long *) &func) == C_FOUND && func)
		{
			short w_out[273];
			short ver;
			__asm__ volatile (
				"\tclr.l		%%d0\n"
				"\tjsr		(%1)\n"
				"\tmove.w	%%d0, %0\n"
				: "=d"(ver)                 /* output */
				: "a"(func)                 /* input */
				: "d0","d1","d2", "a0","a1","a2","cc" AND_MEMORY /* clobbered */
			);
			EdDI = ver;
			vq_scrninfo(GRPH_Vdi, w_out);
			if ((w_out[14] & 2) && GRPH_Depth == 16)
			{
				GRPH_Format = SCRN_FalconHigh;
			} else
			{
				GRPH_Format = w_out[0];
			}
		} else if (GRPH_Depth > 8)
		{
			GRPH_Format = -1;
		}
	}
	CmapInit();

	pfrm->depth = pfrm->bitsPerPixel = 1;
	pfrm->scanLinePad = PADD_BITS;
	if (GRPH_Depth > 1)
	{
		n_form++;
		pfrm++;
		pfrm->depth = GRPH_Depth;
		pfrm->bitsPerPixel = (GRPH_Depth + 1) & ~1;
		pfrm->scanLinePad = PADD_BITS;

		GRPH_DepthNum++;
		GRPH_DepthMSB[1]->dpth.depth = r_dpth = GRPH_Depth;
		if (GRPH_Depth > 8)
		{
			GRPH_DepthMSB[1]->visl.colormapEntries = 256;
			GRPH_DepthMSB[1]->visl.class = TrueColor;
			wht_px = (1uL << GRPH_Depth) - 1;
			blk_px = 0;
			if (GRPH_Format == SCRN_FalconHigh)
			{
				GRPH_DepthMSB[1]->visl.redMask = 0x0000F800uL;
				GRPH_DepthMSB[1]->visl.greenMask = 0x000007C0uL;
				GRPH_DepthMSB[1]->visl.blueMask = 0x0000001FuL;
				GRPH_DepthMSB[1]->visl.bitsPerRGB = 5;
				GrphRasterPut = _r_put_16;
				GrphRasterGet = _r_get_16;
			} else if (GRPH_Depth == 16)
			{
				GRPH_DepthMSB[1]->visl.redMask = 0x0000F800uL;
				GRPH_DepthMSB[1]->visl.greenMask = 0x000007E0uL;
				GRPH_DepthMSB[1]->visl.blueMask = 0x0000001FuL;
				GRPH_DepthMSB[1]->visl.bitsPerRGB = 6;
				GrphRasterPut = _r_put_16;
				GrphRasterGet = _r_get_16;
			} else
			{
				GRPH_DepthMSB[1]->visl.redMask = 0x00FF0000uL;
				GRPH_DepthMSB[1]->visl.greenMask = 0x0000FF00uL;
				GRPH_DepthMSB[1]->visl.blueMask = 0x000000FFuL;
				GRPH_DepthMSB[1]->visl.bitsPerRGB = 8;
				if (GRPH_Depth == 24)
				{
					GrphRasterPut = _r_put_24;
					GrphRasterGet = _r_get_24;
				} else if (GRPH_Depth == 32)
				{
					GrphRasterPut = _r_put_32;
					GrphRasterGet = _r_get_32;
				}
			}
		} else
		{
			GRPH_DepthMSB[1]->visl.colormapEntries = 1 << GRPH_Depth;
			GRPH_DepthMSB[1]->visl.bitsPerRGB = pfrm->depth;
			if (GRPH_Depth == 4)
			{
				if (GRPH_Format == SCRN_Interleaved)
				{
					GrphRasterPut = _r_put_I4;
					GrphRasterGet = _r_get_I4;
				}
			} else if (GRPH_Depth == 8)
			{
				if (GRPH_Format == SCRN_Interleaved)
				{
					GrphRasterPut = _r_put_I8;
					GrphRasterGet = _r_get_I8;
				} else if (GRPH_Format == SCRN_PackedPixel)
				{
					GrphRasterPut = _r_put_P8;
					GrphRasterGet = _r_get_P8;
				}
			}
		}
		r_visl = GRPH_DepthMSB[1]->visl.visualID;
	}
	root = (xWindowRoot *) (pfrm + 1);
	root->windowId = ROOT_WINDOW;
	root->defaultColormap = DFLT_COLORMAP;
	root->whitePixel = wht_px;
	root->blackPixel = blk_px;
	root->currentInputMask = DFLT_IMSK;
	root->pixWidth = WIND_Root.Rect.g_w;
	root->pixHeight = WIND_Root.Rect.g_h;
#ifdef _CONST_DPI
	root->mmWidth = (((long) WIND_Root.Rect.g_w * 6502u + (_CONST_DPI / 2)) / _CONST_DPI) >> 8;
	root->mmHeight = (((long) WIND_Root.Rect.g_h * 6502u + (_CONST_DPI / 2)) / _CONST_DPI) >> 8;
#else
	root->mmWidth = ((long) WIND_Root.Rect.g_w * GRPH_muWidth + 500) / 1000;
	root->mmHeight = ((long) WIND_Root.Rect.g_h * GRPH_muHeight + 500) / 1000;
#endif
	root->minInstalledMaps = 1;
	root->maxInstalledMaps = 1;
	root->backingStore = NotUseful;
	root->saveUnders = xTrue;
	root->rootVisualID = r_visl;
	root->rootDepth = WIND_Root.Depth = r_dpth;
	root->nDepths = GRPH_DepthNum;

	for (i = 0; i < GRPH_DepthNum; ++i)
	{
		GRPH_DepthLSB[i]->dpth.depth = GRPH_DepthMSB[i]->dpth.depth;
		GRPH_DepthLSB[i]->dpth.nVisuals = Swap16(GRPH_DepthMSB[i]->dpth.nVisuals);
		memcpy(&GRPH_DepthLSB[i]->visl, &GRPH_DepthMSB[i]->visl, sz_xVisualType);
		ClntSwap(&GRPH_DepthLSB[i]->visl, "v2:lll");
	}

	x_printf("  AES id #%i at VDI #%i:%i [%i/%i] (%i*%i mm) %i plane%s\n",
			gl_apid, GRPH_Handle, GRPH_Vdi, WIND_Root.Rect.g_w, WIND_Root.Rect.g_h,
			root->mmWidth, root->mmHeight, GRPH_Depth, (GRPH_Depth == 1 ? "" : "s"));
	x_printf("  screen format");
	if (EdDI)
		x_printf(" (EdDI %X.%02X)", EdDI >> 8, EdDI & 0xFF);
	x_printf(" is %s \n",
		GRPH_Format == SCRN_Interleaved ? "interleaved planes" :
		GRPH_Format == SCRN_Standard ? "standard format" :
		GRPH_Format == SCRN_PackedPixel ? "packed pixels" :
		GRPH_Format == SCRN_FalconHigh ? "falcon truecolor" : "<unknown>");

	GRPH_Fonts &= 0x7FFF;
	if (vq_gdos())
		GRPH_Fonts += vst_load_fonts(GRPH_Vdi, 0);
	FontInit(GRPH_Fonts);

	return n_form;
}


/* ============================================================================== */
BOOL GrphIntersect(GRECT *dst, const GRECT *src)
{
#if 1
	register BOOL res __asm("d0");
	__asm__ volatile(
		"\tmoveq.l	#1, d0\n"
		"\tmovem.l	(%1), d1/d2\n" /* a.x:a.y/a.w:a.h */
		"\tmovem.l	(%2), d3/d4\n" /* b.x:b.y/b.w:b.h */
		"\tadd.w		d1, d2\n" /* a.h += a.y */
		"\tadd.w		d3, d4\n" /* b.h += b.y */
			"\tcmp.w		d1, d3\n"
			"\tble.b		1f\n"   /* ? b.y <= a.y */
			"\tmove.w	d3, d1\n" /* a.y = b.y */
		"1:	cmp.w		d2, d4\n"
		"\tbge.b		2f\n"   /* ? b.h >= a.h */
		"\tmove.w	d4, d2\n" /* a.h = b.h */
		"2: sub.w		d1, d2\n" /* a.h -= a.y */
		"\tbhi.b		3f\n"   /* ? a.h > a.y */
		"\tmoveq.l	#0, d0\n"
		"3:\n"
		"\tswap		d1\n" /* a.y:a.x */
		"\tswap		d2\n" /* a.h:a.w */
		"\tadd.w		d1, d2\n" /* a.w += a.x */
		"\tswap		d3\n" /* b.y:b.x */
		"\tswap		d4\n" /* b.h:b.w */
		"\tadd.w		d3, d4\n" /* b.w += b.x */
			"\tcmp.w		d1, d3\n"
			"\tble.b		5f\n"   /* ? b.x <= a.x */
			"\tmove.w	d3, d1\n" /* a.x = b.x */
		"5:	cmp.w		d2, d4\n"
		"\tbge.b		6f\n"   /* ? b.w >= a.w */
		"\tmove.w	d4, d2\n" /* a.w = b.w */
		"6:	sub.w		d1, d2\n" /* a.w -= a.x */
		"\tbhi.b		7f\n"   /* ? a.w > a.x */
		"\tmoveq.l	#0, d0\n"
		"7:\n"
		"\tswap		d1\n"
		"\tswap		d2\n"
		"\tmovem.l	d1/d2, (%1)\n" /* a.x:a.y/a.w:a.h */
		: "=d"(res)           /* output */
		: "a"(dst),"a"(src)   /* input */
		: "d1","d2","d3","d4","cc" AND_MEMORY /* clobbered */
	);
	return res;
#else
	short p = dst->g_x + dst->g_w - 1;
	short q = src->g_x + src->g_w - 1;

	dst->g_x = (dst->g_x >= src->g_x ? dst->g_x : src->g_x);
	dst->g_w = (p <= q ? p : q) - dst->g_x + 1;
	p = dst->g_y + dst->g_h - 1;
	q = src->g_y + src->g_h - 1;
	dst->g_y = (dst->g_y >= src->g_y ? dst->g_y : src->g_y);
	dst->g_h = (p <= q ? p : q) - dst->g_y + 1;

	return dst->g_w > 0 && dst->g_h > 0;
#endif
}

/* ============================================================================== */
BOOL GrphIntersectP(PRECT *dst, const PRECT *src)
{
#if 1
	register BOOL res __asm("d0");
	__asm__ volatile (
		"\tmoveq.l	#0, d0\n"
		"\tmovem.l	(%1), d1/d2\n" /* a.x0:a.y0/a.x1:a.y1 */
		"\tmovem.l	(%2), d3/d4\n" /* b.x0:b.y0/b.x1:b.y1 */
		"\t	cmp.w		d1, d3\n"
		"\t	ble.b		1f\n"   /* ? b.y0 <= a.y0 */
		"\t	move.w	d3, d1\n" /* a.y0 = b.y0 */
		"1:	cmp.w		d2, d4\n"
		"\t	bge.b		2f\n"   /* ? b.y1 >= a.y1 */
		"\t	move.w	d4, d2\n" /* a.y1 = b.y1 */
		"2:	cmp.w		d1, d2\n"
		"\t	blt.b		9f\n"   /* ? a.y1 >= a.y0 */
		"\tswap		d1\n" /* a.y0:a.x0 */
		"\tswap		d2\n" /* a.y1:a.x1 */
		"\tswap		d3\n" /* b.y0:b.x0 */
		"\tswap		d4\n" /* b.y1:b.x1 */
		"\t	cmp.w		d1, d3\n"
		"\t	ble.b		5f\n"   /* ? b.x0 <= a.x0 */
		"\t	move.w	d3, d1\n" /* a.x0 = b.x0 */
		"5:	cmp.w		d2, d4\n"
		"\t	bge.b		6f\n"   /* ? b.x1 >= a.x1 */
		"\t	move.w	d4, d2\n" /* a.x1 = b.x1 */
		"6:	cmp.w		d1, d2\n"
		"\t	blt.b		9f\n"   /* ? a.x1 >= a.x0 */
		"\tswap		d1\n"
		"\tswap		d2\n"
		"\tmovem.l	d1/d2, (%1)\n" /* a.x:a.y/a.w:a.h */
		"\tmoveq.l	#1, d0\n"
		"9:\n"
		: "=d"(res)           /* output */
		: "a"(dst),"a"(src)   /* input */
		: "d1","d2","d3","d4","cc" AND_MEMORY /* clobbered */
	);
	return res;
#else
	if (dst->lu.x < src->lu.x)
		dst->lu.x = src->lu.x;
	if (dst->lu.y < src->lu.y)
		dst->lu.y = src->lu.y;
	if (dst->rd.x > src->rd.x)
		dst->rd.x = src->rd.x;
	if (dst->rd.y > src->rd.y)
		dst->rd.y = src->rd.y;

	return (dst->lu.x <= dst->rd.x && dst->lu.y <= dst->rd.y);
#endif
}

/* ============================================================================== */
CARD16 GrphInterList(GRECT *r, const GRECT *a, CARD16 n, const GRECT *b, CARD16 m)
{
	CARD16 cnt = 0;

	while (n--)
	{
		const GRECT *bb = b;
		CARD16 mm = m;

		while (mm--)
		{
			*r = *a;
			if (GrphIntersect(r, bb++))
			{
				r++;
				cnt++;
			}
		}
		a++;
	}
	return cnt;
}

/* ============================================================================== */
void GrphCombine(GRECT *a, const GRECT *b)
{
	short d;

	if ((d = a->g_x - b->g_x) > 0)
	{
		a->g_w += d;
		a->g_x = b->g_x;
		if (a->g_w < b->g_w)
			a->g_w = b->g_w;
	} else if ((d = (b->g_x + b->g_w) - (a->g_x + a->g_w)) > 0)
	{
		a->g_w += d;
	}
	if ((d = a->g_y - b->g_y) > 0)
	{
		a->g_h += d;
		a->g_y = b->g_y;
		if (a->g_h < b->g_h)
			a->g_h = b->g_h;
	} else if ((d = (b->g_y + b->g_h) - (a->g_y + a->g_h)) > 0)
	{
		a->g_h += d;
	}
}


/* ============================================================================== */
void GrphError(void)
{
	const char txt[] = "[3]"
		"[ X Server:" "|===========" "| " "|Unsupported screen format!" "|(%i planes)]" "[ Abort ]";
	char buf[sizeof(txt) + 10];

	sprintf(buf, txt, GRPH_Depth);
	form_alert(1, buf);

	exit(1);
}


/*
 *==============================================================================
 * Graphic depth depending functions for converting images sent from clients to
 * device dependent bitmaps as used by the VDI.  Only necessary for depths with
 * more than 1 plane.
 */
BOOL(*GrphRasterPut) (MFDB *, CARD16 width, CARD16 height) = (void *) GrphError;

/* ------------------------------------------------------------------------------ */
static BOOL _r_put_I4(MFDB *mfdb, CARD16 width, CARD16 height)
{
	CARD8 val[16] = { 0, 15, 1, 2, 4, 6, 3, 5, 7, 8, 9, 10, 12, 14, 11, 13 };
	short *dst;
	char *src = mfdb->fd_addr;
	int inc = mfdb->fd_wdwidth * 2 * 4;

	if ((dst = malloc(inc * height)) == NULL)
		return xFalse;

	mfdb->fd_addr = dst;

	while (height--)
	{
		int c,
		 w = (width + 1) / 2;

		while (w)
		{
			dst[0] = dst[1] = dst[2] = dst[3] = 0;
			c = 15;
			do
			{
				register unsigned char q = *(src++);
				register unsigned short p;

				p = val[q >> 4];
				dst[0] |= (p & 1) << c;
				dst[1] |= ((p >> 1) & 1) << c;
				dst[2] |= ((p >> 2) & 1) << c;
				dst[3] |= ((p >> 3) & 1) << c;
				c--;
				p = val[q & 15];
				dst[0] |= (p & 1) << c;
				dst[1] |= ((p >> 1) & 1) << c;
				dst[2] |= ((p >> 2) & 1) << c;
				dst[3] |= ((p >> 3) & 1) << c;
			} while (--w && c--);
			dst += 4;
		}
		src += (int) src & 1;
	}
	return xTrue;
}

/* ------------------------------------------------------------------------------ */
static BOOL _r_put_I8(MFDB *mfdb, CARD16 width, CARD16 height)
{
	short *dst;
	char *src = mfdb->fd_addr;
	int inc = mfdb->fd_wdwidth * 2 * 8;

	if ((dst = malloc(inc * height)) == NULL)
		return xFalse;

	mfdb->fd_addr = dst;

	__asm__ volatile(
		"\tmovea.l	%0, a0\n" /* src */
		"\tmovea.l	%1, a1\n" /* dst */
		"\tlea		%3, a2\n" /* height */
		
		"1:\n"
		"\tmove.w	%2, d7\n" /* width counter */
		
		"4:\n" /*line */
		"\tmoveq.l	#0, d0\n"
		"\tmoveq.l	#0, d1\n"
		"\tmoveq.l	#0, d2\n"
		"\tmoveq.l	#0, d3\n"
		"\tmoveq.l	#15, d6\n" /* cluster counter */
		
		"5:\n" /*cluster */
		"\tmoveq.l	#0, d4\n"
		"\tmove.b	(a0)+, d4\n"
		"\tbeq		7f\n"
		"\tcmpi.b	#1, d4\n"
		"\tbne.s		6f\n"
		"\tmove.l	#0x00010001, d5\n"
		/* "\tmove.b	#255, d4\n" */
		"\tlsl.l		d6, d5\n"
		"\tor.l		d5, d0\n"
		"\tor.l		d5, d1\n"
		"\tor.l		d5, d2\n"
		"\tor.l		d5, d3\n"
		"\tbra		7f\n"
		"6:\n"
		"\tlsl.l  #8, d4\n"
		"\tmove.l d4, d5\n"
		"\tandi.w #0x0300, d5\n"
		"\tlsl.l #7, d5\n"
		"\trol.w #1, d5\n"
		"\tswap   d5\n"
		"\tlsl.l  d6, d5\n"
		"\tor.l  d5, d0\n"
		"\tmove.l d4, d5\n"
		"\tandi.w #0x0C00, d5\n"
		"\tlsl.l #5, d5\n"
		"\trol.w #1, d5\n"
		"\tswap   d5\n"
		"\tlsl.l  d6, d5\n"
		"\tor.l  d5, d1\n"
		"\tmove.l d4, d5\n"
		"\tandi.w #0x3000, d5\n"
		"\tlsl.l #3, d5\n"
		"\trol.w #1, d5\n"
		"\tswap   d5\n"
		"\tlsl.l  d6, d5\n"
		"\tor.l  d5, d2\n"
		"\tmove.l d4, d5\n"
		"\tandi.w #0xC000, d5\n"
		"\tlsl.l #1, d5\n"
		"\trol.w #1, d5\n"
		"\tswap   d5\n"
		"\tlsl.l  d6, d5\n"
		"\tor.l  d5, d3\n"
		"7:\n"
		"\tsubq.w	#1, d7\n"
		"\tdbeq		d6, 5b\n" /*cluster */
		
		"\tmovem.l	d0-d3, (a1)\n"
		"\tadda.w	#16, a1\n"
		"\ttst.w		d7\n"
		"\tbgt		4b\n" /*line */
		
		"\tsub.w		#1, (a2)\n"
		"\tble		9f\n"/*end */
		
		"\tmove		a0, d4\n"
		"\tbtst		#0, d4\n"
		"\tbeq		1b\n"
		"\taddq.w	#1, a0\n"
		"\tbra		1b\n"
		"9:\n"
		:                                          /* output */
		: "g"(src),"g"(dst),"g"(width),"m"(height) /* input */
		: "d0","d1","d2","d3","d4","d5","d6","d7", "a0","a1","a2","cc" AND_MEMORY /* clobbered */
	);
	return xTrue;
}

/* ------------------------------------------------------------------------------ */
static BOOL _r_put_P8(MFDB *mfdb, CARD16 width, CARD16 height)
{
	char *dst;
	char *src = mfdb->fd_addr;
	int inc = mfdb->fd_wdwidth * 2 * 8;

	if ((dst = malloc(inc * height)) == NULL)
		return xFalse;

	mfdb->fd_addr = dst;

	while (height--)
	{
		char *d = dst;
		short w = width;

		while (w--)
		{
			char c = *(src++);

			*(d++) = (c == 1 ? 255 : c);
		}
		src += (int) src & 1;
		dst += inc / sizeof(*dst);
	}
	return xTrue;
}

/* ------------------------------------------------------------------------------ */
static BOOL _r_put_16(MFDB *mfdb, CARD16 width, CARD16 height)
{
	short *dst;
	short *src = mfdb->fd_addr;
	int inc = mfdb->fd_wdwidth * 2 * 16;

	if ((mfdb->fd_w & 0x000F) == 0)
	{
		return xTrue;
	} else if ((dst = malloc(inc * height)) == NULL)
	{
		return xFalse;
	}
	mfdb->fd_addr = dst;

	while (height--)
	{
		short *d = dst;
		short w = width;

		while (w--)
		{
			*(d++) = *(src++);
		}
		dst += inc / sizeof(*dst);
	}
	return xTrue;
}

/* ------------------------------------------------------------------------------ */
static BOOL _r_put_24(MFDB *mfdb, CARD16 width, CARD16 height)
{
	short *dst;
	short *src = mfdb->fd_addr;
	int inc = mfdb->fd_wdwidth * 2 * 24;

	if ((mfdb->fd_w & 0x000F) == 0)
	{
		return xTrue;
	} else if ((dst = malloc(inc * height)) == NULL)
	{
		return xFalse;
	}
	mfdb->fd_addr = dst;

	width = (width * 3 + 1) / 2;

	while (height--)
	{
		short *d = dst;
		short w = width;

		while (w--)
		{
			*(d++) = *(src++);
		}
		dst += inc / sizeof(*dst);
	}
	return xTrue;
}

/* ------------------------------------------------------------------------------ */
static BOOL _r_put_32(MFDB *mfdb, CARD16 width, CARD16 height)
{
	long *src = mfdb->fd_addr;
	long *dst;
	int inc = mfdb->fd_wdwidth * 2 * 32;

	if ((mfdb->fd_w & 0x000F) == 0)
	{
		return xTrue;
	} else if ((dst = malloc(inc * height)) == NULL)
	{
		return xFalse;
	}
	mfdb->fd_addr = dst;

	while (height--)
	{
		long *d = dst;
		short w = width;

		while (w--)
		{
			*(d++) = *(src++);
		}
		dst += inc / sizeof(*dst);
	}
	return xTrue;
}


/*
 *==============================================================================
 * Graphic depth depending functions for converting device dependent bitmaps as
 * used by the VDI to the image format needed by clients.  Only necessary for
 * depths with more than 1 plane.
 */
BOOL(*GrphRasterGet) (MFDB *, PRECT *, MFDB *) = (void *) GrphError;

/* ------------------------------------------------------------------------------ */
static BOOL _r_get_I4(MFDB *mfdb, PRECT *pxy, MFDB *ptr)
{
	static const char pix_idx[16] = {
		0, 2, 3, 6, 4, 7, 5, 8, 9, 10, 11, 14, 12, 15, 13, 1
	};
	short *src;
	char *dst = mfdb->fd_addr;
	int inc = mfdb->fd_wdwidth * 2 * 4;
	int h = mfdb->fd_h;
	int x = 0;

	if (ptr)
	{
		src = (short *) ((char *) ptr->fd_addr + (inc * pxy[0].lu.p_y)) + (pxy[0].lu.p_x / 16);
		x = pxy[0].lu.p_x & 0x0F;

	} else if ((src = malloc(inc * h)))
	{
		MFDB scrn = { NULL, };
		mfdb->fd_addr = src;
		v_hide_c(GRPH_Vdi);
		vro_cpyfm(GRPH_Vdi, S_ONLY, (short *) pxy, &scrn, mfdb);
		v_show_c(GRPH_Vdi, 1);

	} else
	{
		return xFalse;
	}

	while (h--)
	{
		CARD16 mask = 1 << (15 - x);
		short *s = src;
		int w = mfdb->fd_w;

		while (w--)
		{
			char p = 0;
			char c;

			if (s[0] & mask)
				p |= 0x01;
			if (s[1] & mask)
				p |= 0x02;
			if (s[2] & mask)
				p |= 0x04;
			if (s[3] & mask)
				p |= 0x08;
			c = pix_idx[(int) p] << 4;
			if (!w--)
			{
				*(dst++) = c;
				break;
			}
			if (!(mask >>= 1))
			{
				mask = 1 << 15;
				s += 4;
			}
			if (s[0] & mask)
				p |= 0x01;
			if (s[1] & mask)
				p |= 0x02;
			if (s[2] & mask)
				p |= 0x04;
			if (s[3] & mask)
				p |= 0x08;
			*(dst++) = c | pix_idx[(int) p];
			if (!(mask >>= 1))
			{
				mask = 1 << 15;
				s += 4;
			}
		}
		src += inc / sizeof(*src);
		dst += (int) dst & 1;
	}
	return xTrue;
}

/* ------------------------------------------------------------------------------ */
static BOOL _r_get_I8(MFDB *mfdb, PRECT *pxy, MFDB *ptr)
{
	static const char pix_idx[256] = {
		0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
		9, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
		4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
		12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
		3, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
		11, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
		5, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
		13, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
		2, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
		10, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
		7, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
		15, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
		6, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
		14, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
		8, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
		255, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 1
	};
	short *src;
	char *dst = mfdb->fd_addr;
	int inc = mfdb->fd_wdwidth * 2 * 8;
	int h = mfdb->fd_h;
	int x = 0;

	if (ptr)
	{
		src = (short *) ((char *) ptr->fd_addr + (inc * pxy[0].lu.p_y)) + (pxy[0].lu.p_x / 16);
		x = pxy[0].lu.p_x & 0x0F;
	} else if ((src = malloc(inc * h)) != NULL)
	{
		MFDB scrn = { NULL, };
		mfdb->fd_addr = src;
		v_hide_c(GRPH_Vdi);
		vro_cpyfm(GRPH_Vdi, S_ONLY, (short *) pxy, &scrn, mfdb);
		v_show_c(GRPH_Vdi, 1);
	} else
	{
		return xFalse;
	}

	while (h--)
	{
		CARD16 mask = 1 << (15 - x);
		short *s = src;
		int w = mfdb->fd_w;

		while (w--)
		{
			int p = 0;
			int i;

			for (i = 0; i < 8; i++)
			{
				p <<= 1;
				if (*(s++) & mask)
					p |= 1;
			}
			*(dst++) = pix_idx[p];
			if (!(mask >>= 1))
				mask = 1 << 15;
			else
				s -= 8;
		}
		src += inc / sizeof(*src);
		dst += (int) dst & 1;
	}
	return xTrue;
}

/* ------------------------------------------------------------------------------ */
static BOOL _r_get_P8(MFDB *mfdb, PRECT *pxy, MFDB *ptr)
{
	char *src;
	char *dst = mfdb->fd_addr;
	int inc = mfdb->fd_wdwidth * 2 * 8;
	int h = mfdb->fd_h;

	if (ptr)
	{
		src = (char *) ptr->fd_addr + (inc * pxy[0].lu.p_y) + pxy[0].lu.p_x;
	} else if ((src = malloc(inc * h)))
	{
		MFDB scrn = { NULL, };
		mfdb->fd_addr = src;
		v_hide_c(GRPH_Vdi);
		vro_cpyfm(GRPH_Vdi, S_ONLY, (short *) pxy, &scrn, mfdb);
		v_show_c(GRPH_Vdi, 1);
	} else
	{
		return xFalse;
	}

	while (h--)
	{
		char *s = src;
		short w = mfdb->fd_w;

		while (w--)
		{
			char c = *(s++);

			if (c < 16)
			{
				static const char pix_idx[16] = {
					0, 2, 3, 6, 4, 7, 5, 8, 9, 10, 11, 14, 12, 15, 13, 1
				};
				c = pix_idx[(int) c];
			}
			*(dst++) = c;
		}
		src += inc / sizeof(*src);
		dst += (int) dst & 1;
	}
	return xTrue;
}

/* ------------------------------------------------------------------------------ */
static BOOL _r_get_16(MFDB *mfdb, PRECT *pxy, MFDB *ptr)
{
	short *src = NULL;
	short *dst = mfdb->fd_addr;
	int inc = mfdb->fd_wdwidth * 2 * 16;
	int h = mfdb->fd_h;

	if (ptr)
	{
		src = (short *) ((char *) ptr->fd_addr + (inc * pxy[0].lu.p_y)) + pxy[0].lu.p_x;

	} else if (!(mfdb->fd_w & 0x000F) || (src = malloc(inc * h)))
	{
		MFDB scrn = { NULL, };
		if (src)
			mfdb->fd_addr = src;
		else
			h = 0;
		v_hide_c(GRPH_Vdi);
		vro_cpyfm(GRPH_Vdi, S_ONLY, (short *) pxy, &scrn, mfdb);
		v_show_c(GRPH_Vdi, 1);

	} else
	{
		return xFalse;
	}

	while (h--)
	{
		short *s = src;
		short w = mfdb->fd_w;

		while (w--)
		{
			*(dst++) = *(s++);
		}
		src += (short) (inc / sizeof(*src));
	}
	return xTrue;
}

/* ------------------------------------------------------------------------------ */
static BOOL _r_get_24(MFDB *mfdb, PRECT *pxy, MFDB *ptr)
{
	short *src = NULL;
	short *dst = mfdb->fd_addr;
	int inc = mfdb->fd_wdwidth * 2 * 24;
	int h = mfdb->fd_h;

	if (ptr)
	{
		src = (short *) ((char *) ptr->fd_addr + (inc * pxy[0].lu.p_y)) + pxy[0].lu.p_x;

	} else if (!(mfdb->fd_w & 0x000F) || (src = malloc(inc * h)))
	{
		MFDB scrn = { NULL, };
		if (src)
			mfdb->fd_addr = src;
		else
			h = 0;
		v_hide_c(GRPH_Vdi);
		vro_cpyfm(GRPH_Vdi, S_ONLY, (short *) pxy, &scrn, mfdb);
		v_show_c(GRPH_Vdi, 1);

	} else
	{
		return xFalse;
	}

	while (h--)
	{
		short *s = src;
		short w = (mfdb->fd_w * 3 + 1) / 2;

		while (w--)
		{
			*(dst++) = *(s++);
		}
		src += inc / sizeof(*src);
	}
	return xTrue;
}

/* ------------------------------------------------------------------------------ */
static BOOL _r_get_32(MFDB *mfdb, PRECT *pxy, MFDB *ptr)
{
	long *src = NULL;
	long *dst = mfdb->fd_addr;
	int inc = mfdb->fd_wdwidth * 2 * 32;
	int h = mfdb->fd_h;

	if (ptr)
	{
		src = (long *) ((char *) ptr->fd_addr + (inc * pxy[0].lu.p_y)) + pxy[0].lu.p_x;

	} else if (!(mfdb->fd_w & 0x000F) || (src = malloc(inc * h)))
	{
		MFDB scrn = { NULL, };
		if (src)
			mfdb->fd_addr = src;
		else
			h = 0;
		v_hide_c(GRPH_Vdi);
		vro_cpyfm(GRPH_Vdi, S_ONLY, (short *) pxy, &scrn, mfdb);
		v_show_c(GRPH_Vdi, 1);

	} else
	{
		return xFalse;
	}

	while (h--)
	{
		long *s = src;
		short w = mfdb->fd_w;

		while (w--)
		{
			*(dst++) = *(s++);
		}
		src += inc / sizeof(*src);
	}
	return xTrue;
}


/* ============================================================================== */
/* */
/* Byte order dependent Callback Functions */

/* ------------------------------------------------------------------------------ */
void FT_Grph_ShiftArc_MSB(const PXY *origin, xArc *arc, size_t num, short mode)
{
	while (num--)
	{
		if (mode == ArcChord)
		{
			arc->angle1 = 0;
			arc->angle2 = 3600;
		} else
		{
			INT16 beg = (INT16) (((long) arc->angle2 * 5) >> 5);
			INT16 end = (INT16) (((long) arc->angle1 * 5) >> 5);

			arc->angle1 = (beg > 0 ? (beg >= 3600 ? 0 : 3600 - beg) : (beg <= -3600 ? 3600 : -beg));
			arc->angle2 = (end > 0 ? (end >= 3600 ? 0 : 3600 - end) : (end <= -3600 ? 3600 : -end));
		}
		arc->x += origin->p_x + (arc->width /= 2);
		arc->y += origin->p_y + (arc->height /= 2);
		arc++;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void FT_Grph_ShiftArc_LSB(const PXY *origin, xArc *arc, size_t num, short mode)
{
	while (num--)
	{
		if (mode == ArcChord)
		{
			arc->angle1 = 0;
			arc->angle2 = 3600;
		} else
		{
			INT16 beg = (INT16) (((long) Swap16(arc->angle2) * 5) >> 5);
			INT16 end = (INT16) (((long) Swap16(arc->angle1) * 5) >> 5);

			arc->angle1 = (beg > 0 ? (beg >= 3600 ? 0 : 3600 - beg) : (beg <= -3600 ? 3600 : -beg));
			arc->angle2 = (end > 0 ? (end >= 3600 ? 0 : 3600 - end) : (end <= -3600 ? 3600 : -end));
		}
		arc->width = Swap16(arc->width) / 2;
		arc->height = Swap16(arc->height) / 2;
		arc->x = Swap16(arc->x) + origin->p_x + arc->width;
		arc->y = Swap16(arc->y) + origin->p_y + arc->height;
		arc++;
	}
}

/* ------------------------------------------------------------------------------ */
void FT_Grph_ShiftPnt_MSB(const PXY *origin, PXY *pxy, size_t num, short mode)
{
	if (!num)
		return;

	if (origin)
	{
		pxy->p_x += origin->p_x;
		pxy->p_y += origin->p_y;
	}
	if (mode == CoordModePrevious)
	{
		while (--num)
		{
			PXY *prev = pxy++;

			pxy->p_x += prev->p_x;
			pxy->p_y += prev->p_y;
		}

	} else if (origin)
	{									/* && CoordModeOrigin */
		while (--num)
		{
			pxy++;
			pxy->p_x += origin->p_x;
			pxy->p_y += origin->p_y;
		}
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void FT_Grph_ShiftPnt_LSB(const PXY *origin, PXY *pxy, size_t num, short mode)
{
	if (!num)
		return;

	if (origin)
	{
		pxy->p_x = Swap16(pxy->p_x) + origin->p_x;
		pxy->p_y = Swap16(pxy->p_y) + origin->p_y;
	} else
	{
		SwapPXY(pxy, pxy);
	}
	if (mode == CoordModePrevious)
	{
		while (--num)
		{
			PXY *prev = pxy++;

			pxy->p_x = Swap16(pxy->p_x) + prev->p_x;
			pxy->p_y = Swap16(pxy->p_y) + prev->p_y;
		}

	} else if (origin)
	{									/* && CoordModeOrigin */
		while (--num)
		{
			pxy++;
			pxy->p_x = Swap16(pxy->p_x) + origin->p_x;
			pxy->p_y = Swap16(pxy->p_y) + origin->p_y;
		}

	} else
	{									/* CoordModeOrigin && !origin */
		while (--num)
		{
			pxy++;
			SwapPXY(pxy, pxy);
		}
	}
}

/* ------------------------------------------------------------------------------ */
void FT_Grph_ShiftR2P_MSB(const PXY *origin, GRECT *rct, size_t num)
{
	if (origin)
	{
		while (num--)
		{
			rct->g_w += (rct->g_x += origin->p_x) - 1;
			rct->g_h += (rct->g_y += origin->p_y) - 1;
			rct++;
		}
	} else
	{
		while (num--)
		{
			rct->g_w += rct->g_x - 1;
			rct->g_h += rct->g_y - 1;
			rct++;
		}
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void FT_Grph_ShiftR2P_LSB(const PXY *origin, GRECT *rct, size_t num)
{
	if (origin)
	{
		while (num--)
		{
			rct->g_w = Swap16(rct->g_w) + (rct->g_x = Swap16(rct->g_x) + origin->p_x) - 1;
			rct->g_h = Swap16(rct->g_h) + (rct->g_y = Swap16(rct->g_y) + origin->p_y) - 1;
			rct++;
		}
	} else
	{
		while (num--)
		{
			rct->g_w = Swap16(rct->g_w) + (rct->g_x = Swap16(rct->g_x)) - 1;
			rct->g_h = Swap16(rct->g_h) + (rct->g_y = Swap16(rct->g_y)) - 1;
			rct++;
		}
	}
}



/* ============================================================================== */
/* */
/* Callback Functions */

#include "Request.h"

/* ------------------------------------------------------------------------------ */
void RQ_QueryBestSize(CLIENT *clnt, xQueryBestSizeReq *q)
{
	ClntReplyPtr(QueryBestSize, r, 0);

	PRINT(-X_QueryBestSize, " #%i of D:%lX (%u/%u)", q->class, q->drawable, q->width, q->height);

	r->width = 16;
	r->height = 16;

	ClntReply(QueryBestSize, 0, "P");
}

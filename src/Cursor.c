/*
 *==============================================================================
 *
 * Cursor.c -- Implementation of struct 'CURSOR' related functions.
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-07 - Module released for beta state.
 * 2000-09-07 - Initial Version.
 *==============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "clnt.h"
#include "tools.h"
#include "Cursor.h"
#include "pixmap.h"
#include "grph.h"
#include "colormap.h"
#include "x_gem.h"
#include "Xapp.h"
#include "x_printf.h"

#include <X11/X.h>


static CARD16 _CRSR_LastGlyph = 0;;
static OBJECT *_CRSR_Glyph = NULL;
static CURSOR *_CRSR_Active = NULL;
static XRSCPOOL(CURSOR, _CRSR_Orphaned, 4);


/* ============================================================================== */
void CrsrInit(BOOL initNreset)
{
	if (initNreset)
	{
		rsrc_gaddr(R_TREE, GLYPH, &_CRSR_Glyph);
		_CRSR_LastGlyph = (_CRSR_Glyph->ob_tail - _CRSR_Glyph->ob_head) * 2 + 1;
		XrscPoolInit(_CRSR_Orphaned);

	} else
	{
		int i;
		int n = 0;

		CrsrSelect(NULL);

		for (i = 0; i < XrscPOOLSIZE(_CRSR_Orphaned); ++i)
		{
			p_CURSOR crsr;

			while ((crsr = XrscPOOLITEM(_CRSR_Orphaned, i)))
			{
				if (!n)
					x_printf("  delete orphaned Cursor(s)");
				x_printf(" [%p]:0x%X(%lu)", crsr, crsr->Id, crsr->Reffs);
				XrscDelete(_CRSR_Orphaned, crsr);
				n++;
			}
		}
		if (n)
			x_printf("  (%i)\n", n);
	}
}


/* ------------------------------------------------------------------------------ */
static p_CURSOR _Crsr_Find(CARD32 id)
{
	/* Search cursor in the given list, either in the clients list or in the */
	/* list of freed cursors. */

	CLIENT *clnt = ClntFind(id);
	CURSOR *crsr = (clnt ? Xrsc(CURSOR, id, clnt->Cursors) : NULL);

	return crsr;
}

/* ============================================================================== */
p_CURSOR CrsrGet(CARD32 id)
{
	/* Find cursor in the clients list and increment refference counter.  To be */
	/* used by other resources. */

	return CrsrShare(_Crsr_Find(id));
}

/* ============================================================================== */
void CrsrFree(p_CURSOR crsr, p_CLIENT clnt)
{
	/* If clnt is given, remove cursor from the clients id-pool.  In either case, */
	/* the refference counter will be decremented and the curser deleted it not */
	/* longer refferenced. */

	if (clnt)
	{
		if (--crsr->Reffs)
		{
			XrscRemove(clnt->Cursors, crsr);
			XrscInsert(_CRSR_Orphaned, crsr);
		} else
		{
			XrscDelete(clnt->Cursors, crsr);
		}
	} else if (!--crsr->Reffs && !XrscDelete(_CRSR_Orphaned, crsr))
	{
		x_printf("\033pWARNING\033q: Stale cursor C:%X!\n", crsr->Id);
	}
}


/* ============================================================================== */
void CrsrSelect(CURSOR *crsr)
{
	/* Update mouse pointer.  If no cursor given, switch back to the normal GEM */
	/* pointer. */

	if (crsr != _CRSR_Active)
	{
		if (_CRSR_Active)
		{
			CrsrFree(_CRSR_Active, NULL);
		}
		if ((_CRSR_Active = CrsrShare(crsr)))
		{
			graf_mouse(USER_DEF, (MFORM *) & crsr->HotSpot);
		} else
		{
			graf_mouse(ARROW, NULL);
		}
	}
}


/* ------------------------------------------------------------------------------ */
static void _Crsr_color(CURSOR *crsr, const RGB *fore, const RGB *back)
{
	/* Setup colors. */
	RGB tmp;
	CARD16 bgnd = CmapLookup(&tmp, back);
	CARD16 fgnd = CmapLookup(&tmp, fore);

	crsr->Bgnd = CmapPixelIdx(bgnd == fgnd ? ~bgnd : bgnd, GRPH_Depth);
	crsr->Fgnd = CmapPixelIdx(fgnd, GRPH_Depth);
}


/* ============================================================================== */
void CrsrSetGlyph(p_CURSOR crsr, CARD8 glyph)
{
	int n = _CRSR_Glyph[0].ob_head + (glyph / 2);
	ICONBLK *iblk = _CRSR_Glyph[n].ob_spec.iconblk;

	crsr->Depth = 1;
	crsr->HotSpot = *(PXY *) & iblk->ib_xchar;
	memcpy(crsr->Bmask, iblk->ib_pmask, 32);
	memcpy(crsr->Fmask, iblk->ib_pdata, 32);
}


/*
 *==============================================================================
 *
 * Callback Functions
 */
#include "Request.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void RQ_CreateCursor(CLIENT *clnt, xCreateCursorReq *q)
{
	/*
	 * Create a cursor from one or two pixmaps.
	 *
	 * Cursor cid:
	 * Pixmap source: foreground bitmap
	 * Pixmap mask:   background bitmap or None
	 * CARD16 x,y:    hotspot coordinates
	 * CARD16 foreRed, foreGreen, foreBlue: foreground color
	 * CARD16 backRed, backGreen, backBlue: background color
	 *...........................................................................
	 */
	CURSOR *crsr;
	PIXMAP *fgnd = PmapFind(q->source);
	PIXMAP *bgnd = NULL;

	if (!RID_Match(clnt->Id, q->cid))
	{
		Bad(BadIDChoice, q->cid, X_CreateCursor, "_(): doesn't match ID base.");

	} else if (_Crsr_Find(q->cid))
	{
		Bad(BadIDChoice, q->cid, X_CreateCursor, "_");
	} else if (!fgnd)
	{
		Bad(BadPixmap, q->source, X_CreateCursor, "_(C:%lX):\n" "          invalid sorce.", q->cid);
	} else if (fgnd->Depth != 1)
	{
		Bad(BadMatch, None, X_CreateCursor, "_(C:%lX):\n" "          source depth is %i.", q->cid, fgnd->Depth);
	} else if (q->x >= fgnd->W || q->y >= fgnd->H)
	{
		Bad(BadMatch, None, X_CreateCursor, "_(C:%lX):\n"
			"          hotspot %i,%i outside of source %i,%i.", q->cid, q->x, q->y, fgnd->W, fgnd->H);
	} else if (q->mask != None && !(bgnd = PmapFind(q->mask)))
	{
		Bad(BadPixmap, q->mask, X_CreateCursor, "_(C:%lX):\n" "          invalid mask.", q->cid);
	} else if (bgnd && bgnd->Depth != 1)
	{
		Bad(BadMatch, None, X_CreateCursor, "_(C:%lX):\n" "          mask depth is %i.", q->cid, bgnd->Depth);
	} else if (bgnd && (bgnd->W != fgnd->W || bgnd->H != fgnd->H))
	{
		Bad(BadMatch, None, X_CreateCursor, "_(C:%lX):\n"
			"          mask size %i,%i differes from source %i,%i.", q->cid, bgnd->W, bgnd->H, fgnd->W, fgnd->H);
	} else if (!(crsr = XrscCreate(CURSOR, q->cid, clnt->Cursors, 0)))
	{
		Bad(BadAlloc, None, X_CreateCursor, "_(C:%lX)", q->cid);
	} else
	{
		CARD16 *mem;
		int i;
		int h = (fgnd->H < 16 ? fgnd->H : 16);
		CARD16 msk = (fgnd->W < 16 ? ~((1 >> (fgnd->W - 1)) - 1) : 0xFFFF);

		DEBUG(X_CreateCursor, " C:%lX [%i,%i]", q->cid, fgnd->W, fgnd->H);

		crsr->Reffs = 1;
		crsr->Depth = 1;
		crsr->HotSpot.p_x = (q->x <= 15 ? q->x : 15);
		crsr->HotSpot.p_y = (q->y <= 15 ? q->y : 15);
		_Crsr_color(crsr, (RGB *) & q->foreRed, (RGB *) & q->backRed);
		mem = (CARD16 *) fgnd->Mem;
		for (i = 0; i < h; crsr->Fmask[i++] = *mem & msk)
			mem += fgnd->nPads;
		for (; i < 16; crsr->Fmask[i++] = 0)
			;
		if (!bgnd)
		{
			for (i = 0; i < 16; ++i)
				crsr->Bmask[i] = crsr->Fmask[i];
		} else
		{
			mem = (CARD16 *) bgnd->Mem;
			for (i = 0; i < h; crsr->Bmask[i++] = *mem & msk)
				mem += bgnd->nPads;
			for (; i < 16; crsr->Bmask[i++] = 0)
				;
		}
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void RQ_CreateGlyphCursor(CLIENT *clnt, xCreateGlyphCursorReq *q)
{
	/*
	 * Create a cursor from a font glyph.  The hotspot is taken from the glyphs 
	 * data.
	 *
	 * Cursor cid:
	 * Font   source:     font for foreground glyph         (ignored)
	 * Font   mask:       font for background glyph or None (ignored)
	 * CARD16 sourceChar: foreground glyph
	 * CARD16 maskChar:   background glyph or None          (ignored)
	 * CARD16 foreRed, foreGreen, foreBlue: foreground color
	 * CARD16 backRed, backGreen, backBlue: background color
	 *...........................................................................
	 */
	CURSOR *crsr;

	if (!RID_Match(clnt->Id, q->cid))
	{
		Bad(BadIDChoice, q->cid, X_CreateGlyphCursor, "_(): doesn't match ID base.");
	} else if (_Crsr_Find(q->cid))
	{
		Bad(BadIDChoice, q->cid, X_CreateGlyphCursor, "_");
	} else if (q->sourceChar > _CRSR_LastGlyph)
	{
		Bad(BadValue, q->sourceChar, X_CreateGlyphCursor, "_(C:%lX):\n" "          source glyph not defined.", q->cid);
	} else if (!(crsr = XrscCreate(CURSOR, q->cid, clnt->Cursors, 0)))
	{
		Bad(BadAlloc, None, X_CreateGlyphCursor, "_(C:%lX).", q->cid);
	} else
	{
		DEBUG(X_CreateGlyphCursor, " C:%lX #%u", q->cid, q->sourceChar);

		crsr->Reffs = 1;
		_Crsr_color(crsr, (RGB *) & q->foreRed, (RGB *) & q->backRed);
		CrsrSetGlyph(crsr, q->sourceChar);
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void RQ_FreeCursor(CLIENT *clnt, xFreeCursorReq *q)
{
	/*
	 * Deletes the association between the cursor and its id.  Its storage will
	 * be freed when it's no more refferenced.
	 *
	 * CARD32 id: cursor id
	 *...........................................................................
	 */
	CURSOR *crsr = _Crsr_Find(q->id);

	if (!crsr)
	{
		Bad(BadCursor, q->id, X_FreeCursor, "_");
	} else
	{
		DEBUG(X_FreeCursor, " C:%lX", q->id);

		CrsrFree(crsr, ClntFind(q->id));
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void RQ_RecolorCursor(CLIENT *clnt, xRecolorCursorReq *q)
{
	/*
	 * Change the cursor's color
	 *
	 * Cursor cursor: 
	 * CARD16 foreRed, foreGreen, foreBlue: foreground color
	 * CARD16 backRed, backGreen, backBlue: background color
	 *...........................................................................
	 */
	CURSOR *crsr = _Crsr_Find(q->cursor);

	if (!crsr)
	{
		Bad(BadCursor, q->cursor, X_RecolorCursor, "_");
	} else
	{
		DEBUG(X_RecolorCursor, " C:%lX", q->cursor);

		_Crsr_color(crsr, (RGB *) & q->foreRed, (RGB *) & q->backRed);
		if (_CRSR_Active == crsr)
		{
			graf_mouse(USER_DEF, (MFORM *) & crsr->HotSpot);
		}
	}
}

/*
 *==============================================================================
 *
 * gcontext.c
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-06-05 - Initial Version.
 *==============================================================================
 */
#ifndef __GCONTEXT_H__
#define __GCONTEXT_H__

#include "fontable.h"
#include "types.h"

#include <X11/X.h>
#include <X11/Xproto.h>


typedef struct s_GC
{
	XRSC(FONTABLE, isFont);

	struct s_FONTFACE *FontFace;
	short FontIndex;
	unsigned short FontEffects;
	unsigned short FontPoints;
	unsigned short FontWidth;

	CARD16 Depth;
	CARD8 Function;
	BOOL GraphExpos;
	CARD8 SubwindMode;
	CARD8 DashList;
	CARD16 DashOffset;
	CARD16 LineWidth;
	CARD8 LineStyle;
	CARD8 CapStyle;
	CARD8 JoinStyle;
	CARD8 FillStyle;
	CARD8 FillRule;
	CARD8 ArcMode;
	CARD32 PlaneMask;
	CARD32 Foreground;
	CARD32 Background;
	p_PIXMAP Tile;
	p_PIXMAP Stipple;
	p_PIXMAP ClipMask;
	PXY TileStip;
	PXY Clip;

	GRECT *ClipRect;
	short ClipNum;
} GC;


void GcntDelete(p_GC, p_CLIENT);

static inline p_GC GcntFind(CARD32 id)
{
	p_GC gc = FablFind(id).Gc;

	if ((DBG_XRSC_TypeError = (gc && gc->isFont)))
		gc = NULL;
	return gc;
}


#endif /* __GCONTEXT_H__ */

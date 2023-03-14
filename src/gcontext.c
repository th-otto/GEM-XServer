//==============================================================================
//
// gcontext.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-06-05 - Initial Version.
//==============================================================================
//
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "clnt.h"
#include "tools.h"
#include "gcontext.h"
#include "font.h"
#include "pixmap.h"
#include "grph.h"
#include "colormap.h"
#include "x_gem.h"


//==============================================================================
void
GcntDelete (p_GC gc, p_CLIENT clnt)
{
	if (gc->Tile)     PmapFree (gc->Tile, NULL);
	if (gc->Stipple)  PmapFree (gc->Stipple, NULL);
	if (gc->ClipMask) PmapFree (gc->ClipMask, NULL);
	if (gc->ClipRect) free     (gc->ClipRect);
	XrscDelete (clnt->Fontables, gc);
}


//------------------------------------------------------------------------------
static void
_Gcnt_setup (CLIENT * clnt, GC * gc, CARD32 mask, CARD32 * val, CARD8 req)
{
	int i = 0;
	
	if (clnt->DoSwap) {
		CARD32   m = mask;
		CARD32 * p = val;
		while (m) {
			if (m & 1) {
				*p = Swap32(*p);
				p++;
				i++;
			}
			m >>= 1;
		}
	
	} else if (mask) {
		CARD32 m = mask;
		while (m) {
			if (m & 1) i++;
			m >>= 1;
		}
	
	} else {
		return; // mask == 0, nothing to do
	}
	
	if (i) DEBUG (,"+-\n          (%i):", i);
	
	if (mask & GCFunction) {
		CARD8 funct = *(val++);
		if (funct <= GXset) {
			gc->Function = funct;
			DEBUG (,"+- func=%u", funct);
		} else {
			PRINT(0," ");
			Bad(BadValue, funct, req, "_          invalid function code.");
			return;
		}
	}
	if (mask & GCPlaneMask) {
		gc->PlaneMask = *(val++);
		DEBUG (,"+- pmsk=%lu", gc->PlaneMask);
	}
	if (mask & GCForeground) {
		gc->Foreground = CmapPixelIdx (*(val++), gc->Depth);
		DEBUG (,"+- fgnd=%lu", gc->Foreground);
	}
	if (mask & GCBackground) {
		gc->Background = CmapPixelIdx (*(val++), gc->Depth);
		DEBUG (,"+- bgnd=%lu", gc->Background);
	}
	if (mask & GCLineWidth) {
		gc->LineWidth = *(val++);
		DEBUG (,"+- lwid=%u", gc->LineWidth);
	}
	if (mask & GCLineStyle) {
		CARD8 style = *(val++);
		if (style <= LineDoubleDash) {
			gc->LineStyle = style;
			DEBUG (,"+- lsty=%u", style);
		} else {
			PRINT(0," ");
			Bad(BadValue, style, req, "_          invalid line-style.");
			return;
		}
	}
	if (mask & GCCapStyle) {
		CARD8 style = *(val++);
		if (style <= CapProjecting) {
			gc->CapStyle = style;
			DEBUG (,"+- csty=%u", style);
		} else {
			PRINT(0," ");
			Bad(BadValue, style, req, "_          invalid cap-style.");
			return;
		}
	}
	if (mask & GCJoinStyle) {
		CARD8 style = *(val++);
		if (style <= JoinBevel) {
			gc->JoinStyle = style;
			DEBUG (,"+- jsty=%u", style);
		} else {
			PRINT(0," ");
			Bad(BadValue, style, req, "_          invalid join-style.");
			return;
		}
	}
	if (mask & GCFillStyle) {
		CARD8 style = *(val++);
		if (style <= FillOpaqueStippled) {
			gc->FillStyle = style;
			DEBUG (,"+- fsty=%u", style);
		} else {
			PRINT(0," ");
			Bad(BadValue, style, req, "_          invalid fill-style.");
			return;
		}
	}
	if (mask & GCFillRule) {
		CARD8 rule = *(val++);
		if (rule <= WindingRule) {
			gc->FillRule = rule;
			DEBUG (,"+- frul=%u", rule);
		} else {
			PRINT(0," ");
			Bad(BadValue, rule, req, "_          invalid fill-rule.");
			return;
		}
	}
	if (mask & GCTile) {
		PIXMAP * tile = PmapFind (*val);
		if (!tile) {
			PRINT(0," ");
			Bad(BadPixmap, *val, req, "_          invalid tile.");
			return;
		} else if (tile->Depth != gc->Depth) {
			PRINT(0," ");
			Bad(BadMatch,0, req, "_          Tile depth %u not %u.",
			                  tile->Depth, gc->Depth);
			return;
		} else {
			DEBUG (,"+- tile=0x%lX", *val);
			if (gc->Tile) {
				PmapFree (gc->Tile, NULL);
			}
			gc->Tile = PmapShare(tile);
		}
		val++;
	}
	if (mask & GCStipple) {
		PIXMAP * stip = PmapFind (*val);
		if (!stip) {
			PRINT(0," ");
			Bad(BadPixmap, *val, req, "_          invalid stipple.");
			return;
		} else if (stip->Depth != 1) {
			PRINT(0," ");
			Bad(BadMatch,0, req, "_          Stipple depth %u.", stip->Depth);
			return;
		} else {
			DEBUG (,"+- stip=0x%lX", *val);
			if (gc->Stipple) {
				PmapFree (gc->Stipple, NULL);
			}
			gc->Stipple = PmapShare(stip);
		}
		val++;
	}
	if (mask & GCTileStipXOrigin) {
		gc->TileStip.p_x = *(val++);
		DEBUG (,"+- stpx=%i", gc->TileStip.p_x);
	}
	if (mask & GCTileStipYOrigin) {
		gc->TileStip.p_y = *(val++);
		DEBUG (,"+- stpy=%i", gc->TileStip.p_y);
	}
	if (mask & GCFont) {
		if (!FontValues ((p_FONTABLE)gc, *val)) {
			Bad(BadFont, *val, req,"_");
		} else {
			DEBUG (,"+- font=0x%lX", *val);
		}
		val++;
	}
	if (mask & GCSubwindowMode) {
		CARD8 mode = *(val++);
		if (mode <= IncludeInferiors) {
			gc->SubwindMode = mode;
			DEBUG (,"+- subw=%u", mode);
		} else {
			PRINT(0," ");
			Bad(BadValue, mode, req, "_          invalid subwindow-mode.");
			return;
		}
	}
	if (mask & GCGraphicsExposures) {
		CARD8 mode = *(val++);
		if (mode <= xTrue) {
			gc->GraphExpos = mode;
			DEBUG (,"+- exps=%u", mode);
		} else {
			PRINT(0," ");
			Bad(BadValue, mode, req, "_          invalid graphics-exposures.");
			return;
		}
	}
	if (mask & GCClipXOrigin) {
		gc->Clip.p_x = *(val++);
		DEBUG (,"+- clpx=%i", gc->Clip.p_x);
	}
	if (mask & GCClipYOrigin) {
		gc->Clip.p_y = *(val++);
		DEBUG (,"+- clpy=%i", gc->Clip.p_y);
	}
	if (mask & GCClipMask) {
		if (*val == None) {
			DEBUG (,"+- clip=<none>");
		} else {
			PIXMAP * clip = PmapFind (*val);
			if (!clip) {
				PRINT(0," ");
				Bad(BadPixmap, *val, req, "_          invalid clip-mask.");
				return;
			} else if (clip->Depth != 1) {
				PRINT(0," ");
				Bad(BadMatch,0, req, "_          clip-mask depth %u.", clip->Depth);
				return;
			} else {
				DEBUG (,"+- clip=P:%X", clip->Id);
				gc->ClipMask = PmapShare(clip);
			}
		}
		val++;
	}
	if (mask & GCDashOffset) {
		gc->DashOffset = *(val++);
		DEBUG (,"+- doff=%i", gc->DashOffset);
	}
	if (mask & GCDashList) {
		gc->DashList = *(val++);
		DEBUG (,"+- dlst=%i", gc->DashList);
	}
	if (mask & GCArcMode) {
		CARD8 mode = *(val++);
		if (mode <= ArcPieSlice) {
			gc->ArcMode = mode;
			DEBUG (,"+- arcm=%u", mode);
		} else {
			PRINT(0," ");
			Bad(BadValue, mode, req, "_          invalid arc-mode.");
			return;
		}
	}
}

//==============================================================================
//
// Callback Functions

#include "Request.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_CreateGC (CLIENT * clnt, xCreateGCReq * q)
{
	// Creates a Graphics Context with the given Id.
	//
	// GContext gc:       Id to be created
	// Drawable drawable: source to get the depth from
	// CARD32   mask:     bitmask to specify components to be initialized
	// CARD32 * (q +1):   list of component values
	//...........................................................................
	
	GC       * gc;
	p_DRAWABLE draw;
	
	if (GcntFind (q->gc)) {
		Bad(BadIDChoice, q->gc, X_CreateGC,"_");
	
	} else if (!(draw = DrawFind (q->drawable)).p) {
		Bad(BadDrawable, q->drawable, X_CreateGC,"_");
	
	} else if (q->mask & ~((2uL << GCLastBit) -1)) {
		Bad(BadValue, q->mask, X_CreateGC, "_invalid value mask 0x%lX.", q->mask);
	
	} else if (!(gc = XrscCreate (GC, q->gc, clnt->Fontables,0))) {
		Bad(BadAlloc,0, X_CreateGC,"_");
	
	} else { //..................................................................
	
		DEBUG (CreateGC,"- G:%lX for D:%lX", q->gc, q->drawable);
		
		gc->isFont = xFalse;
		
		FontValues ((p_FONTABLE)gc, None);
		
		gc->Depth       = draw.p->Depth;
		gc->Function    = GXcopy;
		gc->GraphExpos  = xTrue;
		gc->SubwindMode = ClipByChildren;
		gc->DashList    = 4;
		gc->DashOffset  = 0;
		gc->LineWidth   = 0;
		gc->LineStyle   = LineSolid;
		gc->CapStyle    = CapButt;
		gc->JoinStyle   = JoinMiter;
		gc->FillStyle   = FillSolid;
		gc->FillRule    = EvenOddRule;
		gc->ArcMode     = ArcChord;
		gc->PlaneMask   = (1uL << gc->Depth) -1;
		gc->Foreground  = CmapPixelIdx (0, gc->Depth);
		gc->Background  = CmapPixelIdx (1, gc->Depth);
		gc->Tile        = NULL;
		gc->Stipple     = NULL;
		gc->ClipMask    = NULL;
		gc->TileStip.p_x  = gc->TileStip.p_y = 0;
		gc->Clip.p_x      = gc->Clip.p_y     = 0;
		gc->ClipRect    = NULL;
		gc->ClipNum     = 0;
		
		_Gcnt_setup (clnt, gc, q->mask, (CARD32*)(q +1), X_CreateGC);
		
		DEBUG (,"+");
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_ChangeGC (CLIENT * clnt, xChangeGCReq * q)
{
	// Changes components in the given gc.  The value-list and -mask is the same
	// as for CreateGC.
	//
	// GContext gc:     Id to be changed
	// CARD32   mask:   bitmask to specify components to be initialized
	// CARD32 * (q +1): list of component values
	//...........................................................................
	
	GC * gc = GcntFind (q->gc);
	
	if (!gc) {
		Bad(BadGC, q->gc, X_ChangeGC,"_");
	
	} else if (q->mask & ~((2uL << GCLastBit) -1)) {
		Bad(BadValue, q->mask, X_ChangeGC, "_invalid value mask 0x%lX.", q->mask);
	
	} else { //..................................................................
		
		DEBUG (ChangeGC,"- G:%lX", q->gc);
		
		if (q->mask & GCClipMask) {
			if (gc->ClipMask) {
				PmapFree (gc->ClipMask, NULL);
				gc->ClipMask = NULL;
			} else if (gc->ClipRect) {
				free (gc->ClipRect);
				gc->ClipRect = NULL;
			}
			gc->ClipNum  = 0;
		}
		_Gcnt_setup (clnt, gc, q->mask, (CARD32*)(q +1), X_ChangeGC);
		
		DEBUG (,"+");
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_CopyGC (CLIENT * clnt, xCopyGCReq * q)
{
	// Copies components from source gc to destination gc.  The value-list and
	// -mask is the same as for CreateGC.
	//
	// GContext srcGC:  source Id to copy from
	// GContext dstGC:  destination Id
	// CARD32   mask:   bitmask to specify components to be initialized
	// CARD32 * (q +1): list of component values
	//...........................................................................
	
	GC * src = GcntFind (q->srcGC);
	GC * dst = GcntFind (q->dstGC);
	
	if (!src) {
		Bad(BadGC, q->srcGC, X_CopyGC,"_: invalid source.");
	
	} else if (!dst) {
		Bad(BadGC, q->dstGC, X_CopyGC,"_: invalid destination.");
	
	} else if (src->Depth != dst->Depth) {
		Bad(BadMatch,0, X_CopyGC,"_(G:%lX,G:%lX):\n          source depth %u is not %u.",
		            q->srcGC, q->dstGC, src->Depth, dst->Depth);
	
	} else if (q->mask & ~((2uL << GCLastBit) -1)) {
		Bad(BadValue, q->mask, X_CopyGC, "_(G:%lX,G:%lX):\n"
		                    "          invalid value mask 0x%lX.",
		                    q->srcGC, q->dstGC, q->mask);
	
	} else { //..................................................................
		
		DEBUG (CopyGC," G:%lX to G:%lX mask=%06lX", q->srcGC, q->dstGC, q->mask);
		
		if (q->mask & GCTile) {
			if (dst->Tile) PmapFree  (dst->Tile, NULL);
			dst->Tile    = PmapShare (src->Tile);
		}
		if (q->mask & GCStipple) {
			if (dst->Stipple) PmapFree  (dst->Stipple, NULL);
			dst->Stipple    = PmapShare (src->Stipple);
		}
		if (q->mask & GCClipMask) {
			if (dst->ClipMask) {
				PmapFree  (dst->ClipMask, NULL);
			} else if (dst->ClipRect) {
				free (dst->ClipRect);
				dst->ClipRect = NULL;
			}
			dst->ClipNum  = 0;
			dst->ClipMask = PmapShare (src->ClipMask);
		}
		if (q->mask & GCFont) FontCopy ((p_FONTABLE)dst, (p_FONTABLE)src);
		if (q->mask & GCFunction)          dst->Function    = src->Function;
		if (q->mask & GCGraphicsExposures) dst->GraphExpos  = src->GraphExpos;
		if (q->mask & GCSubwindowMode)     dst->SubwindMode = src->SubwindMode;
		if (q->mask & GCDashList)          dst->DashList    = src->DashList;
		if (q->mask & GCDashOffset)        dst->DashOffset  = src->DashOffset;
		if (q->mask & GCLineWidth)         dst->LineWidth   = src->LineWidth;
		if (q->mask & GCLineStyle)         dst->LineStyle   = src->LineStyle;
		if (q->mask & GCCapStyle)          dst->CapStyle    = src->CapStyle;
		if (q->mask & GCJoinStyle)         dst->JoinStyle   = src->JoinStyle;
		if (q->mask & GCFillStyle)         dst->FillStyle   = src->FillStyle;
		if (q->mask & GCFillRule)          dst->FillRule    = src->FillRule;
		if (q->mask & GCArcMode)           dst->ArcMode     = src->ArcMode;
		if (q->mask & GCPlaneMask)         dst->PlaneMask   = src->PlaneMask;
		if (q->mask & GCForeground)        dst->Foreground  = src->Foreground;
		if (q->mask & GCBackground)        dst->Background  = src->Background;
		if (q->mask & GCTileStipXOrigin)   dst->TileStip.p_x  = src->TileStip.p_x;
		if (q->mask & GCTileStipYOrigin)   dst->TileStip.p_y  = src->TileStip.p_y;
		if (q->mask & GCClipXOrigin)       dst->Clip.p_x      = src->Clip.p_x;
		if (q->mask & GCClipYOrigin)       dst->Clip.p_y      = src->Clip.p_y;
	}
}

//------------------------------------------------------------------------------
void
RQ_SetDashes (CLIENT * clnt, xSetDashesReq * q)
{
	// GContext gc:
	// CARD16   dashOffset:
	// CARD16   nDashes:
	// CARD8  * (q +1):
	//...........................................................................
	
	PRINT (- X_SetDashes," G:%lX offs=%u (%u)",
	       q->gc, q->dashOffset, q->nDashes);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_SetClipRectangles (CLIENT * clnt, xSetClipRectanglesReq * q)
{
	// Change the clip-mask(s) and clip origin in the GC.  An empty rectangle
	// list disables output.
	//
	// GContext gc:
	// INT16    xOrigin:
	// INT16    yOrigin:
	// BYTE     ordering:
	GRECT * src = (GRECT*)(q +1);
	//...........................................................................
	
	GC    * gc  = GcntFind (q->gc);
	GRECT * clp = NULL;
	size_t  num = ((q->length *4) - sizeof (xSetClipRectanglesReq))
	                / sizeof(GRECT);
	
	if (!gc) {
		Bad(BadGC, q->gc, X_SetClipRectangles,"_");
	
	} else if (num && !(clp = malloc (sizeof(GRECT) * num))) {
		Bad(BadAlloc,0, X_SetClipRectangles,"_");
	
	} else { //..................................................................
		
		DEBUG (SetClipRectangles," G:%lX (%i,%i) %li",
		       q->gc, q->xOrigin, q->yOrigin, num);
		
		if (gc->ClipMask) {
			PmapFree (gc->ClipMask, NULL);
			gc->ClipMask = NULL;
		}
		if (gc->ClipRect) {
			free (gc->ClipRect);
		}
		if ((gc->ClipRect = clp)) {
			memcpy (clp, src, sizeof(GRECT) * num);
			gc->ClipNum = num;
		} else {
			gc->ClipNum = -1; // disables output
		}
		gc->Clip.p_x = q->xOrigin;
		gc->Clip.p_y = q->yOrigin;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_FreeGC (CLIENT * clnt, xFreeGCReq * q)
{
	// Deletes the given gc.
	//
	// CARD32 id: gcontext
	//...........................................................................
	
	GC * gc = GcntFind (q->id);
	
	if (!gc) {
		Bad(BadGC, q->id, X_FreeGC,"_");
	
	} else { //..................................................................
		
		DEBUG (FreeGC," G:%lX", q->id);
		
		GcntDelete (gc, ClntFind (q->id));
	}
}

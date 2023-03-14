//==============================================================================
//
// pixmap.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-06-05 - Initial Version.
//==============================================================================
//
#include "pixmap_P.h"
#include "grph.h"
#include "colormap.h"
#include "x_gem.h"
#include "gemx.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h> // memcpy memset
#include <setjmp.h>

#include <X11/X.h>


static XRSCPOOL(PIXMAP, _PMAP_Orphaned, 4);
static PIXMAP         * _PMAP_Offscreen[16];
static BYTE             _PMAP_Offs_Count = 0;


//==============================================================================
void
PmapInit (BOOL initNreset)
{
	if (initNreset) {
		XrscPoolInit (_PMAP_Orphaned);
		memset (_PMAP_Offscreen, 0, sizeof(_PMAP_Offscreen));
		
	} else {
		int i, n = 0;
		for (i = 0; i < XrscPOOLSIZE (_PMAP_Orphaned); ++i) {
			p_PIXMAP pmap;
			while ((pmap = XrscPOOLITEM (_PMAP_Orphaned, i))) {
				if (!n) printf ("  delete orphaned Pixmap(s)");
				printf (" 0x%X(%u)", pmap->Id, pmap->Reffs);
				XrscDelete (_PMAP_Orphaned, pmap);
				n++;
			}
		}
		if (n) printf ("  (%i)\n", n);
	}
}


//------------------------------------------------------------------------------
static void
_Pmap_VdiCls (PIXMAP * pmap)
{
	_PMAP_Offscreen[pmap->TabIdx] = NULL;
	if (pmap->Fonts) {
		vst_unload_fonts (pmap->Vdi, 0);
		pmap->Fonts = xFalse;
	}
	v_clsbm (pmap->Vdi);
	pmap->Vdi = 0;
}


//==============================================================================
void
PmapFree (PIXMAP * pmap, p_CLIENT clnt)
{
	// If clnt is given, remove pixmap from the clients id-pool.  In either case,
	// the refference counter will be decremented and the pixmap deleted it not
	// longer refferenced.
	
	if (clnt) {
		if (pmap->Vdi > 0) {
			_Pmap_VdiCls (pmap);
		}
		if (--pmap->Reffs) {
			XrscRemove (clnt->Drawables, pmap);
			XrscInsert (_PMAP_Orphaned,  pmap);
		} else {
			XrscDelete (clnt->Drawables, pmap);
		}
	} else if (!--pmap->Reffs && !XrscDelete (_PMAP_Orphaned, pmap)) {
		printf ("\33pWARNING\33q: Stale pixmap P:%X!\n", pmap->Id);
	}
}


//==============================================================================
short
PmapVdi (p_PIXMAP pmap, p_GC gc, BOOL fonts)
{
	if (!pmap->Vdi) {
		short w_in[20] = { 1, SOLID,gc->Foreground,
		                   MRKR_DOT,gc->Foreground, 1,gc->Foreground,
		                   FIS_SOLID,0,gc->Foreground, 2,
		                   pmap->nPads * 16 -1, pmap->H -1,
		                   GRPH_muWidth, GRPH_muHeight, 0,0,0,0,0 };
		short w_out[57];
		short hdl = GRPH_Handle;
		
		v_opnbm (w_in, PmapMFDB(pmap), &hdl, w_out);
		if (hdl <= 0) {
			extern CLIENT * CLNT_Requestor;
			extern jmp_buf  CLNT_Error;
			CARD8 t = ((xReq*)(CLNT_Requestor->iBuf.Mem))->reqType & 0x7F;
			printf ("\33pError\33q"
			        " Can't initialize VDI offscreen bitmap for P:%X (%s)!\n",
			        pmap->Id, RequestTable[t].Name);
			longjmp (CLNT_Error, 4);
		}
		pmap->Vdi = hdl;
		vsf_perimeter (hdl, PERIMETER_OFF);
		if (pmap->Depth > 8) CmapPalette (hdl);
		
		if (_PMAP_Offscreen[_PMAP_Offs_Count]) {
			_Pmap_VdiCls (_PMAP_Offscreen[_PMAP_Offs_Count]);
		}
		_PMAP_Offscreen[_PMAP_Offs_Count] = pmap;
		pmap->TabIdx     = _PMAP_Offs_Count;
		_PMAP_Offs_Count = (_PMAP_Offs_Count +1) & 0x000F;
		
	//	printf ("owk -> #%i \n", pmap->Vdi);
	
	}
	
	if (fonts) {
		short dmy;
		if (!pmap->Fonts) {
			vst_alignment  (pmap->Vdi, TA_LEFT, TA_BASE, &dmy, &dmy);
			vst_load_fonts (pmap->Vdi, 0);
			pmap->Fonts = xTrue;
		}
		vst_font    (pmap->Vdi, gc->FontIndex);
		vst_color   (pmap->Vdi, gc->Foreground);
		vst_effects (pmap->Vdi, gc->FontEffects);
		if (gc->FontWidth) {
			vst_height (pmap->Vdi, gc->FontPoints, &dmy,&dmy,&dmy,&dmy);
			vst_width  (pmap->Vdi, gc->FontWidth,  &dmy,&dmy,&dmy,&dmy);
		} else {
			vst_point  (pmap->Vdi, gc->FontPoints, &dmy, &dmy, &dmy, &dmy);
		}
	}
	
	return (pmap->Vdi);
}


//==============================================================================
void
PmapPutMono (PIXMAP * pmap, p_GC gc, p_GRECT r, p_MFDB src)
{
	if (gc->Function == GXcopy  &&  pmap->Depth == 1
	    && !r[0].x && !r[1].x  &&  r[0].w == pmap->W) {
		memcpy ((char*)pmap->Mem    + (pmap->nPads *2 * r[1].y),
		        (char*)src->fd_addr + (pmap->nPads *2 * r[0].y),
		        pmap->Depth         *  pmap->nPads *2 * r[0].h);
	
	} else {
		short pxy[8] = { r[0].x,             r[0].y,
		                 r[0].x + r[0].w -1, r[0].y + r[0].h -1,
		                 r[1].x,             r[1].y,
		                 r[1].x + r[1].w -1, r[1].y + r[1].h -1 };
		short colors[2] = { gc->Foreground, gc->Background };
		short mode;
		
		if (pmap->Depth >= 16  &&  gc->Function == GXxor) {
			mode      = MD_TRANS;
			colors[0] = CmapRevertIdx (gc->Foreground);
		} else {
			mode      = (gc->Function == GXor ? MD_TRANS : MD_REPLACE);
			colors[0] = gc->Foreground;
			colors[1] = gc->Background;
		}
		vrt_cpyfm (GRPH_Vdi, mode, pxy, src, PmapMFDB(pmap), colors);
	}
}

//==============================================================================
void
PmapPutColor (PIXMAP * pmap, p_GC gc, p_GRECT r, p_MFDB src)
{
	short pxy[8] = { r[0].x,             r[0].y,
	                 r[0].x + r[0].w -1, r[0].y + r[0].h -1,
	                 r[1].x,             r[1].y,
	                 r[1].x + r[1].w -1, r[1].y + r[1].h -1 };
	
	vro_cpyfm (GRPH_Vdi, gc->Function, pxy, src, PmapMFDB(pmap));
}


//==============================================================================
void
PmapDrawPoints (p_PIXMAP pmap, p_GC gc, p_PXY pxy, int num)
{
	INT16 x = 0, y = 0;
	
#	define clnt NULL
	DEBUG (PolyPoint," P:%X G:%X (%i)", pmap->Id, gc->Id, num);
#	undef clnt
	
	if (gc->Foreground & 1) while (num--) {
		x =  pxy->x; y =  pxy->y;
		if (x >= 0  &&  x < pmap->W  &&  y >= 0  &&  y < pmap->H) {
			pmap->Mem[(pmap->nPads *2 * y) + (x /8)] |= 0x0080 >> (x & 7);
		}
		pxy++;
	} else while (num--) {
		x =  pxy->x; y =  pxy->y;
		if (x >= 0  &&  x < pmap->W  &&  y >= 0  &&  y < pmap->H) {
			pmap->Mem[(pmap->nPads *2 * y) + (x /8)] &= 0xFF7F >> (x & 7);
		}
		pxy++;
	}
}
	
//==============================================================================
void
PmapFillRects (p_PIXMAP pmap, p_GC gc, p_GRECT rec, int num)
{
#	define clnt NULL
	
	void (*f_col) (CARD16 *, CARD16 inc, CARD16 n, CARD16 pat);
	void (*f_rec) (CARD16 *, CARD16 inc, CARD16 n, CARD16 len);
	CARD32 pat = _Pmap_MonoFuncts (gc, &f_col, &f_rec);
	
	DEBUG (PolyFillRectangle," P:%X G:%X (%i)", pmap->Id, gc->Id, num);
	
	if (pat) while (num--) {
		int y   = (rec->y          <  0       ? 0                : rec->y);
		int h   = (rec->y + rec->h >= pmap->H ? pmap->H - rec->y : rec->h +1);
		int beg = (rec->x          <  0       ? 0                : rec->x);
		int end = (rec->x + rec->w >  pmap->W ?
		                                    pmap->W -1 : rec->x + rec->w);
		CARD16 * mem = ((CARD16*)pmap->Addr) + y * pmap->nPads;
		CARD16   lft = pat >> (beg & 0xF);
		CARD16   rgt = pat >> (end & 0xF);
		rec++;
		if (h <= 0  ||  beg > end) continue;
		
		beg /= 16;
		end /= 16;
		if (beg == end) {       (*f_col) (mem + beg,   pmap->nPads, h,
		                                                         lft & rgt);
		} else {
			if (lft != 0x0000)   (*f_col) (mem + beg++, pmap->nPads, h, lft);
			if (rgt != 0xFFFF)   (*f_col) (mem + end--, pmap->nPads, h, rgt);
			if      (beg == end) (*f_col) (mem + beg,   pmap->nPads, h, pat);
			else if (beg <  end) (*f_rec) (mem + beg,   pmap->nPads, h,
			                                                   end - beg +1);
		}
	}
#	undef clnt
}


//==============================================================================
//
// Callback Functions

#include "Request.h"

//------------------------------------------------------------------------------
void
RQ_CreatePixmap (CLIENT * clnt, xCreatePixmapReq * q)
{
	CARD16   pads = (q->width + 15) / 16;
	size_t   size = ((size_t)pads *2) * q->depth * q->height;
	PIXMAP * pmap = NULL;
	
	if (DrawFind(q->pid).p) {
		Bad(IDChoice, q->pid, CreatePixmap,);
		
	} else if ((short)q->width <= 0  ||  (short)q->height <= 0) {
		Bad(Value, (short)((short)q->width <= 0 ? q->width : q->height),
		           CreatePixmap," width = %i height = %i",
		           (short)q->width, (short)q->height);
	
	} else if (!q->depth) {
		Bad(Value, 0, CreatePixmap," depth = %i", q->depth);
	
	// else check depth 
	
	} else if (!(pmap = XrscCreate (PIXMAP, q->pid, clnt->Drawables, size -2))) {
		Bad(Alloc,, CreatePixmap,);
	
	} else {
		DEBUG (CreatePixmap," P:%lX [%i: %i(%u),%i] of D:%lX",
		       q->pid, q->depth, q->width,pads, q->height, q->drawable);
		
		pmap->isWind    = xFalse;
		
		pmap->Addr      = pmap->Mem;
		pmap->W         = q->width;
		pmap->H         = q->height;
		pmap->nPads     = pads;
		pmap->StdFormat = 0;
		pmap->Depth     = q->depth;
		
		pmap->Fonts     = xFalse;
		pmap->Reffs     = 1;
		pmap->Vdi       = 0;

memset (pmap->Mem, 0x55, size);
	}
}

//------------------------------------------------------------------------------
void
RQ_FreePixmap (CLIENT * clnt, xFreePixmapReq * q)
{
	p_PIXMAP pmap = PmapFind (q->id);
	
	if (!pmap) {
		Bad(Pixmap, q->id, FreePixmap,);
	
	} else {
		DEBUG (FreePixmap," P:%lX", q->id);
		
		PmapFree (pmap, ClntFind(q->id));
	}
}

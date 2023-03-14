//==============================================================================
//
// pixmap.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-06-05 - Initial Version.
//==============================================================================
//
#ifndef __PIXMAP_H__
#	define __PIXMAP_H__

#include "drawable.h"


typedef struct s_PIXMAP {
	XRSC(DRAWABLE, isWind);
	
	// match MFDB
	void * Addr;
	CARD16 W, H;
	CARD16 nPads;
	CARD16 StdFormat;
	CARD16 Depth;
	short  _reserved[3];
	//
	BOOL   Fonts : 1;
	CARD32 Reffs :31;
	short  Vdi;
	short  TabIdx;
	long   __magic;
	CARD8  Mem[2];
} PIXMAP;

static inline void WATCH_PMAP (const char * f, PIXMAP * p) { if (p) {
	long mgx = (long)p->NextXRSC.p + p->isWind + p->Id
	         + (long)p->Addr + p->W + p->H + p->nPads + p->StdFormat + p->Depth
	         + p->Fonts + p->Reffs + p->Vdi + p->TabIdx;
	if (!f) {
		p->__magic = mgx;
	} else if (p->__magic != mgx) {
		extern int printf (const char *format, ...);
		printf("\n\nDAMAGED in '%s' !!!\n\n\n", f);
	}
}}

#define PmapMFDB(pmap)   ((MFDB*)&(pmap)->Addr)

void PmapInit   (BOOL initNreset);

void PmapFree (p_PIXMAP , p_CLIENT);

static inline p_PIXMAP PmapFind (CARD32 id) {
	p_PIXMAP pmap = DrawFind(id).Pixmap;
	if ((DBG_XRSC_TypeError = (pmap && pmap->isWind))) pmap = NULL;
	return pmap;
}

static inline p_PIXMAP PmapShare (p_PIXMAP pmap) {
	if (pmap) pmap->Reffs++;
	return pmap;
}

static inline p_PIXMAP PmapGet (CARD32 id) {
	return PmapShare (PmapFind (id));
}

short PmapVdi (p_PIXMAP , p_GC , BOOL fonts);

void PmapPutMono    (p_PIXMAP , p_GC , p_GRECT src_dst , p_MFDB src);
void PmapPutColor   (p_PIXMAP , p_GC , p_GRECT src_dst , p_MFDB src);
void PmapDrawPoints (p_PIXMAP , p_GC , p_PXY   , int num);
void PmapDrawRects  (p_PIXMAP , p_GC , p_GRECT , int num);
void PmapFillRects  (p_PIXMAP , p_GC , p_GRECT , int num);

#endif __PIXMAP_H__

//==============================================================================
//
// Cursor.h -- Declaration of struct 'CURSOR' and related functions.
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-07 - Module released for beta state.
// 2000-09-07 - Initial Version.
//==============================================================================
//
#ifndef __CURSOR_H__
# define __CURSOR_H__

#include "types.h"


typedef struct s_CURSOR {
	XRSC(CURSOR, _unused);
	CARD32       Reffs;
	
	// this must exactly match struct MFORM
	PXY    HotSpot;
	CARD16 Depth;   // always 1!
	CARD16 Bgnd;
	CARD16 Fgnd;
	CARD16 Bmask[16];
	CARD16 Fmask[16];
} CURSOR;


void CrsrInit (BOOL initNreset);

p_CURSOR CrsrGet  (CARD32 id);
void     CrsrFree (p_CURSOR, p_CLIENT);

static inline p_CURSOR CrsrShare (p_CURSOR crsr) {
	if (crsr) crsr->Reffs++;
	return crsr;
}

void CrsrSetGlyph (p_CURSOR, CARD8 glyph);
void CrsrSelect   (p_CURSOR);


#endif __CURSOR_H__

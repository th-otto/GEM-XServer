//==============================================================================
//
// font.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-11-15 - Initial Version.
//==============================================================================
//
#ifndef __FONT_H__
# define __FONT_H__

#include "fontable.h"


typedef struct s_FONT {
	XRSC(FONTABLE, isFont);
	
	struct s_FONTFACE * FontFace;
	short               FontIndex   :16;
	unsigned            FontEffects : 3;
	unsigned            FontPoints  :13;
	unsigned            FontWidth;
} FONT;


void FontInit (short count);

void FontDelete (p_FONT , p_CLIENT);
BOOL FontValues (p_FONTABLE , CARD32 id);

static inline void FontCopy (p_FONTABLE dst, const p_FONTABLE src) {
	long * d = (long*)&dst.p->FontFace, * s = (long*)&src.p->FontFace;
	*(d++) = *(s++); *(d) = *(s); dst.p->FontWidth = src.p->FontWidth;
}

short * FontTrans_C (short * arr, const char  * str, int len,
                     const struct s_FONTFACE * face);
short * FontTrans_W (short * arr, const short * str, int len,
                     const struct s_FONTFACE * face);


#endif __FONT_H__

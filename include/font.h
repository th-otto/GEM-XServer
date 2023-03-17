/*
 *==============================================================================
 *
 * font.h
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-11-15 - Initial Version.
 *==============================================================================
 */
#ifndef __FONT_H__
#define __FONT_H__

#include "fontable.h"


typedef struct s_FONT
{
	XRSC(FONTABLE, isFont);

	struct s_FONTFACE *FontFace;
	short FontIndex;
	unsigned short FontEffects;
	unsigned short FontPoints;
	unsigned short FontWidth;
} FONT;


void FontInit(short count);

void FontDelete(p_FONT, p_CLIENT);
BOOL FontValues(p_FONTABLE, CARD32 id);

static inline void FontCopy(p_FONTABLE dst, const p_FONTABLE src)
{
	dst.p->FontFace = src.p->FontFace;
	dst.p->FontIndex = src.p->FontIndex;
	dst.p->FontEffects = src.p->FontEffects;
	dst.p->FontPoints = src.p->FontPoints;
	dst.p->FontWidth = src.p->FontWidth;
}

const short *FontTrans_C(short *arr, const char *str, int len, const struct s_FONTFACE *face);
const short *FontTrans_W(short *arr, const short *str, int len, const struct s_FONTFACE *face);


#endif /* __FONT_H__ */

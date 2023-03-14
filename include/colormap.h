//==============================================================================
//
// colormap.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-07-27 - Initial Version.
//==============================================================================
//
#ifndef __COLORMAP_H__
# define __COLORMAP_H__


typedef struct {
	CARD16 r, g, b;
} RGB;


void CmapInit(void);

void            CmapPalette   (CARD16 handle);
extern CARD32 (*CmapLookup)   (RGB * dst, const RGB * src);
CARD16          CmapRevertIdx (CARD16 idx);

static inline CARD16 CmapPixelIdx (CARD32 pixel, CARD16 depth)
{
	if (GRPH_Depth > 8) {
		extern CARD16 (*Cmap_PixelIdx) (CARD32 );
		if (depth == 1) pixel =               ~pixel;
		else            pixel = Cmap_PixelIdx (pixel);
	}
	return (pixel & ((1uL << depth) -1));
}


#endif __COLORMAP_H__

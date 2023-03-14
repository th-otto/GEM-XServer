/*
 *==============================================================================
 *
 * grph.h
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-06-05 - Initial Version.
 *==============================================================================
 */
#ifndef __GRPH_H__
#define __GRPH_H__

#include <X11/Xproto.h>


BOOL GrphInit(void);
void GrphExit(void);
int GrphSetup(void *format_arr);

BOOL GrphIntersect(GRECT *a, const GRECT *b);
BOOL GrphIntersectP(PRECT *a, const PRECT *b);
CARD16 GrphInterList(GRECT *r, const GRECT *a, CARD16 n, const GRECT *b, CARD16 m);
void GrphCombine(GRECT *a, const GRECT *b);


extern short GRPH_Handle;
extern short GRPH_Vdi;
extern short GRPH_ScreenW;
extern short GRPH_ScreenH;
extern short GRPH_Depth;
extern short GRPH_muWidth;
extern short GRPH_muHeight;
extern int GRPH_DepthNum;

struct xDepthAndVisual
{
	xDepth dpth;
	xVisualType visl;
};
extern struct xDepthAndVisual *GRPH_DepthMSB[2];
extern struct xDepthAndVisual *GRPH_DepthLSB[2];

enum
{
	SCRN_Interleaved = 0,
	SCRN_Standard = 1,
	SCRN_PackedPixel = 2,
	SCRN_FalconHigh = 3
};
extern short GRPH_Format;

extern BOOL(*GrphRasterPut) (MFDB *, CARD16 wdth, CARD16 hght);
extern BOOL(*GrphRasterGet) (MFDB *, PRECT *pxy, MFDB *ptr);

void GrphError(void);


void FT_Grph_ShiftArc_MSB(const PXY *origin, xArc *arc, size_t num, short mode);
void FT_Grph_ShiftArc_LSB(const PXY *origin, xArc *arc, size_t num, short mode);
void FT_Grph_ShiftPnt_MSB(const PXY *origin, PXY *pxy, size_t num, short mode);
void FT_Grph_ShiftPnt_LSB(const PXY *origin, PXY *pxy, size_t num, short mode);
void FT_Grph_ShiftR2P_MSB(const PXY *origin, GRECT *rct, size_t num);
void FT_Grph_ShiftR2P_LSB(const PXY *origin, GRECT *rct, size_t num);


#endif /* __GRPH_H__ */

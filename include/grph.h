//==============================================================================
//
// grph.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-06-05 - Initial Version.
//==============================================================================
//
#ifndef __GRPH_H__
# define __GRPH_H__
# ifdef _grph_
#  define CONST
# else
#  define CONST   const
# endif


BOOL GrphInit  (void);
void GrphExit  (void);
int  GrphSetup (void * format_arr);

BOOL   GrphIntersect  (p_GRECT a, const struct s_GRECT * b);
BOOL   GrphIntersectP (p_PRECT a, const struct s_PRECT * b);
CARD16 GrphInterList  (p_GRECT r, const struct s_GRECT * a, CARD16 n,
                                  const struct s_GRECT * b, CARD16 m);
void   GrphCombine    (p_GRECT a, const struct s_GRECT * b);


extern CONST short GRPH_Handle;
extern CONST short GRPH_Vdi;
extern CONST short GRPH_ScreenW, GRPH_ScreenH, GRPH_Depth,
                   GRPH_muWidth, GRPH_muHeight;
enum {
	SCRN_Interleaved = 0,
	SCRN_Standard    = 1,
	SCRN_PackedPixel = 2,
	SCRN_FalconHigh  = 3
};
extern CONST short GRPH_Format;

struct memory_form;
extern BOOL (*GrphRasterPut)(struct memory_form * , CARD16 wdth, CARD16 hght);
extern BOOL (*GrphRasterGet)(struct memory_form * , PRECT * pxy,
                             struct memory_form * ptr);

void GrphError (void);


#ifndef __p_xArc
# define __p_xArc
	struct _xArc; typedef struct _xArc * p_xArc;
#endif
void FT_Grph_ShiftArc_MSB (const p_PXY , p_xArc  arc, size_t num, short mode);
void FT_Grph_ShiftArc_LSB (const p_PXY , p_xArc  arc, size_t num, short mode);
void FT_Grph_ShiftPnt_MSB (const p_PXY , p_PXY   pxy, size_t num, short mode);
void FT_Grph_ShiftPnt_LSB (const p_PXY , p_PXY   pxy, size_t num, short mode);
void FT_Grph_ShiftR2P_MSB (const p_PXY , p_GRECT rct, size_t num);
void FT_Grph_ShiftR2P_LSB (const p_PXY , p_GRECT rct, size_t num);


#endif __GRPH_H__

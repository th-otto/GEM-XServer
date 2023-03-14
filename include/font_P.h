//==============================================================================
//
// font_P.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-12-07 - Initial Version.
//==============================================================================
//
#ifndef __FONT_P_H__
# define __FONT_P_H__

#include "main.h"
#include "clnt.h"
#include "tools.h"
#include "font.h"
#include "grph.h"
#include "x_gem.h"
#include "gemx.h"

#include <X11/Xproto.h>


typedef struct s_FONTFACE {
	struct s_FONTFACE * Next;
	const short       * CharSet;
	short               Index   :16;
	unsigned            Effects : 3;
	unsigned            Type    : 3;
	BOOL                isSymbol: 1;
	BOOL                isMono  : 1;
	unsigned            Points  : 8;
	
	short Width;
	short Height;
	// min-bounds
	short MinLftBr, MinRgtBr;
	short MinWidth;
	short MinAsc, MinDesc;
	short MinAttr;   // unused
	short _pad1;
	short HalfLine;
	// max-bounds
	short MaxLftBr, MaxRgtBr;
	short MaxWidth;
	short MaxAsc, MaxDesc;
	short MaxAttr;   // unused
	//
	short MinChr, MaxChr;
	short Ascent;
	short Descent;
	
	xCharInfo * CharInfos;
	
	CARD8  Length;
	char   Name[1];
} FONTFACE;
extern FONTFACE * _FONT_List;

typedef struct s_FONTALIAS {
	struct s_FONTALIAS * Next;
	char               * Pattern;
	char                 Name[2];
} FONTALIAS;
extern FONTALIAS * _FONT_Subst;
extern FONTALIAS * _FONT_Alias;

FONTFACE * _Font_Create (const char * name, size_t len,
                         unsigned type, BOOL sym, BOOL mono);
void       _Font_Bounds (FONTFACE * face, BOOL mono);


#endif __FONT_P_H__

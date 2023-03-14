//==============================================================================
//
// types.h
//
// Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2001-02-22 - Initial Version.
//==============================================================================
//
#ifndef __TYPES_H__
# define __TYPES_H__


typedef struct s_GRECT {
	short x, y;
	short w, h;
} GRECT;
#define __GRECT

typedef struct s_PXY {
	short x;
	short y;
} PXY;
#define __PXY

typedef struct s_PRECT {
	PXY lu;
	PXY rd;
} PRECT;


#endif __TYPES_H__

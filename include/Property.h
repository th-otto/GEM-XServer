//==============================================================================
//
// Property.h -- Declaration of struct 'PROPERTY' and related functions.
//
// Properties  are  intended  as a general-purpose naming mechanism for clients
// and  haven't  placed  some  interpretation  on it by the protocol.  Only the
// format value is needed by the server to do byte swapping, if neccessary.
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-07 - Module released for beta state.
// 2000-06-24 - Initial Version.
//==============================================================================
//
#ifndef __PROPERTY_H__
#	define __PROPERTY_H__

#include "xrsc.h"

#include <X11/Xdefs.h>


typedef struct s_PROPERTY * p_PROPERTY;

typedef struct s_PROPERTIES {
	
	// ICCC values, used by the built-in window manager
	
	const char * WindName;
	union { long valid; PXY Size; } Base;
	union { long valid; PXY Size; } Min;
	union { long valid; PXY Size; } Max;
	union { long valid; PXY Step; } Inc;
	// unused: _x,_y,_w,_h, min_aspect, max_aspect, win_gravity
	
	const char * IconName;
	p_PIXMAP     IconPmap;
	p_PIXMAP     IconMask;
	// unused: input, initial_state, icon_window, icon_x, icon_y, window_group
	
	BOOL FixedSize;
	
	BOOL ProtoDelWind;
	
	XRSCPOOL(PROPERTY, Pool, 4);
} PROPERTIES;


void PropDelete (PROPERTIES ** );

void * PropValue   (const PROPERTIES * , Atom name, Atom type, size_t min_len);
BOOL   PropHasAtom (const PROPERTIES * , Atom name, Atom which);


#endif __PROPERTY_H__

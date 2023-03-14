//==============================================================================
//
// Property_P.h -- Private stuff for property functions.
//
// Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2001-06-30 - Initial Version.
//==============================================================================
//
#ifndef __PROPERTY_P_H__
#	define __PROPERTY_P_H__

#include "main.h"
#include "clnt.h"
#include "window.h"
#include "Property.h"
#include "Atom.h"

#include <X11/Xproto.h>
#include <X11/Xatom.h>


typedef struct s_PROPERTY {
	XRSC(PROPERTY, unused);
	Atom       Type;
	BOOL       ICCC   : 1;
	CARD8      Format : 7; // 8/16/32 bit
	size_t     Length :24; // in bytes
	char       Data[4];
} PROPERTY;


void _Prop_ICCC (p_WINDOW wind, PROPERTY * prop, BOOL changed);

#endif __PROPERTY_P_H__

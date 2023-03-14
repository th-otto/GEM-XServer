//==============================================================================
//
// Pointer.h -- Handling of Mouse Pointer Behavior.
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-11-02 - Initial Version.
//==============================================================================
//
#ifndef __POINTER_H__
# define __POINTER_H__


void PntrInit (BOOL initNreset);

static inline CARD16 PntrMap (int idx) {
	extern CARD16 PNTR_Mapping[4];
	return PNTR_Mapping[idx];
}


#endif __POINTER_H__

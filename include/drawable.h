//==============================================================================
//
// drawable.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-06-05 - Initial Version.
//==============================================================================
//
#ifndef __DRAWABLE_H__
# define __DRAWABLE_H__

#include "xrsc.h"


typedef struct memory_form * p_MFDB;


typedef struct s_DRAWABLE {
	XRSC(DRAWABLE, isWind);
	
	// following must match MFDB for Pixmap
	                 // MFDB         Pixmap  Window
	long   _unused1;  // fd_addr      Mem     Rect.X,Y
	CARD16 W, H;      // fd_w, fd_h   W, H    Rect.W,H
	CARD16 _unused2;  // fd_wdwidth   nPads   ..
	short  _unused3;  // fd_stand     0       ..
	CARD16 Depth;     // fd_nplanes   Depth   Depth
	                  // fd_r[3]      _res[3]
} DRAWABLE;


void DrawDelete (p_DRAWABLE, p_CLIENT);

static inline p_DRAWABLE DrawFind (CARD32 id) {
	DRAWABLE * draw;
	if (RID_Base(id)) {
		CLIENT * clnt = ClntFind (id);
		draw = (clnt ? Xrsc(DRAWABLE, id, clnt->Drawables) : NULL);
	} else if (id == ROOT_WINDOW) {
		extern struct s_WINDOW WIND_Root;
		draw = (DRAWABLE*)&WIND_Root;
	} else {
		draw = NULL;
	}
	return (p_DRAWABLE)draw;
}


#endif __DRAWABLE_H__

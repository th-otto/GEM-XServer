//==============================================================================
//
// fontable.h
//
// Copyright (C) 2000 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-11-05 - Initial Version.
//==============================================================================
//
#ifndef __FONTABLE_H__
# define __FONTABLE_H__

#include "xrsc.h"


typedef struct s_FONTABLE {
	XRSC(FONTABLE, isFont);
	
	struct s_FONTFACE * FontFace;
	short               FontIndex   :16;
	unsigned            FontEffects : 3;
	unsigned            FontPoints  :13;
	unsigned            FontWidth;
} FONTABLE;


void FablDelete (p_FONTABLE, p_CLIENT);

static inline p_FONTABLE FablFind (CARD32 id) {
	CLIENT   * clnt = ClntFind (id);
	FONTABLE * fabl = (clnt ? Xrsc(FONTABLE, id, clnt->Fontables) : NULL);
	return (p_FONTABLE)fabl;
}


#endif __FONTABLE_H__

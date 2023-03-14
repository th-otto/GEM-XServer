//==============================================================================
//
// pixmap_P.h
//
// Copyright (C) 2000 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-10-15 - Initial Version.
//==============================================================================
//
#ifndef __PIXMAP_P_H__
#	define __PIXMAP_P_H__

#include "main.h"
#include "clnt.h"
#include "gcontext.h"
#include "pixmap.h"

CARD32 _Pmap_MonoFuncts (p_GC gc,
                  void (**f_col)(CARD16 *, CARD16 inc, CARD16 num, CARD16 pat),
                  void (**f_rec)(CARD16 *, CARD16 inc, CARD16 num, CARD16 len));


#endif __PIXMAP_P_H__

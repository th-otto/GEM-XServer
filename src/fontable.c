//==============================================================================
//
// keyboard.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-10-05 - Initial Version.
//==============================================================================
//
#include "main.h"
#include "clnt.h"
#include "font.h"
#include "gcontext.h"


//==============================================================================
void
FablDelete (p_FONTABLE fabl, p_CLIENT clnt)
{
	if (fabl.p->isFont) {
		FontDelete (fabl.Font, clnt);
	} else {
		GcntDelete (fabl.Gc, clnt);
	}
}
